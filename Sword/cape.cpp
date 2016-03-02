#include "cape.hpp"
#include "../openclrenderer/vertex.hpp"
#include "../openclrenderer/objects_container.hpp"
#include "../openclrenderer/texture.hpp"
#include "../openclrenderer/object.hpp"
#include "../openclrenderer/clstate.h"
#include "../openclrenderer/engine.hpp"
#include "../openclrenderer/obj_mem_manager.hpp"
#include <vec/vec.hpp>
#include "physics.hpp"
#include "fighter.hpp"
#include "../openclrenderer/vec.hpp"
#include "../sword_server/teaminfo_shared.hpp"

///10, 30
///8, 10
#define WIDTH 8
#define HEIGHT 10

void cape::load_cape(objects_container* pobj, int team)
{
    constexpr int width = WIDTH;
    constexpr int height = HEIGHT;

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
    //tris.resize(width*height*2);

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

    for(int i=0; i<width-1; i++)
    {
        triangle t;

        t.vertices[0] = verts[0][0];
        t.vertices[1] = verts[0][1];
        t.vertices[2] = verts[1][0];

        tris.push_back(t);

        t.vertices[0] = verts[0][1];
        t.vertices[1] = verts[1][1];
        t.vertices[2] = verts[1][0];

        tris.push_back(t);
    }



    texture_context* tex_ctx = &pobj->parent->tex_ctx;


    texture* tex;

    tex = tex_ctx->make_new_cached(team_info::get_texture_cache_name(team));

    vec3f col = team_info::get_team_col(team);

    tex->set_create_colour({col.v[0], col.v[1], col.v[2]}, 128, 128);


    cl_uint normal_id = -1;
    texture* normal;

    if(pobj->normal_map != "")
    {
        normal = tex_ctx->make_new_cached(pobj->normal_map.c_str());
        normal_id = normal->id;
    }

    object obj;
    obj.tri_list = tris;

    obj.tri_num = obj.tri_list.size();

    obj.tid = tex->id;
    obj.bid = -1;
    obj.rid = normal_id;

    obj.has_bump = 0;

    obj.pos = pobj->pos;
    obj.rot = pobj->rot;

    pobj->objs.push_back(obj);

    //pobj->set_rot({0, M_PI, 0});

    pobj->isloaded = true;
}

cape::cape(object_context& cpu, object_context_data& gpu)
{
    width = WIDTH;
    height = HEIGHT;
    depth = 1;

    loaded = false;

    cpu_context = &cpu;
    gpu_context = &gpu;

    death_height_offset = 0.f;

    hit_floor = false;
}

void cape::load(int team)
{
    saved_team = team;

    hit_floor = false;

    death_height_offset = 0.f;

    model = cpu_context->make_new();

    model->set_load_func(std::bind(cape::load_cape, std::placeholders::_1, team));
    model->set_active(true);
    model->cache = false; ///why?
    //model->set_normal("res/norm_body.png");

    //obj_mem_manager::load_active_objects();

    cpu_context->load_active();

    model->set_two_sided(true);
    model->set_specular(0.7);

    //obj_mem_manager::g_arrange_mem();
    //obj_mem_manager::g_changeover();

    cpu_context->build();
    gpu_context = cpu_context->fetch();

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

    loaded = true;

    context_id = cpu_context->get_context_id();
}

void cape::make_stable(fighter* parent)
{
    if(!loaded)
        return;

    int num = 100;

    for(int i=0; i<num; i++)
    {
        tick(parent);
    }
}

