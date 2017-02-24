#ifndef FIGHTER_HPP_INCLUDED
#define FIGHTER_HPP_INCLUDED

#include "../openclrenderer/objects_container.hpp"
#include "trombone_manager.hpp"

#include <vec/vec.hpp>

#include <SFML/Graphics.hpp>

#include "bbox.hpp"

#include "cape.hpp"
//#include "map_tools.hpp"

struct damage_info
{
    float hp_delta = 0;
    int32_t id_hit_by = -1;
};

namespace mov
{
    enum movement_type : unsigned int
    {
        NONE = 0,
        DAMAGING = 1,
        BLOCKING = 1 << 1,
        WINDUP = 1 << 2,
        MOVES = 1 << 3, ///physically moves character
        CAN_STOP = 1 << 4, ///movement can be interrupted
        FINISH_INDEPENDENT = 1 << 5, ///for hand movements, are they independent of the view
        START_INDEPENDENT = 1 << 6, ///for hand movements, are they independent of the view
        ///as a consequence of the animation system, start_independent is not necessary. I might keep it for clarity however
        LOCKS_ARMS = 1 << 7, ///for visual reasons, some attacks might want to lock the arms
        PASS_THROUGH_SCREEN_CENTRE = 1 << 8,
        FINISH_AT_90 = 1 << 9, ///degrees, ie perpendicular to the normal sword rotation
        ///we need a CAN_BE_COMBINED tag, which means that two movements can be applied at once
        FINISH_AT_SCREEN_CENTRE = 1 << 10,
        OVERHEAD_HACK = 1 << 11, ///hack to fix overhead through centre
        NOSLOW_END = 1 << 12,
        NOSLOW_START = 1 << 13,
        INVERSE_OVERHEAD_HACK = 1 << 14,
        IS_RECOIL = 1 << 15,
        NO_MOVEMENT = 1 << 16,
        ALT_ATTACK = 1 << 17,
        CONTINUOUS_SPRINT = 1 << 18, ///terminates the moment we stop doing the attack
        NO_POST_QUEUE = 1 << 19, ///cannot be queued after
        NO_CAMERA_LIMIT = 1 << 20,
        ORIENT_TOWARDS_FACE = 1 << 21,
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

    constexpr static int which_side[COUNT] =
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

    constexpr static float foot_modifiers[COUNT] =
    {
        2.f/8,
        1.f/5,
        1.f/12,
        1.f/5,
        1.f/12,
        0,
        0,
        1.f/8,
        1.f/4,
        0,
        1.f/4,
        0,
        0,
        0
    };

    constexpr static float overall_bob_modifier = 1.5f;

    constexpr static float idle_modifiers[COUNT] =
    {
        1.f/12,
        1.f/8,
        1.f/12,
        1.f/8,
        1.f/12,
        0,
        0,
        1.f/4,
        0.f/18,
        0,
        0.f/18,
        0,
        0,
        0
    };

    constexpr static float idle_offsets[COUNT] =
    {
        0,
        0,
        -M_PI/8.f,
        0,
        -M_PI/8.f,
        0,
        0,
        M_PI/8.f,
        0,
        0,
        0,
        0,
        0,
        0
    };

    constexpr const float idle_height = 10.f;
    constexpr const float idle_speed = 1.5f;

    ///hip twist in relation to weapon movement
    constexpr static float waggle_modifiers[COUNT] =
    {
        0.f,
        0.f,
        0.f,
        0.f,
        0.f,
        0.f,
        0.f,
        0.f,
        0.3f,
        -0.05f,
        0.3f,
        -0.05f,
        0.f,
        0.f
    };

    ///lets have a forward wiggle thats also based on the hip up/down
    ///fix the god damn reversal
    constexpr static float forward_thrust_hip_relative[COUNT] =
    {
        0.f,
        -1.f,
        -0.5f,
        -1.f,
        -0.5f,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    };

    constexpr float overall_forward_thrust_mod = 0.04f;
    constexpr float overall_forward_thrust_mod_nonsprint = 0.02f;

    constexpr float sprint_acceleration_time_ms = 250.f;

