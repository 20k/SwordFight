#ifndef PARTICLE_EFFECT_HPP_INCLUDED
#define PARTICLE_EFFECT_HPP_INCLUDED

#include <SFML/Graphics.hpp>
#include "vec.hpp"
#include <vector>

struct objects_container;

struct particle_effect
{
    float duration_ms;
    sf::Clock elapsed_time;

    vec3f pos;
    float scale;

    int num;

    std::vector<objects_container> objects;

    void make(float duration, vec3f _pos, float _scale, int _num = 40);
    void push();

    static std::vector<particle_effect> effects;
    static void tick();

    //particle_effect();
    //~particle_effect();
};

#endif // PARTICLE_EFFECT_HPP_INCLUDED
