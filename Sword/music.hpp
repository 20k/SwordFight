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
        ROUND_START = 1,
        ROUND_GENERAL = 2,
        ROUND_NEAR_END = 4, ///time low. Dynamically calculate which song to play based on time left
        ROUND_END = 8,
        MAIN_MENU = 16,
        VICTORY = 32,
        DEFEAT = 64,
        LOW_HP = 128,
        DRAMATIC = 256,
        ROUND_SPANNING  = 512 ///maybe instead of this, chop life support up into 2 clips?

        /*METEOR = 1,
        INDOORS = 2,
        GENERAL = 4,
        BADTHING = 8,
        GOODTHING = 16,
        LOWAIR = 32,
        GAMEOVER = 64,
        RETRY = 128,
        GAMESTART = 256,
        MAINMENU = 512*/
    };

    /*static std::vector<std::string> file_names =
    {
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 01 Castlecool.flac",
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 11 Strutters.flac",
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 06 Search for Legitimacy.flac", ///need to adjust start
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 07 Run!.flac",
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 12 Life Support.flac",
        "Sam_Berrow - HEART AND SOUL -EP- - 02 Maniacle Monacle.flac",
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART I) - 06 Indilgent.flac"
    };*/

    ///Maybe I should only have music at the start and end of rounds? And when stuff happens?
    ///need to get search for legitimacy but without the continuation
    static std::vector<std::string> file_names
    {
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART I) - 06 Indilgent.flac", ///start, general
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 05 Brain Journey.flac", ///general. Also for dramatic things
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 07 Run!.flac", ///near_end, low_hp, or potentially general
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 11 Strutters.flac", ///general? Not sure on this one
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 14 Trash (Trash.).flac", ///start really
        "Sam_Berrow - HEART AND SOUL -EP- - 02 Maniacle Monacle.flac", ///general?
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART I) - 10 Fenk.flac", /// can definitely use the first 8 seconds of this
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 01 Castlecool.flac", ///could use this is something dramatic happens
        "Sam_Berrow - THE EMPLOYMENT TAPES (PART II) - 12 Life Support.flac" ///only use this if I can have it span across two rounds
    };

    static std::vector<uint32_t> purpose
    {
        ROUND_START | ROUND_GENERAL,
        DRAMATIC | ROUND_GENERAL,
        LOW_HP,
        ROUND_GENERAL,
        ROUND_START,
        ROUND_START | ROUND_GENERAL,
        VICTORY,
        DRAMATIC,
        ROUND_SPANNING
    };

    /*static std::vector<uint32_t> purpose =
    {
        GENERAL | INDOORS, ///use bitflag
        GENERAL,
        GOODTHING,
        LOWAIR,
        GAMEOVER | RETRY,
        GENERAL | GAMESTART,
        METEOR | MAINMENU
    };*/

    extern int current_song;
    extern sf::Music currently_playing;

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

            swap_to_song_type(ROUND_GENERAL);
        }
    }
};



#endif // SOUND_H_INCLUDED
