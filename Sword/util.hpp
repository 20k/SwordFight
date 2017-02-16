#ifndef UTIL_HPP_INCLUDED
#define UTIL_HPP_INCLUDED


///has the button been pressed once, and only once
template<sf::Keyboard::Key k>
bool once()
{
    static bool last;

    sf::Keyboard key;

    if(key.isKeyPressed(k) && !last)
    {
        last = true;

        return true;
    }

    if(!key.isKeyPressed(k))
    {
        last = false;
    }

    return false;
}

template<sf::Mouse::Button b>
bool once()
{
    static bool last;

    sf::Mouse m;

    if(m.isButtonPressed(b) && !last)
    {
        last = true;

        return true;
    }

    if(!m.isButtonPressed(b))
    {
        last = false;
    }

    return false;
}

template<sf::Keyboard::Key k1, sf::Keyboard::Key k2>
bool key_combo()
{
    sf::Keyboard key;

    static bool last = false;

    bool b1 = key.isKeyPressed(k1);
    bool b2 = key.isKeyPressed(k2);

    if(b1 && b2 && !last)
    {
        last = true;

        return true;
    }

    if(!b1 || !b2)
    {
        last = false;
    }

    return false;
}

#endif // UTIL_HPP_INCLUDED
