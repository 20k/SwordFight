#include "leap_mocap_wrapper.hpp"


leap_mocap_wrapper::leap_mocap_wrapper(object_context& pctx, int hand_side, vec3f col) : ctx(&pctx), mocap_manager(&capture_manager)
{
    hand = hand_side;

    //capture_manager.init_manual_containers(*ctx, 1);
    capture_manager.init_manual_containers_with_col(*ctx, 1, col);

    capture_manager.load("Mocap/");

    mocap_manager.push_mocap_animation(leap_replay_names::LIDLE1);
    mocap_manager.finish_mocap_building_animation();

    mocap_manager.push_mocap_animation(leap_replay_names::LSNAP);
    mocap_manager.finish_mocap_building_animation();

    mocap_manager.push_mocap_animation(leap_replay_names::LSNAP_FAST);
    mocap_manager.finish_mocap_building_animation();

    mocap_manager.push_mocap_animation(leap_replay_names::RHAND_IDLE);
    mocap_manager.finish_mocap_building_animation();

    perpetual_animation base_loop;

    if(hand == 0)
    {
        base_loop.set_base_animation(mocap_manager.animations[leap_animation_names::LEFT_IDLE]);
    }
    if(hand == 1)
    {
        base_loop.set_base_animation(mocap_manager.animations[leap_animation_names::RIGHT_IDLE]);
    }

    looping_animations.looping_animations.push_back(base_loop);
    looping_animations.currently_going.push_back(base_loop);

    perpetual_animation& anim = looping_animations.currently_going.back();

    anim.start(&capture_manager);


    /*LIDLE1,
    LSNAP,
    LSNAP_FAST,
    RHAND_IDLE,*/
}

void leap_mocap_wrapper::handle_automatic_transitions()
{
    if(hand == 1)
        return;

    if(clk.getElapsedTime().asSeconds() > time_between_snaps_s)
    {
        clk.restart();

        transition(leap_animation_names::LEFT_SNAP);
    }
}

void leap_mocap_wrapper::tick(objects_container* sword)
{
    assert(sword != nullptr);

    handle_automatic_transitions();

    capture_manager.hide_manual_containers();

    capture_manager.tick_replays();

    mocap_manager.tick();
    looping_animations.tick(&capture_manager);

    attach_replays_to_fighter_sword(capture_manager, sword);
    fix_replays_clipping(capture_manager, sword);
}

void leap_mocap_wrapper::set_team(int id)
{
    if(id == team)
        return;

    lg::log("TEAM ", id, "\n\n\n\n\n\n\n\n\n\n\n");

    capture_manager.destroy_manual_containers();

    vec3f col = team_info::get_team_col(id);

    *this = leap_mocap_wrapper(*ctx, hand, col);

    team = id;
}

void leap_mocap_wrapper::transition(leap_animation_names_t animation_id)
{
    if(!leap_animation_names::same_hand(animation_id, hand))
        return;

    //lg::log("ANIM ID ", animation_id);

    mocap_animation& animation = mocap_manager.animations[animation_id];

    looping_animations.currently_going.back().interrupt_with_animation(&capture_manager, animation);
}
