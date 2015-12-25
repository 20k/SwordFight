#include "world_manager.hpp"
#include <random>
#include "state.hpp"
#include "object.hpp"
#include <cstring>
#include "enemy_spawner.hpp"

rect connect_rects(rect r1, rect r2)
{
    vec2f c1, c2;

    c1 = r1.tl + r1.dim/2.f;
    c2 = r2.tl + r2.dim/2.f;

    vec2f new_centre = (c1 + c2)/2.f;

    vec2f new_dim = fabs(c2 - c1);

    const float min_width = 10.f;

    if(new_dim.v[0] < min_width)
        new_dim.v[0] = min_width;
    if(new_dim.v[1] < min_width)
        new_dim.v[1] = min_width;

    return {new_centre - new_dim/2.f, new_dim};
}

float get_float(std::minstd_rand& rnd)
{
    return (rnd() - rnd.min()) / (float)(rnd.max() - rnd.min());
}

rect get_random_room(int seed)
{
    std::minstd_rand rnd(seed);

    int val = rnd();

    //printf("%i\n", val);

    float v1 = get_float(rnd);
    float v2 = get_float(rnd);
    float v3 = get_float(rnd);
    float v4 = get_float(rnd);

    float min_size = 0.5f;

    v1 = (v1 + min_size) / (min_size + 1);
    v2 = (v2 + min_size) / (min_size + 1);
    v3 = (v3 + min_size) / (min_size + 1);
    v4 = (v4 + min_size) / (min_size + 1);

    vec2f pos = {v1, v2};
    vec2f dim = {v3, v4};

    pos = pos * 500.f;
    dim = dim * 100.f;

    return {pos, dim};

    //printf("%f\n", v1);
}

///https://stackoverflow.com/questions/306316/determine-if-two-rectangles-overlap-each-other
///RectA.X1 < RectB.X2 && RectA.X2 > RectB.X1 &&
///RectA.Y1 < RectB.Y2 && RectA.Y2 > RectB.Y1
bool intersect(rect r1, rect r2)
{
    float ax1 = r1.tl.v[0];
    float ax2 = r1.tl.v[0] + r1.dim.v[0];
    float bx1 = r2.tl.v[0];
    float bx2 = r2.tl.v[0] + r2.dim.v[0];

    float ay1 = r1.tl.v[1];
    float ay2 = r1.tl.v[1] + r1.dim.v[1];
    float by1 = r2.tl.v[1];
    float by2 = r2.tl.v[1] + r2.dim.v[1];

    return ax1 < bx2 && ax2 > bx1 && ay1 < by2 && ay2 > by1;
}

struct room_with_connectors
{
    std::vector<rect> rooms;
    std::vector<rect> connectors;
};

room_with_connectors recurse_room(rect parent, int& rooms_left, std::minstd_rand& rnd)
{
    rooms_left--;

    if(rooms_left <= 0)
        return room_with_connectors();

    /*if(rooms_left < 0)
    {
        throw std::runtime_error("broken in recurse room, < 0");
    }*/

    int rooms_offshoot = 1 + std::min(rooms_left - 1, (int)(rnd() % 3));

    int slot = rnd() % 4;

    float min_size = 0.5f;

    vec2f my_dim = ((vec2f){get_float(rnd), get_float(rnd)} + min_size) / (min_size + 1);

    float dim_scale = 50.f;

    my_dim = my_dim * dim_scale;

    ///from center of parent
    float max_distance = my_dim.length() + parent.dim.length()/2.f;

    vec2f pcentre = parent.tl + parent.dim/2.f;

    vec2f my_pos = (vec2f){0.f, -max_distance}.rot((slot/4.f) * 2 * M_PI) + pcentre - my_dim/2.f;

    room_with_connectors connect;

    std::vector<rect> ret;

    rect my_rect = {my_pos, my_dim};

    ret.push_back(my_rect);

    connect.connectors.push_back(connect_rects(my_rect, parent));

    for(int i=0; i<rooms_offshoot; i++)
    {
        room_with_connectors chd = recurse_room({my_pos, my_dim}, rooms_left, rnd);

        ret.insert(ret.end(), chd.rooms.begin(), chd.rooms.end());
        connect.connectors.insert(connect.connectors.end(), chd.connectors.begin(), chd.connectors.end());

        //for(auto& j : child)
        /*for(int j=0; j<chd.rooms.size(); j++)
        {
            rect br = connect_rects(chd.rooms[j], my_rect);
            connect.connectors.push_back(br);
            //ret.push_back(connect_rects(child[j], my_rect));
        }*/
    }

    connect.rooms = ret;

    return connect;
}

