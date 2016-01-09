#ifndef SOUND_H_INCLUDED
#define SOUND_H_INCLUDED

#include <SFML/Audio.hpp>
#include <string>

/*struct sound
{
    std::string file;
};*/

namespace music
{
    enum music_purpose : uint32_t
    {
        NONE = 0,
        METEOR = 1,
        INDOORS = 2,
        GENERAL = 4,
        BADTHING = 8,
        GOODTHING = 16,
        LOWAIR = 32,
        GAMEOVER = 64,
        RETRY = 128,
        GAMESTART = 256,
        MAINMENU = 512
    };

    static std::vector<std::string> file_names =
    {
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 01 Castlecool.flac",
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 11 Strutters.flac",
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 06 Search for Legitimacy.flac", ///need to adjust start
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 07 Run!.flac",
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 12 Life Support.flac",
        "Sam_Berrow - HEART AND SOUL -EP- - 02 Maniacle Monacle.flac"/*,
        "Sam_Berrow - HEART AND SOUL -EP- - 01 Heart And Soul Of A Drug Addicted Cube.flac" ///not sure about this one
        */,
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART I) - 06 Indilgent.flac"
    };

    static std::vector<uint32_t> purpose =
    {
        GENERAL | INDOORS, ///use bitflag
        GENERAL,
        GOODTHING,
        LOWAIR,
        GAMEOVER | RETRY,
        GENERAL | GAMESTART/*,
        GENERAL*/,
        METEOR | MAINMENU
    };

    extern int current_song;
    extern sf::Music currently_playing;
    static float low_air_threshold = 0.2f;

    static std::string get_current_song_name()
    {
        if(current_song < 0 || current_song >= (int)file_names.size())
            return "";

        return file_names[current_song];
    }

    static void swap_to_song_type(music_purpose new_purpose)
    {
        if(new_purpose == NONE)
        {
            currently_playing.stop();
            current_song = -1;
            return;
        }

        ///at the moment do shit
        int new_song = -1;

        for(int i=0; i<(int)file_names.size(); i++)
        {
            int song_offset = (i + current_song) % file_names.size();

            music_purpose current_purpose = (music_purpose)purpose[song_offset];

            if((current_purpose & new_purpose) > 0)
            {
                new_song = song_offset;
                break;
            }
        }

        if(new_song == -1)
            return;

        if(new_song == current_song && currently_playing.getStatus() != sf::SoundSource::Stopped)
            return;

        currently_playing.stop();

        currently_playing.openFromFile("res/" + file_names[new_song]);

        currently_playing.play();

        current_song = new_song;
    }

    static void tick()
    {
        if(currently_playing.getStatus() == sf::SoundSource::Stopped)
        {
            current_song++;

            swap_to_song_type(GENERAL);
        }
    }
};



#endif // SOUND_H_INCLUDED
