#ifndef TEAMINFO_SHARED_HPP_INCLUDED
#define TEAMINFO_SHARED_HPP_INCLUDED

#include <vec/vec.hpp>

namespace team_info
{
    vec3f get_team_col(int team);

    std::string get_texture_cache_name(int team);
}

#endif // TEAMINFO_SHARED_HPP_INCLUDED
