#ifndef ENEMY_SPAWNER_HPP_INCLUDED
#define ENEMY_SPAWNER_HPP_INCLUDED

#include <random>

struct state;
struct ai_character;

struct enemy_spawner
{
    float cur_time_ms = 0.f;

    void start();

    void tick(state& s, float dt);

    void stop();

    ai_character* get_random_character(std::minstd_rand& rnd);
};


#endif // ENEMY_SPAWNER_HPP_INCLUDED