compute::buffer cape::fighter_to_fixed_vec(vec3f p1, vec3f p2, vec3f p3, vec3f rot)
{
    vec3f rotation = rot;

    vec3f diff = p3 - p1;

    float shrink = 0.12f;

    diff = diff * shrink;

    p3 = p3 - diff;
    p1 = p1 + diff;

    vec3f lpos = p1;
    vec3f rpos = p3;

    ///approximation
    ///could also use body scaling
    float ldepth = (p3 - p1).length() / 3.f;
    float rdepth = ldepth;
    ///we should move perpendicularly away, not zdistance away

    vec2f ldir = {p3.v[0], p3.v[2]};

    ldir = ldir - (vec2f){p1.v[0], p1.v[2]};

    vec2f perp = perpendicular(ldir.norm());

    vec3f perp3 = {perp.v[0], 0.f, perp.v[1]};

    lpos = lpos + perp3 * ldepth;
    rpos = rpos + perp3 * ldepth;

    lpos.v[1] += bodypart::scale / 4;
    rpos.v[1] += bodypart::scale / 4;

    ///dir could also just be (p3 - p1).rot ???
    vec3f dir = rpos - lpos;

    int len = width;

    vec3f step = dir / (float)(len - 1);

    vec3f current = lpos;

    compute::buffer buf = compute::buffer(cl::context, sizeof(float)*width*3, CL_MEM_READ_WRITE, nullptr);

    if(!cape_init)
    {
        gpu_cape.resize(width * 3);
        cape_init = true;
    }

    //cl_float* mem_map = (cl_float*) clEnqueueMapBuffer(cl::cqueue.get(), buf.get(), CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, 0, sizeof(cl_float)*width*3, 0, NULL, NULL, NULL);

    float sag = bodypart::scale/32.f;

    //sag = 0;

    for(int i=0; i<len; i++)
    {
        float xf = (float)i / len;

        float yval = 4 * xf * (xf - 1) * sag + sin(xf * 30);

        /*mem_map[i*3 + 0] = current.v[0];
        mem_map[i*3 + 1] = current.v[1] + yval;
        mem_map[i*3 + 2] = current.v[2];*/

        gpu_cape[i*3 + 0] = current.v[0];
        gpu_cape[i*3 + 1] = current.v[1] + yval;
        gpu_cape[i*3 + 2] = current.v[2];

        current = current + step;
    }

    clEnqueueWriteBuffer(cl::cqueue.get(), buf.get(), CL_FALSE, 0, sizeof(cl_float) * width * 3, gpu_cape.data(), 0, NULL, NULL);

    //clEnqueueUnmapMemObject(cl::cqueue.get(), buf.get(), mem_map, 0, NULL, NULL);

    return buf;
}

///use pcie instead etc
compute::buffer body_to_gpu(fighter* parent)
{
    std::vector<cl_float4> pos;

    for(auto& i : parent->parts)
    {
        vec3f p = i.global_pos;

        pos.push_back({p.v[0], p.v[1], p.v[2]});
    }

    vec3f half = (parent->parts[bodypart::LUPPERLEG].global_pos + parent->parts[bodypart::RUPPERLEG].global_pos)/2.f;
    vec3f half2 = (parent->parts[bodypart::LLOWERLEG].global_pos + parent->parts[bodypart::RLOWERLEG].global_pos)/2.f;

    vec3f half3 = (half + parent->parts[bodypart::BODY].global_pos)/2.f;

    pos.push_back({half.v[0], half.v[1], half.v[2]});
    pos.push_back({half2.v[0], half2.v[1], half2.v[2]});
    pos.push_back({half3.v[0], half3.v[1], half3.v[2]});

    compute::buffer buf = compute::buffer(cl::context, sizeof(cl_float4)*pos.size(), CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, pos.data());
    //compute::buffer buf = compute::buffer(cl::context, sizeof(cl_float4)*pos.size(), CL_MEM_READ_WRITE, nullptr);

    //clEnqueueWriteBuffer(cl::cqueue.get(), buf.get(), CL_FALSE, 0, sizeof(cl_float4)*pos.size(), pos.data(), 0, nullptr, nullptr);

    return buf;
}

struct wind
{
    cl_float4 dir = (cl_float4){0.f, 0.f, 0.f, 0.f};
    cl_float amount = 0;

    cl_float gust = 0;

    sf::Clock clk;

    compute::buffer tick()
    {
        dir = {1, 0, 0};

        float weight = 100.f;
        amount = (amount*weight + randf_s(0.f, 0.9f)) / (1.f + weight);

        float time = clk.getElapsedTime().asMicroseconds() / 1000.f;

        std::vector<cl_float4> accel;

        for(int i=0; i<WIDTH; i++)
        {
            vec3f dir = randf<3, float>(0.f, 1.f);

            dir = dir * dir * dir;

            dir = dir * amount;

            dir = dir * 5.f;

            cl_float4 ext = {dir.v[0], dir.v[1], dir.v[2]};

            accel.push_back(ext);
        }

        return compute::buffer(cl::context, sizeof(cl_float4)*accel.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, accel.data());
    }

