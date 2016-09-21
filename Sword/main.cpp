#include <winsock2.h>
#include "../openclrenderer/engine.hpp"
#include "../openclrenderer/ocl.h"

#include "../openclrenderer/text_handler.hpp"
#include <sstream>
#include <string>
#include "../openclrenderer/vec.hpp"

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

#include "menu.hpp"

#include "version.h"

#include "util.hpp"
#include "../openclrenderer/logging.hpp"

#include <fstream>

#include <my_myo/my_myo.hpp>

#include "../imgui/imgui.h"
#include "../imgui/imgui-SFML.h"

#include "ui_manager.hpp"

#include "trombone_manager.hpp"

#include "../openclrenderer/camera_effects.hpp"

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

    if(once<sf::Keyboard::V>())
    {
        my_fight->queue_attack(attacks::RECOIL);
    }

    if(once<sf::Keyboard::Z>())
    {
        my_fight->queue_attack(attacks::TROMBONE);
    }

    if(once<sf::Mouse::XButton1>())
        my_fight->queue_attack(attacks::SLASH_ALT);

    if(once<sf::Mouse::XButton2>())
        my_fight->queue_attack(attacks::OVERHEAD_ALT);


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

    bool crouching = key.isKeyPressed(sf::Keyboard::LControl);

    my_fight->crouch_tick(crouching);

    if(crouching)
        sprint = false;

    if(sprint && walk_dir.v[0] < 0)
    {
        my_fight->queue_attack(attacks::SPRINT);
    }

    if(my_fight->cube_info)
        walk_dir = my_fight->cube_info->transform_move_dir_no_rot(walk_dir);

    my_fight->walk_dir(walk_dir, sprint);
}

void fps_controls(fighter* my_fight, engine& window)
{
    sf::Keyboard key;

    if(key.isKeyPressed(sf::Keyboard::F10))
        window.request_close();

    vec2f walk_dir = {0,0};

    if(key.isKeyPressed(sf::Keyboard::W))
        walk_dir.v[0] = -1;

    if(key.isKeyPressed(sf::Keyboard::S))
        walk_dir.v[0] = 1;

    if(key.isKeyPressed(sf::Keyboard::A))
        walk_dir.v[1] = -1;

    if(key.isKeyPressed(sf::Keyboard::D))
        walk_dir.v[1] = 1;

    bool crouching = key.isKeyPressed(sf::Keyboard::LControl);

    my_fight->crouch_tick(crouching);

    bool sprint = key.isKeyPressed(sf::Keyboard::LShift);

    if(crouching)
        sprint = false;

    if(my_fight->cube_info)
        walk_dir = my_fight->cube_info->transform_move_dir_no_rot(walk_dir);

    my_fight->walk_dir(walk_dir, sprint);

    my_fight->update_headbob_if_sprinting(sprint);

    if(once<sf::Mouse::Left>())
        my_fight->queue_attack(attacks::SLASH);

    if(once<sf::Mouse::Middle>())
        my_fight->queue_attack(attacks::OVERHEAD);

    if(window.get_scrollwheel_delta() > 0.01)
        my_fight->queue_attack(attacks::STAB);

    if(once<sf::Mouse::Right>())
        my_fight->queue_attack(attacks::BLOCK);

    if(once<sf::Mouse::XButton1>())
        my_fight->queue_attack(attacks::SLASH_ALT);

    if(once<sf::Mouse::XButton2>())
        my_fight->queue_attack(attacks::OVERHEAD_ALT);

    if(once<sf::Keyboard::Z>())
        my_fight->queue_attack(attacks::TROMBONE);

    if(sprint && walk_dir.v[0] < 0)
        my_fight->queue_attack(attacks::SPRINT);

    if(once<sf::Keyboard::Q>())
        my_fight->try_feint();

    if(once<sf::Keyboard::Space>())
        my_fight->try_jump();

    //window.c_rot.x = clamp(window.c_rot.x, -M_PI/2.f, M_PI/2.f);

    ///this will probably break
    my_fight->set_look({-window.c_rot_keyboard_only.s[0], window.get_mouse_sens_adjusted_x() / 1.f, 0});


    vec2f m;
    m.v[0] = window.get_mouse_sens_adjusted_x();
    m.v[1] = window.get_mouse_sens_adjusted_y();

    float source_m_yaw = 0.0003839723;

    float yout = m.v[0] * source_m_yaw;

    ///ok, so this is essentially c_rot_keyboard_only
    ///ie local
    ///ie we give no fucks anymore because the mouse look scheme is fully consistent from local -> global
    ///ie we can ignore this, just apply the overall matrix rotation and offset to te position
    ///and etc
    my_fight->set_rot_diff({0, -yout, 0.f});
}

