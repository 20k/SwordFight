#include "../openclrenderer/proj.hpp"
#include "../openclrenderer/ocl.h"
#include "../openclrenderer/texture_manager.hpp"

#include "../openclrenderer/text_handler.hpp"
#include <sstream>
#include <string>
#include "../openclrenderer/vec.hpp"

#include "../openclrenderer/ui_manager.hpp"

#include "fighter.hpp"

#include "physics.hpp"

#include "../openclrenderer/network.hpp"

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

void debug_controls(fighter* my_fight, engine& window)
{
    sf::Keyboard key;

    window.input();

    if(once<sf::Keyboard::T>())
    {
        my_fight->queue_attack(attacks::OVERHEAD);
    }

    if(once<sf::Keyboard::Y>())
    {
        my_fight->queue_attack(attacks::SLASH);
    }

    if(once<sf::Keyboard::G>())
    {
        my_fight->queue_attack(attacks::REST);
    }

    if(once<sf::Keyboard::R>())
    {
        my_fight->queue_attack(attacks::BLOCK);
    }

    if(key.isKeyPressed(sf::Keyboard::U))
    {
        my_fight->rot.v[1] += 0.01f;
    }

    if(key.isKeyPressed(sf::Keyboard::O))
    {
        my_fight->rot.v[1] -= 0.01f;
    }

    static float look_height = 0.f;

    if(key.isKeyPressed(sf::Keyboard::Comma))
    {
        look_height += 0.01f;
    }

    if(key.isKeyPressed(sf::Keyboard::Period))
    {
        look_height += -0.01f;
    }

    my_fight->set_look({look_height, 0.f, 0.f});

    vec2f walk_dir = {0,0};

    if(key.isKeyPressed(sf::Keyboard::I))
        walk_dir.v[0] = -1;

    if(key.isKeyPressed(sf::Keyboard::K))
        walk_dir.v[0] = 1;

    if(key.isKeyPressed(sf::Keyboard::J))
        walk_dir.v[1] = -1;

    if(key.isKeyPressed(sf::Keyboard::L))
        walk_dir.v[1] = 1;

    my_fight->walk_dir(walk_dir);
}

void fps_controls(fighter* my_fight, engine& window)
{
    sf::Keyboard key;

    if(key.isKeyPressed(sf::Keyboard::Escape))
        exit(0);

    vec2f walk_dir = {0,0};

    if(key.isKeyPressed(sf::Keyboard::W))
        walk_dir.v[0] = -1;

    if(key.isKeyPressed(sf::Keyboard::S))
        walk_dir.v[0] = 1;

    if(key.isKeyPressed(sf::Keyboard::A))
        walk_dir.v[1] = -1;

    if(key.isKeyPressed(sf::Keyboard::D))
        walk_dir.v[1] = 1;

    my_fight->walk_dir(walk_dir);

    if(once<sf::Mouse::Left>())
        my_fight->queue_attack(attacks::SLASH);

    if(once<sf::Mouse::Middle>())
        my_fight->queue_attack(attacks::OVERHEAD);

    if(once<sf::Mouse::Right>())
        my_fight->queue_attack(attacks::BLOCK);

    //static vec3f old_rot = xyz_to_vec(window.c_rot);

    //vec3f diff = xyz_to_vec(window.c_rot) - old_rot;

    my_fight->set_look({-window.c_rot.s[0], 0, 0});


    part* head = &my_fight->parts[bodypart::HEAD];

    vec3f pos = head->pos + my_fight->pos;

    window.set_camera_pos({pos.v[0], pos.v[1], pos.v[2]});

    vec2f m;
    m.v[0] = window.get_mouse_delta_x();
    m.v[1] = window.get_mouse_delta_y();

    vec3f* c_rot = &my_fight->rot;

    c_rot->v[1] = c_rot->v[1] - m.v[0] / 100.f;

    vec3f o_rot = xyz_to_vec(window.c_rot);

    o_rot.v[1] = c_rot->v[1];
    o_rot.v[0] += m.v[1] / 200.f;

    window.set_camera_rot({o_rot.v[0], -o_rot.v[1] + M_PI, o_rot.v[2]});
}