    static std::vector<std::string> short_names =
    {
        "Head",
        "RShldr",
        "RElbw",
        "LShldr",
        "LElbw",
        "RHand",
        "LHand",
        "Body",
        "RHip",
        "RKnee",
        "LHip",
        "LKnee",
        "RFoot",
        "LFoot",
        "Error"
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

    ///the body is backwards... i really need to fix this
    static std::vector<std::string> ui_names =
    {
        "head",
        "right shoulder",
        "right elbow",
        "left shoulder",
        "left elbow",
        "right hand",
        "left hand",
        "chest",
        "right hip",
        "right knee",
        "left hip",
        "left knee",
        "right foot",
        "left foot",
        "Uh oh, something broke"
    };

    constexpr static float scale = 100.f;

    ///internal storage
    extern const vec3f* init_default();

    ///for all the bodyparts
    static const vec3f* default_position = init_default();

    constexpr const float specular = 0.75f;
}

typedef bodypart::bodypart bodypart_t;

namespace fighter_stats
{
    const static float speed = 1.4f;
    const static float sprint_speed = 1.3f;
    const static float bubble_modifier_relative_to_approx_dim = 1.f;
    const static float flinch_time_ms = 400.f;
}

struct damage_information
{
    float hp_delta = 0.f;
    int32_t id_hit_by = -1;
};

struct delayed_delta
{
    damage_info delayed_info;
    float delay_time_ms = 200.f;
    sf::Clock clk;
    bool part_hit_before_block = false;
    bool part_blocked = false;
};

///this was a good idea
struct network_part
{
    //bool hp_dirty = false;
    //float hp_delta = 0.f;

    //damage_information damage_info;

    std::vector<delayed_delta> delayed_delt;

    int32_t play_hit_audio = 0;
};

struct local_part
{
    int32_t play_hit_audio = 0;
    int32_t send_hit_audio = 0;
};

struct fighter;

///need to network part hp
struct part
{
    int quality; ///graphics quality, 0 is low, 1 is high

    bodypart_t type;
    vec3f pos;
    vec3f rot;
    vec3f global_pos;
    vec3f global_rot;

    bool is_active;

    int team;

    float hp;

    network_part net;
    local_part local;

    //bool performed_death;

    void set_type(bodypart_t); ///sets me up in the default position
    void set_pos(vec3f pos);
    void set_rot(vec3f rot);
    void set_global_pos(vec3f global_pos);
    void set_global_rot(vec3f global_rot);
    void update_model();

    void set_team(int _team);
    void load_team_model();
    void damage(fighter* parent, float dam, bool do_effect = true, int32_t network_id_hit_by = -1);
    void set_hp(float h);
    void request_network_hp_delta(float delta, fighter* parent);
    void perform_death(bool do_effect = true);

    void set_quality(int _quality);

    bool alive(); ///if the model is inactive, its considered dead

    //void tick();

    part(object_context& _cpu);
    part(bodypart_t, object_context& _cpu);
    ~part();

    void set_active(bool);
    void scale();

    objects_container* obj();

    object_context* cpu_context = nullptr;

    void update_texture_by_hp();

private:
    void network_hp(float delta);
    objects_container* model;
    float old_hp = 0.f; ///purely for the purpose of updating model tex

    /*objects_container* hp_display;
    texture display_tex;*/

    //sf::Texture tex;
    //texture cylinder_tex;
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

    ///this is initialised under queue_attack
    ///this is almost certainly the wrong place
    ///even if !i.does(damaging), still might have a damage value
    ///if its part of an attack with some damage value to it
    float damage = 0.f;
    int was_set_this_frame = 0;
};

namespace attacks
{
    enum attacks
    {
        SLASH = 0,
        OVERHEAD,
        SLASH_ALT,
        OVERHEAD_ALT,
        STAB,
        REST,
        BLOCK,
        RECOIL,
        FEINT,
        SPRINT,
        TROMBONE,
        FAST_REST,
        COUNT
    };

