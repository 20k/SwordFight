#include "text.hpp"

std::vector<std::string> text::txt;
std::vector<int> text::alive_milliseconds;
std::vector<vec2f> text::pos;

std::vector<sf::Clock> text::elapsed;

//sf::RenderWindow* text::win;
int text::width;
int text::height;

sf::Font font;
int loaded = 0;


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

/*void text::set_renderwindow(sf::RenderWindow& w)
{
    win = &w;

    width = win->getSize().x;
    height = win->getSize().y;
}*/

void text::draw(sf::RenderTarget* draw_to)
{
    if(!loaded)
    {
        font.loadFromFile("Res/VeraMono.ttf");
        loaded = 1;
    }

    if(!draw_to)
        return;

    width = draw_to->getSize().x;
    height = draw_to->getSize().y;

    for(int i=0; i<txt.size(); i++)
    {
        std::string cur = txt[i];
        vec2f loc = pos[i];

        sf::String str;

        str = cur;

        sf::Text to_draw(cur, font, 12);

        to_draw.setPosition((int)loc.v[0], ceil(height - loc.v[1]));
        to_draw.setColor(sf::Color(255, 255, 255, 255));

        draw_to->draw(to_draw);

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

void text::immediate(sf::RenderTarget* draw_to, const std::string& cur, vec2f pos, int size, bool centre, vec3f col)
{
    if(!loaded)
    {
        font.loadFromFile("Res/VeraMono.ttf");
        loaded = 1;
    }

    if(!draw_to)
        return;

    //sf::String str = cur;

    sf::Text to_draw(cur.c_str(), font, size);

    //to_draw.setString(str);
    //to_draw.setFont(font);
    to_draw.setPosition(pos.v[0], draw_to->getSize().y - pos.v[1]);
    //to_draw.setCharacterSize(size);

    col = clamp(col, 0.f, 1.f);

    col = col * 255;

    to_draw.setColor(sf::Color(col.v[0], col.v[1], col.v[2], 255));

    if(centre)
    {
        float width = to_draw.getLocalBounds().width;
        float height = to_draw.getLocalBounds().height;

        to_draw.setPosition(pos.v[0] - (width)/2.f, (draw_to->getSize().y - pos.v[1]) - height);
    }

    draw_to->draw(to_draw);
}
