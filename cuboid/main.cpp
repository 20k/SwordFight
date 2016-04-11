#include "../openclrenderer/proj.hpp"

#include "../openclrenderer/vec.hpp"

#include <vec/vec.hpp>

#include <math.h>

///todo eventually
///split into dynamic and static objects

///todo
///fix memory management to not be atrocious

///we're completely hampered by memory latency

///rift head movement is wrong

//compute::event render_tris(engine& eng, cl_float4 position, cl_float4 rotation, compute::opengl_renderbuffer& g_screen_out);

void callback (cl_event event, cl_int event_command_exec_status, void *user_data)
{
    //printf("%s\n", user_data);

    std::cout << (*(sf::Clock*)user_data).getElapsedTime().asMicroseconds()/1000.f << std::endl;
}

objects_container ctr;

void lobj()
{
    float dim = 4;

    texture tex;

    obj_cube_by_extents(&ctr, tex, {dim, dim, dim});
}

float calc_third_areas_i(float x1, float x2, float x3, float y1, float y2, float y3, float x, float y)
{
    return (fabs(x2*y-x*y2+x3*y2-x2*y3+x*y3-x3*y) + fabs(x*y1-x1*y+x3*y-x*y3+x1*y3-x3*y1) + fabs(x2*y1-x1*y2+x*y2-x2*y+x1*y-x*y1)) * 0.5f;
}

float calc_area(cl_float3 x, cl_float3 y)
{
    return fabs((x.x*(y.y - y.z) + x.y*(y.z - y.x) + x.z*(y.x - y.y)) * 0.5f);
}


void get_barycentric(vec3f p, vec3f a, vec3f b, vec3f c, float* u, float* v, float* w)
{
    vec3f v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    *v = (d11 * d20 - d01 * d21) / denom;
    *w = (d00 * d21 - d01 * d20) / denom;
    *u = 1.0f - *v - *w;
}


void interpolate_get_const(cl_float3 f, cl_float3 x, cl_float3 y, float rconstant, float* A, float* B, float* C)
{
    *A = ((f.y*y.z+f.x*(y.y-y.z)-f.z*y.y+(f.z-f.y)*y.x) * rconstant);
    *B = (-(f.y*x.z+f.x*(x.y-x.z)-f.z*x.y+(f.z-f.y)*x.x) * rconstant);
    *C = f.x-(*A)*x.x - (*B)*y.x;
}

void interpolate_get_const_v(vec3f f, vec3f x, vec3f y, float rconstant, float* A, float* B, float* C)
{
    return interpolate_get_const({f.v[0], f.v[1], f.v[2]}, {x.v[0], x.v[1], x.v[2]}, {y.v[0], y.v[1], y.v[2]}, rconstant, A, B, C);
}

float calc_rconstant_v(cl_float3 x, cl_float3 y)
{
    return 1.f / (x.y*y.z+x.x*(y.y-y.z)-x.z*y.y+(x.z-x.y)*y.x);
}

struct graph_vert
{
    std::vector<graph_vert*> vert;

    vec2f pos;

    graph_vert(vec2f _pos){pos = _pos;}

    bool is_connected(graph_vert* v)
    {
        for(auto& i : vert)
        {
            if(i == v)
                return true;
        }

        return false;
    }

    void rem(graph_vert* v)
    {
        for(int i=0; i<vert.size(); i++)
        {
            if(vert[i] == v)
            {
                vert.erase(vert.begin() + i);
                return;
            }
        }
    }

    void unlink(graph_vert* v)
    {
        v->rem(this);
        this->rem(v);
    }

    void link(graph_vert* v)
    {
        vert.push_back(v);
        v->vert.push_back(this);
    }
};

struct tri_n
{
    vec2f p[3];
};


///this is not a good heuristic, we need to use actual point within triangle, and then fallback to nearest 3 if none
///actuallly.... by the definition of delauny trianglulatitnitnon, this might be correct due to the circumcircle thing
std::vector<graph_vert*> get_nearest_3(const std::vector<graph_vert*>& graph, graph_vert* node)
{
    std::vector<std::pair<graph_vert*, float>> arr;

    arr.resize(3);

    for(auto& i : arr)
    {
        i.second = FLT_MAX;
    }

    std::cout << "Graph size " << graph.size() << std::endl;

    for(auto& i : graph)
    {
        vec2f pos = i->pos;
        vec2f mpos = node->pos;

        float dist = (pos - mpos).length();

        for(int j=0; j<3; j++)
        {
            if(dist < arr[j].second)
            {
                arr.insert(arr.begin() + j, {i, dist});
                break;
            }
        }

        arr.resize(3);
    }

    std::vector<graph_vert*> verts;

    for(auto& i : arr)
    {
        if(i.first != nullptr)
            verts.push_back(i.first);
    }


    return verts;
}