pos_rot update_screenshake_camera(fighter* my_fight, cl_float4 c_pos, cl_float4 c_rot, float time_ms)
{
    static screenshake_effect effect;

    pos_rot offset;

    offset.rot = {0,0,0};

    if(my_fight->reset_screenshake_flinch)
    {
        effect.init(200.f, 5.f, 1.f);

        my_fight->reset_screenshake_flinch = false;
    }

    effect.tick(time_ms, c_pos, c_rot);

    vec3f noffset = effect.get_offset();

    offset.pos = noffset;

    return offset;
}

void fps_trombone_controls(fighter* my_fight, engine& window, trombone_manager& trombone)
{
    sf::Keyboard key;

    if(key.isKeyPressed(sf::Keyboard::F10))
        window.request_close();

    vec2f walk_dir = {0,0};

    if(key.isKeyPressed(sf::Keyboard::W))
        walk_dir.v[0] = -1;

    if(key.isKeyPressed(sf::Keyboard::S))
        walk_dir.v[0] = 1;

    if(key.isKeyPressed(sf::Keyboard::A))
        walk_dir.v[1] = -1;

    if(key.isKeyPressed(sf::Keyboard::D))
        walk_dir.v[1] = 1;

    bool crouching = key.isKeyPressed(sf::Keyboard::LControl);

    my_fight->crouch_tick(crouching);

    bool sprint = key.isKeyPressed(sf::Keyboard::LShift);

    if(crouching)
        sprint = false;

    if(my_fight->cube_info)
        walk_dir = my_fight->cube_info->transform_move_dir_no_rot(walk_dir);

    my_fight->walk_dir(walk_dir, sprint);

    my_fight->update_headbob_if_sprinting(sprint);

    if(once<sf::Mouse::Left>())
        trombone.play(my_fight);

    //if(once<sf::Keyboard::Z>())
    my_fight->queue_attack(attacks::TROMBONE);

    if(once<sf::Keyboard::Space>())
        my_fight->try_jump();

    ///this will probably break
    my_fight->set_look({-window.c_rot_keyboard_only.s[0], window.get_mouse_sens_adjusted_x() / 1.f, 0});


    vec2f m;
    m.v[0] = window.get_mouse_sens_adjusted_x();
    m.v[1] = window.get_mouse_sens_adjusted_y();

    float source_m_yaw = 0.0003839723;

    float yout = m.v[0] * source_m_yaw;

    ///ok, so this is essentially c_rot_keyboard_only
    ///ie local
    ///ie we give no fucks anymore because the mouse look scheme is fully consistent from local -> global
    ///ie we can ignore this, just apply the overall matrix rotation and offset to te position
    ///and etc
    my_fight->set_rot_diff({0, -yout, 0.f});

    trombone.set_active(true);
}

input_delta fps_camera_controls(float frametime, const input_delta& input, engine& window, fighter* my_fight)
{
    vec3f pos = (vec3f){0, my_fight->smoothed_crouch_offset, 0} + my_fight->pos + my_fight->camera_bob * my_fight->camera_bob_mult;

    cl_float4 c_pos = {pos.v[0], pos.v[1], pos.v[2]};

    vec2f m;
    m.v[0] = window.get_mouse_sens_adjusted_x();
    m.v[1] = window.get_mouse_sens_adjusted_y();

    ///sigh. Do matrices
    vec3f o_rot = xyz_to_vec(input.c_rot_keyboard_only);

    float source_m_yaw = 0.0003839723f;

    float out = m.v[1] * source_m_yaw;

    o_rot.v[1] = my_fight->rot.v[1];
    //o_rot.v[0] += m.v[1] / 150.f;
    o_rot.v[0] += out;

    //window.set_camera_rot({o_rot.v[0], -o_rot.v[1] + M_PI, o_rot.v[2]});

    cl_float4 c_rot = {o_rot.v[0], -o_rot.v[1] + M_PI, o_rot.v[2]};

    pos_rot coffset = update_screenshake_camera(my_fight, c_pos, c_rot, frametime / 1000.f);

    c_pos = sub(c_pos, (cl_float4){coffset.pos.v[0], coffset.pos.v[1], coffset.pos.v[2]});

    ///using c_rot_backup breaks everything, but its fine beacuse we're not needing to use this immutably
    return {sub(c_pos, input.c_pos), sub(c_rot, input.c_rot_keyboard_only), 0.f};
    //return {sub(c_pos, input.c_pos), sub(c_rot, input.c_rot)};
}

