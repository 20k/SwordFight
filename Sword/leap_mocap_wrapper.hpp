#ifndef LEAP_MOCAP_WRAPPER_HPP_INCLUDED
#define LEAP_MOCAP_WRAPPER_HPP_INCLUDED

#include "../Leap_motioncapture/leap_motion_capture_management.hpp"
#include "../Leap_motioncapture/mocap_animation_management.hpp"
#include "../Leap_motioncapture/perpetual_animation_management.hpp"
#include "../sword_server/teaminfo_shared.hpp"

struct object_context;
struct objects_container;

namespace leap_replay_names
{
    ///actual filenames
    enum replay_names
    {
        LIDLE1,
        LSNAP,
        LSNAP_FAST,
        LTROMBONE,
        RHAND_IDLE,
        RTROMBONE,
        COUNT
    };
}

namespace leap_animation_names
{
    enum animation_names
    {
        LEFT_IDLE,
        LEFT_SNAP,
        LEFT_SNAP_FAST,
        LEFT_TROMBONE_IDLE,
        RIGHT_IDLE,
        RIGHT_TROMBONE_IDLE,
        COUNT
    };

    static
    std::vector<bool> is_left =
    {
        true,
        true,
        true,
        true,
        false,
        false,
    };

    inline
    bool same_hand(leap_animation_names::animation_names name, int hand)
    {
        ///throw?
        if(name < 0 || name >= is_left.size())
            return false;

        if(is_left[name] && hand == 0)
            return true;

        if(!is_left[name] && hand == 1)
            return true;

        return false;
    }
}

using leap_replay_name_t = leap_replay_names::replay_names;
using leap_animation_names_t = leap_animation_names::animation_names;

///call tick after updating fighter render positions
struct leap_mocap_wrapper
{
    object_context* ctx;
    sf::Clock clk;

    leap_motion_capture_manager capture_manager;
    mocap_animation_manager mocap_manager;
    perpetual_animation_manager looping_animations;

    int hand;
    int team = 0;
    int pose = 0; ///0 -> sword, 1 -> trombone

    float time_between_snaps_s = 10.f;

    ///hand_side 0 -> left, hand_side 1 -> right
    ///we're moving out of leap stuff and into fighter territory mashing together
    ///this this is now allowed to drastically venture into the wilds of fighter specific code
    leap_mocap_wrapper(object_context& ctx, int hand_side, vec3f col = team_info::get_team_col(0));
    ~leap_mocap_wrapper();

    void tick(objects_container* sword);

    ///animation ID, NOT replay ID, although these may be the same initially
    void transition(leap_animation_names_t animation_id);

    void set_team(int id);
    void set_animation_pose(int pose_id);

    vec3f get_hand_pos();

private:
    ///called in tick
    void handle_automatic_transitions();
};

#endif // LEAP_MOCAP_WRAPPER_HPP_INCLUDED
