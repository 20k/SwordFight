#include "../openclrenderer/proj.hpp"
#include "../openclrenderer/ocl.h"
#include "../openclrenderer/texture_manager.hpp"

#include "../openclrenderer/text_handler.hpp"
#include <sstream>
#include <string>
#include "../openclrenderer/vec.hpp"

#include "../openclrenderer/ui_manager.hpp"

#include "../openclrenderer/network.hpp"

#include "../openclrenderer/settings_loader.hpp"
#include "../sword/vec.hpp" ///really need to figure out where to put this
#include "../openclrenderer/planet_gen/planet_gen.hpp"
#include "../openclrenderer/smoke.hpp"

///has the button been pressed once, and only once
template<sf::Keyboard::Key k>
bool once()
{
    static bool last;

    sf::Keyboard key;

    if(key.isKeyPressed(k) && !last)
    {
        last = true;

        return true;
    }

    if(!key.isKeyPressed(k))
    {
        last = false;
    }

    return false;
}

template<sf::Mouse::Button b>
bool once()
{
    static bool last;

    sf::Mouse m;

    if(m.isButtonPressed(b) && !last)
    {
        last = true;

        return true;
    }

    if(!m.isButtonPressed(b))
    {
        last = false;
    }

    return false;
}


inline float cosine_interpolate(float a, float b, float x)
{
    float ft = x * M_PI;
    float f = ((1.0 - cos(ft)) * 0.5);

    return  (a*(1.0-f) + b*f);
    //return linear_interpolate(a, b, x);
}

inline float linear_interpolate(float a, float b, float x)
{
    return  (a*(1.0-x) + b*x);
    //return cosine_interpolate(a, b, x);
}

inline float interpolate(float a, float b, float x)
{
    //return linear_interpolate(a, b, x);
    return cosine_interpolate(a, b, x);
}


