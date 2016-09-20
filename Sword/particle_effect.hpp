#ifndef PARTICLE_EFFECT_HPP_INCLUDED
#define PARTICLE_EFFECT_HPP_INCLUDED

#include <SFML/Graphics.hpp>
#include <vec/vec.hpp>
#include <vector>

struct object_context;

struct objects_container;

struct light;

struct effect
{
    float duration_ms = 0;
    sf::Clock elapsed_time;

    bool finished = false;

    virtual void tick() = 0;
    virtual void activate() {};

    virtual ~effect() = default;
};

struct cube_effect : effect
{
    static std::vector<objects_container*> precached_objects;
    static std::vector<int> in_use;

    static void precache(int reserve_size, object_context& _cpu_context);

    vec3f pos;
    float scale;

    int num;
    int team;

    std::vector<int> currently_using;
    std::vector<vec3f> offset_pos;

    void tick();
    void make(float duration, vec3f _pos, float _scale, int _team, int _num, object_context& cpu);
    void activate();

    ~cube_effect() = default;

    object_context* cpu_context = nullptr;
};

struct light_effect : effect
{
    vec3f start;
    vec3f pos;
    light* l;

    void tick();
    void make(float duration, light* _l);
    //void activate();

    ~light_effect() = default;
};

struct clang_light_effect : effect
{
    vec3f pos;

    light* l;

    void tick();
    void make(float duration, vec3f pos, vec3f col, float rad);
};

struct particle_effect
{
    static std::vector<effect*> effects;
    static void tick();

    template<typename T>
    static void push(const T& thing)
    {
        effect* e = new T(thing);

        particle_effect::effects.push_back(e);

        e->activate();
    }
};

#endif // PARTICLE_EFFECT_HPP_INCLUDED
