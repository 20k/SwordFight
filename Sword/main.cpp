//#include "../openclrenderer/proj.hpp"
#include <winsock2.h>
#include "../openclrenderer/engine.hpp"
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
#include "map_tools.hpp"

#include "server_networking.hpp"
#include "../openclrenderer/game/space_manager.hpp" ///yup
#include "../openclrenderer/game/galaxy/galaxy.hpp"

#include "game_state_manager.hpp"

#include "../openclrenderer/obj_load.hpp"

#include "version.h"

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

    if(once<sf::Keyboard::F>())
    {
        my_fight->queue_attack(attacks::STAB);
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

    if(key.isKeyPressed(sf::Keyboard::P))
        my_fight->try_jump();

    bool sprint = key.isKeyPressed(sf::Keyboard::LShift);

    my_fight->walk_dir(walk_dir, sprint);

    bool crouching = key.isKeyPressed(sf::Keyboard::LControl);

    my_fight->crouch_tick(crouching);
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

    if(window.get_scrollwheel_delta() > 0.01)
        my_fight->queue_attack(attacks::STAB);

    if(once<sf::Mouse::Right>())
        my_fight->queue_attack(attacks::BLOCK);

    if(once<sf::Keyboard::Q>())
        my_fight->try_feint();

    if(once<sf::Keyboard::Space>())
        my_fight->try_jump();

    bool crouching = key.isKeyPressed(sf::Keyboard::LControl);

    my_fight->crouch_tick(crouching);

    window.c_rot.x = clamp(window.c_rot.x, -M_PI/2.f, M_PI/2.f);

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

///make textures go from start to dark to end
///need to make sound not play multiple times
int main(int argc, char *argv[])
{
    /*texture tex;
    tex.type = 0;
    tex.set_unique();
    //tex.set_texture_location("res/blue.png");
    tex.set_load_func(std::bind(texture_make_blank, std::placeholders::_1, 256, 256, sf::Color(255, 255, 255)));
    tex.push();*/

    sf::Clock clk;

    object_context context;
    object_context_data* gpu_context = context.fetch();

    object_context transparency_context;

    world_map default_map;
    //default_map.init(map_namespace::map_one, 11, 12);
    default_map.init(0);

    gameplay_state current_state;
    current_state.set_map(default_map);

    objects_container* floor = context.make_new();
    floor->set_load_func(default_map.get_load_func());

    ///need to extend this to textures as well
    floor->set_normal("./res/norm.png");
    floor->cache = false;
    floor->set_active(true);
    //floor->set_pos({0, bodypart::default_position[bodypart::LFOOT].v[1] - bodypart::scale/3, 0});
    floor->offset_pos({0, bodypart::default_position[bodypart::LFOOT].v[1] - bodypart::scale/3, 0});


    /*objects_container* rect = context.make_new();

    rect->set_load_func(std::bind(obj_rect, std::placeholders::_1, tex, (cl_float2){100, 100}));
    rect->cache = false;
    rect->set_active(true);*/

    const std::string title = std::string("SwordFight V") + std::to_string(AutoVersion::MAJOR) + "." + std::to_string(AutoVersion::MINOR);


    settings s;
    s.load("./res/settings.txt");

    engine window;
    window.load(s.width,s.height, 1000, title, "../openclrenderer/cl2.cl", true);
    window.manual_input = true;

    window.set_camera_pos((cl_float4){-800,150,-570});

    window.window.setVerticalSyncEnabled(false);

    //window.window.setFramerateLimit(60.f);

    printf("loaded\n");

    //text::set_renderwindow(window.window);

    window.set_camera_pos({-1009.17, -94.6033, -317.804});
    window.set_camera_rot({0, 1.6817, 0});

    fighter fight(context, *gpu_context);
    fight.set_team(0);
    fight.set_quality(s.quality);
    fight.set_gameplay_state(&current_state);
    //fight.my_cape.make_stable(&fight);

    fighter fight2(context, *gpu_context);
    fight2.set_team(1);
    fight2.set_pos({0, 0, -650});
    fight2.set_rot({0, M_PI, 0});
    fight2.set_quality(s.quality);
    fight2.set_gameplay_state(&current_state);

    physics phys;
    phys.load();

    printf("preload\n");

    context.load_active();

    printf("postload\n");

    fight.set_physics(&phys);
    fight2.set_physics(&phys);

    printf("loaded net fighters\n");

    ///a very high roughness is better (low spec), but then i think we need to remove the overhead lights
    ///specular component
    floor->set_specular(0.01f);
    floor->set_diffuse(4.f);

    //auto tex_gpu = texture_manager::build_descriptors();

    //window.set_tex_data(tex_gpu);

    printf("textures\n");

    context.build(true);

    //context.fetch()->tex_gpu = tex_gpu;

    ///should be a constant ptr
    //window.set_tex_data(context.fetch()->tex_gpu);

    auto ctx = context.fetch();
    window.set_object_data(*ctx);

    printf("loaded memory\n");

    sf::Event Event;

    light l;
    //l.set_col({1.0, 1.0, 1.0, 0});
    l.set_col({1.0, 1.0, 1.0, 0});
    l.set_shadow_casting(0);
    l.set_brightness(0.515f);
    //l.set_brightness(0.415f);
    l.set_diffuse(1.f);
    l.set_pos({0, 10000, -300, 0});
    //l.set_godray_intensity(0.5f);

    light::add_light(&l);

    auto light_data = light::build();

    window.set_light_data(light_data);

    printf("light\n");

    server_networking server;

    sf::Mouse mouse;
    sf::Keyboard key;

    vec3f original_pos = fight.parts[bodypart::LFOOT].pos;

    vec3f seek_pos = original_pos;

    vec3f rest_position = {0, -200, -100};

    fighter* my_fight = &fight;

    printf("Presspace\n");

    space_manager space_res;
    space_res.init(s.width, s.height);

    point_cloud stars = get_starmap(1);
    point_cloud_info g_star_cloud = point_cloud_manager::alloc_point_cloud(stars);

    printf("Postspace\n");

    ///debug;
    int controls_state = 0;
    bool central_pip = false;

    printf("loop\n");

    {
        printf("%i\n", context.containers.size());
    }

    fight.set_secondary_context(&transparency_context);
    fight2.set_secondary_context(&transparency_context);

    fight.set_name(s.name);
    fight2.set_name("Philip");

    fighter* net_test = nullptr;

    ///fix depth ordering  with transparency
    while(window.window.isOpen())
    {
        sf::Clock c;

        window.reset_scrollwheel_delta();

        bool fullscreen = window.is_fullscreen;
        bool do_resize = false;
        int r_x = window.get_width();
        int r_y = window.get_height();

        while(window.window.pollEvent(Event))
        {
            if(Event.type == sf::Event::Closed)
                window.window.close();

            if(Event.type == sf::Event::Resized)
            {
                do_resize = true;

                r_x = Event.size.width;
                r_y = Event.size.height;
            }

            if(Event.type == sf::Event::MouseWheelScrolled)
            {
                window.update_scrollwheel_delta(Event);
            }

            if(Event.type == sf::Event::GainedFocus)
            {
                window.set_focus(true);
            }

            if(Event.type == sf::Event::LostFocus)
            {
                window.set_focus(false);
            }
        }

        if(window.check_alt_enter() && window.focus)
        {
            sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

            do_resize = true;
            fullscreen = !fullscreen;

            r_x = desktop.width;
            r_y = desktop.height;
        }

        if(do_resize)
        {
            window.render_block();

            cl::cqueue.finish();
            cl::cqueue2.finish();

            window.load(r_x, r_y, 1000, title, "../openclrenderer/cl2.cl", true, fullscreen);

            if(fullscreen)
            {
                window.window.setPosition({0,0});
            }

            light_data = light::build();

            window.set_light_data(light_data);

            context.load_active();
            context.build(true);

            transparency_context.load_active();
            transparency_context.build(true);

            gpu_context = context.fetch();

            g_star_cloud = point_cloud_manager::alloc_point_cloud(stars);

            window.set_object_data(*gpu_context);
            window.set_light_data(light_data);
            //window.set_tex_data(gpu_context->tex_gpu);

            space_res.init(window.width, window.height);

            //text::set_renderwindow(window.window);

            cl::cqueue.finish();
            cl::cqueue2.finish();
        }

        if(once<sf::Keyboard::Tab>())
        {
            network_player play = server.make_networked_player(100, &context, &transparency_context, &current_state, &phys);

            net_test = play.fight;

            //net_test->set_team(0);
            //my_fight->set_team(1);

            memcpy(&net_test->net.net_name.v[0], "Pierre", strlen("Pierre"));

            play.fight->set_name("Loading");
        }

        if(net_test)
        {
            net_test->manual_check_part_alive();

            ///we need a die if appropriate too
            net_test->respawn_if_appropriate();
            net_test->checked_death();

            net_test->overwrite_parts_from_model();
            net_test->manual_check_part_death();

            net_test->my_cape.tick(net_test);

            net_test->do_foot_sounds();

            net_test->update_texture_by_part_hp();

            net_test->check_and_play_sounds();


            ///death is dynamically calculated from part health
            if(!net_test->dead())
            {
                net_test->update_name_info(true);

                net_test->update_lights();
            }
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

        if(once<sf::Keyboard::U>())
        {
            central_pip = !central_pip;
        }

        server.set_my_fighter(my_fight);
        server.tick(&context, &transparency_context, &current_state, &phys);

        ///debugging
        if(!server.joined_game)
            server.set_game_to_join(0);

        std::string display_string = server.game_info.get_display_string();

        text::add(display_string, 0, (vec2f){window.width/2.f, window.height - 20});

        if(server.game_info.game_over())
        {
            text::add(server.game_info.get_game_over_string(), 0, (vec2f){window.width/2.f, window.height/2.f});
        }

        ///network players don't die on a die
        ///because dying doesn't update part hp
        if(server.just_new_round && !my_fight->dead())
        {
            my_fight->die();
        }

        if(my_fight->dead())
        {
            std::string disp_string = server.respawn_inf.get_display_string();

            text::add(disp_string, 0, (vec2f){window.width/2.f, 20});
        }

        if(once<sf::Keyboard::B>())
        {
            my_fight->respawn();
        }

        ///so just respawning doesnt fix, sometimes (mostly) doing enter does
        ///but not always
        ///something very odd. Rewrite texturing
        if(once<sf::Keyboard::Return>())
        {
            context.build(true);
            transparency_context.build(true);

            context.flip();
            transparency_context.flip();
        }

        //static float debug_look = 0;
        //my_fight->set_look({sin(debug_look), 0, 0});
        //debug_look += 0.1f;

        /*phys.tick();
        vec3f v = phys.get_pos();
        c1.set_pos({v.v[0], v.v[1], v.v[2]});
        c1.g_flush_objects();*/

        std::vector<fighter*> fighter_list = server.get_fighters();

        if(std::find(fighter_list.begin(), fighter_list.end(), my_fight) == fighter_list.end())
        {
            fighter_list.push_back(my_fight);
        }

        fighter_list.push_back(&fight2);

        my_fight->set_other_fighters(fighter_list);

        if(network::network_state == 0)
        {
            fight2.queue_attack(attacks::SLASH);
            //fight2.queue_attack(attacks::BLOCK);

            fight2.tick();
            fight2.tick_cape();

            fight2.update_render_positions();

            //fight2.walk_dir({-1, -1});

            fight2.do_foot_sounds();

            fight2.update_texture_by_part_hp();

            fight2.check_and_play_sounds();

            //fight2.crouch_tick(true);

            if(!fight2.dead())
            {
                fight2.update_name_info();

                fight2.update_lights();
            }

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

        my_fight->update_render_positions();

        my_fight->update_texture_by_part_hp();

        my_fight->check_and_play_sounds();


        ///we can use the foot rest position to play a sound when the
        ///current foot position is near that
        ///remember, only the y component!

        if(!my_fight->dead())
        {
            my_fight->update_name_info();

            my_fight->update_lights();
        }

        particle_effect::tick();

        ///about 0.2ms slower than not doing this
        light_data = light::build();
        window.set_light_data(light_data);

        ///ergh
        sound::set_listener(my_fight->parts[bodypart::BODY].global_pos, my_fight->parts[bodypart::BODY].global_rot);

        ///so that the listener position is exactly the body part
        my_fight->do_foot_sounds(true);

        sound::update_listeners();

        ///so, we blit space to screen, but that might not have finished before
        ///the async event for the draw_bulk_objs_n event has finished
        ///and this can fire
        window.blit_to_screen(*context.fetch());

        ///I need to reenable text drawing
        ///possibly split up window.display into display and flip
        ///then have display set a flag if its appropriate to flip the screen
        ///that way we can still keep the async rendering
        ///but also allow drawing on top of teh 3d scene
        ///we'll need to allow window querying to say should we draw
        ///otherwise in async we'll waste huge performance
        ///in synchronous that's not a problem

        if(window.render_me)
        {
            text::draw(&window.window);
        }

        if(window.render_me && central_pip)
        {
            float rad = 2;

            sf::CircleShape circle;
            circle.setRadius(rad);
            circle.setOutlineThickness(1.f);
            circle.setOutlineColor(sf::Color(255, 255, 255, 128));
            circle.setFillColor(sf::Color(255, 255, 255, 255));

            circle.setPointCount(100);

            circle.setPosition({window.width/2.f - (rad + 0.5f), window.height/2.f - (rad + 0.5f)});

            window.window.draw(circle);
        }

        window.flip();

        ///so this + render_event is basically causing two stalls
        window.render_block(); ///so changing render block above blit_to_screen also fixes

        context.flip();
        transparency_context.flip();
        object_context_data* cdat = context.fetch();

        //window.set_tex_data(cdat->tex_gpu);

        window.set_object_data(*cdat);

        context.flush_locations();
        transparency_context.flush_locations();

        window.process_input();
        window.c_rot.x = clamp(window.c_rot.x, -M_PI/2.f, M_PI/2.f);

        space_res.update_camera(window.c_pos, window.c_rot);

        space_res.set_depth_buffer(cdat->depth_buffer[cdat->nbuf]);
        space_res.set_screen(cdat->g_screen);

        compute::event event;

        if(window.can_render())
        {
            ///if we make this after and put the clear in here, we can then do blit_space every time
            ///with no flickering, fewer atomics, and better performance
            ///marginally though
            space_res.draw_galaxy_cloud_modern(g_star_cloud, (cl_float4){-5000,-8500,0});

            window.draw_bulk_objs_n(*cdat);

            //window.draw_godrays(*cdat);
        }

        if(window.can_render())
            event = space_res.blit_space_to_screen(*cdat);

        if(window.can_render())
        {
            object_context_data* tctx = transparency_context.fetch();

            window.draw_bulk_objs_n(*tctx);

            event = window.blend_with_depth(*tctx, *cdat);

            cdat->swap_depth_buffers();
            tctx->swap_depth_buffers();

            window.increase_render_events();
        }

        ///it might be this event which is causing a hang
        ///YUP

        ///so adding a finish here fixes stuff

        ///for some reason, a delay here prevents space from being blitted

        ///so, we need to fix this double sync
        window.set_render_event(event);

        if(key.isKeyPressed(sf::Keyboard::M))
            std::cout << c.getElapsedTime().asMicroseconds() << std::endl;

        /*printf("TTL %f\n", clk.getElapsedTime().asMicroseconds() / 1000.f);

        return 0;*/
    }

    cl::cqueue.finish();
    cl::cqueue2.finish();
}
