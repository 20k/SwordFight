
#include "../openclrenderer/proj.hpp"
#include "../openclrenderer/ocl.h"
#include "../openclrenderer/texture_manager.hpp"

#include "../openclrenderer/text_handler.hpp"
#include <sstream>
#include <string>
#include "../openclrenderer/vec.hpp"

#include "../openclrenderer/ui_manager.hpp"

#include "fighter.hpp"
#include "text.hpp"
#include "physics.hpp"

#include "../openclrenderer/network.hpp"

#include "sound.hpp"

#include "object_cube.hpp"
#include "particle_effect.hpp"

#include "../openclrenderer/settings_loader.hpp"
#include "../openclrenderer/controls.hpp"

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

///none of these affect the camera, so engine does not care about them
///assume main is blocking
void debug_controls(fighter* my_fight, engine& window)
{
    sf::Keyboard key;

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

    if(once<sf::Keyboard::H>())
    {
        my_fight->try_feint();
    }

    if(once<sf::Keyboard::SemiColon>())
    {
        my_fight->die();
    }

    float y_diff = 0;

    if(key.isKeyPressed(sf::Keyboard::U))
    {
        y_diff = 0.01f * window.get_frametime()/2000.f;
    }

    if(key.isKeyPressed(sf::Keyboard::O))
    {
        y_diff = -0.01f * window.get_frametime()/2000.f;
    }

    my_fight->set_rot_diff({0, y_diff, 0});

    static float look_height = 0.f;

    if(key.isKeyPressed(sf::Keyboard::Comma))
    {
        look_height += 0.01f * window.get_frametime() / 8000.f;
    }

    if(key.isKeyPressed(sf::Keyboard::Period))
    {
        look_height += -0.01f * window.get_frametime() / 8000.f;
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

    bool sprint = key.isKeyPressed(sf::Keyboard::LShift);

    my_fight->walk_dir(walk_dir, sprint);
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

    bool sprint = key.isKeyPressed(sf::Keyboard::LShift);

    my_fight->walk_dir(walk_dir, sprint);

    if(once<sf::Mouse::Left>())
        my_fight->queue_attack(attacks::SLASH);

    if(once<sf::Mouse::Middle>())
        my_fight->queue_attack(attacks::OVERHEAD);

    if(once<sf::Mouse::Right>())
        my_fight->queue_attack(attacks::BLOCK);

    if(once<sf::Keyboard::Q>())
        my_fight->try_feint();

    my_fight->set_look({-window.c_rot.s[0], window.get_mouse_delta_x() / 1.f, 0});

    //part* head = &my_fight->parts[bodypart::HEAD];

    //vec3f pos = head->pos + my_fight->pos;

    //window.set_camera_pos({pos.v[0], pos.v[1], pos.v[2]});

    vec2f m;
    m.v[0] = window.get_mouse_delta_x();
    m.v[1] = window.get_mouse_delta_y();

    my_fight->set_rot_diff({0, -m.v[0] / 100.f, 0.f});

    //vec3f o_rot = xyz_to_vec(window.c_rot);

    //o_rot.v[1] = my_fight->rot.v[1];
    //o_rot.v[0] += m.v[1] / 200.f;

    //window.set_camera_rot({o_rot.v[0], -o_rot.v[1] + M_PI, o_rot.v[2]});
}

input_delta fps_camera_controls(float frametime, const input_delta& input, engine& window, const fighter* my_fight)
{
    const part* head = &my_fight->parts[bodypart::HEAD];

    vec3f pos = head->pos + my_fight->pos;

    //window.set_camera_pos({pos.v[0], pos.v[1], pos.v[2]});

    cl_float4 c_pos = {pos.v[0], pos.v[1], pos.v[2]};

    vec2f m;
    m.v[0] = window.get_mouse_delta_x();
    m.v[1] = window.get_mouse_delta_y();

    vec3f o_rot = xyz_to_vec(input.c_rot);

    o_rot.v[1] = my_fight->rot.v[1];
    o_rot.v[0] += m.v[1] / 150.f;

    //window.set_camera_rot({o_rot.v[0], -o_rot.v[1] + M_PI, o_rot.v[2]});

    cl_float4 c_rot = {o_rot.v[0], -o_rot.v[1] + M_PI, o_rot.v[2]};

    return {sub(c_pos, input.c_pos), sub(c_rot, input.c_rot)};
}

///a fighter we own has its hp determined by someone else?

///slave i.hp
/*void net_host(fighter& fight)
{
    for(auto& i : fight.parts)
    {
        network::host_object(&i.model);
        network::slave_var(&i.hp); ///nobody owns HP
    }

    network::host_object(&fight.weapon.model);

    //network::host_var(&fight.net.is_blocking);
}*/

void net_slave(fighter& fight)
{
    for(auto& i : fight.parts)
    {
        network::slave_object(i.obj());
        network::slave_var(&i.hp); ///this is not an error, hp transmission is handled when hp takes damage
    }

    network::slave_object(&fight.weapon.model);

    network::slave_var(&fight.net.is_blocking);

    network::slave_var(&fight.net.recoil);

    //network::slave_var(&fight.net.dead);
}

void make_host(fighter& fight)
{
    for(auto& i : fight.parts)
    {
        network::transform_host_object(i.obj());
        ///no need to touch hp here as its always a slave variable
    }

    network::transform_host_object(&fight.weapon.model);

    network::transform_host_var(&fight.net.is_blocking);
}

int main(int argc, char *argv[])
{
    /*objects_container c1;
    c1.set_load_func(std::bind(load_object_cube, std::placeholders::_1, (vec3f){0, 0, 0}, (vec3f){0, 100, -100}, 20.f));
    c1.cache = false;
    c1.set_active(true);

    objects_container c2;
    c2.set_load_func(std::bind(load_object_cube, std::placeholders::_1, (vec3f){0, 0, 0}, (vec3f){100, -100, 100}, 20.f));
    c2.cache = false;
    c2.set_active(true);*/

    objects_container floor;
    floor.set_load_func(std::bind(load_object_cube, std::placeholders::_1,
                                  (vec3f){0, bodypart::default_position[bodypart::LFOOT].v[1] - bodypart::scale/3, 0},
                                  (vec3f){0, bodypart::default_position[bodypart::LFOOT].v[1] - 42.f, 0},
                                  3000.f, "./res/gray.png"));

    ///need to extend this to textures as well
    floor.set_normal("./res/norm.png");
    floor.cache = false;
    floor.set_active(false);

    objects_container file_map;
    file_map.set_file("./res/maps/map1.obj");
    file_map.set_active(true);
    file_map.set_pos({0, bodypart::default_position[bodypart::LFOOT].v[1] - bodypart::scale/3 - 0, 0});

    settings s;
    s.load("./res/settings.txt");

    engine window;
    window.load(s.width,s.height,1000, "SwordFight", "../openclrenderer/cl2.cl", true);

    window.set_camera_pos((cl_float4){-800,150,-570});

    window.window.setVerticalSyncEnabled(true);

    //window.window.setFramerateLimit(24.f);

    printf("loaded\n");

    text::set_renderwindow(window.window);

    window.set_camera_pos({-1009.17, -94.6033, -317.804});
    window.set_camera_rot({0, 1.6817, 0});

    fighter fight;
    fight.set_team(0);
    fight.set_quality(s.quality);
    //fight.my_cape.make_stable(&fight);

    fighter fight2;
    fight2.set_team(1);
    fight2.set_pos({0, 0, -600});
    fight2.set_rot({0, M_PI, 0});
    fight2.set_quality(s.quality);

    std::vector<fighter*> net_fighters;

    ///tmp
    for(int i=0; i<10; i++)
    {
        net_fighters.push_back(new fighter);
        net_fighters[i]->set_team(1);
        net_fighters[i]->set_pos({0, 0, -3000000});
        net_fighters[i]->set_rot({0, 0, 0});
        net_fighters[i]->set_quality(s.quality);
    }


    physics phys;
    phys.load();

    printf("preload\n");

    obj_mem_manager::load_active_objects();

    file_map.scale(1000);

    printf("postload\n");


    fight.set_physics(&phys);
    fight2.set_physics(&phys);

    for(auto& i : net_fighters)
    {
        i->set_physics(&phys);

        net_slave(*i);

        i->update_render_positions();
    }

    printf("loaded net fighters\n");

    floor.set_specular(0.9f);
    floor.set_diffuse(8.f);


    texture_manager::allocate_textures();

    printf("textures\n");

    obj_mem_manager::g_arrange_mem();
    obj_mem_manager::g_changeover();

    printf("loaded memory\n");

    sf::Event Event;

    light l;
    //l.set_col({1.0, 1.0, 1.0, 0});
    l.set_col({1.0, 1.0, 1.0, 0});
    l.set_shadow_casting(0);
    l.set_brightness(0.2f);
    l.set_diffuse(1.f);
    l.set_pos({0, 10000, -300, 0});

    window.add_light(&l);

    printf("light\n");


    sf::Mouse mouse;
    sf::Keyboard key;

    vec3f original_pos = fight.parts[bodypart::LFOOT].pos;

    vec3f seek_pos = original_pos;

    vec3f rest_position = {0, -200, -100};

    fighter* my_fight = &fight;


    ///debug;
    int controls_state = 0;

    printf("loop\n");


    while(window.window.isOpen())
    {
        sf::Clock c;

        if(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();
        }

        if(controls_state == 0)
            window.update_mouse();
        if(controls_state == 1)
            window.update_mouse(window.width/2, window.height/2, true, true);

        if(once<sf::Keyboard::X>())
        {
            controls_state = (controls_state + 1) % 2;

            ///call once to reset mouse to centre
            window.update_mouse(window.width/2, window.height/2, true, true);
            ///call again to reset mouse dx and dy to 0
            window.update_mouse(window.width/2, window.height/2, true, true);
        }

        if(controls_state == 0)
            debug_controls(my_fight, window);
        if(controls_state == 1)
            fps_controls(my_fight, window);

        control_input c_input;

        if(controls_state == 0)
            c_input = control_input();

        if(controls_state == 1)
            c_input = control_input(std::bind(fps_camera_controls, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, my_fight),
                              process_controls_empty);

        window.set_input_handler(c_input);

        if(once<sf::Keyboard::V>() && network::network_state == 0)
        {
            network::join(s.ip);

            network::ping();
        }

        if(once<sf::Keyboard::C>() && network::network_state == 0)
        {
            network::host();

            my_fight = net_fighters[0];

            make_host(*my_fight);

            my_fight->set_pos({0,0,0});
            my_fight->set_team(0);

            fight.die();
            fight2.die();

            obj_mem_manager::load_active_objects();

            obj_mem_manager::g_arrange_mem();
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

        if(network::network_state == 0)
        {
            fight2.queue_attack(attacks::SLASH);
            //fight2.queue_attack(attacks::BLOCK);

            fight2.tick();
            fight2.tick_cape();

            fight2.update_render_positions();

            if(!fight2.dead())
                fight2.update_lights();

            if(once<sf::Keyboard::N>())
            {
                vec3f loc = fight2.parts[bodypart::BODY].global_pos;
                vec3f rot = fight2.parts[bodypart::BODY].global_rot;

                fight2.respawn({loc.v[0], loc.v[2]});
                fight2.set_rot(rot);
            }
        }

        /*int hit_p = phys.sword_collides(fight.weapon, &fight, {0, 0, -1});
        if(hit_p != -1)
            printf("%s\n", bodypart::names[hit_p % (bodypart::COUNT)].c_str());*/

        my_fight->tick(true);
        my_fight->tick_cape();


        bool need_realloc = network::tick();

        if(need_realloc)
        {
            printf("Reallocating\n");

            obj_mem_manager::load_active_objects();
            obj_mem_manager::g_arrange_mem();
        }

        audio_packet pack;

        while(network::pop_audio(pack))
        {
            sound::add(pack.type, {pack.x, pack.y, pack.z});
        }

        ///we've joined the game!
        if(network::join_id != -1 && !network::loaded)
        {
            printf("In load func\n");

            if(network::join_id < 10)
            {
                printf("Joining game\n");

                my_fight = net_fighters[network::join_id];

                my_fight->set_pos(fight2.pos);
                my_fight->set_rot(fight2.rot);
                my_fight->set_team(0);

                make_host(*my_fight);

                fight.die();
                fight2.die();

                obj_mem_manager::load_active_objects();

                obj_mem_manager::g_arrange_mem();
            }
            else
            {
                printf("Full (temp)\n");
            }

            network::loaded = true;
        }


        my_fight->update_render_positions();

        if(!my_fight->dead())
            my_fight->update_lights();

        for(auto& i : net_fighters)
        {
            if(my_fight == i)
                continue;

            ///this copies the model positions back to the part global positions so that it works with the physics
            ///ideally we'd want a net.pos and net.rot for them, would be less cumbersome?
            i->overwrite_parts_from_model();

            if(!i->dead())
                i->update_lights();
        }

        particle_effect::tick();

        ///about 0.2ms slower than not doing this
        engine::realloc_light_gmem();

        ///ergh
        sound::set_listener(my_fight->parts[bodypart::BODY].global_pos, my_fight->parts[bodypart::BODY].global_rot);

        obj_mem_manager::g_changeover();

        window.draw_bulk_objs_n();

        //window.render_buffers();
        //text::draw();

        window.display();
        window.render_block();

        std::cout << c.getElapsedTime().asMicroseconds() << std::endl;
    }
}
