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

    c1.scale(0.1f);

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

    vec3f seek_pos = fight.parts[bodypart::LHAND].pos;

    seek_pos.v[2] = -100.f;

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

        if(key.isKeyPressed(sf::Keyboard::O))
        {
            seek_pos.v[1] += 0.5f;
        }
        if(key.isKeyPressed(sf::Keyboard::U))
        {
            seek_pos.v[1] -= 0.5f;
        }

        fight.IK_hand(0, seek_pos);

        window.draw_bulk_objs_n();

        window.render_buffers();

        window.display();

        //std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
