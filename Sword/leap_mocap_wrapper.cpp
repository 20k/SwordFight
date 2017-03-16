#include "leap_mocap_wrapper.hpp"

leap_mocap_wrapper::leap_mocap_wrapper(object_context& ctx, int hand_side) : mocap_manager(&capture_manager)
{
    hand = hand_side;

    capture_manager.init_manual_containers(ctx, 1);

    capture_manager.load("Mocap/");

    ///left_hand
    /*if(hand == 0)
    {
        mocap_manager.push_mocap_animation(leap_replay_names::LIDLE1);
    }
    if(hand == 1)
    {
        mocap_manager.push_mocap_animation(leap_replay_names::RHAND_IDLE);
    }

    mocap_manager.finish_mocap_building_animation();*/

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

    perpetual_animation& anim = looping_animations.looping_animations.back();

    anim.start(&capture_manager);


    /*LIDLE1,
    LSNAP,
    LSNAP_FAST,
    RHAND_IDLE,*/
}

void leap_mocap_wrapper::tick(objects_container* sword)
{
    capture_manager.hide_manual_containers();

    capture_manager.tick_replays();

    mocap_manager.tick();
    looping_animations.tick(&capture_manager);

    attach_replays_to_fighter_sword(capture_manager, sword);
    fix_replays_clipping(capture_manager, sword);
}

void leap_mocap_wrapper::transition(leap_animation_names_t animation_id)
{
    if(!leap_animation_names::same_hand(animation_id, hand))
        return;

    mocap_animation& animation = mocap_manager.animations[animation_id];

    looping_animations.currently_going.back().interrupt_with_animation(&capture_manager, animation);
}
