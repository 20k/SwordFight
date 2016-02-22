#ifndef MENU_HPP_INCLUDED
#define MENU_HPP_INCLUDED

#include <SFML/Graphics.hpp>

#include <vec/vec.hpp>

#include <functional>

struct engine;

struct menu_system;

struct ui_box
{
    vec2f pos; ///relative to whole screen
    vec2f dim;

    std::string label;
    int font_size;

    std::function<void(menu_system&)> callback;
};

struct menu_system
{
    enum state : unsigned int
    {
        MENU,
        INGAME, ///ingame is special in that do_menu returns false and the game runs
        COUNT
    };

    state current_menu_state = MENU;

    bool should_do_menu();

    void do_menu(engine& eng);

    menu_system();

    sf::RenderTexture tex;

    int lw = 0, lh = 0;

    std::vector<ui_box> ui_elements;
};


#endif // MENU_HPP_INCLUDED
