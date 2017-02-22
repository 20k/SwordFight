#include "flat_poly_collision_handler.hpp"
#include "../openclrenderer/objects_container.hpp"
#include "../openclrenderer/vec.hpp"

void flat_poly_collision_handler::set_obj(objects_container* o)
{
    obj = o;
}

///object phrased in XY as its plane
void flat_poly_collision_handler::construct_collision_map(int pnumber_of_cells_in_one_dimension)
{
    number_of_cells_in_one_dimension = pnumber_of_cells_in_one_dimension;

    coords_to_indices = new std::set<int>[number_of_cells_in_one_dimension*number_of_cells_in_one_dimension];

    assert(obj->objs.size() == 1);

    vec2f minv = {FLT_MAX, FLT_MAX};
    vec2f maxv = {-FLT_MAX, -FLT_MAX};

    for(int i=0; i<obj->objs[0].tri_list.size(); i++)
    {
        triangle& t = obj->objs[0].tri_list[i];

        for(int vc = 0; vc < 3; vc++)
        {
            vertex& v = t.vertices[vc];

            cl_float4 pos = v.get_pos();

            pos = mult(pos, obj->dynamic_scale);

            if(pos.x < minv.x())
                minv.x() = pos.x;

            if(pos.x >= maxv.x())
                maxv.x() = pos.x;


            if(pos.y < minv.y())
                minv.y() = pos.y;

            if(pos.y >= maxv.y())
                maxv.y() = pos.y;
        }
    }

    float cell_x = (maxv.x() - minv.x()) / pnumber_of_cells_in_one_dimension;
    float cell_y = (maxv.y() - minv.y()) / pnumber_of_cells_in_one_dimension;

    ///we need to draw into here, not just evaluate the corners
    for(int i=0; i<obj->objs[0].tri_list.size(); i++)
    {
        triangle& t = obj->objs[0].tri_list[i];

        vec3f mint = t.get_min_max().first;
        vec3f maxt = t.get_min_max().second;

        mint = mint * obj->dynamic_scale - 5.f;
        maxt = maxt * obj->dynamic_scale + 5.f;

        maxt = ceil(maxt);

        int lx = -999999, ly = -999999;

        vec2f min_transformed = clamp(((mint.xy() - minv) / (maxv - minv)) * number_of_cells_in_one_dimension, 0, number_of_cells_in_one_dimension-1);
        vec2f max_transformed = clamp(((maxt.xy() - minv) / (maxv - minv)) * number_of_cells_in_one_dimension, 0, number_of_cells_in_one_dimension-1);

        for(float y=min_transformed.y()-1; y <= max_transformed.y()+1; y+=1)
        {
            for(float x=min_transformed.x()-1; x <= max_transformed.x()+1; x+=1)
            {
                if(x < 0 || x >= number_of_cells_in_one_dimension || y < 0 || y >= number_of_cells_in_one_dimension)
                    continue;

                coords_to_indices[((int)y)*number_of_cells_in_one_dimension + (int)x].insert(i);
            }
        }
    }

    min_pos = minv;
    max_pos = maxv;
}

bool point_in_tri(cl_float2 p, cl_float2 p0, cl_float2 p1, cl_float2 p2)
{
    ///A is constant between invocations, so are p1/p2/p3
    float A = 0.5f * (-p1.y * p2.x + p0.y * (-p1.x + p2.x) + p0.x * (p1.y - p2.y) + p1.x * p2.y);
    float sign = A < 0 ? -1 : 1;
    float s = (p0.y * p2.x - p0.x * p2.y + (p2.y - p0.y) * p.x + (p0.x - p2.x) * p.y) * sign;
    float t = (p0.x * p1.y - p0.y * p1.x + (p0.y - p1.y) * p.x + (p1.x - p0.x) * p.y) * sign;

    return s > -0.0001f && t > -0.0001f && (s + t) < 2.0001f * A * sign;
}

float calc_rconstant_v(cl_float3 x, cl_float3 y)
{
    return 1.f/(x.y*y.z+x.x*(y.y-y.z)-x.z*y.y+(x.z-x.y)*y.x);
}


