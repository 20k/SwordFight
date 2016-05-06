#include <SFML/Graphics.hpp>

#include "browser.hpp"

int main()
{
    sf::RenderWindow win(sf::VideoMode(800, 600), "hello");

    ui_element root(ui::TEXT);

    root.text = "hi";

    root.set_relative_pos({10, 10});

    ui_element subnode(ui::TEXT);

    subnode.text = "bum";

    subnode.set_parent(&root);

    subnode.set_relative_pos({10, 0});


    ui_element r2(ui::TEXT);
    r2.text = "r2";

    r2.set_relative_pos({0,0});

    r2.set_parent(&subnode);

    ui_element r3(ui::TEXT);
    r3.text = "r3";

    r3.set_relative_pos({0,0});

    r3.set_parent(&root);

    root.propagate_positions({0,0});


    while(win.isOpen())
    {
        sf::Event event;

        while(win.pollEvent(event))
        {

        }

        root.draw(win);
        subnode.draw(win);
        r2.draw(win);
        r3.draw(win);

        win.display();
        win.clear(sf::Color(0,0,0));
    }

}
