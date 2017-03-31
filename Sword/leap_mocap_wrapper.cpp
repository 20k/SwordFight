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

    mocap_manager.push_mocap_animation(leap_replay_names::LTROMBONE);
    mocap_manager.finish_mocap_building_animation();

    mocap_manager.push_mocap_animation(leap_replay_names::RHAND_IDLE);
    mocap_manager.finish_mocap_building_animation();

    mocap_manager.push_mocap_animation(leap_replay_names::RTROMBONE);
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

    ///replace this if we want to swap perpetual animations in short term
    looping_animations.looping_animations.push_back(base_loop);
    looping_animations.currently_going.push_back(base_loop);

    perpetual_animation& anim = looping_animations.currently_going.back();

    anim.start(&capture_manager);


    /*LIDLE1,
    LSNAP,
    LSNAP_FAST,
    RHAND_IDLE,*/
}

leap_mocap_wrapper::~leap_mocap_wrapper()
{
    for(auto& o : capture_manager.ctrs)
    {
        //o->set_active(false);
        //o->parent->build_request();
        //o->parent->destroy(o);
    }
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

vec3f get_trombone_lhand_offset(vec3f up)
{
    return up * 40;
}

vec3f get_trombone_rhand_offset(vec3f up)
{
    return up * -10;
}

void leap_mocap_wrapper::tick(objects_container* sword)
{
    assert(sword != nullptr);

    handle_automatic_transitions();

    capture_manager.hide_manual_containers();

    capture_manager.tick_replays();

    mocap_manager.tick();
    looping_animations.tick(&capture_manager);

    hand_offset_info offset_info;

    if(pose == 1)
    {
        quat lhand_quat;
        quat rhand_quat;

        lhand_quat.load_from_axis_angle({0, 0, 1, -M_PI/2});
        rhand_quat.load_from_axis_angle({0, 0, 1, M_PI/2});

        offset_info.lhand_rot = lhand_quat;
        offset_info.rhand_rot = rhand_quat;

        offset_info.get_lhand_offset = get_trombone_lhand_offset;
        offset_info.get_rhand_offset = get_trombone_rhand_offset;
    }

    attach_replays_to_fighter_sword(capture_manager, sword, 0.6f, 10 * 0.8f, offset_info);

    if(pose == 0)
        fix_replays_clipping(capture_manager, sword);
}

void leap_mocap_wrapper::set_team(int id)
{
    if(id == team)
        return;

    capture_manager.destroy_manual_containers();

    vec3f col = team_info::get_team_col(id);

    *this = leap_mocap_wrapper(*ctx, hand, col);

    team = id;
}

void leap_mocap_wrapper::set_animation_pose(int pose_id)
{
    pose = pose_id;
}

void leap_mocap_wrapper::transition(leap_animation_names_t animation_id)
{
    if(!leap_animation_names::same_hand(animation_id, hand))
        return;

    //lg::log("ANIM ID ", animation_id);

    mocap_animation& animation = mocap_manager.animations[animation_id];

    //looping_animations.currently_going.back().interrupt_with_animation(&capture_manager, animation);
    looping_animations.currently_going.back().add_animation(animation);
}

vec3f leap_mocap_wrapper::get_hand_pos()
{
    for(auto& rmap : capture_manager.currently_replaying_map)
    {
        current_replay& replay = rmap.second;

        int cid = 2 * 4 + 0;

        if(cid >= replay.containers.size())
            return {0.f,0,0};

        //printf("ello %f %f %f\n", replay.containers[cid]->pos.x, replay.containers[cid]->pos.y, replay.containers[cid]->pos.z);

        ///this is baffling. Where the christ are the nans coming from?
        ///Oh well. IM SURE NO ISSUES WHATSOEVER WILL ARISE FROM ME THINKING THIS IS FIXED
        ///Ok. Underlying data has nans in. We've fixed the capture management to not allow nans to filter though
        ///We really need to process underlying data and remove any nan frames. How did nans even sneak into the data in the first place?
        //if(any_nan(xyz_to_vec(replay.containers[cid]->pos)))
        //    continue;

        return xyz_to_vec(replay.containers[cid]->pos);
    }

    return {0.f,0,0};
}
