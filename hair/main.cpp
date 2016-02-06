#include "../openclrenderer/proj.hpp"

#include <vec/vec.hpp>

#include <boost/compute/core.hpp>

///todo eventually
///split into dynamic and static objects

///todo
///fix memory management to not be atrocious

///we're completely hampered by memory latency

///rift head movement is wrong

void hair_load(objects_container* pobj)
{
    texture tex;
    tex.type = 0;
    tex.set_texture_location("res/red.png");
    tex.push();

    object obj;
    obj.two_sided = 1;

    obj.tid = tex.id;

    obj.isloaded = true;

    int segments = 10;
    float length = 100;

    float scale = length / segments;
    float width = 5.f; ///ie offset from centre

    std::vector<triangle> tri_list;

    for(int i=0; i<segments; i++)
    {
        triangle tri;

        for(int i=0; i<3; i++)
            tri.vertices[i].set_normal({1, 1, 0});

        for(int i=0; i<3; i++)
            tri.vertices[i].set_vt({0.3f, 0.5f});

        triangle tri2;

        for(int i=0; i<3; i++)
            tri2.vertices[i].set_normal({1, 1, 0});

        for(int i=0; i<3; i++)
            tri2.vertices[i].set_vt({0.3f, 0.5f});

        quad q;
        q.p1 = { i * scale, width, 0.f};
        q.p2 = { i * scale, -width, 0.f};
        q.p3 = { (i + 1) * scale, width, 0.f};
        q.p4 = { (i + 1) * scale, -width, 0.f};

        std::array<cl_float4, 6> pos = q.decompose();

        tri.vertices[0].set_pos(pos[0]);
        tri.vertices[1].set_pos(pos[1]);
        tri.vertices[2].set_pos(pos[2]);

        tri2.vertices[0].set_pos(pos[3]);
        tri2.vertices[1].set_pos(pos[4]);
        tri2.vertices[2].set_pos(pos[5]);

        tri_list.push_back(tri);
        tri_list.push_back(tri2);
    }

    obj.tri_list = tri_list;
    obj.tri_num = tri_list.size();

    obj.isloaded = true;

    pobj->objs.push_back(obj);

    pobj->isloaded = true;
}

template<typename T>
struct double_buf
{
    int n = 0;
    T bufs[2];

    void flip()
    {
        n = (n + 1) % 2;
    }

    T cur()
    {
        return bufs[n];
    }

    T next()
    {
        return bufs[(n + 1) % 2];
    }

    T old()
    {
        return next();
    }

    T operator[](int i) const
    {
        return bufs[i];
    }

    T& operator[](int i)
    {
        return bufs[i];
    }
};

///the stall may be because the cape functions write to tris
///this may leave a hole
///although i don't know why it would
///investigate, with stringy bits!
///also, stringy bits needs to be generic
///able to deform dynamic objects
///ie proper bone stuff
///ie we take a model of length x, then deform along a bone based on the bone
///we want thick dwarven tassles
namespace compute = boost::compute;

struct hair
{
    float len = 100;
    int segments = 10;

    double_buf<compute::buffer> seg_buf;

    void init()
    {
        float scale = len / segments;

        ///i literally cant see any reason why this wouldn't work
        ///everywhere else on the face of the planet i'm using arrays of cl_float
        std::vector< vec<3, cl_float> > to_init;

        for(int i=0; i<segments; i++)
        {
            to_init.push_back({segments * scale, 0.f, 0.f});
        }

        for(int i=0; i<2; i++)
            seg_buf[i] = compute::buffer(cl::context, segments * sizeof(to_init.front()), CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, to_init.data());
    }

    void tick()
    {

    }
};

///gamma correct mipmap filtering
///7ish pre tile deferred
int main(int argc, char *argv[])
{
    sf::Clock load_time;

    object_context context;

    objects_container* hair = context.make_new();
    //hair->set_file("../openclrenderer/sp2/sp2.obj");
    hair->set_load_func(hair_load);
    hair->set_active(true);
    hair->cache = false;

    engine window;

    window.load(1680,1050,1000, "turtles", "../openclrenderer/cl2.cl", true);

    window.set_camera_pos((cl_float4){-0,0,-0});

    //window.window.setPosition({-20, -20});

    //#ifdef OCULUS
    //window.window.setVerticalSyncEnabled(true);
    //#endif

    ///we need a context.unload_inactive
    context.load_active();

    //hair->scale(100.f);
    hair->set_pos({0,0,100});

    texture_manager::allocate_textures();

    auto tex_gpu = texture_manager::build_descriptors();
    window.set_tex_data(tex_gpu);

    context.build();
    auto object_dat = context.fetch();
    window.set_object_data(*object_dat);

    sf::Event Event;

    light l;
    l.set_col((cl_float4){1.0f, 1.0f, 1.0f, 0.0f});
    l.set_shadow_casting(0);
    l.set_brightness(1);
    l.radius = 100000;
    l.set_pos((cl_float4){-200, 500, -100, 0});
    //window.add_light(&l);

    light::add_light(&l);

    l.set_col((cl_float4){0.0f, 0.0f, 1.0f, 0});

    l.set_pos((cl_float4){-0, 200, -500, 0});
    l.set_shadow_casting(0);
    l.radius = 100000;

    light::add_light(&l);

    auto light_data = light::build();
    ///

    window.set_light_data(light_data);
    window.construct_shadowmaps();

    /*sf::Texture updated_tex;
    updated_tex.loadFromFile("../openclrenderer/Res/test.png");

    for(auto& i : sponza->objs)
    {
        texture* tex = i.get_texture();

        if(tex->c_image.getSize() == updated_tex.getSize())
        {
            tex->update_gpu_texture(updated_tex, tex_gpu);
        }
    }*/


    sf::Keyboard key;

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

        ///do manual async on thread
        auto event = window.draw_bulk_objs_n();

        window.set_render_event(event);

        window.blit_to_screen();

        window.flip();

        window.render_block();

        if(key.isKeyPressed(sf::Keyboard::M))
            std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }

    ///if we're doing async rendering on the main thread, then this is necessary
    cl::cqueue.finish();
    glFinish();
}
