#ifndef RENDERABLE_HPP_INCLUDED
#define RENDERABLE_HPP_INCLUDED

#include <vec/vec.hpp>
#include <vector>
#include <SFML/Graphics.hpp>

namespace render_type
{
    enum render_type
    {
        SQUARE
    };
}

typedef render_type::render_type render_t;

struct render_info
{
    ///vector?
    vec2f pos;
    render_t type;

    render_info(vec2f _pos, render_t _type)
    {
        pos = _pos;
        type = _type;
    }
};

struct renderable
{
    renderable(){}

    virtual void tick(sf::RenderWindow& win){};

    virtual ~renderable(){};
};

struct render_square : renderable
{
    vec2f pos;
    vec2f dim;
    vec3f col;

    render_square(vec2f _pos, vec2f _dim, vec3f _col)
    {
        pos = _pos;
        dim = _dim;
        col = _col;
    }

    void tick(sf::RenderWindow& win)
    {
        //vec2f lpos = pos - dim/2.f;

        if(pos.v[0] + dim.v[0] < 0 ||
           pos.v[0] - dim.v[0] >= win.getSize().x ||
           pos.v[1] + dim.v[1] < 0 ||
           pos.v[1] - dim.v[1] >= win.getSize().y)
            return;

        vec3f lcol = col * 255.f;

        sf::RectangleShape shape;

        shape.setSize({dim.v[0], dim.v[1]});
        shape.setFillColor({lcol.v[0], lcol.v[1], lcol.v[2]});
        shape.setOrigin(dim.v[0]/2.f, dim.v[1]/2.f);
        shape.setPosition(pos.v[0], pos.v[1]);

        win.draw(shape);
    }
};

struct renderer
{
    std::vector<renderable*> render_list;

    template<typename T>
    void add(T inf)
    {
        renderable* rn = new T(inf);

        render_list.push_back(rn);
    }

    void tick(sf::RenderWindow& win)
    {
        for(int i=0; i<render_list.size(); i++)
        {
            renderable* inf = render_list[i];

            inf->tick(win);

            delete inf;
        }

        render_list.clear();
    }
};

#endif // RENDERABLE_HPP_INCLUDED