    ///out of 100
    ///uuh. This won't work due to attacks just being a naive
    ///compilation of movements
    const static std::vector<float> damage_amounts
    {
        0.4f,
        0.6f,
        0.4f,
        0.6f,
        0.5f,
        0.f,
        0.f,
        0.f,
        0.f,
        0.f,
        0.f,
        0.f,
        0.f,
    };

    ///old = 100.f;
    ///human reaction time (of me) is ~185ms, so a perfectly timed feint
    ///may be below the minimum reactable time
    ///if the sword is pointed such that it starts exactly inside the enemy character
    ///however I'm not sure that this is possible in the constraints of the game
    ///and the attacking player would be at a large visual disadvantage for doing this
    ///as they'd be turned completely sideways
    ///this might be a concern for a stab
    const static float unfeintable_time = 150.f;
}


typedef attacks::attacks attack_t;

struct attack
{
    std::vector<movement> moves;
};

///integrate this with sprint timings, so it takes exactly 500ms to windup sprint
///make sure we use our real sprint change in velocity, so you cant just hold shift and attack all the time
static std::vector<movement> sprint_dummy =
{
    {0, {0,0,0}, 500, 0, bodypart::LHAND, mov::NO_MOVEMENT}
};

static std::vector<movement> overhead =
{
    {0, {-150, -0, -40}, 400, 0, bodypart::LHAND, mov::WINDUP}, ///windup
    //{0, {-120, -0, -40}, 400, 0, bodypart::LHAND, mov::WINDUP}, ///windup
    {0, {100, -150, -140}, 500, 1, bodypart::LHAND,  (movement_t)(mov::DAMAGING | mov::LOCKS_ARMS | mov::PASS_THROUGH_SCREEN_CENTRE | mov::OVERHEAD_HACK)}, ///attack
    {0, {0, -150, -140}, 200, 1, bodypart::LHAND,  (movement_t)(mov::NONE)}
};

static std::vector<movement> overhead_alt =
{
    {0, {-150, -0, -40}, 400, 0, bodypart::LHAND, mov::WINDUP}, ///windup
    //{0, {-120, -0, -40}, 400, 0, bodypart::LHAND, mov::WINDUP}, ///windup
    {0, {100, -150, -140}, 500, 1, bodypart::LHAND,  (movement_t)(mov::DAMAGING | mov::LOCKS_ARMS | mov::PASS_THROUGH_SCREEN_CENTRE | mov::OVERHEAD_HACK)}, ///attack
    {0, {0, -150, -140}, 200, 1, bodypart::LHAND,  (movement_t)(mov::NONE)}
};

static std::vector<movement> recoil =
{
    {0, {-120, -150, -60}, 300, 4, bodypart::LHAND,  (movement_t)(mov::NONE | mov::LOCKS_ARMS | mov::IS_RECOIL)}, ///recoiling doesnt block or damage
    {0, {-0, -150, -140}, 500, 4, bodypart::LHAND,  (movement_t)(mov::NONE | mov::IS_RECOIL)} ///attack
    //{0, {-120, -10, -60}, 800, 0, bodypart::LHAND,  (movement_t)(mov::NONE | mov::LOCKS_ARMS | mov::IS_RECOIL)} ///recoiling doesnt block or damage
};

static std::vector<movement> slash =
{
    {0, {-180, -60, -10}, 350, 0, bodypart::LHAND, mov::WINDUP}, ///windup
    {0, {120, -60, -140}, 450, 1, bodypart::LHAND,  (movement_t)(mov::DAMAGING | mov::LOCKS_ARMS | mov::PASS_THROUGH_SCREEN_CENTRE)}, ///attack
    {0, {0, -150, -140}, 200, 5, bodypart::LHAND,  (movement_t)(mov::NONE)}

    //{0, {120, -60, -140}, 450, 1, bodypart::LHAND,  (movement_t)(mov::DAMAGING | mov::LOCKS_ARMS | mov::PASS_THROUGH_SCREEN_CENTRE)} ///attack
};

static std::vector<movement> slash_alt =
{
    {0, {120, -60, -140}, 350, 0, bodypart::LHAND, mov::WINDUP}, ///windup
    {0, {-180, -60, -10}, 450, 1, bodypart::LHAND,  (movement_t)(mov::DAMAGING | mov::LOCKS_ARMS | mov::PASS_THROUGH_SCREEN_CENTRE)}, ///attack
    {0, {0, -150, -140}, 200, 1, bodypart::LHAND,  (movement_t)(mov::NONE)}

    //{0, {120, -60, -140}, 450, 1, bodypart::LHAND,  (movement_t)(mov::DAMAGING | mov::LOCKS_ARMS | mov::PASS_THROUGH_SCREEN_CENTRE)} ///attack
};

/*static std::vector<movement> slash =
{
    {0, {-150, -80, -40}, 200, 0, bodypart::LHAND, mov::WINDUP}, ///windup
    {0, {100, -80, -140}, 450, 1, bodypart::LHAND, mov::DAMAGING} ///attack
};*/

static std::vector<movement> rest =
{
    {0, {0, -200, -100}, 500, 1, bodypart::LHAND, mov::NONE}
};


static std::vector<movement> stab =
{
    //{0, {-80, -120, -10}, 450, 0, bodypart::LHAND, (movement_t)(mov::WINDUP | mov::NONE)}, ///windup
    //{0, {-80, -120, +100}, 450, 0, bodypart::LHAND, (movement_t)(mov::WINDUP | mov::NONE)}, /
    //{0, {-80, -0, -10}, 450, 0, bodypart::LHAND, (movement_t)(mov::WINDUP | mov::NONE)},
    {0, {-80, -120, -10}, 450, 4, bodypart::LHAND, (movement_t)(mov::WINDUP | mov::NONE)}, ///windup
    {0, {-40, -180, -180}, 400, 4, bodypart::LHAND,  (movement_t)(mov::DAMAGING | mov::LOCKS_ARMS | mov::FINISH_AT_SCREEN_CENTRE)}, ///attack
    //{0, {100, -150, -140}, 200, 3, bodypart::LHAND,  (movement_t)(mov::NONE)} ///attack
    {0, {0, -150, -140}, 200, 3, bodypart::LHAND,  (movement_t)(mov::NONE)},


    //{0, {-40, -60, -180}, 350, 1, bodypart::LHAND,  (movement_t)(mov::DAMAGING | mov::LOCKS_ARMS | mov::FINISH_AT_SCREEN_CENTRE)}, ///attack
    //{0, {-40, -60, -180}, 350, 0, bodypart::LHAND,  (movement_t)(mov::DAMAGING | mov::LOCKS_ARMS | mov::FINISH_AT_SCREEN_CENTRE)} ///attack
    //{0, {-80, -200, -100}, 200, 1, bodypart::LHAND, mov::NOSLOW_START}
};


/*static std::vector<movement> block_old =
{
    {0, {-50, -80, -20}, 300, 0, bodypart::LHAND, (movement_t)(mov::BLOCKING | mov::FINISH_INDEPENDENT | mov::FINISH_AT_90)},
    {0, {100, -150, -140}, 400, 0, bodypart::LHAND, mov::START_INDEPENDENT}
};*/

static std::vector<movement> block =
{
    {0, {-110, -40, -40}, 300, 0, bodypart::LHAND, (movement_t)(mov::BLOCKING | mov::FINISH_INDEPENDENT | mov::FINISH_AT_90)},
    {0, {100, -150, -140}, 400, 0, bodypart::LHAND, mov::START_INDEPENDENT}
};

///try a third kind of movement, that starts very slow then goes normal speed
static std::vector<movement> feint =
{
    //{0, {-150, -80, -40}, 350, 0, bodypart::LHAND, mov::NONE}
    //{0, {0, -200, -100}, 300, 1, bodypart::LHAND, mov::NONE}
    //{0, {100, -150, -140}, 300, 3, bodypart::LHAND,  mov::NONE}
    {0, {0, -150, -140}, 300, 3, bodypart::LHAND,  (movement_t)(mov::NONE)} ///attack
};

///remember that this needs to be in 3 parts eventually
///windup, continuous, winddown
///with windup staggerable
static std::vector<movement> sprint
{
    ///for gameplay balance a camera limit is probably good
    ///but to be honest its fairly horrible to play with, you should be able to freely sprint and look
    {0, {100, -200, -100}, 200, 0, bodypart::LHAND, (movement_t)(mov::CONTINUOUS_SPRINT | mov::NO_POST_QUEUE | mov::NO_CAMERA_LIMIT)}
};

static std::vector<movement> trombone_hold
{
    {0, {-50, -50, -100}, 200, 0, bodypart::LHAND, mov::ORIENT_TOWARDS_FACE}
};

static std::vector<movement> fast_rest =
{
    {0, {0, -200, -100}, 200, 1, bodypart::LHAND, mov::NONE}
};



///?
//static std::vector<movement> jump;
///hmm. This is probably a bad plan
///just use a jump struct

static std::map<attack_t, attack> attack_list =
{
    {attacks::OVERHEAD, {overhead}},
    {attacks::SLASH, {slash}},
    {attacks::SLASH_ALT, {slash_alt}},
    {attacks::OVERHEAD_ALT, {overhead_alt}},
    {attacks::STAB, {stab}},
    {attacks::REST, {rest}},
    {attacks::BLOCK, {block}},
    {attacks::RECOIL, {recoil}},
    {attacks::FEINT, {feint}},
    {attacks::SPRINT, {sprint}},
    {attacks::TROMBONE, {trombone_hold}},
    {attacks::FAST_REST, {fast_rest}},
};

/*static std::map<attack_t, attack> attack_list2 =
{
    {attacks::OVERHEAD, {attacks::OVERHEAD,
        {{0, {-150, -0, -20}, 400.f, 0}}
    }}
};*/

struct sword
{
    ///if you want global, you have to poke around in here ( sadface )
    objects_container* model;

