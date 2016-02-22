#include "menu.hpp"

#include "../openclrenderer/engine.hpp"

#include "text.hpp"

#include "util.hpp"

static sf::Font font;

void start_game(menu_system& menu)
{
    menu.current_menu_state = menu_system::INGAME;
}

menu_system::menu_system()
{
    font.loadFromFile("Res/VeraMono.ttf");

    ui_box start;
    start.pos = {0.5f, 0.5f};
    start.dim = {100, 100};
    start.label = "Start";
    start.font_size = 60;
    start.callback = start_game;

    sf::Text text(start.label, font, start.font_size);

    float pad = 1.2f;

    start.dim = (vec2f){text.getGlobalBounds().width, text.getGlobalBounds().height} * pad;

    ui_elements.push_back(start);
}

bool menu_system::should_do_menu()
{
    return current_menu_state != INGAME;
}

void menu_system::do_menu(engine& window)
{
    if(!window.render_me)
        return;

    if(window.width != lw || window.height != lh)
    {
        lw = window.width;
        lh = window.height;

        tex.create(lw, lh);

        tex.clear(sf::Color(60, 60, 60, 255));

        tex.display();
    }

    ///Note to self, I have totally broken the OpenGL rendering internals
    ///which makes SFML really very sad
    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);

    //text::immediate(&window.window, "Start", {window.width/2.f, window.height/1.5f}, 60, true);

    bool lclick = once<sf::Mouse::Left>();

    vec2f wind = (vec2f){window.width, window.height};

    for(auto& i : ui_elements)
    {
        vec2f mouse_coords = {window.get_mouse_x(), window.get_mouse_y()};

        bool is_within = mouse_coords > i.pos * wind - i.dim/2.f && mouse_coords < i.pos * wind + i.dim/2.f;

        vec3f col = {0.7,0.7,0.7};

        if(is_within)
        {
            col = {1,1,1};

            if(lclick)
            {
                i.callback(*this);
            }
        }

        text::immediate(&window.window, i.label, i.pos * (vec2f){window.width, window.height}, i.font_size, true, col);
    }

    sf::Sprite spr;
    spr.setTexture(tex.getTexture());

    spr.setColor(sf::Color(255, 255, 255, 60));

    window.window.draw(spr);

    //window.window.clear(sf::Color(0,0,0,255));
}
