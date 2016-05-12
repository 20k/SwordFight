#ifndef BROWSER_HPP_INCLUDED
#define BROWSER_HPP_INCLUDED

#include <string>
#include <vec/vec.hpp>
#include <SFML/Graphics.hpp>

namespace ui
{
    enum ui
    {
        NONE = 0,
        TEXT = 1,
        CIRCLE = 2,
        LINESNAP_45 = 3,
        BOX = 4
    };
}

typedef ui::ui ui_type;

///this should really be a tagged union, but that's a little too much work
struct ui_element
{
    ui_type type = ui::NONE;

    std::string text;
    int font_size = 12;
    float rad = 1;
    vec2f dim = {1, 1};
    bool centered = false;
    float vertical_pad = 10;

    vec4f col = {1,1,1,1};

    ui_element* parent = nullptr;
    std::vector<ui_element*> children; ///automagically updated

    vec2f relative_pos = {0,0};

    vec2f calculated_absolute_pos = {0,0};

    void set_relative_pos(vec2f pos);
    void set_parent(ui_element* par);
    vec2f get_global_pos();
    vec2f get_local_pos();

    void draw(sf::RenderWindow& win);

    ui_element(ui_type _type);

    vec2f calculated_interact_dim = {0,0};
    vec2f calculated_render_pos = {-5, -5};

    bool is_mouse_hover(vec2f& m);

    vec2f get_element_size();

    int get_which_child_am_i(ui_element* elem);

    vec2f propagate_positions(vec2f offset);

    void draw_tree(sf::RenderWindow& win);
};

#endif // BROWSER_HPP_INCLUDED
