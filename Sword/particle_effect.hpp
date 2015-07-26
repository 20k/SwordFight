#ifndef PARTICLE_EFFECT_HPP_INCLUDED
#define PARTICLE_EFFECT_HPP_INCLUDED

#include <SFML/Graphics.hpp>
#include "vec.hpp"
#include <vector>

struct objects_container;

struct effect
{
    float duration_ms = 0;
    sf::Clock elapsed_time;

    bool finished = false;

    virtual void tick() = 0;

    virtual ~effect() = default;
};

struct cube_effect : effect
{
    vec3f pos;
    float scale;

    int num;

    std::vector<objects_container> objects;

    void tick();

    void make(float duration, vec3f _pos, float _scale, int _team, int _num);
    void push();

    ~cube_effect() = default;
};

struct light_effect : effect
{
    void tick();

    ~light_effect();
};

struct particle_effect
{
    static std::vector<effect*> effects;
    static void tick();

    //particle_effect();
    //~particle_effect();
};

#endif // PARTICLE_EFFECT_HPP_INCLUDED
