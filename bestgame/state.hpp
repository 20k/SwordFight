#ifndef STATE_HPP_INCLUDED
#define STATE_HPP_INCLUDED

#include <vector>
#include <SFML/Graphics.hpp>

struct sf::RenderWindow;
struct renderer;
struct game_entity;
struct ai_manager;
struct world_manager;

struct state
{
    sf::RenderWindow* win;
    renderer* render_2d;
    std::vector<game_entity*>* entities;
    ai_manager* ai;
    world_manager* world;

    ///vector of all characters
};

#endif // STATE_HPP_INCLUDED
