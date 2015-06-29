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

int main(int argc, char *argv[])
{
    objects_container c1;
    c1.set_file("../openclrenderer/objects/cylinder.obj");
    c1.set_pos({0, 0, 0});
    c1.set_active(true);

    engine window;
    window.load(1680,1050,1000, "SwordFight", "../openclrenderer/cl2.cl");

    //window.set_camera_pos({0,-85,-1000,0});
    window.set_camera_pos({-1009.17, -94.6033, -317.804});
    window.set_camera_rot({0, 1.6817, 0});

    fighter fight;
    fight.load_files(0);

    fighter fight2;
    fight2.load_files(1);
    fight2.set_pos({0, 0, -600});
    fight2.set_rot({0, M_PI, 0});

    physics phys;
    phys.load();

    obj_mem_manager::load_active_objects();

    fight.scale();
    fight2.scale();

    c1.scale(0.001f);


    //phys.add_objects_container(&c1);
    //phys.add_objects_container(&c1);

    for(part& i : fight2.parts)
    {
        phys.add_objects_container(&i.model);
    }


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

    while(window.window.isOpen())
    {
        sf::Clock c;

        if(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();
        }

        window.input();

        //seek_pos.v[2] -= 0.1f;

        /*if(key.isKeyPressed(sf::Keyboard::O))
            seek_pos.v[1] += 0.5f;

        if(key.isKeyPressed(sf::Keyboard::U))
            seek_pos.v[1] -= 0.5f;

        if(key.isKeyPressed(sf::Keyboard::I))
            seek_pos.v[2] += 0.5f;

        if(key.isKeyPressed(sf::Keyboard::K))
            seek_pos.v[2] -= 0.5f;

        if(key.isKeyPressed(sf::Keyboard::J))
            seek_pos.v[0] += 0.5f;

        if(key.isKeyPressed(sf::Keyboard::L))
            seek_pos.v[0] -= 0.5f;

        fight.IK_foot(0, seek_pos);*/

        /*if(once<sf::Keyboard::G>())
        {
            fight.linear_move(0, seek_pos, 400);
        }

        if(once<sf::Keyboard::H>())
        {
            fight.linear_move(0, original_pos, 400);
        }

        if(once<sf::Keyboard::J>())
        {
            fight.spherical_move(0, seek_pos, 400);
        }

        if(once<sf::Keyboard::K>())
        {
            fight.spherical_move(0, original_pos, 400);
        }*/

        if(once<sf::Keyboard::T>())
        {
            fight.queue_attack(attacks::OVERHEAD);
        }

        if(once<sf::Keyboard::Y>())
        {
            fight.queue_attack(attacks::SLASH);
        }

        if(once<sf::Keyboard::G>())
        {
            fight.queue_attack(attacks::REST);
        }

        /*if(key.isKeyPressed(sf::Keyboard::I))
        {
            //fight.pos.v[2] -= 0.3f;
        }*/

        if(key.isKeyPressed(sf::Keyboard::Numpad4))
        {
            fight.rot.v[1] += 0.01f;
        }

        if(key.isKeyPressed(sf::Keyboard::Numpad6))
        {
            fight.rot.v[1] -= 0.01f;
        }

        fight.tick();


        if(once<sf::Keyboard::C>())
            fight.walk(0);

        if(once<sf::Keyboard::V>())
            fight.walk(1);

        vec2f walk_dir = {0,0};

        if(key.isKeyPressed(sf::Keyboard::I))
            walk_dir.v[0] = 1;

        if(key.isKeyPressed(sf::Keyboard::K))
            walk_dir.v[0] = -1;

        if(key.isKeyPressed(sf::Keyboard::J))
            walk_dir.v[1] = 1;

        if(key.isKeyPressed(sf::Keyboard::L))
            walk_dir.v[1] = -1;

        if(walk_dir.v[0] != 0 || walk_dir.v[1] != 0)
        {
            fight.walk_dir(walk_dir);
        }

        phys.tick();

        int collide_id = phys.sword_collides(fight.weapon);

        if(collide_id != -1)
            printf("%s %i\n", bodypart::names[collide_id].c_str(), collide_id);

        vec3f v = phys.get_pos();

        c1.set_pos({v.v[0], v.v[1], v.v[2]});
        c1.g_flush_objects();

        fight2.tick();

        fight2.update_render_positions();

        fight.update_render_positions();

        window.draw_bulk_objs_n();

        window.render_buffers();

        window.display();

        //std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