    ///everything here is *local*,
    vec3f pos;
    vec3f rot;
    vec3f dir; ///look dir

    float length; ///from centre to end

    int team;

    bbox bound;

    sword(object_context& cpu);

    void set_pos(vec3f _pos); //? model now has different pos to actual due to top down approach
    void set_rot(vec3f _rot);

    void set_team(int _team);
    void load_team_model();

    void scale();

    objects_container* obj();
    void update_model();

    void set_active(bool active);

private:
    object_context* cpu_context = nullptr;
    bool is_active = true;
};

///define attacks in terms of a start, an end, a time, and possibly a smoothing function

struct pos_rot
{
    vec3f pos;
    vec3f rot;
};

pos_rot to_world_space(vec3f world_pos, vec3f world_rot, vec3f local_pos, vec3f local_rot);

struct jump_descriptor
{
    vec2f dir = {0,0};

    bool should_play_foot_sounds = false;
    float current_time = 0;
    bool is_jumping = false;
    vec3f pre_jump_pos = {0,0,0};
    float last_speed = 1.f;

    float max_height = 200;
    float time_ms = 750;

    ///this will let you jump into walls
    vec3f get_absolute_jump_displacement_tick(float dt, fighter* fight);
    vec3f get_relative_jump_displacement_tick(float dt, fighter* fight);

