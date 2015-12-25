#ifndef OBJECT_HPP_INCLUDED
#define OBJECT_HPP_INCLUDED

#include "state.hpp"
#include "renderer.hpp"

#include "ai_manager.hpp"

namespace team
{
    enum team : int32_t
    {
        FRIENDLY = 0,
        ENEMY,
        OBJECT,
        NONE ///skip
    };

    static std::vector<vec3f> normal_cols
    {
        {0.3, 0.3, 1.f},
        {1.f, 0.3f, 0.3f},
        {0.7f, 0.7f, 0.7f},
        {1.f, 1.f, 1.f}
    };

    static std::vector<vec3f> hit_cols
    {
        {0.9f, 0.9f, 1.f},
        {1.f, 0.6f, 0.6f},
        {0.9f, 0.9f, 0.9f},
        {1.f, 1.f, 1.f}
    };
}

namespace game
{
     const float hit_timer_s = 0.2f;
     const float death_fade_s = 0.5f;
}

typedef team::team team_t;

struct game_entity
{
    vec2f pos;
    float hp;
    bool to_remove;
    team_t team_colour;
    team_t team_collision;
    vec2f dim;
    sf::Clock hit_timer; ///if less than hit_timer_s do hit effect
    bool do_hit_effect;
    sf::Clock death_timer;
    bool do_death_effect;

    virtual ~game_entity();

    game_entity();

    virtual void tick(state& s, float dt);

    void set_team(team_t _team);

    virtual void do_damage(float frac);

    virtual bool is_dead();

    void move_at_speed(vec2f _dir, float speed, float dt);

    void translate(vec2f _dir);

    void set_pos(vec2f _pos);

    void set_dim(vec2f _dim);
};

struct collider
{
    int get_collider_id(game_entity* me, const std::vector<game_entity*>& entities);
};

struct projectile : game_entity
{
    vec2f dir;
    float speed;
    float damage;
    float time_to_live_s;

    sf::Clock clk;

    projectile();
    virtual void tick(state& s, float dt);
    void set_damage(float _damage);
    void set_speed(float _speed);
    void set_dir(vec2f _dir);
    void set_time_to_live(float _ttl);
};

struct character : game_entity
{
    float time_fired_s;
    float current_time;

    character();

    vec3f get_current_col();
    virtual void tick(state& s, float dt);
    virtual projectile* fire(vec2f dir, float time_between_shots, float speed = 1.f);
};

struct seeker
{
    bool seeking = false;
    vec2f seek_vec;

    vec2f get_seek(vec2f current_pos);

    ///distance is a random variable added to my_pos with randf<2, float>(-1, 1) * distance
    void start_seek(vec2f my_pos, float distance);

    bool is_complete(vec2f current_pos);
    bool running();

    void terminate();
};

struct ai_character : character
{
    ///if false, the ai is off
    bool enabled = true;

    virtual void tick(state& s, float dt);

    virtual void do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai);
};

struct vanilla_enemy : ai_character
{
    vanilla_enemy();

    virtual void tick(state& s, float dt);

    virtual void do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai);
};

struct robot_enemy : ai_character
{
    sf::Clock laser_duration_clock;
    sf::Clock laser_refire_clock;
    seeker seek;

    vec2f locked_fire_dir;
    bool firing;

    robot_enemy();

    projectile* fire_laser(vec2f dir, float time_between_pulses, float duration);

    virtual void tick(state& s, float dt);

    virtual void do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai);
};

struct hound_enemy : ai_character
{
    ///am I controlling myself, or am i being controilled
    //bool ai_active = false;
    float move_clock;
    float last_move_time = 0.f;
    float move_probability = 0.7f;

    void set_owned();
    void set_free();

    hound_enemy();

    seeker seek;

    virtual void tick(state& s, float dt);

    ///disorganised AI
    ///hound master controls organised AI
    virtual void do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai);
};

struct hound_master : ai_character
{
    bool hounds_init;
    seeker seek;

    hound_master();

    std::vector<hound_enemy*> hounds;

    virtual void tick(state& s, float dt);

    virtual void do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai);
};

struct tank_enemy : ai_character
{
    tank_enemy();

    bool seeking;
    vec2f seek_vec;

    std::vector<projectile*> fire_circle(float time_between_shots, int num, float speed);

    virtual void tick(state& s, float dt);

    virtual void do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai);
};

struct player : character
{
    player();

    void do_keyboard_controls(state& s, float dt);

    virtual void tick(state& s, float dt);
};

#endif // OBJECT_HPP_INCLUDED
