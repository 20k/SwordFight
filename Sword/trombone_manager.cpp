#include "trombone_manager.hpp"
#include <SFML/Graphics.hpp>
#include "util.hpp"
#include "../openclrenderer/engine.hpp"
#include <vec/vec.hpp>
#include "sound.hpp"
#include "fighter.hpp"

void trombone_manager::init(object_context* _ctx)
{
    ctx = _ctx;

    std::string volume = "mezzo-forte";
    std::string duration = "05";
    std::string octave = "3";
    std::string dir = "./Res/trombone/";

    std::string tones[12] =
    {
        "C",
        "Cs",
        "D",
        "Ds",
        "E",
        "F",
        "Fs",
        "G",
        "Gs",
        "A",
        "As",
        "B",
    };

    for(int i=0; i<12; i++)
    {
        std::string str;

        str = dir + "trombone_" + tones[i] + octave + "_" + duration + "_" + volume + "_normal.ogg";

        sound::add_extra_soundfile(str);
    }

    trombone = ctx->make_new();
    trombone->set_active(true);
    trombone->set_file("./Res/trombone_cutdown_nomod.obj");
}

void trombone_manager::tick(engine& window, fighter* my_fight)
{
    if(!is_active)
        return;

    tone += window.get_scrollwheel_delta() > 0.01 ? 1 : 0;
    tone += window.get_scrollwheel_delta() < -0.01 ? -1 : 0;

    tone = clamp(tone, 0, 11);

    sf::Keyboard key;

    if(once<sf::Keyboard::Num1>())
    {
        sound::add(11 + tone, my_fight->parts[bodypart::BODY].global_pos, true, false);
    }
}

void trombone_manager::play(fighter* my_fight)
{
    sound::add(11 + tone, my_fight->parts[bodypart::BODY].global_pos, true, false);
}

void trombone_manager::set_active(bool active)
{
    is_active = active;

    if(!is_active)
    {
        trombone->hide();
    }
}
