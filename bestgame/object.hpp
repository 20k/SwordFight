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

    projectile();
    virtual void tick(state& s, float dt);
    void set_damage(float _damage);
    void set_speed(float _speed);
    void set_dir(vec2f _dir);
};

struct character : game_entity
{
    float time_fired_s;
    float current_time;

    character();
    virtual void tick(state& s, float dt);
    virtual projectile* fire(vec2f dir, float time_between_shots);
};

struct ai_character : character
{
    virtual void tick(state& s, float dt);
};

struct player : character
{
    void do_keyboard_controls(float dt);

    virtual void tick(state& s, float dt);
};

#endif // OBJECT_HPP_INCLUDED
