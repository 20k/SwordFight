#ifndef ENEMY_SPAWNER_HPP_INCLUDED
#define ENEMY_SPAWNER_HPP_INCLUDED

struct state;

struct enemy_spawner
{
    float cur_time_ms = 0.f;

    void start();

    void tick(state& s, float dt);

    void stop();
};


#endif // ENEMY_SPAWNER_HPP_INCLUDED