float get_vec_angle(vec2f v1, vec2f v2, vec2f v3)
{
    return acos(dot(v3 - v2, v1 - v2));
}

std::vector<graph_vert*> flippity_flip_flip_floppity_flip(const std::vector<graph_vert*>& graph)
{
    if(graph.size() < 4)
        return graph;

    for(auto& i : graph)
    {
        ///sort connections anticlockwise

        std::vector<std::pair<float, graph_vert*>> verts;

        vec2f base = i->pos;

        for(auto& j : i->vert)
        {
            vec2f their_pos = j->pos;

            vec2f rel = their_pos - base;

            float angle = rel.angle();

            verts.push_back({angle, j});
        }

        std::sort(verts.begin(), verts.end());

        ///hmm. should it be n, n+1, n+2, check if thats delaunay?
        ///I think thats actually equivalent


        for(int j=0; j<verts.size(); j++)
        {
            int n = j;
            int n1 = (j + 1) % verts.size();
            int n2 = (j + 2) % verts.size();

            graph_vert* v0, *v1, *v2, *v3;

            v0 = i;
            v1 = verts[n].second;
            v2 = verts[n1].second;
            v3 = verts[n2].second;

            float a1, a2;

            ///v2 is the middle point

            ///so we want 0 -> 1 -> 2
            ///0 -> 3 -> 2
            a1 = get_vec_angle(v0->pos, v1->pos, v2->pos);
            a2 = get_vec_angle(v0->pos, v3->pos, v2->pos);


            if(fabs(a1) + fabs(a2) > M_PI/2.f)
            {
                ///swap
                ///so we have triangulation
                ///v0 -> v1, v1 -> v2, v2 -> v0
                ///v0 -> v3, v3 -> v2, v2 -> v0
                ///we want to swap it to
                ///v0 -> v1, v1 -> v3, v3 -> v0
                ///v1 -> v2, v2 -> v3, v3 -> v1

                ///so really we only want to unlink 0 -> 2
                ///and replace it with 3 -> 1
                v0->unlink(v2);

                v3->link(v1);
            }
        }

        ///purely for sorting purposes
        for(int j=0; j<verts.size(); j++)
        {
            i->vert[j] = verts[j].second;
        }
    }

    return graph;
}

///node needs to be allocated memory remember!
std::vector<graph_vert*> insert_into_graph_with_connectivity(const std::vector<graph_vert*>& graph, graph_vert* node)
{
    std::vector<graph_vert*> nearest = get_nearest_3(graph, node);

    for(auto& i : nearest)
    {
        //i->vert.push_back(node);

        //node->vert.push_back(i);

        i->link(node);



        std::cout << "npos " << i->pos << std::endl;
    }

    std::cout << "enpos " << std::endl;


    auto ngraph = graph;

    ngraph.push_back(node);


    return flippity_flip_flip_floppity_flip(ngraph);
}

std::vector<tri_n> get_delauny(const std::vector<vec2f>& in_p)
{
    std::vector<graph_vert*> verts;

    for(auto& i : in_p)
    {
        graph_vert* ng = new graph_vert(i);

        std::cout << "in " << i << std::endl;

        verts = insert_into_graph_with_connectivity(verts, ng);
    }

    std::vector<tri_n> ret;

    for(auto& i : verts)
    {
        for(int j=0; j<i->vert.size(); j++)
        {
            int n = j;
            int n1 = (j+1) % i->vert.size();

            tri_n t1;

            t1.p[0] = i->pos;
            t1.p[1] = i->vert[n]->pos;
            t1.p[2] = i->vert[n1]->pos;

            std::cout << t1.p[0] << std::endl;
            std::cout << t1.p[1] << std::endl;
            std::cout << t1.p[2] << std::endl;

            std::cout << std::endl;

            ret.push_back(t1);
        }
    }

    return ret;

    /*vec2f none = {0};

    return {{none, none, none}};*/
}

