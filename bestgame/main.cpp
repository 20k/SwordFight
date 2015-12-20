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

#include "renderer.hpp"
#include "object.hpp"
#include "state.hpp"

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


struct w
{
    sf::RenderWindow window;
};

int main(int argc, char *argv[])
{
    engine window;
    window.load(1200, 800, 1000, "SwordFight", "../openclrenderer/cl2.cl", true);

    #define IN_2D
    #ifdef IN_2D
    PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
    PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
    PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
    PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
    PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
    PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");


    glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
    glBindRenderbufferEXT(GL_RENDERBUFFER, 0);

    compute::opengl_enqueue_release_gl_objects(1, &window.g_screen.get(), cl::cqueue);
    cl::cqueue.finish();
    window.window.resetGLStates();
    //window.window.setVerticalSyncEnabled(true);
    #endif

    //w window;
    //window.window.create(sf::VideoMode(800, 600), "test");

    #ifdef IN_3D
    object_context context;

    auto sponza = context.make_new();
    sponza->set_file("../openclrenderer/sp2/sp2.obj");
    sponza->set_active(true);
    sponza->cache = false;

    window.set_camera_pos({0, 300, -1000});

    context.load_active();

    sponza->set_specular(0.7f);

    texture_manager::allocate_textures();

    auto tex_gpu = texture_manager::build_descriptors();
    window.set_tex_data(tex_gpu);

    context.build();
    auto object_dat = context.fetch();
    window.set_object_data(*object_dat);


    sf::Event Event;

    light l;
    //l.set_col({1.0, 1.0, 1.0, 0});
    l.set_col({1.0, 1.0, 1.0, 0});
    l.set_shadow_casting(1);
    l.set_brightness(0.8f);
    l.set_diffuse(1.f);
    //l.set_pos({200, 1000, 0, 0});
    //l.set_pos({5000, 10000, 4000, 0});
    l.set_pos({-50, 1000, 0, 0});
    l.set_radius(100000.f);

    light::add_light(&l);

    auto light_data = light::build();

    window.set_light_data(light_data);
    window.construct_shadowmaps();

    printf("light\n");
    #endif

    float dt = 0;
    sf::Clock clk;

    renderer render_2d;

    state st;
    st.render_2d = &render_2d;
    st.win = &window.window;

    player* play = new player;
    play->set_pos({100, 100});
    play->set_dim({10, 10});
    play->set_team(team::FRIENDLY);

    character* hostile = new character;
    hostile->set_pos({200, 150});
    hostile->set_dim({10, 10});
    hostile->set_team(team::ENEMY);

    std::vector<game_entity*> entities;

    st.entities = &entities;

    entities.push_back(play);
    entities.push_back(hostile);

    vec2f mouse_pos = {0, 0};

    sf::Mouse mouse;

    sf::Event Event;

    while(window.window.isOpen())
    {
        sf::Clock c;

        while(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();
        }

        dt = clk.getElapsedTime().asMicroseconds() / 1000.f;
        clk.restart();

        mouse_pos.v[0] = mouse.getPosition(window.window).x;
        mouse_pos.v[1] = mouse.getPosition(window.window).y;

        /*window.draw_bulk_objs_n();

        window.render_block();
        window.display();*/

        for(int i=0; i<entities.size(); i++)
        {
            entities[i]->tick(st, dt);

            if(entities[i]->is_dead() || entities[i]->to_remove)
            {
                delete entities[i];

                entities.erase(entities.begin() + i);
                i--;
            }
        }

        if(mouse.isButtonPressed(sf::Mouse::Left))
        {
            vec2f dir = mouse_pos - play->pos;

            game_entity* en = play->fire(dir, 0.25f);

            if(en != nullptr)
                entities.push_back(en);
        }

        render_2d.tick(window.window);

        ///temp hack
        window.window.display();
        window.window.clear();

        std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
