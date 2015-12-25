#include "enemy_spawner.hpp"

#include "object.hpp"

void enemy_spawner::start()
{
    cur_time_ms = 0.f;
}

void enemy_spawner::tick(state& s, float dt)
{
    const float enemies_every_ms = 3100.f;

    int tick = (int) cur_time_ms / enemies_every_ms;
    int ntick = (int) (cur_time_ms + dt) / enemies_every_ms;

    if(ntick != tick)
    {
        int type = rand() % 4;

        ai_character* ch = nullptr;

        if(type == 0)
            ch = new vanilla_enemy;
        if(type == 1)
            ch = new robot_enemy;
        if(type == 2)
            ch = new tank_enemy;
        if(type == 3)
            ch = new hound_master;

        if(ch == nullptr)
        {
            throw std::runtime_error("Error in enemy spawner, invalid type???");
        }

        ch->set_pos(randf<2, float>(0, 700));
        ch->set_team(team::ENEMY);

        s.entities->push_back(ch);
    }

    cur_time_ms += dt;
}

void enemy_spawner::stop()
{

}

///remember, these are not added to the entity list!
ai_character* enemy_spawner::get_random_character(std::minstd_rand& rnd)
{
    int type = rnd() % 4;

    ai_character* ch = nullptr;

    if(type == 0)
        ch = new vanilla_enemy;
    if(type == 1)
        ch = new robot_enemy;
    if(type == 2)
        ch = new tank_enemy;
    if(type == 3)
        ch = new hound_master;

    if(ch == nullptr)
    {
        throw std::runtime_error("Error in enemy spawner, invalid type???");
    }

    ch->set_team(team::ENEMY);

    return ch;
}