int main(int argc, char *argv[])
{
    objects_container c1;
    c1.set_file("./Res/bodypart_red.obj");
    c1.set_pos({0, 0, 0});
    c1.set_active(true);

    engine window;
    window.load(1365,765,1000, "SwordFight", "../openclrenderer/cl2.cl");

    printf("loaded\n");

    window.set_camera_pos({-1009.17, -94.6033, -317.804});
    window.set_camera_rot({0, 1.6817, 0});

    fighter fight;
    fight.set_team(0);

    fighter fight2;
    fight2.set_team(1);
    fight2.set_pos({0, 0, -600});
    fight2.set_rot({0, M_PI, 0});

    physics phys;
    phys.load();

    obj_mem_manager::load_active_objects();

    fight.scale();
    fight2.scale();


    fight.set_physics(&phys);
    fight2.set_physics(&phys);


    c1.scale(0.001f);


    texture_manager::allocate_textures();

    obj_mem_manager::g_arrange_mem();
    obj_mem_manager::g_changeover();

    sf::Event Event;

    light l;
    l.set_col({1.0, 1.0, 1.0, 0});
    l.set_shadow_casting(0);
    l.set_brightness(1);
    l.set_pos({100, 350, -300, 0});
    l.shadow=0;

    window.add_light(&l);


    sf::Mouse mouse;
    sf::Keyboard key;

    vec3f original_pos = fight.parts[bodypart::LFOOT].pos;

    vec3f seek_pos = original_pos;


    vec3f rest_position = {0, -200, -100};

    fighter* my_fight = &fight;


    ///debug;
    int controls_state = 0;


    while(window.window.isOpen())
    {
        float ftime = window.get_frametime();

        sf::Clock c;

        if(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();
        }

        window.update_mouse();

        if(once<sf::Keyboard::X>())
        {
            controls_state = (controls_state + 1) % 2;
        }


        if(controls_state == 0)
            debug_controls(my_fight, window);
        if(controls_state == 1)
            fps_controls(my_fight, window);

        if(once<sf::Keyboard::V>() && network::network_state == 0)
        {
            network::join("192.168.1.55");

            for(auto& i : fight2.parts)
            {
                network::slave_object(&i.model);
            }

            network::slave_object(&fight2.weapon.model);

            network::slave_var(&fight2.net.is_blocking);


            for(auto& i : fight.parts)
            {
                network::host_object(&i.model);
            }

            network::host_object(&fight.weapon.model);

            network::host_var(&fight.net.is_blocking);

            fight.set_pos(fight2.pos);
            fight.set_rot(fight2.rot);
        }

        if(once<sf::Keyboard::C>() && network::network_state == 0)
        {
            network::host();

            for(auto& i : fight.parts)
            {
                network::host_object(&i.model);
            }

            network::host_object(&fight.weapon.model);

            network::host_var(&fight.net.is_blocking);

            for(auto& i : fight2.parts)
            {
                network::slave_object(&i.model);
            }

            network::slave_object(&fight2.weapon.model);

            network::slave_var(&fight2.net.is_blocking);
        }

        if(once<sf::Keyboard::B>())
        {
            my_fight->respawn();
        }

        //static float debug_look = 0;

        //my_fight->set_look({sin(debug_look), 0, 0});

        //debug_look += 0.1f;


        /*phys.tick();

        vec3f v = phys.get_pos();

        c1.set_pos({v.v[0], v.v[1], v.v[2]});
        c1.g_flush_objects();*/

        static int second_tick = 0;

        second_tick++;

        if(network::network_state == 0)
        {
            if(second_tick % 200)
            {
                fight2.queue_attack(attacks::SLASH);
            }

            //fight2.tick();

            fight2.update_render_positions();
        }

        fight.tick();

        bool need_realloc = network::tick();

        if(need_realloc)
        {
            printf("Reallocating\n");

            obj_mem_manager::load_active_objects();
            obj_mem_manager::g_arrange_mem();
            obj_mem_manager::g_changeover();
        }

        fight.update_render_positions();


        window.draw_bulk_objs_n();

        window.render_buffers();

        window.display();

        std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
