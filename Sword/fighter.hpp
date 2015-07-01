#ifndef FIGHTER_HPP_INCLUDED
#define FIGHTER_HPP_INCLUDED

#include "../openclrenderer/objects_container.hpp"

#include "vec.hpp"

#include <SFML/Graphics.hpp>

namespace mov
{
    enum movement_type : unsigned int
    {
        NONE = 0,
        DAMAGING = 1,
        BLOCKING = 2,
        WINDUP = 4,
        MOVES = 8, ///physically moves character
        CAN_STOP = 16 ///movement can be interrupted
    };
}

typedef mov::movement_type movement_t;

namespace bodypart
{
    enum bodypart : unsigned int
    {
        HEAD,
        LUPPERARM,
        LLOWERARM,
        RUPPERARM,
        RLOWERARM,
        LHAND,
        RHAND,
        BODY,
        LUPPERLEG,
        LLOWERLEG,
        RUPPERLEG,
        RLOWERLEG,
        LFOOT,
        RFOOT,
        COUNT
    };

    static int which_side[COUNT] =
    {
        2,
        0,
        0,
        1,
        1,
        0,
        1,
        2,
        0,
        0,
        1,
        1,
        0,
        1
    };

    static float foot_modifiers[COUNT] =
    {
        1.f/12,
        1.f/5,
        1.f/12,
        1.f/5,
        1.f/12,
        0,
        0,
        1.f/8.f,
        0,
        0,
        0,
        0,
        0,
        0
    };

    static std::vector<std::string> names =
    {
        "HEAD",
        "LUPPERARM",
        "LLOWERARM",
        "RUPPERARM",
        "RLOWERARM",
        "LHAND",
        "RHAND",
        "BODY",
        "LUPPERLEG",
        "LLOWERLEG",
        "RUPPERLEG",
        "RLOWERLEG",
        "LFOOT",
        "RFOOT",
        "COUNT"
    };

    static float scale = 100.f;

    ///internal storage
    extern const vec3f* init_default();

    ///for all the bodyparts
    static const vec3f* default_position = init_default();
}

typedef bodypart::bodypart bodypart_t;

struct part
{
    bodypart_t type;
    vec3f pos;
    vec3f rot;
    objects_container model;

    int team;

    float hp;

    void set_type(bodypart_t); ///sets me up in the default position
    void set_pos(vec3f pos);
    void set_rot(vec3f rot);
    void set_team(int _team);
    void damage(float dam);

    part();
    part(bodypart_t);
    ~part();
};

///one single movement

///need a bool for does_damage
///use a bitfield enum for stuff
struct movement
{
    ///don't really know where this should go. Id of the bodypart hit potentially with this move
    int hit_id;

    size_t id;
    static size_t gid;

    sf::Clock clk;

    float end_time;
    vec3f fin;
    vec3f start; ///this is internal, but because i'm bad programmer this is public
    int type; ///0 for linear, 1 for slerp

    int hand; ///need leg support too
    bodypart_t limb;

    bool going;

    movement_t move_type;

    bool does(movement_t t);
    void set(movement_t t);

    ///swap me for an enum
    void load(int hand, vec3f end_pos, float time, int type, bodypart_t, movement_t move_type);

    float time_remaining();

    float get_frac();

    void fire();

    bool finished();

    movement();
    movement(int hand, vec3f end_pos, float time, int type, bodypart_t, movement_t move_type);
};

namespace attacks
{
    enum attacks
    {
        SLASH,
        OVERHEAD,
        REST,
        BLOCK,
        COUNT,
        RECOIL
    };
}


typedef attacks::attacks attack_t;

struct attack
{
    std::vector<movement> moves;
};

static std::vector<movement> overhead =
{
    {0, {-150, -0, -20}, 400, 0, bodypart::LHAND, mov::WINDUP}, ///windup
    {0, {100, -150, -140}, 500, 1, bodypart::LHAND, mov::DAMAGING} ///attack
};

static std::vector<movement> recoil =
{
    {0, {-150, -0, -20}, 800, 0, bodypart::LHAND, mov::NONE} ///recoiling doesnt block or damage
};

