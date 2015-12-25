#include "ai_manager.hpp"

#include "object.hpp"

#include <numeric>

///hooray for duck typing
template<typename T, typename U>
vec2f get_pad_vector(T me, const std::vector<U>& to_distance_from, float pad_distance)
{
    vec2f pos = me->pos;

    vec2f accum = {0,0};

    int c = 0;

    for(int i=0; i<to_distance_from.size(); i++)
    {
        vec2f me_to_them = to_distance_from[i]->pos - pos;

        float len = me_to_them.length();

        if(len < pad_distance)
        {
            c++;
            accum = accum + me_to_them;
        }
    }

    if(c != 0)
        accum = accum / c;

    return -accum;
}

void ai_manager::tick(state& s, float dt)
{
    if(character_list.size() == 0)
        return;

    ///for the moment lets just use the player 0
    character* current_enemy = character_list[0];

    ///also want to pad
    for(int i=0; i<ai_list.size(); i++)
    {
        ai_character* ai = ai_list[i];

        if(ai->is_dead())
            continue;

        vec2f pad_vector = get_pad_vector(ai, ai_list, 20.f);

        ///if ai not spot player, continue

        ///all this below is very temporary. We want individual unit ai to return a position
        ///then coordinate that
        ///then do attacks based on that

        //if(ai->enabled)
        //    ai->do_ai(s, dt, pad_vector, current_enemy, ai_list);
    }

    ai_list.clear();
    character_list.clear();
}

void ai_manager::add_hostile_ai(ai_character* ai)
{
    ai_list.push_back(ai);
}

void ai_manager::add_friendly_player(character* ch)
{
    character_list.push_back(ch);
}