///I mean, not ready for primetime on objects with overlapping textures or non optimal winding order for the vt coordinates vs the triangle coords
///but other than that, doing alright
///could write the winding vs the vt winding order into buffer, then flip at the end
std::vector<triangle> volygonate(object& o, object_context& ctx)
{
    texture_context& tex_ctx = ctx.tex_ctx;

    texture* tex = tex_ctx.id_to_tex(o.tid);

    tex->load();

    std::vector<triangle> ret;

    std::vector<vec4f*> raster_buf;

    vec4f* layer_0 = new vec4f[tex->get_largest_dimension()*tex->get_largest_dimension()]();

    //memset(layer_0, -1, sizeof(vec3f)*tex->get_largest_dimension()*tex->get_largest_dimension());

    raster_buf.push_back(layer_0);

    for(auto& tri : o.tri_list)
    {
        vec2f vt[3];
        vec3f v[3];
       // vec3f rv[3];


        for(int i=0; i<3; i++)
        {
            vt[i] = xy_to_vec(tri.vertices[i].get_vt()) * tex->get_largest_dimension();
            v[i] = xyz_to_vec(tri.vertices[i].get_pos());

            //rv[i] = round(v[i]);

            vt[i] = round(vt[i]);
        }

        vec3f xfs = {v[0].v[0], v[1].v[0], v[2].v[0]};
        vec3f yfs = {v[0].v[1], v[1].v[1], v[2].v[1]};
        vec3f zfs = {v[0].v[2], v[1].v[2], v[2].v[2]};

        vec3f vtxf = {vt[0].v[0], vt[1].v[0], vt[2].v[0]};
        vec3f vtyf = {vt[0].v[1], vt[1].v[1], vt[2].v[1]};

        float area = calc_area({vt[0].v[0], vt[1].v[0], vt[2].v[0]}, {vt[0].v[1], vt[1].v[1], vt[2].v[1]});

        float rconstant = calc_rconstant_v({vtxf.v[0], vtxf.v[1], vtxf.v[2]}, {vtyf.v[0], vtyf.v[1], vtyf.v[2]});

        vec3f cst[3];

        interpolate_get_const_v(xfs, vtxf, vtyf, rconstant, &cst[0].v[0], &cst[0].v[1], &cst[0].v[2]);
        interpolate_get_const_v(yfs, vtxf, vtyf, rconstant, &cst[1].v[0], &cst[1].v[1], &cst[1].v[2]);
        interpolate_get_const_v(zfs, vtxf, vtyf, rconstant, &cst[2].v[0], &cst[2].v[1], &cst[2].v[2]);

        float mod = area / 5000.f;

        vec2f mn, mx;

        mn = min(vt[0], min(vt[1], vt[2])) - 1.f;
        mx = max(vt[0], max(vt[1], vt[2]));

        for(float j = mn.v[1]; j < mx.v[1]; j += 1.f)
        {
            for(float i = mn.v[0]; i < mx.v[0]; i += 1.f)
            {
                vec2f loc = {i, j};

                vec3f interp_pos;

                for(int k=0; k<3; k++)
                    interp_pos.v[k] = cst[k].v[0] * i + cst[k].v[1] * j + cst[k].v[2];

                ///clamp temporarily

                if(loc.v[0] < 0 || loc.v[0] >= tex->get_largest_dimension() || loc.v[1] < 0 || loc.v[1] >= tex->get_largest_dimension())
                    continue;

                ///if point within triangle!

                //float calc_third_areas_i(float x1, float x2, float x3, float y1, float y2, float y3, float x, float y)

                float s1 = calc_third_areas_i(vtxf.v[0], vtxf.v[1], vtxf.v[2], vtyf.v[0], vtyf.v[1], vtyf.v[2], i, j);

                bool cond = s1 < area + mod;

                if(!cond)
                    continue;

                vec4f* buf = raster_buf[0];

                buf[((int)j)*tex->get_largest_dimension() + (int)i] = (vec4f){interp_pos.v[0], interp_pos.v[1], interp_pos.v[2], 1.f};
            }
        }
    }


    float jiggle_distance = 15.f;

    int one_side_volygon_density = 20;

    float scale = ((float)tex->get_largest_dimension()-1) / one_side_volygon_density;

    ///seed in a rectangular grid
    ///eliminate bad ones
    ///jitter
    ///elimiate (?)
    ///generate volygon shapes somehow like voronoi

    int width = tex->get_largest_dimension();

    std::vector<vec2f> sample_points;

    ///this might... actually have worked???
    ///there's discontinuities being joined across texture boundaries which needs fixing
    ///but for a first pass???
    for(int j=0; j<one_side_volygon_density; j++)
    {
        for(int i=0; i<one_side_volygon_density; i++)
        {
            vec2f scale_pos = (vec2f){i, j} * scale;

            float rangle = randf_s(0.f, M_PI);

            float rlen = randf_s(jiggle_distance/1.f, jiggle_distance);

            vec2f offset_pos = (vec2f){rlen * cos(rangle), rlen * sin(rangle)};


            scale_pos = scale_pos + offset_pos;

            ///modf eh
            scale_pos = clamp(scale_pos, 0.f, tex->get_largest_dimension()-1.f);

            vec4f* buf = raster_buf[0];

            if(buf[((int)scale_pos.v[1])*width + (int)scale_pos.v[0]].v[3] < 1)
                continue;

            sample_points.push_back(scale_pos);
        }
    }

    auto dret = get_delauny(sample_points);

    for(auto& i : dret)
    {
        vec2f s1, s2, s3;

        s1 = i.p[0];
        s2 = i.p[1];
        s3 = i.p[2];

        vec4f* buf = raster_buf[0];

        vec4f l1, l2, l3;

        l1 = buf[((int)s1.v[1])*width + (int)s1.v[0]];
        l2 = buf[((int)s2.v[1])*width + (int)s2.v[0]];
        l3 = buf[((int)s3.v[1])*width + (int)s3.v[0]];

        triangle tri;
        tri.vertices[0].set_pos({l1.v[0], l1.v[1], l1.v[2]});
        tri.vertices[1].set_pos({l2.v[0], l2.v[1], l2.v[2]});
        tri.vertices[2].set_pos({l3.v[0], l3.v[1], l3.v[2]});

        ret.push_back(tri);
    }


    #ifdef SQ_RECONST
    int square_half_size = 10;

    for(int i=0; i<sample_points.size(); i++)
    {
        vec2f sample = sample_points[i];

        sample = clamp(sample, 0.f, tex->get_largest_dimension()-1.f);

        vec2i locs[4];

        locs[0] = {sample.v[0] - square_half_size, sample.v[1] - square_half_size};
        locs[1] = {sample.v[0] + square_half_size, sample.v[1] - square_half_size};
        locs[2] = {sample.v[0] - square_half_size, sample.v[1] + square_half_size};
        locs[3] = {sample.v[0] + square_half_size, sample.v[1] + square_half_size};

        vec4f* buf = raster_buf[0];


        std::vector<vec4f> points;

        bool quit = false;

        for(int j=0; j<4; j++)
        {
            locs[j] = clamp(locs[j], 0.f, tex->get_largest_dimension()-1.f);

            vec4f pos = buf[((int)locs[j].v[1])*width + (int)locs[j].v[0]];

            if(pos.v[3] < 1)
            {
                quit = true;
                break;
            }

            points.push_back(pos);
        }

        if(quit)
            continue;

        vec4f v1 = points[0];
        vec4f v2 = points[1];
        vec4f v3 = points[2];
        vec4f v4 = points[3];

        triangle tri1, tri2;

        tri1.vertices[0].set_pos({v1.v[0], v1.v[1], v1.v[2]});
        tri1.vertices[1].set_pos({v2.v[0], v2.v[1], v2.v[2]});
        tri1.vertices[2].set_pos({v3.v[0], v3.v[1], v3.v[2]});

        tri2.vertices[0].set_pos({v3.v[0], v3.v[1], v3.v[2]});
        tri2.vertices[1].set_pos({v2.v[0], v2.v[1], v2.v[2]});
        tri2.vertices[2].set_pos({v4.v[0], v4.v[1], v4.v[2]});

        ret.push_back(tri1);
        ret.push_back(tri2);
    }
    #endif



    #ifdef RECONSTRUCT
    int skip_amount = 5;

    for(int j=0; j<tex->get_largest_dimension(); j+=skip_amount)
    {
        for(int i=0; i<tex->get_largest_dimension(); i+=skip_amount)
        {
            vec2i next_loc = {i + skip_amount, j + skip_amount};

            next_loc = clamp(next_loc, 0, tex->get_largest_dimension()-1);

            vec4f v1, v2, v3, v4;

            v1 = raster_buf[0][j*tex->get_largest_dimension() + i];
            v2 = raster_buf[0][j*tex->get_largest_dimension() + next_loc.v[0]];
            v3 = raster_buf[0][next_loc.v[1]*tex->get_largest_dimension() + i];
            v4 = raster_buf[0][next_loc.v[1]*tex->get_largest_dimension() + next_loc.v[0]];

            if(v1.v[3] < 1 || v2.v[3] < 1 || v3.v[3] < 1 || v4.v[3] < 1)
                continue;

            triangle tri1, tri2;

            tri1.vertices[0].set_pos({v1.v[0], v1.v[1], v1.v[2]});
            tri1.vertices[1].set_pos({v2.v[0], v2.v[1], v2.v[2]});
            tri1.vertices[2].set_pos({v3.v[0], v3.v[1], v3.v[2]});

            tri2.vertices[0].set_pos({v3.v[0], v3.v[1], v3.v[2]});
            tri2.vertices[1].set_pos({v2.v[0], v2.v[1], v2.v[2]});
            tri2.vertices[2].set_pos({v4.v[0], v4.v[1], v4.v[2]});

            tri1.vertices[0].set_vt({i, j});
            tri1.vertices[1].set_vt({next_loc.v[0], j});
            tri1.vertices[2].set_vt({i, next_loc.v[1]});

            tri2.vertices[0].set_vt({i, next_loc.v[1]});
            tri2.vertices[1].set_vt({next_loc.v[0], j});
            tri2.vertices[2].set_vt({next_loc.v[0], next_loc.v[1]});

            for(int k=0; k<3; k++)
            {
                tri1.vertices[k].set_normal({0, 1, 0});
                tri2.vertices[k].set_normal({0, 1, 0});

                cl_float2
                nvt = tri1.vertices[k].get_vt();
                nvt.x /= tex->get_largest_dimension();
                nvt.y /= tex->get_largest_dimension();
                tri1.vertices[k].set_vt(nvt);

                nvt = tri2.vertices[k].get_vt();
                nvt.x /= tex->get_largest_dimension();
                nvt.y /= tex->get_largest_dimension();
                tri2.vertices[k].set_vt(nvt);
            }

            //printf("%f %f %f\n", EXPAND_3(v1));

            ret.push_back(tri1);
            ret.push_back(tri2);
        }
    }
    #endif

    delete [] layer_0;

    ///so we essentially want to rasterise the triangles into texture space
    ///then run voronoi on that
    ///then go back from voronoi to 3d positions by that voronoi

    return ret;
}

