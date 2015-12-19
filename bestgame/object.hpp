#ifndef OBJECT_HPP_INCLUDED
#define OBJECT_HPP_INCLUDED

#include "state.hpp"
#include "renderer.hpp"

struct game_entity
{
    vec2f pos;
    float hp;
    bool to_remove;

    void set_pos(vec2f _pos)
    {
        pos = _pos;
    }

    void translate(vec2f _dir)
    {
        pos = pos + _dir;
    }

    void move_at_speed(vec2f _dir, float speed, float dt)
    {
        pos = pos + _dir * speed * dt;
    }

    bool is_dead()
    {
        return hp <= 0.f;
    }

    void damage(float frac)
    {
        hp -= frac;
    }

    virtual void tick(state& s, float dt){}

    game_entity(){hp = 1.f; to_remove = false;}
    virtual ~game_entity(){}
};

struct projectile : game_entity
{
    vec2f dir;
    float speed;

    void set_dir(vec2f _dir)
    {
        dir = _dir;
    }

    void set_speed(float _speed)
    {
        speed = _speed;
    }

    virtual void tick(state& s, float dt)
    {
        move_at_speed(dir.norm(), speed, dt);

        render_square sq(pos, {3, 3}, {1, 1, 0.3});

        s.render_2d->add(sq);
    }

    projectile()
    {
        speed = 1.f;
    }
};

struct character : game_entity
{
    float time_fired_s;
    float current_time;

    character(){time_fired_s = 0.f; current_time = 0.f;}

    virtual void tick(state& s, float dt)
    {
        render_square sq(pos, {10, 10}, {0.3, 0.3, 1.f});

        s.render_2d->add(sq);

        current_time += dt / 1000.f;
    }

    virtual projectile* fire(vec2f dir, float time_between_shots)
    {
        if(time_fired_s + time_between_shots > current_time)
            return nullptr;

        time_fired_s = current_time;

        projectile* new_projectile = new projectile;

        new_projectile->set_dir(dir);
        new_projectile->set_pos(pos);

        return new_projectile;
    }
};

struct player : character
{
    void do_keyboard_controls(float dt)
    {
        vec2f dir = {0,0};

        sf::Keyboard key;

        if(key.isKeyPressed(sf::Keyboard::W))
        {
            dir.v[1] = -1;
        }
        if(key.isKeyPressed(sf::Keyboard::S))
        {
            dir.v[1] = 1;
        }
        if(key.isKeyPressed(sf::Keyboard::A))
        {
            dir.v[0] = -1;
        }
        if(key.isKeyPressed(sf::Keyboard::D))
        {
            dir.v[0] = 1;
        }

        float speed = 0.3;

        move_at_speed(dir.norm(), speed, dt);
    }

    virtual void tick(state& s, float dt)
    {
        do_keyboard_controls(dt);

        character::tick(s, dt);
    }
};

#endif // OBJECT_HPP_INCLUDED
