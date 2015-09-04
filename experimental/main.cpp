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
    noisemod(mx, my, 0.f, 0.f, 0.00243, 10.5f);
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

std::vector<triangle> get_mountain()
{
    int sz = 1000;

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

    int width = 1000;
    int height = 1000;

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

            float centre_height = smoothnoise(x * 5.f, 0.f, 0.f, 0.f) * 5.f + xgauss;

            float yfrac = 1.f - fabs(y);

            int imin = -5;
            int imax = -0;

            ///tinker with this
            cl_float3 rval = y_of(i, j, 1.f, sz, sz, 5, tw1, tw2, tw3, imin, imax);

            float random = rval.z / 10.f;

            pos->v[0] = x * size;
            pos->v[1] = centre_height * yfrac * yfrac + random * 10.f;// + noisemult(x, y);// + fabs(x) * fabs(y) * val/;
            pos->v[2] = y * size;

            pos->v[1] *= 10;

            //pos->v[1] = ;//noisemult((float)i/width, (float)j/height) + val;

            //*pos = mult(*pos, 100.0f);

            *pos = *pos;
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

            //normal_accumulate[j*width + i].x += cr1.x;
            //normal_accumulate[j*width + i].y += cr1.y;
            //normal_accumulate[j*width + i].z += cr1.z;

            normal_accumulate[j*width + i] += cr1;
            normal_accumulate[j*width + i + 1] += cr1 + cr2;;

            normal_accumulate[(j+1)*width + i] += cr1 + cr2;;
            normal_accumulate[(j+1)*width + i + 1] += cr2;;

            /*normal_accumulate[j*width + i + 1].x += cr1.x + cr2.x;
            normal_accumulate[j*width + i + 1].y += cr1.y + cr2.y;
            normal_accumulate[j*width + i + 1].z += cr1.z + cr2.z;

            normal_accumulate[(j+1)*width + i].x += cr1.x + cr2.x;
            normal_accumulate[(j+1)*width + i].y += cr1.y + cr2.y;
            normal_accumulate[(j+1)*width + i].z += cr1.z + cr2.z;

            normal_accumulate[(j+1)*width + i + 1].x += cr2.x;
            normal_accumulate[(j+1)*width + i + 1].y += cr2.y;
            normal_accumulate[(j+1)*width + i + 1].z += cr2.z;*/
        }
    }

    srand(2);

    for(int j=0; j<height; j++)
    {
        for(int i=0; i<width; i++)
        {
            //normal_accumulate[j*width + i] = normalise(add(normal_accumulate[j*width + i], noisemod(i, j, 1, 1, 0.25, 4000)));

            //float norm_rand = noisemod(i, j, 1, 1, 0.25, 0.001);

            //if(rand() % 2)
            //    norm_rand = -norm_rand;

            normal_accumulate[j*width + i] = normal_accumulate[j*width + i].norm();// normalise(add(normalise(normal_accumulate[j*width + i]), norm_rand));
            //normal_accumulate[j*width + i] =  normalise(add(normalise(normal_accumulate[j*width + i]), norm_rand));
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

            tri.vertices[0].set_vt({0.1, 0.1});
            tri.vertices[1].set_vt({0.2, 0.5});
            tri.vertices[2].set_vt({0.5, 0.7});


            triangle tri2;
            tri2.vertices[0].set_pos(tr);
            tri2.vertices[1].set_pos(bl);
            tri2.vertices[2].set_pos(br);

            tri2.vertices[0].set_normal(ntr);
            tri2.vertices[1].set_normal(nbl);
            tri2.vertices[2].set_normal(nbr);


            tri2.vertices[0].set_vt({0.1, 0.1});
            tri2.vertices[1].set_vt({0.2, 0.5});
            tri2.vertices[2].set_vt({0.5, 0.7});


            tris.push_back(tri);
            tris.push_back(tri2);

        }
    }

    delete [] vertex_positions;
    delete [] normal_accumulate;

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

void load_mountain(objects_container* pobj)
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

    auto new_tris = get_mountain();

   // o.tri_list.push_back(t);
    o.tri_list.insert(o.tri_list.begin(), new_tris.begin(), new_tris.end());


    o.tri_num = o.tri_list.size();

    o.tid = tex.id;

    o.pos = pobj->pos;
    o.rot = pobj->rot;

    o.isloaded = true;

    pobj->objs.push_back(o);

    pobj->isloaded = true;
    pobj->set_two_sided(true);

    //tex.set_texture_location()
}

int main(int argc, char *argv[])
{
    objects_container floor;
    //floor.set_file("../openclrenderer/sp2/sp2.obj");
    floor.set_load_func(load_mountain);

    ///need to extend this to textures as well
    floor.cache = false;
    floor.set_active(true);

    engine window;
    window.load(800, 600,1000, "SwordFight", "../openclrenderer/cl2.cl", true);

    window.set_camera_pos({500, 200, -700});

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
    l.set_pos({5000, 10000, 4000, 0});

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

        window.input();

        window.draw_bulk_objs_n();

        window.render_buffers();

        window.display();

        std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
