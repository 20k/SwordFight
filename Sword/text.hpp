#ifndef TEXT_HPP_INCLUDED
#define TEXT_HPP_INCLUDED

#include <vec/vec.hpp>
#include <vector>

#include <SFML/Graphics.hpp>

struct text
{
    static void add(const std::string& str, int time, vec2f pos);
    static void add_random(const std::string& str, int time);
    static void draw(sf::RenderTarget* draw_to);
    static void immediate(sf::RenderTarget* draw_to, const std::string& str, vec2f pos);
    //static void set_renderwindow(sf::RenderWindow& win);

private:
    static std::vector<sf::Clock> elapsed;

    static std::vector<std::string> txt;
    static std::vector<int> alive_milliseconds;
    static std::vector<vec2f> pos;

    //static sf::RenderWindow* win;
    static int width;
    static int height;
};


#endif // TEXT_HPP_INCLUDED
