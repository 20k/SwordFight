#ifndef STATE_HPP_INCLUDED
#define STATE_HPP_INCLUDED

#include <vector>

struct sf::RenderWindow;
struct renderer;
struct game_entity;

struct state
{
    sf::RenderWindow* win;
    renderer* render_2d;
    std::vector<game_entity*>* entities;

    ///vector of all characters
};

#endif // STATE_HPP_INCLUDED