///still fucked
std::vector<triangle> tri_to_cubes(const triangle& tri)
{
    std::vector<triangle> new_tris;

    vec3f pos[3];

    for(int i=0; i<3; i++)
    {
        pos[i] = xyz_to_vec(tri.vertices[i].get_pos());
    }

    //int c = 0;

    vec3f v1 = pos[1] - pos[0];
    vec3f v2 = pos[2] - pos[0];

    float len = v1.length();

    vec3f vn = v1.norm();

    bool at_least_once = false;

    for(float i=0; i<len || !at_least_once; i += 70.f)
    {
        vec3f p = vn * i + pos[0];

        vec3f dist_to_v2 = point2line_shortest(pos[0], v2, p);

        float dv2l = dist_to_v2.length();

        for(float j=0; j<dv2l || !at_least_once; j += 70.f)
        {
            vec3f p2 = p + j * dist_to_v2.norm();

            std::vector<triangle> cube_tris = ctr.objs[0].tri_list;

            int vtc = 0;

            for(auto& i : cube_tris)
            {
                for(auto& k : i.vertices)
                {
                    cl_float4 rel = add(k.get_pos(), {p2.v[0], p2.v[1], p2.v[2]});

                    k.set_pos(rel);

                    k.set_vt(tri.vertices[vtc].get_vt());

                    vtc = (vtc + 1) % 3;
                }

                new_tris.push_back(i);
            }

            at_least_once = true;
        }
    }

    return new_tris;
}

