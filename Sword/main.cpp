#include "../openclrenderer/hologram.hpp"
#include "../openclrenderer/proj.hpp"
#include "../openclrenderer/ocl.h"
#include "../openclrenderer/texture_manager.hpp"
#include "../openclrenderer/game/newtonian_body.hpp"
#include "../openclrenderer/game/collision.hpp"
#include "../openclrenderer/interact_manager.hpp"
#include "../openclrenderer/game/game_object.hpp"

#include "../openclrenderer/text_handler.hpp"
#include <sstream>
#include <string>
#include "../openclrenderer/vec.hpp"

#include "../openclrenderer/game/galaxy/galaxy.hpp"

#include "../openclrenderer/game/game_manager.hpp"
#include "../openclrenderer/game/space_dust.hpp"
#include "../openclrenderer/game/asteroid/asteroid_gen.hpp"
#include "../openclrenderer/ui_manager.hpp"

#include "../openclrenderer/game/ship.hpp"
#include "../openclrenderer/terrain_gen/terrain_gen.hpp"

#include "fighter.hpp"

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

    window.set_camera_pos({0,-85,-1000,0});

    fighter fight;

    obj_mem_manager::load_active_objects();

    fight.scale();

    c1.scale(0.0001f);

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

    vec3f original_pos = fight.parts[bodypart::LHAND].pos;

    vec3f seek_pos = original_pos;

    seek_pos.v[2] = -170.f;
    seek_pos.v[1] = -00.f;

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

        fight.IK_hand(0, seek_pos);*/

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

        fight.tick();

        window.draw_bulk_objs_n();

        window.render_buffers();

        window.display();

        std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