    void terminate_early();
};

#define MAX_NAME_LENGTH 16

struct physics;

/*struct name_struct
{
    char v[MAX_NAME_LENGTH + 1] = {0};
};*/

///networked stuff
struct networked_components
{
    ///this is authoritative
    int32_t is_damaging = 0; ///currently doing a damaging attack
    int32_t is_blocking = 0;
    //int dead = 0; ///networked status of killing, can be updated remotely
    //int32_t recoil = 0; ///this is a recoil request, not necessarily saying i AM(?)
    //int32_t force_recoil = 0;

    //bool recoil_dirty = false;

    int32_t reported_dead = 0;

    int32_t play_clang_audio = 0;

    float ping = 0;
};

struct local_components
{
    int32_t play_clang_audio = 0;
    int32_t send_clang_audio = 0;
};

struct link
{
    part* p1;
    part* p2;

    objects_container* obj;

    vec3f offset;

    float squish_frac;

    float length = 0.f;
};

struct cosmetics
{
    objects_container* tophat = nullptr;

    ///void load()

    void set_active(bool status)
    {
        return;

        tophat->set_file("./res/tophat.obj");

        tophat->set_active(status);
    }
};

struct clientside_parry_info
{
    int32_t player_id_i_parried = -1;
    sf::Clock clk;