///scale from -pos to +pos to 0 -> 2pos
///world to collision
vec2i world_manager::world_to_collision(vec2f pos)
{
    vec2f clamped = clamp(pos - minvec, (vec2f){0.f, 0.f}, (vec2f){width, height} - 1);

    vec2i ret = {clamped.v[0], clamped.v[1]};

    return ret;
}

vec2f world_manager::collision_to_world(vec2f cpos)
{
    //vec2f clamped = clamp(cpos + minvec, minvec, maxvec);

    vec2f clamped = cpos + minvec;

    vec2f ret = {clamped.v[0], clamped.v[1]};

    return ret;
}

bool world_manager::is_open(int x, int y)
{
    if(x < 0 || y < 0 || x >= width || y >= width)
    {
        printf("warning, oob\n");
        return false;
    }

    return collision_map[y*width + x];
}

void world_manager::set_wall_state(int x, int y, bool is_open)
{
    if(x < 0 || y < 0 || x >= width || y >= width)
    {
        printf("warning, oob set\n");
        return;
    }

    collision_map[y*width + x] = is_open;
}

bool world_manager::within_any_room(vec2f pos)
{
    for(auto& i : rooms)
    {
        if(pos >= i.tl && pos < i.tl + i.dim)
            return true;
    }

    return false;
}

///the problem is dt meaning that its never really in the right place
///:[
bool world_manager::entity_in_wall(vec2f world_pos, vec2f dim)
{
    vec2f half_dim = ceil_away_from_zero(dim/2.f);
    vec2f fcol = world_pos;

    vec2f point[4];

    point[0] = fcol - half_dim;
    point[1] = fcol + (vec2f){half_dim.v[0], -half_dim.v[1]};
    point[2] = fcol + (vec2f){-half_dim.v[0], half_dim.v[1]};
    point[3] = fcol + half_dim;

    for(int i=0; i<4; i++)
    {
        vec2i coll = world_to_collision(ceil_away_from_zero(point[i]));

        if(!is_open(coll.v[0], coll.v[1]))
            return true;
    }

    return false;
}

vec2f world_manager::raycast(vec2f world_pos, vec2f dir)
{
    dir = dir.norm();

    vec2i collision_pos = world_to_collision(world_pos);

    if(!is_open(collision_pos.v[0], collision_pos.v[1]))
        return world_pos;

    vec2f cpos = {collision_pos.v[0], collision_pos.v[1]};

    int max_raycast_length = 4000;

    vec2f draw_dir;
    int num;

    line_draw_helper(cpos, cpos + dir * max_raycast_length, draw_dir, num);

    vec2f pos = cpos;

    for(int i=0; i<num; i++)
    {
        vec2f rpos = round(pos);

        if(!is_open(rpos.v[0], rpos.v[1]))
            return collision_to_world(pos);

        pos = pos + draw_dir;
    }

    return world_pos;
}

///need to shrink procedurally generated rooms to be fixed distance from each other

