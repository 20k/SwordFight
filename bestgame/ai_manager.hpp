#ifndef AI_MANAGER_HPP_INCLUDED
#define AI_MANAGER_HPP_INCLUDED

#include <vector>

namespace ai_type
{
    enum ai_type
    {
        BASE, ///approach then mill around. Possibly use swarm tactics and cover
        ROBOT, ///stop, charge, fire
        HOUND, ///move in short stints while doing damage
        TANK ///fire 360
    };
}

struct ai_character;
struct character;
struct state;

struct ai_manager
{
    std::vector<ai_character*> ai_list;
    std::vector<character*> character_list;

    void tick(state& s, float dt);

    ///;_;
    void add_hostile_ai(ai_character* ai);

    void add_friendly_player(character* ch);
};

#endif // AI_MANAGER_HPP_INCLUDED