inline float noise1(int x)
{
    x = (x<<13) ^ x;
    return ( 1.0 - (float)( (x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

inline float noise(int x, int y, int z, int w)
{
    int n=x*271 + y*1999 + 3631*z + w*7919;

    n=(n<<13)^n;

    int nn=(n*(n*n*41333 +53307781)+1376312589)&0x7fffffff;

    return ((1.0-((float)nn/1073741824.0)));// + noise1(x) + noise1(y) + noise1(z) + noise1(w))/5.0;
}

inline float smoothnoise(float fx, float fy, float fz, float fw)
{
    int x, y, z, w;
    x = fx, y = fy, z = fz, w = fw;

    float V1=noise(x, y, z, w);

    float V2=noise(x+1,y, z, w);

    float V3=noise(x, y+1, z, w);

    float V4=noise(x+1,y+1, z, w);


    float V1l=noise(x, y, z+1, w);

    float V2l=noise(x+1,y, z+1, w);

    float V3l=noise(x, y+1, z+1, w);

    float V4l=noise(x+1,y+1, z+1, w);



    float I1=interpolate(V1, V2, fx-int(fx));

    float I2=interpolate(V3, V4, fx-int(fx));

    float I3=interpolate(I1, I2, fy-int(fy));


    float I1l=interpolate(V1l, V2l, fx-int(fx));

    float I2l=interpolate(V3l, V4l, fx-int(fx));

    float I3l=interpolate(I1l, I2l, fy-int(fy));

    float I4=interpolate(I3l, I3, fz-int(fz)); ///should be 1.f - (fz - int(fz)) methinks

    return I4;
}

inline float noisemod(float x, float y, float z, float w, float ocf, float amp)
{
    return smoothnoise(x*ocf, y*ocf, z*ocf, w)*amp;
}

inline float noisemult_old(int x, int y, int z)
{
    //return noisemod(x, y, z, 4.03, 0.25) + noisemod(x, y, z, 1.96, 0.5) + noisemod(x, y, z, 1.01, 1.00) + noisemod(x, y, z, 0.485, 2.00) + noisemod(x, y, z, 0.184, 4.01) + noisemod(x, y, z, 0.067, 8.23) + noisemod(x, y, z, 0.0343, 14.00);

    float mx, my, mz;

    /*float mwx = noisemod(x, y, z, 0, 0.048, 2) + noisemod(x, y, z, 1, 0.15, 0.8);
    float mwy = noisemod(x, y, z, 8, 0.048, 2) + noisemod(x, y, z, 15, 0.15, 0.8);
    float mwz = noisemod(x, y, z, 2, 0.048, 2) + noisemod(x, y, z, 32, 0.15, 0.8);*/

    float mwx = noisemod(x, y, z, 0, 0.0025, 2);
    float mwy = noisemod(x, y, z, 4, 0.0025, 2);
    float mwz = noisemod(x, y, z, 8, 0.0025, 2);

    mx = x + mwx*2;
    my = y + mwy*2;
    mz = z + mwz*2;

    return noisemod(mx, my, mz, 0, 0.174, 4.01) + noisemod(mx, my, mz, 0, 0.067, 4.23); + noisemod(mx, my, mz, 0, 0.0243, 4.00);
    //return noisemod(mx, my, mz, 0, 0.067, 8.23) + noisemod(mx, my, mz, 0, 0.0383, 14.00);
    //return noisemod(mx, my, mz, 0, 0.0383, 14.00);
}

///make this resolution independent
inline float noisemult(float x, float y)
{
    int internal = 1000;

    x = x * internal;
    y = y * internal;

    float mx, my, mz;

    /*float mwx = noisemod(x, y, z, 0, 0.048, 2) + noisemod(x, y, z, 1, 0.15, 0.8);
    float mwy = noisemod(x, y, z, 8, 0.048, 2) + noisemod(x, y, z, 15, 0.15, 0.8);
    float mwz = noisemod(x, y, z, 2, 0.048, 2) + noisemod(x, y, z, 32, 0.15, 0.8);*/

    float mwx = noisemod(x, y, 0, 0, 0.0025, 2);
    float mwy = noisemod(x, y, 0, 4, 0.0025, 2);
    //float mwz = noisemod(x, y, 0, 8, 0.0025, 2);

    mx = x + mwx*2;
    my = y + mwy*2;
    //mz = z + mwz*2;

    return noisemod(mx, my, 0, 0, 0.174, 0.1) + noisemod(mx, my, 0, 0, 0.067, 0.5) + noisemod(mx, my, 0, 0, 0.0243, 0.5f) +
    noisemod(mx, my, 0.f, 0.f, 0.00243, 1.5f);
    //return noisemod(x, y, 0.f, 0, 0.1, 4.f) +
    //noisemod(x, y, 0.f, 0, 1.f, 1.f);
}


inline float sample_noisefield(float fx, float fy, float fz, float*** nf)
{
    int x = fx;
    int y = fy;
    int z = fz;

    if(x < 0)
        x = 0;
    if(y < 0)
        y = 0;
    if(z < 0)
        z = 0;

    float V1=nf[x][y][z];

    float V2=nf[x+1][y][z];

    float V3=nf[x][y+1][z];

    float V4=nf[x+1][y+1][z];


    float V1l=nf[x][y][z+1];

    float V2l=nf[x+1][y][z+1];

    float V3l=nf[x][y+1][z+1];

    float V4l=nf[x+1][y+1][z+1];



    float I1=interpolate(V1, V2, fx-int(fx));

    float I2=interpolate(V3, V4, fx-int(fx));

    float I3=interpolate(I1, I2, fy-int(fy));


    float I1l=interpolate(V1l, V2l, fx-int(fx));

    float I2l=interpolate(V3l, V4l, fx-int(fx));

    float I3l=interpolate(I1l, I2l, fy-int(fy));

    float I4=interpolate(I3, I3l, fz-int(fz));

    return I4;
}

inline float sample_noisefield_arr(float p[3], float*** nf)
{
    return sample_noisefield(p[0], p[1], p[2], nf);
}

cl_float4* get_y_array(int width, int height, int sz)
{
    float* tw1, *tw2, *tw3;

    ///needs to be nw, nh, nd
    tw1 = new float[sz*sz*5];
    tw2 = new float[sz*sz*5];
    tw3 = new float[sz*sz*5];

    for(unsigned int i = 0; i<sz*sz*5; i++)
    {
        tw1[i] = (float)rand() / RAND_MAX;
        tw2[i] = (float)rand() / RAND_MAX;
        tw3[i] = (float)rand() / RAND_MAX;
    }

    cl_float4* arr = new cl_float4[width*height];

    for(int j=0; j<height; j++)
    {
        for(int i=0; i<width; i++)
        {
            int imin = -5;
            int imax = -0;

            cl_float3 ret = y_of(i + 100, j + 100, 1.f, sz, sz, 5, tw1, tw2, tw3, imin, imax);

            arr[j*width + i] = ret;
        }
    }

    delete [] tw1;
    delete [] tw2;
    delete [] tw3;

    return arr;
}

cl_float* get_smoothnoise_array(int len, int width)
{
    cl_float* arr = new cl_float[width];

    for(int i=0; i<width; i++)
    {
        float x = (i - width/2.f) / (width/2.f);

        float nv = smoothnoise((x+1)*5.f, 0.f, 0.f, 0.f);

        arr[i] = nv;
    }

    return arr;
}

compute::buffer g_y_array(int width, int height, int sz)
{
    cl_float4* arr = get_y_array(width, height, sz);

    auto buf = engine::make_read_write(width*height*sizeof(cl_float4), arr);

    delete [] arr;

    return buf;
}

compute::buffer g_sm_array(int len, int width)
{
    cl_float* arr = get_smoothnoise_array(len, width);

    auto buf = engine::make_read_write(len*sizeof(cl_float), arr);

    delete [] arr;

    return buf;
}

std::vector<triangle> get_mountain_old(int width, int height);

std::vector<triangle> reserve_cpu_space(int width, int height)
{
    return get_mountain_old(width, height); ///doesnt really matter anymore
}

std::vector<triangle> get_mountain_old(int width, int height)
{
    cl_float4* y_array = get_y_array(width+1, height+1, 100);
    cl_float* sm_array = get_smoothnoise_array(width+1, width);

    vec3f* vertex_positions = new vec3f[(width+1)*(height+1)];
    vec3f* normal_accumulate = new vec3f[(width+1)*(height+1)];

    memset(normal_accumulate, 0, sizeof(vec3f)*(width+1)*(height+1));

    int size = 500;

    for(int j=0; j<height+1; j++)
    {
        for(int i=0; i<width+1; i++)
        {
            vec3f* pos = &vertex_positions[j*width + i];

            float x = (i - width/2) / (width/2.f);
            float y = (j - height/2) / (width/2.f);

            float spr = 2.f;

            float val = 60.f * exp(- ((x*x) / (2*spr*spr) + (y*y) / (2*spr*spr)));

            float xgauss = 60.f * exp(- x*x / (2*spr*spr));

            //float centre_height = smoothnoise((x + 1) * 5.f, 0.f, 0.f, 0.f) * 5.f + xgauss;

            float centre_height = sm_array[i] * 5.f + xgauss;

            float yfrac = 1.f - fabs(y);

            //int imin = -5;
            //int imax = -0;

            ///tinker with this
            cl_float3 rval = y_array[j*width + i];

            float random = rval.z;

            pos->v[0] = x * size;
            pos->v[1] = centre_height * yfrac * yfrac + random;// + noisemult(x, y);// + fabs(x) * fabs(y) * val/;
            pos->v[2] = y * size;

            pos->v[1] *= 10;

            *pos = *pos / 10.f;
        }
    }

    for(int j=0; j<height-1; j++)
    {
        for(int i=0; i<width-1; i++)
        {
            vec3f tl = vertex_positions[j*width + i];
            vec3f tr = vertex_positions[j*width + i + 1];
            vec3f bl = vertex_positions[(j+1)*width + i];
            vec3f br = vertex_positions[(j+1)*width + i + 1];

            vec3f p1p0;
            p1p0 = tl - tr;

            vec3f p2p0;
            p2p0 = bl - tr;

            vec3f cr1 = cross(p1p0, p2p0);

            p1p0 = bl - tr;
            p2p0 = br - tr;

            vec3f cr2 = cross(p1p0, p2p0);

            cr1 = cr1.norm();
            cr2 = cr2.norm();

            //do both normals

            normal_accumulate[j*width + i] += cr1;
            normal_accumulate[j*width + i + 1] += cr1 + cr2;;

            normal_accumulate[(j+1)*width + i] += cr1 + cr2;;
            normal_accumulate[(j+1)*width + i + 1] += cr2;;
        }
    }

    srand(2);

    for(int j=0; j<height; j++)
    {
        for(int i=0; i<width; i++)
        {
            normal_accumulate[j*width + i] = normal_accumulate[j*width + i].norm();// normalise(add(normalise(normal_accumulate[j*width + i]), norm_rand));
        }
    }

    std::vector<triangle> tris;

    for(int j=0; j<height-1; j++)
    {
        for(int i=0; i<width-1; i++)
        {
            cl_float4 tl = V3to4(vertex_positions[j*width + i]);
            cl_float4 tr = V3to4(vertex_positions[j*width + i + 1]);
            cl_float4 bl = V3to4(vertex_positions[(j+1)*width + i]);
            cl_float4 br = V3to4(vertex_positions[(j+1)*width + i + 1]);

            cl_float4 ntl = V3to4(normal_accumulate[j*width + i]);
            cl_float4 ntr = V3to4(normal_accumulate[j*width + i + 1]);
            cl_float4 nbl = V3to4(normal_accumulate[(j+1)*width + i]);
            cl_float4 nbr = V3to4(normal_accumulate[(j+1)*width + i + 1]);

            triangle tri;
            tri.vertices[0].set_pos(tl);
            tri.vertices[1].set_pos(bl);
            tri.vertices[2].set_pos(tr);

            tri.vertices[0].set_normal(ntl);
            tri.vertices[1].set_normal(nbl);
            tri.vertices[2].set_normal(ntr);

            ///do this using i and j instead
            tri.vertices[0].set_vt({tl.x/width, tl.z/height});
            tri.vertices[1].set_vt({bl.x/width, bl.z/height});
            tri.vertices[2].set_vt({tr.x/width, tr.z/height});


            triangle tri2;
            tri2.vertices[0].set_pos(tr);
            tri2.vertices[1].set_pos(bl);
            tri2.vertices[2].set_pos(br);

            tri2.vertices[0].set_normal(ntr);
            tri2.vertices[1].set_normal(nbl);
            tri2.vertices[2].set_normal(nbr);

            tri2.vertices[0].set_vt({tr.x/width, tr.z/height});
            tri2.vertices[1].set_vt({bl.x/width, bl.z/height});
            tri2.vertices[2].set_vt({br.x/width, br.z/height});

            //tri2.vertices[0].set_vt({0.1, 0.1});
            //tri2.vertices[1].set_vt({0.2, 0.5});
            //tri2.vertices[2].set_vt({0.5, 0.7});


            tris.push_back(tri);
            tris.push_back(tri2);

        }
    }

    delete [] vertex_positions;
    delete [] normal_accumulate;

    delete [] y_array;
    delete [] sm_array;

    return tris;
}

void load_template(objects_container* pobj)
{
    if(!pobj)
    {
        std::cout << "something has gone horribly wrong" << std::endl;
        exit(0xDEAD);
    }

    texture tex;
    tex.type = 0;
    tex.set_texture_location("./fname.png");
    tex.push();

    object o;

    ///tinker with tri_list
    ///set tri_num; TODO REMOVE THIS

    o.tid = tex.id;

    o.pos = pobj->pos;
    o.rot = pobj->rot;

    o.isloaded = true;

    pobj->objs.push_back(o);

    pobj->isloaded = true;
}

void mountain_texture_generate(texture* tex)
{
    tex->c_image.create(128, 128, sf::Color(255, 255, 255));
    tex->is_loaded = true;
    tex->cacheable = false;
}

void load_mountain(objects_container* pobj, int width, int height)
{
    texture tex;
    tex.type = 0;
    tex.set_load_func(mountain_texture_generate);
    tex.push();

    triangle t;

    vertex v[3];
    v[0].set_pos({100.f, 0.f, 200.f});
    v[1].set_pos({0.f, 0.f, 200.f});
    v[2].set_pos({100.f, 100.f, 200.f});

    t.vertices[0] = v[0];
    t.vertices[1] = v[1];
    t.vertices[2] = v[2];


    object o;

    auto new_tris = reserve_cpu_space(width, height);

   // o.tri_list.push_back(t);
    o.tri_list.insert(o.tri_list.begin(), new_tris.begin(), new_tris.end());

    o.tri_num = o.tri_list.size();

    o.tid = tex.id;

    o.pos = pobj->pos;
    o.rot = pobj->rot;

    o.isloaded = true;

    pobj->objs.push_back(o);

    pobj->isloaded = true;
    pobj->set_two_sided(false);
}

int main(int argc, char *argv[])
{
    engine window;
    window.load(1200, 800, 1000, "SwordFight", "../openclrenderer/cl2.cl", true);

    objects_container floor;

    int width = 1000, height = 1000;

    floor.set_load_func(std::bind(load_mountain, std::placeholders::_1, width+1, height+1));

    floor.cache = false;
    floor.set_active(true);

    auto g_y_buf = g_y_array(width+1, height+1, 100);
    auto g_sm_buf = g_sm_array(width+1, width);
    auto g_hmap = engine::make_read_write((width+1)*(height+1)*sizeof(cl_float), nullptr);

    auto g_dmap = engine::make_read_write(engine::width*engine::height*sizeof(cl_float), nullptr);
    auto g_dmap2 = engine::make_read_write(engine::width*engine::height*sizeof(cl_float), nullptr);

    auto txo = engine::make_read_write((width+1)*(height+1)*sizeof(cl_float)*6, nullptr);
    auto tyo = engine::make_read_write((width+1)*(height+1)*sizeof(cl_float)*6, nullptr);
    auto tzo = engine::make_read_write((width+1)*(height+1)*sizeof(cl_float)*6, nullptr);


    window.set_camera_pos({0, 300, -1000});

    printf("loaded\n");

    //window.set_camera_pos({-1009.17, -94.6033, -317.804});
    //window.set_camera_rot({0, 1.6817, 0});

    printf("preload\n");

    obj_mem_manager::load_active_objects();

    printf("postload\n");

    floor.set_specular(0.0f);
    floor.set_diffuse(1.f);

    texture_manager::allocate_textures();

    printf("textures\n");

    obj_mem_manager::g_arrange_mem();
    obj_mem_manager::g_changeover(true);

    printf("loaded memory\n");

    sf::Event Event;

    light l;
    //l.set_col({1.0, 1.0, 1.0, 0});
    l.set_col({1.0, 1.0, 1.0, 0});
    l.set_shadow_casting(0);
    l.set_brightness(0.8f);
    l.set_diffuse(1.f);
    //l.set_pos({200, 1000, 0, 0});
    //l.set_pos({5000, 10000, 4000, 0});
    l.set_pos({-50, 1000, 0, 0});

    window.add_light(&l);

    printf("light\n");

    while(window.window.isOpen())
    {
        sf::Clock c;

        if(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();
        }

        cl_uint h_size[] = {width+1, height+1};

        window.input();

        //window.draw_bulk_objs_n();

        arg_list hmap_args;
        hmap_args.push_back(&width);
        hmap_args.push_back(&height);
        hmap_args.push_back(&g_hmap);
        hmap_args.push_back(&g_y_buf);
        hmap_args.push_back(&g_sm_buf);
        hmap_args.push_back(&engine::c_pos);
        hmap_args.push_back(&engine::c_rot);

        run_kernel_with_string("generate_heightmap", {width+1, height+1}, {16, 16}, 2, hmap_args);


        arg_list clear_args;
        clear_args.push_back(&engine::g_screen);

        run_kernel_with_string("clear_screen_tex", {engine::width, engine::height}, {16, 16}, 2, clear_args);


        arg_list db_args;
        db_args.push_back(&g_dmap);

        run_kernel_with_string("clear_depth_buffer", {engine::width, engine::height}, {16, 16}, 2, db_args);

        arg_list db_args2;
        db_args2.push_back(&g_dmap2);

        run_kernel_with_string("clear_depth_buffer", {engine::width, engine::height}, {16, 16}, 2, db_args2);


        arg_list rh_args;
        rh_args.push_back(&width);
        rh_args.push_back(&height);
        rh_args.push_back(&g_hmap);
        rh_args.push_back(&g_dmap);
        rh_args.push_back(&engine::c_pos);
        rh_args.push_back(&engine::c_rot);
        rh_args.push_back(&engine::g_screen);

        run_kernel_with_string("render_heightmap", {width, height}, {16, 16}, 2, rh_args);

        for(int i=0; i<1; i++)
        {
            compute::buffer args[2];

            if(i % 2 == 0)
            {
                args[0] = g_dmap;
                args[1] = g_dmap2;
            }
            else
            {
                args[0] = g_dmap2;
                args[1] = g_dmap;
            }

            arg_list blur_args;
            blur_args.push_back(&args[0]);
            blur_args.push_back(&args[1]);
            blur_args.push_back(&engine::width);
            blur_args.push_back(&engine::height);

            run_kernel_with_string("blur_depthmap", {engine::width, engine::height}, {16, 16}, 2, blur_args);
        }

        rh_args.args[3] = &g_dmap2;

        run_kernel_with_string("render_heightmap_p2", {engine::width, engine::height}, {16, 16}, 2, rh_args);

        /*arg_list triangleize_args;
        triangleize_args.push_back(&width);
        triangleize_args.push_back(&height);
        triangleize_args.push_back(&g_hmap);
        triangleize_args.push_back(&g_y_buf);
        triangleize_args.push_back(&g_sm_buf);
        triangleize_args.push_back(&engine::c_pos);
        triangleize_args.push_back(&engine::c_rot);
        triangleize_args.push_back(&floor.objs[0].gpu_tri_start);
        triangleize_args.push_back(&floor.objs[0].gpu_tri_end);
        triangleize_args.push_back(&obj_mem_manager::g_tri_mem);

        run_kernel_with_string("triangleize_grid", {width, height}, {16, 16}, 2, triangleize_args);*/

        window.render_buffers();

        window.display();

        std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