    ///ping + jitter + fudge (fudge incorporates if they hit slightly later).
    ///I don't think its ping*2, we only have to wait for them to have a good laugh
    float max_invuln_time_ms = 500.f;
};

struct light;
struct gameplay_state;

struct network_fighter;
struct server_networking;

///what a clusterfuck
///OK, its not the skeleton thats an issue, that's actually seemingly necessarily quite heavily integrated
///its the networking model, and directly networking components thats extremely problematic
struct fighter
{
    trombone_manager trombone_manage;

    ///this is the modifiable network representation of the network fighter
    ///the player does not have a valid instance of this
    network_fighter* net_fighter_copy;

    bool reset_screenshake_flinch = false;

    bool name_info_initialised = false;

    std::vector<clientside_parry_info> clientside_parry_inf;

    int32_t last_part_id = -1;
    float last_hp_delta = 0;
    int32_t player_id_i_was_last_hit_by = -1;
    int32_t network_id = -1;

    int gpu_name_dirty;
    float rand_offset_ms;

    cosmetics cosmetic;

    const float name_resend_time = 5000.f; ///ms
    sf::Clock name_resend_timer;
    sf::Clock name_reset_timer;

    jump_descriptor jump_info;

    vec3f sword_rotation_offset;

    std::vector<light*> my_lights;

    std::vector<link> joint_links;

    float frametime;
    float my_time;

    sf::Clock frame_clock;

    networked_components net;
    local_components local;

    int side; ///as in team?

    gameplay_state* game_state;

    const vec3f* rest_positions;

    //part parts[bodypart::COUNT];
    ///bodypart::COUNT numbers of these
    std::vector<part> parts;
    vec3f old_pos[bodypart::COUNT];

    fighter(object_context& cpu_context, object_context_data& gpu_context);
    ~fighter();
    void load();

    ///ideally we want movements to be ptrs, then delete them on removal
    std::map<bodypart_t, movement> action_map;

    std::vector<movement> moves;

    sword weapon;
    int stance; ///0 means perpendicular to velocity, 1 means parallel

    vec3f focus_pos; ///where to put my hands and sword
    ///this should almost certainly be relative

    float shoulder_rotation;

    int team;

    float camera_bob_mult;

    vec3f camera_bob;
    vec3f pos; ///need to swap references to body->rot and body->pos to these (unless i explicitly need them)
    vec3f rot;
    vec3f rot_diff;
    vec2f momentum; ///global or local?
    vec2f last_walk_dir_diff;

    physics* phys;

    vec3f look;

    bool is_sprinting;
    float sprint_frac;

    bool idling;
    bool performed_death; ///have i done my death stuff locally
    bool is_offline_client = false; ///offline compatability for bots

    int quality; /// 0 = low, 1 = high

    void set_quality(int _quality);
    void set_gameplay_state(gameplay_state* st);

    cape my_cape;

    void IK_hand(int hand, vec3f pos, float upper_rotation = 0.f, bool arms_are_locked = false, bool force_positioning = false);
    void IK_foot(int foot, vec3f pos, vec3f off1 = {0,0,0}, vec3f off2 = {0,0,0}, vec3f off3 = {0,0,0});

    void linear_move(int hand, vec3f pos, float time, bodypart_t b);
    void spherical_move(int hand, vec3f pos, float time, bodypart_t b);

    void move_hands(vec3f pos);

    void set_stance(int _stance);

    void queue_attack(attack_t type);
    void add_move(const movement& m);
    void try_jump();

    void update_sword_rot();

    void tick(bool is_player = false);
    void shared_tick(server_networking* networking);

    void manual_check_part_death(); ///interate over parts, if < 0 and active then die
    void manual_check_part_alive(); ///interate over parts, if > 0 and inactive then activate

