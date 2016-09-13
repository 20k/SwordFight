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

std::vector<sf::SoundBuffer> s;

std::deque<sf::Sound*> sounds;
std::deque<vec3f> positions;
std::deque<bool> relative_positioning;

std::vector<std::string> extra_sfiles;

int sound::extra_start = 0;

void sound::add_extra_soundfile(const std::string& file)
{
    extra_sfiles.push_back(file);
}

///1 is clang, 0 is hrrk
///we need per-sound attenuation
///also, the distance needs to drop much more rapidly. This has likely just never been noticed
///because not playing with 4+ players
void sound::add(int type, vec3f pos, bool relative, bool random)
{
    static int loaded = 0;

    if(!loaded)
    {
        s.resize(11);

        s[0].loadFromFile("Res/hitm.wav");
        s[1].loadFromFile("Res/clangm.wav");

        s[2].loadFromFile("Res/footsteps/asphalt/1.wav");
        s[3].loadFromFile("Res/footsteps/asphalt/2.wav");
        s[4].loadFromFile("Res/footsteps/asphalt/3.wav");
        s[5].loadFromFile("Res/footsteps/asphalt/4.wav");
        s[6].loadFromFile("Res/footsteps/asphalt/5.wav");
        s[7].loadFromFile("Res/footsteps/asphalt/6.wav");
        s[8].loadFromFile("Res/footsteps/asphalt/7.wav");
        s[9].loadFromFile("Res/footsteps/asphalt/8.wav");

        s[10].loadFromFile("Res/emphasis.wav");

        extra_start = s.size();

        for(auto& i : extra_sfiles)
        {
            s.push_back(sf::SoundBuffer());

            s.back().loadFromFile(i);
        }

        loaded = 1;
    }

    /*if(type == 0 || type == 1)
    {
        add(10, pos, relative);
    }*/

    vec3f rel = {0,0,0};

    rel = pos - listener_pos;
    rel = rel.back_rot({0,0,0}, listener_rot);

    sf::Sound* sound = new sf::Sound;

    sounds.push_back(sound);
    positions.push_back(pos);
    relative_positioning.push_back(relative);

    sf::Sound& sd = *sounds.back();

    sd.setBuffer(s[type]);

    if(random)
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
        if(sounds[i]->getStatus() == sf::Sound::Status::Stopped)
        {
            delete sounds[i];

            sounds.erase(sounds.begin() + i);
            positions.erase(positions.begin() + i);
            relative_positioning.erase(relative_positioning.begin() + i);
            i--;
        }
    }
}

void sound::update_listeners()
{
    for(int i=0; i<sounds.size(); i++)
    {
        if(relative_positioning[i])
            continue;

        sf::Sound& sd = *sounds[i];
        vec3f pos = positions[i];

        vec3f rel = {0,0,0};

        rel = pos - listener_pos;
        rel = rel.back_rot({0,0,0}, listener_rot);

        sd.setPosition(-rel.v[0], rel.v[1], -rel.v[2]);
    }
}
