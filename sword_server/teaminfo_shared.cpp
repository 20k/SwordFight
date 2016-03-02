#include "teaminfo_shared.hpp"

vec3f team_info::get_team_col(int team)
{
    vec3f rcol = {248, 63, 95};
    vec3f bcol = {63, 95, 248};

    if(team == 0)
        return rcol;

    if(team == 1)
        return bcol;

    return {255, 255, 255};
}
