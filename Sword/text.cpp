#include "text.hpp"

std::vector<std::string> text::txt;
std::vector<int> text::alive_milliseconds;
std::vector<vec2f> text::pos;

std::vector<sf::Clock> text::elapsed;

sf::RenderWindow* text::win;
int text::width;
int text::height;

void text::add(const std::string& str, int time, vec2f p)
{
    txt.push_back(str);
    alive_milliseconds.push_back(time);
    pos.push_back(p);

    sf::Clock clk;
    clk.restart();

    elapsed.push_back(clk);
}

void text::add_random(const std::string& str, int time)
{
    vec2f centre = (vec2f){width/2.f, height/2.f};

    vec2f ran = (vec2f){randf<1, float>(), randf<1, float>()};

    ran = ran - 0.5f;
    ran = ran * 0.4f;

    ran = ran * (float)height;

    //printf("%f %f\n", ran.v[0], ran.v[1]);

    ran = centre + ran;

    add(str, time, ran);
}

void text::set_renderwindow(sf::RenderWindow& w)
{
    win = &w;

    width = win->getSize().x;
    height = win->getSize().y;
}

void text::draw()
{
    static int loaded = 0;

    static sf::Font font;

    if(!loaded)
    {
        font.loadFromFile("Res/VeraMono.ttf");
        loaded = 1;
    }

    if(win == nullptr)
        return;

    for(int i=0; i<txt.size(); i++)
    {
        std::string cur = txt[i];
        vec2f loc = pos[i];

        sf::String str;

        str = cur;

        sf::Text to_draw;

        to_draw.setString(str);
        to_draw.setFont(font);
        to_draw.setPosition(loc.v[0], height - loc.v[1]);
        to_draw.setCharacterSize(12);
        to_draw.setColor(sf::Color(255, 255, 255, 255));

        win->draw(to_draw);


        float t = alive_milliseconds[i];
        float cur_t = elapsed[i].getElapsedTime().asMicroseconds() / 1000.f;

        if(cur_t > t)
        {
            txt.erase(txt.begin() + i);
            alive_milliseconds.erase(alive_milliseconds.begin() + i);
            pos.erase(pos.begin() + i);
            elapsed.erase(elapsed.begin() + i);

            i--;
        }
    }
}
