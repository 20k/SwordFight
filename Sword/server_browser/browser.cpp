#include "browser.hpp"

sf::Font font;

void check_load_font()
{
    static bool loaded_font = false;

    if(!loaded_font)
    {
        font.loadFromFile("VeraMono.ttf");
        loaded_font = true;

        printf("loaded font\n");
    }
}

vec2f ui_element::get_local_pos()
{
    return relative_pos;
}

vec2f ui_element::get_global_pos()
{
    if(!parent)
        return relative_pos;

    return parent->get_global_pos() + relative_pos;
}

vec2f ui_element::propagate_positions(vec2f offset)
{
    ///to render ME at
    vec2f my_render_offset = offset + get_local_pos() + (vec2f){0, vertical_pad*2};

    calculated_render_pos = my_render_offset;

    float total_voffset = get_element_size().v[1] + vertical_pad*2;

    vec2f children_offset_base = my_render_offset + (vec2f){0, get_element_size().v[1]};

    for(auto& i : children)
    {
        vec2f dim = i->propagate_positions(children_offset_base);

        total_voffset += dim.v[1];

        children_offset_base.v[1] += dim.v[1];
    }

    return (vec2f){my_render_offset.v[0], total_voffset};

    ///propagate_positions return vertical dim
    ///draw next child at that offset
}

void ui_element::set_relative_pos(vec2f pos)
{
    relative_pos = pos;
}

void ui_element::set_parent(ui_element* par)
{
    parent = par;

    for(auto& i : parent->children)
    {
        if(i == this)
            return;
    }

    parent->children.push_back(this);
}

void ui_element::draw_tree(sf::RenderWindow& win)
{
    for(auto& i : children)
    {
        i->draw_tree(win);
    }

    draw(win);
}

void ui_element::draw(sf::RenderWindow& win)
{
    col = clamp(col, (vec4f){0,0,0,0}, (vec4f){1,1,1,1});

    check_load_font();

    vec2f global_pos = calculated_render_pos;//get_global_pos();

    sf::Color sfcol(col.v[0] * 255, col.v[1] * 255, col.v[2] * 255, col.v[3] * 255);

    if(type == ui::TEXT)
    {
        sf::Text txt;
        txt.setFont(font);
        txt.setCharacterSize(font_size);
        txt.setColor(sfcol);
        txt.setString(text.c_str());

        txt.setPosition({global_pos.v[0], global_pos.v[1]});

        win.draw(txt);

        calculated_interact_dim = {txt.getGlobalBounds().width, txt.getGlobalBounds().height};

        //printf("hi %f %f %s\n", global_pos.v[0], global_pos.v[1], text.c_str());
    }

    /*if(type == ui::CIRCLE)
    {
        sf::CircleShape circle;
        circle.setRadius(rad);
        circle.setFillColor(sfcol);

        float alpha = col.v[3];

        float edge_alpha = alpha * 1.5;

        edge_alpha = std::min(edge_alpha, 1.f);

        circle.setOutlineColor(sf::Color(col.v[0] * 255, col.v[1] * 255, col.v[2] * 255, edge_alpha * 255));

        circle.setOutlineThickness(3);

        circle.setPosition({global_pos.v[0], global_pos.v[1]});

        win.draw(circle);

        ///ergh, circles are centered but text is not ;_;
        calculated_interact_dim = {rad*2, rad*2};
    }*/

    if(type == ui::LINESNAP_45)
    {

    }

    if(type == ui::BOX)
    {

        calculated_interact_dim = dim;
    }
}

ui_element::ui_element(ui_type _type)
{
    type = _type;
}

bool ui_element::is_mouse_hover(vec2f& m)
{

}

vec2f ui_element::get_element_size()
{
    return calculated_interact_dim;
}


int ui_element::get_which_child_am_i(ui_element* elem)
{
    for(int i=0; i<children.size(); i++)
    {
        if(children[i] == elem)
            return i;
    }

    return -1;
}