    void walk_dir(vec2f dir, bool sprint = false); ///z, x
    void crouch_tick(bool do_crouch);

    void set_pos(vec3f);
    void set_rot(vec3f);
    void set_rot_diff(vec3f diff);
    vec2f get_approx_dim();

    movement* get_movement(size_t id);

    void update_headbob_if_sprinting(bool sprinting);
    void update_render_positions();
    void network_update_render_positions(); /// only if im a network fighter
    void respawn_if_appropriate(); ///network
    void update_lights();
    void recalculate_link_positions_from_parts();
    void overwrite_parts_from_model();
    void save_old_pos(); ///network only
    void update_texture_by_part_hp();
    void update_last_hit_id();
    void check_clientside_parry(fighter* non_networked_fighter);

    void process_delayed_deltas();
    void eliminate_clientside_parry_invulnerability_damage();
    void set_network_id(int32_t net_id);

    ///player only
    void save_network_representation(network_fighter& net_fight);
    ///per frame
    network_fighter construct_network_fighter();
    void construct_from_network_fighter(network_fighter& net_fight);
    network_fighter get_modified_network_fighter();
    void modify_existing_network_fighter_with_local(network_fighter& net_fight);

    void position_cosmetics();

    void set_team(int _team);

    void set_physics(physics* phys);

    void cancel(bodypart_t type);

    bool can_attack(bodypart_t type);

    movement* get_current_move(bodypart_t type);

    void cancel_hands();
    void recoil();
    bool can_windup_recoil();
    bool currently_recoiling();
    void checked_recoil(); ///if we're hit, do a recoil if we're in windup
    void try_feint();

    void override_rhand_pos(vec3f global_position);

    void damage(bodypart_t type, float d, int32_t network_id_hit_by, bool hit_by_offline_client = false);

    void flinch(float time_ms);

    ///resets all vars
    void respawn(vec2f pos = (vec2f){0,0});
    void die();
    bool should_die();
    int num_dead();
    int num_needed_to_die();
    void checked_death(); ///only die if we should
    bool dead();

    ///0 = sword, 1 = trombone, if we need more add an enum
    int current_weapon = 0;
    void set_weapon(int weapon_id);

    void tick_cape();

    vec2f get_wall_corrected_move(vec2f pos, vec2f move_dir);
    vec2f get_external_fighter_corrected_move(vec2f pos, vec2f move_dir, const std::vector<fighter*>& fighter_list);

    ///rotation
    void set_look(vec3f look);

    vec3f look_displacement;
    vec3f old_look_displacement;

    void set_contexts(object_context* _cpu, object_context_data* _gpu);

    sf::Clock foot_supression_timer;
    float time_to_suppress_foot_sounds_s = 0.f;
    bool suppress_foot_sounds = false;
    void do_foot_sounds(bool is_player = false);

    void set_name(std::string _name);
    void update_gpu_name();
    void set_secondary_context(object_context* _transparency_context);
    void update_name_info(bool networked_fighter = false);

    float crouch_frac = 0.f;
    float smoothed_crouch_offset = 0.f;
    float smoothed_crouch_offset_old = 0.f;

    std::string local_name = "Err";

    void check_and_play_sounds(bool player = false);

    void set_other_fighters(const std::vector<fighter*>& other_fight);

    ///so, i guess this is including me
    std::vector<fighter*> fighter_list;

private:
    sf::Clock walk_clock;
    //std::map<bodypart_t, vec3f> up_pos;

    vec2f last_walk_dir;

    object_context* cpu_context = nullptr;
    object_context_data* gpu_context = nullptr;

    object_context* transparency_context = nullptr;

    objects_container* name_container = nullptr;

    sf::RenderTexture name_tex;

    texture* name_tex_gpu = nullptr;
    cl_float2 name_dim = (cl_float2){128, 128};
    cl_float2 name_obj_dim = (cl_float2){128, 128};

    bool left_foot_sound;
    bool right_foot_sound;

    bool just_spawned = true;

    bool rhand_overridden = false;
    vec3f rhand_override_pos;
};


#endif // FIGHTER_HPP_INCLUDED
