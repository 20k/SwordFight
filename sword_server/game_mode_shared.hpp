#ifndef GAME_MODE_SHARED_HPP_INCLUDED
#define GAME_MODE_SHARED_HPP_INCLUDED

///between client and server
///we're doing it. Its getting unified folks
///Then I can implement stuff
///Ok. First decision. This class IS going to know if its client or server
///we're going to keep all gamemode logic in one place EVEN THOUGH aspects of this won't be run for each client
///This will make it a lot more straightforward to manage gamemodes as EVERYTHING (pretty much) will be in ONE place-ish
struct game_mode_handler_shared
{

};

#endif // GAME_MODE_SHARED_HPP_INCLUDED