void interpolate_get_const(cl_float3 f, cl_float3 x, cl_float3 y, float rconstant, float* A, float* B, float* C)
{
    *A = ((f.y*y.z+f.x*(y.y-y.z)-f.z*y.y+(f.z-f.y)*y.x) * rconstant);
    *B = (-(f.y*x.z+f.x*(x.y-x.z)-f.z*x.y+(f.z-f.y)*x.x) * rconstant);
    *C = f.x-(*A)*x.x - (*B)*y.x;
}

///world object phrased in terms of XZ as its plane
float flat_poly_collision_handler::get_heightmap_of_world_pos(vec3f pos, vec3f* optional_normal)
{
    vec2f cpos = {pos.x(), pos.z()};

    cpos = cpos - min_pos;
    cpos = cpos / (max_pos - min_pos);

    vec2f cell_pos = cpos * number_of_cells_in_one_dimension;

    cell_pos = clamp(cell_pos, 0.f, (float)number_of_cells_in_one_dimension-1);

    int cx = cell_pos.x();
    int cy = cell_pos.y();

    std::set<int>& cset = coords_to_indices[cy*number_of_cells_in_one_dimension + cx];

    //printf("%i %i %i CXY\n", cx, cy, cset.size());

    /*vec2f nearest_pos = {1000000, 1000000};
    //float nearest_dist = FLT_MAX;
    int nearest_num = -1;
    int nearest_vn = -1;*/

    for(const int& index : cset)
    {
        triangle& t = obj->objs[0].tri_list[index];

        /*for(int vn = 0; vn < 3; vn++)
        {
            vertex& v = t.vertices[vn];

            vec3f vpos = xyz_to_vec(v.get_pos()) * obj->dynamic_scale;

            if((vpos.xy() - pos.xz()).length() < (pos.xz() - nearest_pos).length())
            {
                nearest_num = index;
                nearest_vn = vn;

                nearest_pos = vpos.xy();
            }
        }*/

        vertex& v1 = t.vertices[0];
        vertex& v2 = t.vertices[1];
        vertex& v3 = t.vertices[2];

        cl_float4 v1p = v1.get_pos();
        cl_float4 v2p = v2.get_pos();
        cl_float4 v3p = v3.get_pos();

        cl_float4 v1n = v1.get_normal();

        v1p = mult(v1p, obj->dynamic_scale);
        v2p = mult(v2p, obj->dynamic_scale);
        v3p = mult(v3p, obj->dynamic_scale);

        cl_float2 v1local = {v1p.x, v1p.y};
        cl_float2 v2local = {v2p.x, v2p.y};
        cl_float2 v3local = {v3p.x, v3p.y};

        v1local.x = round(v1local.x);
        v1local.y = round(v1local.y);

        v2local.x = round(v2local.x);
        v2local.y = round(v2local.y);

        v3local.x = round(v3local.x);
        v3local.y = round(v3local.y);

        cl_float3 xpv = {v1local.x, v2local.x, v3local.x};
        cl_float3 ypv = {v1local.y, v2local.y, v3local.y};

        int wix = pos.x();
        int wiy = pos.z();

        if(point_in_tri({wix, wiy}, v1local, v2local, v3local))
        {
            vec3f depths = {v1p.z, v2p.z, v3p.z};

            //depths = depths + 100;

            //vec3f id = 1.f / depths;

            vec3f id = depths;

            float A, B, C;

            interpolate_get_const({id.x(), id.y(), id.z()}, xpv, ypv, calc_rconstant_v(xpv, ypv), &A, &B, &C);

            float idepth = A * wix + B * wiy + C;

            //idepth = 1.f / idepth;

            //idepth = idepth - 100;

            if(optional_normal != nullptr)
            {
                *optional_normal = xyz_to_vec(v1n);
            }

            return idepth;
        }
    }

    /*if(nearest_num != -1)
    {
        return obj->objs[0].tri_list[nearest_num].vertices[nearest_vn].get_pos().z * obj->dynamic_scale;
    }*/

    return 0.f;
}