    compute::buffer get_null()
    {
        std::vector<cl_float4> accel;

        for(int i=0; i<WIDTH; i++)
        {
            accel.push_back({1, 0, 1});
        }

        return compute::buffer(cl::context, sizeof(cl_float4)*accel.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, accel.data());
    }
};

///function now really messy
///shouldn't code while tired
void cape::tick(fighter* parent)
{
    if(!loaded || saved_team != parent->team || context_id != cpu_context->get_context_id())
    {
        load(parent->team);
        return;
    }

    if(!parent->dead())
    {
        death_height_offset = 0.f;
        hit_floor = false;
    }

    if(hit_floor && timeout_time.getElapsedTime().asSeconds() > 2)
    {
        return;
    }

    static wind w;
    compute::buffer wind_buf;

    if(!hit_floor)
        wind_buf = w.tick();
    else
        wind_buf = w.get_null();

    cl_float4 wind_dir = w.dir;
    cl_float wind_str = w.amount;
    //cl_float wind_side = w.gust;

    auto buf = body_to_gpu(parent);
    cl_int num = bodypart::COUNT + 3;

    //cl_float frametime = parent->frametime;

    cl_float frametime = frametime_clock.getElapsedTime().asMicroseconds() / 1000.f;
    frametime_clock.restart();

    ///its fine to do this
    ///as model gpu tri start/ends are only updated on a flip
    ///I'm not a retard! Hooray!
    gpu_context = cpu_context->get_current_gpu();

    arg_list cloth_args;

    cloth_args.push_back(&gpu_context->g_tri_mem);
    cloth_args.push_back(&model->objs[0].gpu_tri_start);
    cloth_args.push_back(&model->objs[0].gpu_tri_end);
    cloth_args.push_back(&width);
    cloth_args.push_back(&height);
    cloth_args.push_back(&depth);

    ///these are invalid on a context switch
    ///but irritatingly amd does not notice this
    compute::buffer b1 = which == 0 ? in : out;
    compute::buffer b2 = which == 0 ? out : in;

    vec3f lp = parent->parts[bodypart::LUPPERARM].global_pos;
    vec3f mp = parent->parts[bodypart::BODY].global_pos;
    vec3f rp = parent->parts[bodypart::RUPPERARM].global_pos;

    lp.v[1] += death_height_offset;
    mp.v[1] += death_height_offset;
    rp.v[1] += death_height_offset;

    if(parent->dead())
    {
        death_height_offset -= 4.f;
    }

    cl_float floor_min = FLOOR_CONST + 5.f;

    if(lp.v[1] <= floor_min)
    {
        if(!hit_floor)
            timeout_time.restart();

        hit_floor = true;
    }

    lp.v[1] = std::max(lp.v[1], floor_min);
    mp.v[1] = std::max(mp.v[1], floor_min);
    rp.v[1] = std::max(rp.v[1], floor_min);

    vec3f grot = parent->rot;

    compute::buffer fixed = fighter_to_fixed_vec(lp, mp, rp, grot);

    cl_float floor_const = floor_min;

    cloth_args.push_back(&b1);
    cloth_args.push_back(&b2);
    cloth_args.push_back(&fixed);
    cloth_args.push_back(&engine::g_screen);
    cloth_args.push_back(&buf);
    cloth_args.push_back(&num);
    cloth_args.push_back(&wind_dir);
    cloth_args.push_back(&wind_str);
    cloth_args.push_back(&wind_buf);
    cloth_args.push_back(&floor_const);
    cloth_args.push_back(&frametime);

    cl_uint global_ws[1] = {width*height*depth};
    cl_uint local_ws[1] = {256};

    run_kernel_with_string("cloth_simulate", global_ws, local_ws, 1, cloth_args, cl::cqueue);

    //cl::cqueue2.finish();

    which = (which + 1) % 2;
}