std::vector<triangle> cube_by_area(const std::vector<triangle>& in_tris)
{
    std::vector<triangle> new_tris;

    for(auto& i : in_tris)
    {
        auto tris = tri_to_cubes(i);

        for(auto& j : tris)
        {
            new_tris.push_back(j);
        }
    }

    return new_tris;
}

std::vector<triangle> cubulate(const std::vector<triangle>& in_tris)
{
    std::vector<triangle> tris = in_tris;

    float dim = 4;

    texture tex;

    objects_container ctr;

    int skip_c = 0;

    std::vector<triangle> new_tris;

    for(auto& base : tris)
    {
        for(auto& vert : base.vertices)
        {
            skip_c++;

            if(skip_c % 10 != 0)
                continue;

            obj_cube_by_extents(&ctr, tex, {dim, dim, dim});

            std::vector<triangle> cube_tris = ctr.objs[0].tri_list;

            for(auto& i : cube_tris)
            {
                for(auto& k : i.vertices)
                {
                    k.set_pos(add(k.get_pos(), vert.get_pos()));
                    k.set_vt(vert.get_vt());
                }

                new_tris.push_back(i);
            }

            ctr.objs.clear();
        }
    }

    return new_tris;
}

///gamma correct mipmap filtering
///7ish pre tile deferred
///try first bounce in SS, then go to global if fail
int main(int argc, char *argv[])
{
    lg::set_logfile("./logging.txt");

    std::streambuf* b1 = std::cout.rdbuf();

    std::streambuf* b2 = lg::output->rdbuf();

    std::ios* r1 = &std::cout;
    std::ios* r2 = lg::output;

    r2->rdbuf(b1);



    sf::Clock load_time;

    object_context context;

    auto sponza = context.make_new();
    //sponza->set_file("../openclrenderer/sp2/sp2.obj");
    sponza->set_file("crappy_cylinder.obj");
    sponza->set_active(true);
    //sponza->cache = false;

    engine window;

    window.load(1680,1050,1000, "turtles", "../openclrenderer/cl2.cl", true);

    lobj();

    window.set_camera_pos((cl_float4){-800,150,-570});

    //window.window.setPosition({-20, -20});

    //#ifdef OCULUS
    //window.window.setVerticalSyncEnabled(true);
    //#endif

    ///we need a context.unload_inactive
    context.load_active();

    sponza->set_specular(0.f);

    sponza->scale(100.f);

    int io = 0;

    int acc = 0;

    for(object& o : sponza->objs)
    {
        //o.tri_list = cube_by_area(o.tri_list);

        o.tri_list = volygonate(o, context);

        o.tri_num = o.tri_list.size();

        printf("Io %i %i %i\n", o.tri_num, io++, sponza->objs.size());

        acc += o.tri_num;
    }

    printf("TNUM %i\n\n\n\n", acc);


    context.build(true);


    ///if that is uncommented, we use a metric tonne less memory (300mb)
    ///I do not know why
    ///it might be opencl taking a bad/any time to reclaim. Investigate

    auto object_dat = context.fetch();
    window.set_object_data(*object_dat);

    sf::Event Event;

    light l;
    l.set_col((cl_float4){1.0f, 1.0f, 1.0f, 0.0f});
    l.set_shadow_casting(1);
    l.set_brightness(1);
    l.radius = 100000;
    l.set_pos((cl_float4){-200, 2000, -100, 0});
    //l.set_godray_intensity(1.f);
    //window.add_light(&l);

    light::add_light(&l);

    auto light_data = light::build();
    ///

    window.set_light_data(light_data);
    window.construct_shadowmaps();


    printf("load_time %f\n", load_time.getElapsedTime().asMicroseconds() / 1000.f);
    sf::Keyboard key;


    float avg_ftime = 6000;


    ///use event callbacks for rendering to make blitting to the screen and refresh
    ///asynchronous to actual bits n bobs
    ///clSetEventCallback
    while(window.window.isOpen())
    {
        sf::Clock c;

        while(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();
        }

        compute::event event;

        //if(window.can_render())
        {
            ///do manual async on thread
            ///make a enforce_screensize method, rather than make these hackily do it
            event = window.draw_bulk_objs_n(*context.fetch());

            window.increase_render_events();

            context.fetch()->swap_depth_buffers();
        }

        window.set_render_event(event);

        window.blit_to_screen(*context.fetch());

        window.flip();

        window.render_block();

        context.build_tick();

        avg_ftime += c.getElapsedTime().asMicroseconds();

        avg_ftime /= 2;

        if(key.isKeyPressed(sf::Keyboard::M))
            std::cout << c.getElapsedTime().asMicroseconds() << std::endl;

        if(key.isKeyPressed(sf::Keyboard::Comma))
            std::cout << avg_ftime << std::endl;
    }

    ///if we're doing async rendering on the main thread, then this is necessary
    window.render_block();
    cl::cqueue.finish();
    glFinish();
}
