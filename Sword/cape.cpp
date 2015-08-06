#include "cape.hpp"
#include "../openclrenderer/vertex.hpp"
#include "../openclrenderer/objects_container.hpp"
#include "../openclrenderer/texture.hpp"
#include "../openclrenderer/object.hpp"
#include "../openclrenderer/clstate.h"
#include "../openclrenderer/engine.hpp"
#include "../openclrenderer/obj_mem_manager.hpp"
#include "vec.hpp"
#include "physics.hpp"
#include "fighter.hpp"

void cape::load_cape(objects_container* pobj)
{
    constexpr int width = 10;
    constexpr int height = 30;

    vertex verts[height][width];

    const float separation = 10.f;

    for(int j=0; j<height; j++)
    {
        for(int i=0; i<width; i++)
        {
            float xpos = i * separation;
            float ypos = j * separation;
            float zpos = 0;

            verts[j][i].set_pos({xpos, ypos, zpos});
            verts[j][i].set_vt({(float)i/width, (float)j/height});
            verts[j][i].set_normal({0, 1, 0});
        }
    }

    std::vector<triangle> tris;
    tris.resize(width*height*2);

    for(int j=0; j<height-1; j++)
    {
        for(int i=0; i<width-1; i++)
        {
            triangle t;

            ///winding order is apparently clockwise. That or something else is wrong
            ///likely the latter
            t.vertices[0] = verts[j][i];
            t.vertices[1] = verts[j][i+1];
            t.vertices[2] = verts[j+1][i];

            tris.push_back(t);

            t.vertices[0] = verts[j][i+1];
            t.vertices[1] = verts[j+1][i+1];

            t.vertices[2] = verts[j+1][i];

            tris.push_back(t);
        }
    }

    texture tex;
    tex.type = 0;
    tex.set_texture_location("./res/red.png");
    tex.push();

    object obj;
    obj.tri_list = tris;

    obj.tri_num = obj.tri_list.size();

    obj.tid = tex.id;
    obj.bid = -1;

    obj.has_bump = 0;

    obj.pos = pobj->pos;
    obj.rot = pobj->rot;

    pobj->objs.push_back(obj);

    //pobj->set_rot({0, M_PI, 0});

    pobj->isloaded = true;
}

cape::cape()
{
    width = 10;
    height = 30;
    depth = 1;

    model = new objects_container;

    model->set_load_func(cape::load_cape);
    model->set_active(true);

    obj_mem_manager::load_active_objects();

    model->set_two_sided(true);
    model->set_specular(0);

    obj_mem_manager::g_arrange_mem();
    obj_mem_manager::g_changeover();

    which = 0;

    in = compute::buffer(cl::context, sizeof(float)*width*height*depth*3, CL_MEM_READ_WRITE, nullptr);
    out = compute::buffer(cl::context, sizeof(float)*width*height*depth*3, CL_MEM_READ_WRITE, nullptr);

    cl_float* inmap = (cl_float*) clEnqueueMapBuffer(cl::cqueue.get(), in.get(), CL_TRUE, CL_MAP_WRITE, 0, sizeof(cl_float)*width*height*depth*3, 0, NULL, NULL, NULL);
    cl_float* outmap = (cl_float*) clEnqueueMapBuffer(cl::cqueue.get(), out.get(), CL_TRUE, CL_MAP_WRITE, 0, sizeof(cl_float)*width*height*depth*3, 0, NULL, NULL, NULL);

    const float separation = 10.f;

    for(int j=0; j<height; j++)
    {
        for(int i=0; i<width; i++)
        {
            float xpos = i * separation;
            float ypos = j * separation;
            float zpos = 0;

            inmap[(i + j*width)*3 + 0] = xpos;
            inmap[(i + j*width)*3 + 1] = ypos;
            inmap[(i + j*width)*3 + 2] = zpos;

            outmap[(i + j*width)*3 + 0] = xpos;
            outmap[(i + j*width)*3 + 1] = ypos;
            outmap[(i + j*width)*3 + 2] = zpos;
        }
    }

    clEnqueueUnmapMemObject(cl::cqueue.get(), in.get(), inmap, 0, NULL, NULL);
    clEnqueueUnmapMemObject(cl::cqueue.get(), out.get(), outmap, 0, NULL, NULL);
}

void cape::make_stable(fighter* parent)
{
    int num = 100;

    for(int i=0; i<num; i++)
    {
        tick(parent->parts[bodypart::LUPPERARM].obj(),
                               parent->parts[bodypart::BODY].obj(),
                               parent->parts[bodypart::RUPPERARM].obj(),
                               parent
                                );
    }
}

