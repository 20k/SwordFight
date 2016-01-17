#include "sound.hpp"

#include <SFML/Audio.hpp>
#include <deque>

vec3f sound::listener_pos;
vec3f sound::listener_rot;

void sound::set_listener(vec3f pos, vec3f rot)
{
    listener_pos = pos;
    listener_rot = rot;
}

sf::SoundBuffer s[2];

std::deque<sf::Sound> sounds;
std::deque<vec3f> positions;

///1 is clang, 0 is hrrk
void sound::add(int type, vec3f pos)
{
    static int loaded = 0;

    if(!loaded)
    {
        s[0].loadFromFile("res/hitm.wav");
        s[1].loadFromFile("res/clangm.wav");

        loaded = 1;
    }

    vec3f rel = {0,0,0};

    rel = pos - listener_pos;
    rel = rel.back_rot({0,0,0}, listener_rot);


    sf::Sound sound;

    sounds.push_back(sound);
    positions.push_back(pos);

    sf::Sound& sd = sounds.back();

    sd.setBuffer(s[type]);
    sd.setPitch(randf_s(0.8f, 1.20f));
    sd.setRelativeToListener(true);
    sd.setAttenuation(0.005f);
    sd.setVolume(randf_s(80, 100));

    sd.setPosition(-rel.v[0], rel.v[1], -rel.v[2]); ///sfml is probably opengl coords. X may be backwards

    //printf("%f %f %f\n", -pos.v[0], pos.v[1], pos.v[2]);

    sd.play();

    ///sfml is probably 3d opengl
    ///which means my z is backwards to theirs

    for(int i=0; i<sounds.size(); i++)
    {
        if(sounds[i].getStatus() == sf::Sound::Status::Stopped)
        {
            sounds.erase(sounds.begin() + i);
            positions.erase(positions.begin() + i);
            i--;
        }
    }
}

void sound::update_listeners()
{
    //for(auto& sd : sounds)
    for(int i=0; i<sounds.size(); i++)
    {
        sf::Sound& sd = sounds[i];
        vec3f pos = positions[i];

        vec3f rel = {0,0,0};

        rel = pos - listener_pos;
        rel = rel.back_rot({0,0,0}, listener_rot);

        sd.setPosition(-rel.v[0], rel.v[1], -rel.v[2]);
    }
}