#include <stdio.h>

///make textures go from start to dark to end
///need to make sound not play multiple times
///build then flip is invalid
///on self damage, simply queue a move for x ms to do nothing
///try falling sand, do gravity physics on cubes then atomic, then diffuse, could be interesting
int main(int argc, char *argv[])
{
    lg::set_logfile("./logging.txt");

    lg::log("preload");

    settings s;
    s.load("./res/settings.txt");

    ///printf redirect not working on tester's pc
    if(!s.enable_debugging)
    {
        static std::ofstream out("info.txt");
        std::cout.rdbuf( out.rdbuf() );

        freopen("err.txt", "w", stdout);
    }
    else
    {
        std::streambuf* b1 = std::cout.rdbuf();

        std::streambuf* b2 = lg::output->rdbuf();

        std::ios* r1 = &std::cout;
        std::ios* r2 = lg::output;

        r2->rdbuf(b1);

        //std::cout.rdbuf(lg::output->rdbuf());
    }

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

    map_cube_info debug_map_cube;

    world_map default_map;
    //default_map.init(map_namespace::map_one, 11, 12);
    default_map.init(0);
    default_map.cube_info = &debug_map_cube;

    gameplay_state current_state;
    current_state.set_map(default_map);

    objects_container* floor = context.make_new();
    //floor->set_load_func(default_map.get_load_func());

    floor->set_load_func(std::bind(load_map_cube, std::placeholders::_1, map_namespace::map_cube_one, 24, 24));

    ///need to extend this to textures as well
    floor->set_normal("./res/norm.png");
    floor->cache = false;
    floor->set_active(true);
    //floor->set_pos({0, bodypart::default_position[bodypart::LFOOT].v[1] - bodypart::scale/3, 0});
    floor->offset_pos({0, FLOOR_CONST, 0});

    objects_container* debug_cube = context.make_new();

    debug_cube->set_load_func(std::bind(load_object_cube, std::placeholders::_1, (vec3f){0,0,0}, (vec3f){10,10,10}, 100, "res/blue.png"));
    debug_cube->cache = false;
    debug_cube->set_active(false);

    /*objects_container* rect = context.make_new();

    rect->set_load_func(std::bind(obj_rect, std::placeholders::_1, tex, (cl_float2){100, 100}));
    rect->cache = false;
    rect->set_active(true);*/

    const std::string title = std::string("Midguard V") + std::to_string(AutoVersion::MAJOR) + "." + std::to_string(AutoVersion::MINOR);


    engine window;
    window.append_opencl_extra_command_line("-D SHADOWBIAS=150");
    window.load(s.width,s.height, 1000, title, "../openclrenderer/cl2.cl", true);
    ImGui::SFML::Init(window.window);
    window.manual_input = true;


    window.set_mouse_sens(s.mouse_sens);

    window.set_camera_pos((cl_float4){-800,150,-570});

    window.window.setVerticalSyncEnabled(false);
    //window.window.setFramerateLimit(15.f);

    //window.window.setFramerateLimit(60.f);

    lg::log("loaded");

    //text::set_renderwindow(window.window);

    window.set_camera_pos({-1009.17, -94.6033, -317.804});
    window.set_camera_rot({0, 1.6817, 0});
    window.c_rot_keyboard_only = window.c_rot;

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

    lg::log("loaded fighters");

    physics phys;
    phys.load();

    lg::log("preload");

    context.load_active();

    lg::log("postload");

    fight.set_physics(&phys);
    fight2.set_physics(&phys);

    lg::log("set physics");

    ///a very high roughness is better (low spec), but then i think we need to remove the overhead lights
    ///specular component
    floor->set_specular(0.01f);
    floor->set_diffuse(4.f);

    lg::log("prebuild");

    cube_effect::precache(500, context);

    context.build(true);

    lg::log("postbuild");


    auto ctx = context.fetch();
    window.set_object_data(*ctx);

    lg::log("loaded memory");

    sf::Event Event;

    light l;
    //l.set_col({1.0, 1.0, 1.0, 0});
    l.set_col({1.0, 1.0, 1.0, 0});
    l.set_shadow_casting(1);
    l.set_brightness(0.515f);
    //l.set_brightness(0.415f);
    l.set_diffuse(1.f);
    l.set_pos({0, 5000, -000, 0});
    //l.set_godray_intensity(0.5f);

    light::add_light(&l);

    auto light_data = light::build();

    lg::log("built light data");

    window.set_light_data(light_data);

    lg::log("light");

    server_networking server;
    server.set_graphics(s.quality);

    sf::Mouse mouse;
    sf::Keyboard key;

    vec3f original_pos = fight.parts[bodypart::LFOOT].pos;

    vec3f seek_pos = original_pos;

    vec3f rest_position = {0, -200, -100};

    fighter* my_fight = &fight;

    #ifdef SPACE
    lg::log("Presspace");

    space_manager space_res;

    if(s.quality != 0)
        space_res.init(s.width, s.height);

    point_cloud stars;
    point_cloud_info g_star_cloud;

    if(s.quality != 0)
    {
        stars = get_starmap(1);
        g_star_cloud = point_cloud_manager::alloc_point_cloud(stars);
    }

    //printf("SSize %i mb\n", (g_star_cloud.g_colour_mem.size() / 1024) / 1024);

    lg::log("Postspace");

    #endif

    ///debug;
    int controls_state = 1;
    bool central_pip = false;

    if(s.enable_debugging)
        controls_state = 0;

    lg::log("loop");

    {
        lg::log(context.containers.size());
    }

    fight.is_offline_client = true;
    fight2.is_offline_client = true;

    fight.set_secondary_context(&transparency_context);
    fight2.set_secondary_context(&transparency_context);

    lg::log("post set secondary context (transparency)");

    fight.set_name(s.name);
    fight2.set_name("Philip");

    fighter* net_test = nullptr;

    lg::log("post name rendering");

    context.fetch()->ensure_screen_buffers(window.width, window.height);
    transparency_context.fetch()->ensure_screen_buffers(window.width, window.height);

    lg::log("build screens");

    menu_system menu_handler;

    if(s.enable_debugging)
    {
        menu_handler.current_menu_state = menu_system::INGAME;
    }

    context.set_clear_colour({135/255.f, 206/255.f, 250/255.f});

    transparency_context.build(true);

    lg::log("post transparency forced build");

    //#define MYO_ARMBAND
    #ifdef MYO_ARMBAND
    myo_data myo_dat;
    #endif

    ui_manager ui_manage;
    ui_manage.init(s);

    trombone_manager trombone_manage;
    trombone_manage.init(&context);

    bool show_ftime = false;

    bool going = true;

    ///fix depth ordering with transparency
    while(going)
    {
        sf::Clock c;

        trombone_manage.set_active(false);

        bool in_menu = menu_handler.should_do_menu();

        ///if(do_menu) {do_menu = menu(); continue;} ???
        ///would be an easy way to hack in the menu system

        window.reset_scrollwheel_delta();

        bool fullscreen = window.is_fullscreen;
        bool do_resize = false;
        int r_x = window.get_width();
        int r_y = window.get_height();

        while(window.window.pollEvent(Event))
        {
            ImGui::SFML::ProcessEvent(Event);

            if(Event.type == sf::Event::Closed)
            {
                going = false;
            }

            if(Event.type == sf::Event::Resized)
            {
                do_resize = true;

                r_x = Event.size.width;
                r_y = Event.size.height;

                s.width = r_x;
                s.height = r_y;

                s.save("./res/settings.txt");
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

        if(window.is_requested_close())
        {
            going = false;
            break;
        }

        if(window.check_alt_enter() && window.focus)
        {
            sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

            do_resize = true;
            fullscreen = !fullscreen;

            r_x = desktop.width;
            r_y = desktop.height;

            s.width = r_x;
            s.height = r_y;

            s.save("./res/settings.txt");
        }

        if(s.width != window.get_width() || s.height != window.get_height())
        {
            do_resize = true;

            r_x = s.width;
            r_y = s.height;

            s.save("./res/settings.txt");
        }

        if(do_resize)
        {
            glFinish();
            window.render_block();

            cl::cqueue.finish();
            cl::cqueue2.finish();
            cl::cqueue_ooo.finish();

            window.load(r_x, r_y, 1000, title, "../openclrenderer/cl2.cl", true, fullscreen);

            ImGui::SFML::Init(window.window);

            if(fullscreen)
            {
                window.window.setPosition({0,0});
            }

            light::invalidate_buffers();

            light_data = light::build(&light_data);

            window.set_light_data(light_data);

            context.load_active();
            context.build(true);

            transparency_context.load_active();
            transparency_context.build(true);

            context.increment_context_id();
            transparency_context.increment_context_id();

            context.fetch()->ensure_screen_buffers(window.width, window.height);
            transparency_context.fetch()->ensure_screen_buffers(window.width, window.height);

            gpu_context = context.fetch();


            window.set_object_data(*gpu_context);
            window.set_light_data(light_data);
            //window.set_tex_data(gpu_context->tex_gpu);

            #ifdef SPACE

            if(s.quality != 0)
            {
                g_star_cloud = point_cloud_manager::alloc_point_cloud(stars);

                space_res.init(window.width, window.height);
            }

            #endif

            //text::set_renderwindow(window.window);

            cl::cqueue.finish();
            cl::cqueue2.finish();
            cl::cqueue_ooo.finish();
        }

        /*if(s.enable_debugging)
        {
            if(once<sf::Keyboard::Tab>() && window.focus)
            {
                network_player play = server.make_networked_player(100, &context, &transparency_context, &current_state, &phys, s.quality);

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

                //net_test->my_cape.tick(net_test);

                net_test->shared_tick();

                net_test->tick_cape();

                net_test->do_foot_sounds();

                net_test->update_texture_by_part_hp();

                net_test->check_and_play_sounds();

                net_test->position_cosmetics();

                ///death is dynamically calculated from part health
                if(!net_test->dead())
                {
                    net_test->update_name_info(true);

                    net_test->update_lights();
                }
            }
        }*/

        my_fight->cube_info = &debug_map_cube;

        if(controls_state == 0 && window.focus && !in_menu)
        {
            window.update_mouse();
        }
        if((controls_state == 1 || controls_state == 2) && window.focus && !in_menu)
        {
            window.update_mouse(window.width/2, window.height/2, true, true);
        }

        bool should_transition = false;
        bool trombone_transition = false;

        should_transition = once<sf::Keyboard::X>();
        trombone_transition = once<sf::Keyboard::Num1>();

        should_transition |= trombone_transition;

        if(should_transition && window.focus && !in_menu)
        {
            controls_state = (controls_state + 1) % 2;

            ///call once to reset mouse to centre
            window.update_mouse(window.width/2, window.height/2, true, true);
            ///call again to reset mouse dx and dy to 0
            window.update_mouse(window.width/2, window.height/2, true, true);

            window.c_rot_keyboard_only = window.c_rot;
            //window.c_rot = {0,0,0};
            //window.c_rot_keyboard_only = {0,0,0};
        }

        if(trombone_transition)
        {
            if(controls_state < 2)
                controls_state = 2;
            else
                controls_state = 1;
        }

        if(controls_state == 0 && window.focus && !in_menu)
            debug_controls(my_fight, window);
        if(controls_state == 1 && window.focus && !in_menu)
            fps_controls(my_fight, window);
        if(controls_state == 2 && window.focus && !in_menu)
            fps_trombone_controls(my_fight, window, trombone_manage);

        control_input c_input;

        if(controls_state == 0)
            c_input = control_input();

        if(controls_state == 1 || controls_state == 2)
            c_input = control_input(std::bind(fps_camera_controls, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, my_fight),
                              process_controls_empty);

        window.set_input_handler(c_input);

        if(once<sf::Keyboard::U>() && window.focus)
        {
            central_pip = !central_pip;
        }


        /*if(once<sf::Keyboard::Num1>() && window.focus)
        {
            server.ping();
        }*/

        server.set_my_fighter(my_fight);

        if(!in_menu)
            server.tick(&context, &transparency_context, &current_state, &phys);

        #ifdef DELAY_SIMULATE
        delay_tick();
        #endif // DELAY_SIMULATE

        ///debugging
        if(!server.joined_game && !in_menu)
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

        if(once<sf::Keyboard::B>() && window.focus)
        {
            my_fight->respawn();
        }

        ///so just respawning doesnt fix, sometimes (mostly) doing enter does
        ///but not alwaysf
        ///something very odd. Rewrite texturing
        if(once<sf::Keyboard::Return>() && window.focus && s.enable_debugging)
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

        {
            fight2.queue_attack(attacks::OVERHEAD);
            //fight2.queue_attack(attacks::SLASH);
            //fight2.queue_attack(attacks::BLOCK);

            fight2.tick();
            fight2.shared_tick();
            fight2.tick_cape();

            fight2.update_render_positions();

            //fight2.walk_dir({-1, -1});

            fight2.do_foot_sounds();

            fight2.update_texture_by_part_hp();

            fight2.check_and_play_sounds();

            fight2.position_cosmetics();

            //fight2.crouch_tick(true);

            if(!fight2.dead())
            {
                //fight2.update_name_info();

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

        my_fight->shared_tick();

        my_fight->tick_cape();

        my_fight->update_texture_by_part_hp();

        my_fight->check_and_play_sounds(true);

        my_fight->local_name = s.name;


        my_fight->update_render_positions();

        my_fight->position_cosmetics();

        if(!my_fight->dead())
        {
            //my_fight->update_name_info();

            my_fight->update_lights();
        }


        #ifdef MYO_ARMBAND
        myo_dat.tick();

        quat quat = myo_dat.dat.qrot;

        quat = quat.norm();

        mat3f mat_rot = myo_dat.dat.qrot.get_rotation_matrix();

        vec3f irot = mat_rot.get_rotation();

        //irot.v[0] += M_PI;
        //irot.v[1] += M_PI/2;
        //irot.v[2] += M_PI;


        vec3f euler_rot = irot;

        my_fight->weapon.rot = euler_rot;
        #endif // MYO_ARMBAND

        ///we can use the foot rest position to play a sound when the
        ///current foot position is near that
        ///remember, only the y component!

        particle_effect::tick();

        ///about 0.2ms slower than not doing this
        light_data = light::build(&light_data);
        window.set_light_data(light_data);

        ///ergh
        sound::set_listener(my_fight->parts[bodypart::BODY].global_pos, my_fight->parts[bodypart::BODY].global_rot);

        ///so that the listener position is exactly the body part
        my_fight->do_foot_sounds(true);
        trombone_manage.tick(window, my_fight);

        sound::update_listeners();

        ///so, we blit space to screen, but that might not have finished before
        ///the async event for the draw_bulk_objs_n event has finished
        ///and this can fire
        window.blit_to_screen(*context.fetch());

        if(!my_fight->dead())
        {
            my_fight->update_name_info();
        }

        if(!fight2.dead())
        {
            fight2.update_name_info();
        }

        server.update_fighter_name_infos();

        ///I need to reenable text drawing
        ///possibly split up window.display into display and flip
        ///then have display set a flag if its appropriate to flip the screen
        ///that way we can still keep the async rendering
        ///but also allow drawing on top of teh 3d scene
        ///we'll need to allow window querying to say should we draw
        ///otherwise in async we'll waste huge performance
        ///in synchronous that's not a problem

        if(once<sf::Keyboard::F1>() && window.focus)
        {
            show_ftime = !show_ftime;
        }

        ui_manage.tick(window.get_frametime_ms());

        if(controls_state == 0)
        {
            ui_manage.tick_settings(window.get_frametime_ms());
        }

        ui_manage.tick_frametime_graph(window.get_frametime_ms(), show_ftime);

        ui_manage.tick_health_display(my_fight);

        ui_manage.tick_render();

        window.window.resetGLStates();

        if(window.render_me && !in_menu)
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


        ///this is pretty dodgey to do the menu like this
        if(menu_handler.should_do_menu())
        {
            menu_handler.do_menu(window);
        }

        window.flip();

        ///so this + render_event is basically causing two stalls
        window.render_block(); ///so changing render block above blit_to_screen also fixes


        object_context_data* cdat = context.fetch();

        ///I think this is now quite redundant
        window.set_object_data(*cdat);

        #define REAL_INPUT
        #ifdef REAL_INPUT
        if(!in_menu)
            window.process_input();
        #endif

        #ifdef CLAMP_VIEW
        window.c_rot.x = clamp(window.c_rot.x, -M_PI/2.f, M_PI/2.f);
        #endif

        ///lets do the debug stuff here
        ///ok, we're going to have to pass this in to the keyboard controls
        ///and then adjust movement based on these parameters?
        ///split up into tick, and then external control function
        ///use control function in fps and debug control functions
        ///walk_dir walks relative to current angle
        ///we need to do walk dir as normal (we're in local coordinates remember), then transform after with global rotation
        ///and position offset (and local direction moving) which is INDEPENDENT to walk_dir

        ///add a smoothed up vector with a length of pos.v[1]
        #if 0
        {
            vec3f ncamera_rot = debug_map_cube.get_update_smoothed_camera_with_euler_offset(-xyz_to_vec(window.c_rot_keyboard_only), 24 * game_map::scale);

            debug_map_cube.transition_camera(24 * game_map::scale);

            window.set_camera_rot(conv_implicit<cl_float4>(ncamera_rot));
            ///ok, so its the roll component of the camera we need to fiddle with

            vec3f apos = debug_map_cube.get_absolute_3d_coords((vec2f){-0, -0}, 24 * game_map::scale);

            float height_off_ground = my_fight->pos.v[1];

            vec3f up_vector = debug_map_cube.get_smooth_up_vector(debug_map_cube.face, map_cube_info::smooth_offset, 24 * game_map::scale, FLOOR_CONST);

            up_vector = up_vector.norm() * height_off_ground;

            apos = apos + up_vector;

            vec2f mov_dir = {0,0};

            mov_dir.v[0] += key.isKeyPressed(sf::Keyboard::D);
            mov_dir.v[0] -= key.isKeyPressed(sf::Keyboard::A);

            mov_dir.v[1] += key.isKeyPressed(sf::Keyboard::W);
            mov_dir.v[1] -= key.isKeyPressed(sf::Keyboard::S);

            vec2f nmov = debug_map_cube.transform_move_dir(mov_dir);

            ///here we now want to fighter->walk_dir
            ///we need to condense all the above shit

            const float test_speed = 2.f;

            ///so either my_fight->pos can be global position
            ///or it can be strictly local
            ///the below assumes its strictly local
            ///i guess this is a whole local -> global transition
            ///so we want to define a translation between the base face position (my_fight->pos) to the global position
            debug_map_cube.pos_within_plane = {my_fight->pos.v[0], my_fight->pos.v[2]};

            debug_map_cube.pos_within_plane = debug_map_cube.pos_within_plane + (vec2f){24 * game_map::scale, 24 * game_map::scale}/2.f;

            debug_map_cube.transition_camera(24 * game_map::scale);

            ///need to fix walk_dir directions to be nmov

            ///so this fundamentally works, but we need to change fighter to have both global and local pos
            vec2f transitioned_pos = debug_map_cube.get_new_coordinates(debug_map_cube.pos_within_plane, 24 * game_map::scale) - (24*game_map::scale/2.f);
            my_fight->set_pos({transitioned_pos.v[0], my_fight->pos.v[1], transitioned_pos.v[1]});

            ///ok, so we now need to sync fighter keyboarsd controls with this class
            ///works, we need to correct for the floor though
            debug_cube->set_pos({apos.v[0], apos.v[1], apos.v[2]});

            window.set_camera_pos(debug_cube->pos);

            ///ok, flush locations being called just before here, so the position is being overwritten
            {
                my_fight->update_render_positions();

                my_fight->position_cosmetics();

                if(!my_fight->dead())
                {
                    my_fight->update_name_info();

                    my_fight->update_lights();
                }
            }

            ///need to update lights too
            mat3f acc_rot = debug_map_cube.get_smoothed_accumulate(24 * game_map::scale);

            ///ok, we need to rotate relative to centre of fighter, which is... head pos?

            vec3f rotation_centre = my_fight->parts[bodypart::HEAD].global_pos;

            vec3f overall_offset = apos;

            ///ok, so we can make this super easy by just giving them a parent, and rotating it
            for(part& p : my_fight->parts)
            {
                vec3f prot = xyz_to_vec(p.obj()->rot);

                mat3f pmat;

                pmat.load_rotation_matrix(prot);

                mat3f acc = acc_rot.transp() * pmat;

                vec3f erot = acc.get_rotation();

                p.global_rot = erot;
                p.obj()->set_rot({erot.v[0], erot.v[1], erot.v[2]});


                vec3f cpos = p.global_pos;

                vec3f relative = cpos - rotation_centre;

                vec3f new_pos = relative.rot({0,0,0}, acc_rot.transp().get_rotation());

                p.global_pos = new_pos + overall_offset;
                p.update_model();
            }

            {
                sword& p = my_fight->weapon;

                vec3f prot = xyz_to_vec(p.obj()->rot);

                mat3f pmat;

                pmat.load_rotation_matrix(prot);

                mat3f acc = acc_rot.transp() * pmat;

                vec3f erot = acc.get_rotation();

                p.obj()->set_rot({erot.v[0], erot.v[1], erot.v[2]});

                ///recalculated, so we dont accumulate
                vec3f cpos = xyz_to_vec(p.obj()->pos);

                vec3f relative = cpos - rotation_centre;

                vec3f new_pos = relative.rot({0,0,0}, acc_rot.transp().get_rotation());

                p.obj()->set_pos(conv_implicit<cl_float4>(new_pos + overall_offset));
            }

            my_fight->recalculate_link_positions_from_parts();

            if(!my_fight->dead())
                my_fight->update_lights();
        }
        #endif

        context.flush_locations();
        transparency_context.flush_locations();

        #ifdef SPACE

        if(s.quality != 0)
        {
            space_res.update_camera(window.c_pos, window.c_rot);

            space_res.set_depth_buffer(cdat->depth_buffer[cdat->nbuf]);
            space_res.set_screen(cdat->g_screen);
        }

        #endif

        compute::event event;

        if(window.can_render())
        {
            ///if we make this after and put the clear in here, we can then do blit_space every time
            ///with no flickering, fewer atomics, and better performance
            ///marginally though

            //if(s.quality != 0)
            //    space_res.draw_galaxy_cloud_modern(g_star_cloud, (cl_float4){-5000,-8500,0});

            window.generate_realtime_shadowing(*cdat);
            window.draw_bulk_objs_n(*cdat);
            window.do_pseudo_aa();

            if(s.motion_blur_strength > 0.01f)
                window.do_motion_blur(*cdat, s.motion_blur_strength, s.motion_blur_camera_contribution);

            //window.draw_godrays(*cdat);
        }

        fight.update_gpu_name();
        fight2.update_gpu_name();
        server.update_fighter_gpu_name();
        window.window.resetGLStates();

        if(window.can_render())
        {
            //if(s.quality != 0)
            //    event = space_res.blit_space_to_screen(*cdat);
        }

        if(window.can_render())
        {
            object_context_data* tctx = transparency_context.fetch();

            window.draw_bulk_objs_n(*tctx);

            event = window.blend_with_depth(*tctx, *cdat);

            cdat->swap_buffers();
            tctx->swap_buffers();

            window.increase_render_events();

            #ifdef FASTER_BUT_LESS_CONSISTENT
            window.set_render_event(event);
            #endif
        }

        context.build_tick(true);
        transparency_context.build_tick(true);

        context.flip();
        transparency_context.flip();

        ///can get changed by ui
        window.set_mouse_sens(s.mouse_sens);

        if(key.isKeyPressed(sf::Keyboard::M))
            std::cout << c.getElapsedTime().asMicroseconds() << std::endl;

        /*printf("TTL %f\n", clk.getElapsedTime().asMicroseconds() / 1000.f);

        return 0;*/
    }


    glFinish();
    cl::cqueue.finish();
    cl::cqueue2.finish();
    cl::cqueue_ooo.finish();
    glFinish();

    window.window.close();

    ImGui::SFML::Shutdown();
}