void world_manager::generate_level(int seed)
{
    std::minstd_rand rnd(seed);

    int room_variation = rnd() % 20;

    int base_num = 20;

    int room_num = base_num + room_variation;

    //vec2f base = {500.f, 500.f};
    vec2f base = {0,0};
    vec2f base_dim = {10, 10};

    rect parent = {base, base_dim};

    room_with_connectors connect;

    connect = recurse_room(parent, room_num, rnd);

    rooms.push_back(parent);

    rooms.insert(rooms.end(), connect.rooms.begin(), connect.rooms.end());

    rooms_without_corridors = rooms;

    rooms.insert(rooms.end(), connect.connectors.begin(), connect.connectors.end());

    const float scale = 20.f;

    for(auto& i : rooms)
    {
        i.tl = (i.tl - i.dim/2.f) * scale + (i.dim/2.f) * scale;
        i.dim = i.dim * scale;
    }

    for(auto& i : rooms_without_corridors)
    {
        i.tl = (i.tl - i.dim/2.f) * scale + (i.dim/2.f) * scale;
        i.dim = i.dim * scale;
    }

    minvec = {FLT_MAX, FLT_MAX};
    maxvec = {-FLT_MAX, -FLT_MAX};

    for(auto& i : rooms)
    {
        minvec = min(minvec, i.tl);
        maxvec = max(maxvec, i.tl + i.dim);
    }

    width = ceil(maxvec.v[0] - minvec.v[0]);
    height = ceil(maxvec.v[1] - minvec.v[1]);

    collision_map = new bool[width*height]();

    /*printf("pre\n");

    ///you know, it might be faster to bruteforce it rather than do this
    for(int y=0; y<height; y+=1)
    {
        for(int x=0; x<width; x+=1)d
        {
            vec2f cpos = {x, y};

            vec2f wpos = collision_to_world(cpos);

            bool is_openspace = within_any_room(wpos);

            set_wall_state(x, y, !is_openspace);

            //printf("%i", is_openspace);
        }

        //printf("\n");
    }

    printf("post\n");*/

    //printf("pre\n");

    for(auto& i : rooms)
    {
        for(int y = i.tl.v[1]; y < i.tl.v[1] + i.dim.v[1]; y++)
        {
            /*for(int x = i.tl.v[0]; x < i.tl.v[0] + i.dim.v[0]; x++)
            {
                vec2i proj = world_to_collision({x, y});

                set_wall_state(proj.v[0], proj.v[1], true);
            }*/

            vec2i proj = world_to_collision({i.tl.v[0], y});

            memset(&collision_map[proj.v[1]*width + proj.v[0]], true, i.dim.v[0]);
        }
    }

    level_rng = rnd;
}

void world_manager::spawn_level(std::vector<game_entity*>& entities)
{
    vec2f most_remote = {0,0};
    int most_id = -1;

    for(int i=0; i<rooms_without_corridors.size(); i++)
    {
        rect room = rooms_without_corridors[i];

        vec2f centre = room.tl + room.dim/2.f;

        if(fabs(centre.v[0]) > most_remote.v[0] || fabs(centre.v[1]) > most_remote.v[1] || centre.length() > most_remote.length())
        {
            most_remote = centre;
            most_id = i;
        }
    }

    if(most_id == -1)
    {
        throw std::runtime_error("broke");
    }

    ///for spawning in
    ///added the id of the room the player is spawning in!
    ///this is because several rooms can overlap
    //std::vector<int> rooms_disallowed{most_id};

    std::vector<int> rooms_allowed;

    for(int i=0; i<rooms_without_corridors.size(); i++)
    {
        ///definitely not allowed
        if(i == most_id)
            continue;

        if(!intersect(rooms_without_corridors[most_id], rooms_without_corridors[i]))
        {
            //rooms_disallowed.push_back(i);
            rooms_allowed.push_back(i);
        }
    }

    for(int i=0; i <rooms_allowed.size(); i++)
    {
        rect room = rooms_without_corridors[rooms_allowed[i]];

        int min_enemies = 2;
        int variation = 4;

        int num_enemies = min_enemies + (level_rng() % (variation + 1));

        enemy_spawner spawn;

        for(int j=0; j<num_enemies; j++)
        {
            ai_character* ch = spawn.get_random_character(level_rng);

            ///temp!!
            ch->enabled = false;

            vec2f min_spawn = room.dim/4.f;
            vec2f max_spawn = 3 * room.dim / 4.f;

            vec2f pos = rand_det(level_rng, min_spawn, max_spawn);

            pos = pos + room.tl;

            //vec2f pos = room.tl;

            ch->set_pos(pos);

            entities.push_back(ch);
        }
    }

    player* play = new player;
    play->set_team(team::FRIENDLY);
    play->set_pos(most_remote);

    entities.push_back(play);
}

void world_manager::draw_rooms(state& s)
{
    for(int i=0; i<rooms.size(); i++)
    {
        render_square sq(rooms[i].tl + rooms[i].dim/2.f, rooms[i].dim, {0.4f, 0.4f, 0.4f});

        //sq.pos = sq.pos * 20.f;
        //sq.dim = sq.dim * 20.f;

        sq.tick(*s.win);
    }
}