compute::buffer cape::fighter_to_fixed(objects_container* l, objects_container* m, objects_container* r)
{
    vec3f position = xyz_to_vec(m->pos);
    vec3f rotation = xyz_to_vec(m->rot);

    vec3f lpos = xyz_to_vec(l->pos);
    vec3f rpos = xyz_to_vec(r->pos);

    bbox lbbox = get_bbox(l);
    bbox rbbox = get_bbox(r);

    float ldepth = lbbox.max.v[2] - lbbox.min.v[2];
    float rdepth = rbbox.max.v[2] - rbbox.min.v[2];

    lpos = lpos + (vec3f){0, 0, ldepth}.rot({0,0,0}, rotation);
    rpos = rpos + (vec3f){0, 0, rdepth}.rot({0,0,0}, rotation);

    vec3f dir = rpos - lpos;

    int len = width;

    vec3f step = dir / (float)len;

    vec3f current = lpos;

    /*cl_float* xmap = (cl_float*) clEnqueueMapBuffer(cl::cqueue.get(), defx.get(), CL_TRUE, CL_MAP_WRITE, 0, sizeof(cl_float)*w, 0, NULL, NULL, NULL);
    cl_float* ymap = (cl_float*) clEnqueueMapBuffer(cl::cqueue.get(), defy.get(), CL_TRUE, CL_MAP_WRITE, 0, sizeof(cl_float)*w, 0, NULL, NULL, NULL);
    cl_float* zmap = (cl_float*) clEnqueueMapBuffer(cl::cqueue.get(), defz.get(), CL_TRUE, CL_MAP_WRITE, 0, sizeof(cl_float)*w, 0, NULL, NULL, NULL);

    for(int i=0; i < len; i ++)
    {
        xmap[i] = current.v[0];
        ymap[i] = current.v[1];
        zmap[i] = current.v[2];

        current = current + step;
    }

    clEnqueueUnmapMemObject(cl::cqueue.get(), defx.get(), xmap, 0, NULL, NULL);
    clEnqueueUnmapMemObject(cl::cqueue.get(), defy.get(), ymap, 0, NULL, NULL);
    clEnqueueUnmapMemObject(cl::cqueue.get(), defz.get(), zmap, 0, NULL, NULL);*/

    compute::buffer buf = compute::buffer(cl::context, sizeof(float)*width*3, CL_MEM_READ_WRITE, nullptr);

    cl_float* mem_map = (cl_float*) clEnqueueMapBuffer(cl::cqueue.get(), buf.get(), CL_TRUE, CL_MAP_WRITE, 0, sizeof(cl_float)*width*3, 0, NULL, NULL, NULL);

    for(int i=0; i<len; i++)
    {
        mem_map[i*3 + 0] = current.v[0];
        mem_map[i*3 + 1] = current.v[1];
        mem_map[i*3 + 2] = current.v[2];


        current = current + step;
    }

    clEnqueueUnmapMemObject(cl::cqueue.get(), buf.get(), mem_map, 0, NULL, NULL);

    return buf;
}

compute::buffer body_to_gpu(fighter* parent)
{
    std::vector<cl_float4> pos;

    for(auto& i : parent->parts)
    {
        vec3f p = i.global_pos;

        pos.push_back({p.v[0], p.v[1], p.v[2]});
    }

    compute::buffer buf = compute::buffer(cl::context, sizeof(cl_float4)*pos.size(), CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, pos.data());

    return buf;
}

void cape::tick(objects_container* l, objects_container* m, objects_container* r, fighter* parent)
{
    auto buf = body_to_gpu(parent);
    int num = bodypart::COUNT;

    arg_list cloth_args;

    cloth_args.push_back(&obj_mem_manager::g_tri_mem);
    cloth_args.push_back(&model->objs[0].gpu_tri_start);
    cloth_args.push_back(&model->objs[0].gpu_tri_end);
    cloth_args.push_back(&width);
    cloth_args.push_back(&height);
    cloth_args.push_back(&depth);

    compute::buffer b1 = which == 0 ? in : out;
    compute::buffer b2 = which == 0 ? out : in;

    compute::buffer fixed = fighter_to_fixed(l, m, r);

    cloth_args.push_back(&b1);
    cloth_args.push_back(&b2);
    cloth_args.push_back(&fixed);
    cloth_args.push_back(&engine::g_screen);
    cloth_args.push_back(&buf);
    cloth_args.push_back(&num);

    cl_uint global_ws[1] = {width*height*depth};
    cl_uint local_ws[1] = {256};

    run_kernel_with_string("cloth_simulate", global_ws, local_ws, 1, cloth_args);

    which = (which + 1) % 2;
}