static std::vector<movement> slash =
{
    {0, {-150, -80, -40}, 350, 0, bodypart::LHAND, mov::WINDUP}, ///windup
    {0, {100, -80, -140}, 450, 1, bodypart::LHAND, mov::DAMAGING} ///attack
};

static std::vector<movement> rest =
{
    {0, {0, -200, -100}, 500, 1, bodypart::LHAND, mov::NONE}
};

static std::vector<movement> block =
{
    {0, {-50, -80, -20}, 300, 0, bodypart::LHAND, mov::BLOCKING},
    {0, {100, -150, -140}, 400, 0, bodypart::LHAND, mov::NONE}
};

static std::map<attack_t, attack> attack_list =
{
    {attacks::OVERHEAD, {overhead}},
    {attacks::SLASH, {slash}},
    {attacks::REST, {rest}},
    {attacks::BLOCK, {block}},
    {attacks::RECOIL, {recoil}}
};


/*static std::map<attack_t, attack> attack_list2 =
{
    {attacks::OVERHEAD, {attacks::OVERHEAD,
        {{0, {-150, -0, -20}, 400.f, 0}}
    }}
};*/

struct sword
{
    objects_container model;

    vec3f pos;
    vec3f rot;
    vec3f dir;

    int team;

    sword();

    void set_pos(vec3f _pos); //? model now has different pos to actual due to top down approach
    void set_rot(vec3f _rot);

    void set_team(int _team);

    void scale();
};

///define attacks in terms of a start, an end, a time, and possibly a smoothing function


struct pos_rot
{
    vec3f pos;
    vec3f rot;
};

pos_rot to_world_space(vec3f world_pos, vec3f world_rot, vec3f local_pos, vec3f local_rot);

struct physics;

struct fighter
{
    int side;

    const vec3f* rest_positions;

    part parts[bodypart::COUNT];
    vec3f old_pos[bodypart::COUNT];

    fighter();

    ///ideally we want movements to be ptrs, then delete them on removal
    std::map<bodypart_t, movement> action_map;

    std::vector<movement> moves;

    sword weapon;
    int stance; ///0 means perpendicular to velocity, 1 means parallel

    vec3f focus_pos; ///where to put my hands and sword
    ///this should almost certainly be relative


    int team;

    vec3f pos;
    vec3f rot;

    physics* phys;

    sf::Clock idle;

    bool idling;

    ///sigh, cant be on init because needs to be after object load
    void scale();

    void IK_hand(int hand, vec3f pos);
    void IK_foot(int foot, vec3f pos);

    void linear_move(int hand, vec3f pos, float time, bodypart_t b);
    void spherical_move(int hand, vec3f pos, float time, bodypart_t b);

    void move_hands(vec3f pos);

    void set_stance(int _stance);

    void queue_attack(attack_t type);

    void add_move(const movement& m);

    void update_sword_rot();

    void tick();
    //void walk(int which); ///temp

    void walk_dir(vec2f dir); ///z, x

    void set_pos(vec3f);
    void set_rot(vec3f);

    movement* get_movement(size_t id);

    void update_render_positions();
    void set_team(int _team);

    void set_physics(physics* phys);

    void cancel(bodypart_t type);

    bool can_attack(bodypart_t type);

    void damage(bodypart_t type, float d);

    int process_foot(bodypart_t foot, int stage, vec2f dir, float d, std::vector<vec3f> positions, float time_wasted = 0, bool can_skip = true, movement_t extra_tags = mov::NONE);

    void process_foot_g2(bodypart_t foot, vec2f dir, int& stage, float& frac, vec3f seek, vec3f prev, float seek_time, float elapsed_time);

private:
    size_t left_id;
    size_t right_id;

    int left_stage;
    int right_stage;

    bool left_full;

    bool left_fired;
    bool right_fired;

    float left_frac;
    float right_frac;

    bool skip_stride(vec3f, vec3f, bodypart_t, bodypart_t);

    int idle_fired_first;

    vec2f move_dir;
};


#endif // FIGHTER_HPP_INCLUDED
