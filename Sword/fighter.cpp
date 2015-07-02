#include "fighter.hpp"
#include "physics.hpp"
#include "../openclrenderer/obj_mem_manager.hpp"
#include <unordered_map>

const vec3f* bodypart::init_default()
{
    using namespace bodypart;

    static vec3f pos[COUNT];

    pos[HEAD] = {0,0,0};

    float x_arm_displacement = 1;
    float y_arm_displacement = 1;

    pos[LUPPERARM] = {-x_arm_displacement, y_arm_displacement, 0.f};
    pos[RUPPERARM] = {x_arm_displacement, y_arm_displacement, 0.f};

    pos[LLOWERARM] = {-x_arm_displacement, y_arm_displacement + 1, 0.f};
    pos[RLOWERARM] = {x_arm_displacement, y_arm_displacement + 1, 0.f};

    pos[LHAND] = {-x_arm_displacement, pos[LLOWERARM].v[1] + 1, 0.f};
    pos[RHAND] = {x_arm_displacement, pos[RLOWERARM].v[1] + 1, 0.f};

    pos[BODY] = {0, 1, 0};

    float x_leg_displacement = 0.5f;

    pos[LUPPERLEG] = {-x_leg_displacement, pos[BODY].v[1] + 2, 0};
    pos[RUPPERLEG] = {x_leg_displacement, pos[BODY].v[1] + 2, 0};

    pos[LLOWERLEG] = {-x_leg_displacement, pos[LUPPERLEG].v[1] + 2, 0};
    pos[RLOWERLEG] = {x_leg_displacement, pos[RUPPERLEG].v[1] + 2, 0};

    pos[LFOOT] = {-x_leg_displacement, pos[LLOWERLEG].v[1] + 1, 0};
    pos[RFOOT] = { x_leg_displacement, pos[RLOWERLEG].v[1] + 1, 0};

    for(size_t i=0; i<COUNT; i++)
    {
        pos[i] = pos[i] * scale;

        pos[i].v[1] = -pos[i].v[1];
    }

    return pos;
}

void part::set_type(bodypart_t t)
{
    type = t;

    set_pos(bodypart::default_position[t]);
}

part::part()
{
    hp = 1.f;

    set_pos({0,0,0});
    set_rot({0,0,0});

    model.set_file("./Res/bodypart_red.obj");
}

part::part(bodypart_t t) : part()
{
    set_type(t);
}

part::~part()
{
    model.set_active(false);
}

void part::set_pos(vec3f _pos)
{
    pos = _pos;

    //model.set_pos({pos.v[0], pos.v[1], pos.v[2]});

   // model.g_flush_objects();
}

void part::set_rot(vec3f _rot)
{
    rot = _rot;

    //model.set_rot({rot.v[0], rot.v[1], rot.v[2]});

    //model.g_flush_objects();
}

void part::set_team(int _team)
{
    if(_team == 0)
    {
        model.set_file("./Res/bodypart_red.obj");
    }
    else
    {
        model.set_file("./Res/bodypart_blue.obj");
    }

    model.set_active(true);

    team = _team;
}

void part::damage(float dam)
{
    hp -= dam;

    if(model.isactive && hp < 0.0001f)
    {
        printf("I blowed up %s\n", bodypart::names[type].c_str());
        model.set_active(false);

        obj_mem_manager::load_active_objects();
        obj_mem_manager::g_arrange_mem();
        obj_mem_manager::g_changeover();

    }
}

size_t movement::gid = 0;

void movement::load(int _hand, vec3f _end_pos, float _time, int _type, bodypart_t b, movement_t _move_type)
{
    end_time = _time;
    fin = _end_pos;
    type = _type;
    hand = _hand;

    limb = b;

    move_type = _move_type;
}

float movement::time_remaining()
{
    return std::max(end_time - clk.getElapsedTime().asMilliseconds(), 0.f);
}

float movement::get_frac()
{
    return (float)clk.getElapsedTime().asMilliseconds() / end_time;
}

void movement::fire()
{
    clk.restart();
    going = true;
}

bool movement::finished()
{
    if(going && get_frac() >= 1)
        return true;

    return false;
}

movement::movement()
{
    hit_id = -1;

    end_time = 0.f;
    start = {0,0,0};
    fin = {0,0,0};
    type = 0;
    hand = 0;
    going = false;

    /*does_damage = true;
    does_block = false;

    moves_character = false;*/

    move_type = mov::DAMAGING;

    id = gid++;
}

movement::movement(int hand, vec3f end_pos, float time, int type, bodypart_t b, movement_t _move_type) : movement()
{
    load(hand, end_pos, time, type, b, _move_type);
}

bool movement::does(movement_t t)
{
    return move_type & t;
}

void movement::set(movement_t t)
{
    move_type = (movement_t)(move_type | t);
}

void sword::set_team(int _team)
{
    if(_team == 0)
    {
        model.set_file("./Res/sword_red.obj");

    }
    else
    {
        model.set_file("./Res/sword_blue.obj");
    }

    model.set_active(true);

    team = _team;
}

sword::sword()
{
    model.set_pos({0, 0, -100});
    dir = {0,0,0};
    model.set_file("./Res/sword_red.obj");

}

void sword::scale()
{
    model.scale(50.f);
}

void sword::set_pos(vec3f _pos)
{
    pos = _pos;

    //model.set_pos({pos.v[0], pos.v[1], pos.v[2]});

    //model.g_flush_objects();
}

void sword::set_rot(vec3f _rot)
{
    rot = _rot;

    //model.set_rot({rot.v[0], rot.v[1], rot.v[2]});

    //model.g_flush_objects();
}

fighter::fighter()
{
    left_frac = 0.f;
    right_frac = 0.f;

    idle_fired_first = -1;

    idling = false;

    team = 0;

    left_full = false;

    left_id = -1;
    right_id = -1;

    left_stage = 0;
    right_stage = 1;


    left_fired = false;
    right_fired = false;

    stance = 0;

    rest_positions = bodypart::init_default();


    for(size_t i=0; i<bodypart::COUNT; i++)
    {
        parts[i].set_type((bodypart_t)i);
        old_pos[i] = parts[i].pos;
    }

    ///this is a dirty, dirty hack to smooth the knee positions first time around
    for(int i=0; i<100; i++)
    {
        IK_foot(0, parts[bodypart::LFOOT].pos);
        IK_foot(1, parts[bodypart::RFOOT].pos);

        for(int i=0; i<bodypart::COUNT; i++)
        {
            old_pos[i] = parts[i].pos;
        }
    }

    weapon.set_pos({0, -200, -100});

    IK_hand(0, weapon.pos);
    IK_hand(1, weapon.pos);

    focus_pos = weapon.pos;

    pos = {0,0,0};
    rot = {0,0,0};
}

void fighter::scale()
{
    for(size_t i=0; i<bodypart::COUNT; i++)
        parts[i].model.scale(bodypart::scale/3.f);

    weapon.scale();
}

///s2 and s3 define the shoulder -> elbow, and elbow -> hand length
///need to handle double cos problem
float get_joint_angle(vec3f end_pos, vec3f start_pos, float s2, float s3)
{
    float s1 = (end_pos - start_pos).length();

    s1 = clamp(s1, 0.f, s2 + s3);

    float angle = acos ( (s2 * s2 + s3 * s3 - s1 * s1) / (2 * s2 * s3) );

    //printf("%f\n", angle);

    //if(angle >= M_PI/2.f)
    //    angle = M_PI/2.f - angle;

    //printf("%f\n", angle);


    return angle;
}

float get_joint_angle_foot(vec3f end_pos, vec3f start_pos, float s2, float s3)
{
    float s1 = (end_pos - start_pos).length();

    s1 = clamp(s1, 0.f, s2 + s3);

    float angle = acos ( (s2 * s2 + s3 * s3 - s1 * s1) / (2 * s2 * s3) );

    //printf("%f\n", angle);

    //if(angle >= M_PI/2.f)
    //    angle = M_PI/2.f - angle;


    return angle;
}

///p1 shoulder, p2 elbow, p3 hand
void inverse_kinematic(vec3f pos, vec3f p1, vec3f p2, vec3f p3, vec3f& o_p1, vec3f& o_p2, vec3f& o_p3)
{
    float s1 = (p3 - p1).length();
    float s2 = (p2 - p1).length();
    float s3 = (p3 - p2).length();

    float joint_angle = get_joint_angle(pos, p1, s2, s3);

    //o_p1 = p1;

    float max_len = (p3 - p1).length();

    float to_target = (pos - p1).length();

    float len = std::min(max_len, to_target);

    vec3f dir = (pos - p1).norm();

    o_p3 = p1 + dir * len;

    float area = 0.5f * s2 * s3 * sin(joint_angle);

    ///height of scalene triangle is 2 * area / base

    float height = 2 * area / s1;

    ///ah just fuck it
    ///we need to fekin work this out properly
    vec3f halfway = (o_p3 + p1) / 2.f;

    halfway.v[1] -= height;

    vec3f halfway_dir = (halfway - p1).norm();

    o_p2 = p1 + halfway_dir * s2;

    const float shoulder_move_amount = s2/5.f;

    o_p1 = p1 + halfway_dir * shoulder_move_amount;
}

///p1 shoulder, p2 elbow, p3 hand
void inverse_kinematic_foot(vec3f pos, vec3f p1, vec3f p2, vec3f p3, vec3f& o_p1, vec3f& o_p2, vec3f& o_p3)
{
    float s1 = (p3 - p1).length();
    float s2 = (p2 - p1).length();
    float s3 = (p3 - p2).length();

    float joint_angle = M_PI + get_joint_angle_foot(pos, p1, s2, s3);

    //o_p1 = p1;

    float max_len = (p3 - p1).length();

    float to_target = (pos - p1).length();

    float len = std::min(max_len, to_target);

    vec3f dir = (pos - p1).norm();

    //o_p3 = p1 + dir * len;
    o_p3 = pos;


    float area = 0.5f * s2 * s3 * sin(joint_angle);

    ///height of scalene triangle is 2 * area / base

    float height = 2 * area / s1;

    vec3f d1 = (o_p3 - p1);
    vec3f d2 = {1, 0, 0};

    vec3f d3 = cross(d1, d2);

    d3 = d3.norm();

    vec3f half = (p1 + o_p3)/2.f;

    o_p2 = half + height * d3;

    vec3f d = (o_p3 - p1).norm();

    const float leg_move_amount = 10.f;

    o_p1 = p1 + d.norm() * (vec3f){20.f, 4.f, 20.f};

}

void fighter::IK_hand(int which_hand, vec3f pos)
{
    using namespace bodypart;

    auto upper = which_hand ? RUPPERARM : LUPPERARM;
    auto lower = which_hand ? RLOWERARM : LLOWERARM;
    auto hand = which_hand ? RHAND : LHAND;

    //printf("%f\n", pos.v[0]);

    vec3f o1, o2, o3;

    inverse_kinematic(pos, rest_positions[upper], rest_positions[lower], rest_positions[hand], o1, o2, o3);

    //printf("%f\n", o2.v[2]);

    parts[upper].set_pos(o1);
    parts[lower].set_pos(o2);
    parts[hand].set_pos(o3);
}


void fighter::IK_foot(int which_foot, vec3f pos)
{
    using namespace bodypart;

    auto upper = which_foot ? RUPPERLEG : LUPPERLEG;
    auto lower = which_foot ? RLOWERLEG : LLOWERLEG;
    auto hand = which_foot ? RFOOT : LFOOT;

    //printf("%f\n", pos.v[0]);

    vec3f o1, o2, o3;

    inverse_kinematic_foot(pos, rest_positions[upper], rest_positions[lower], rest_positions[hand], o1, o2, o3);

    //printf("%f\n", o2.v[2]);

    parts[upper].set_pos(o1);
    parts[lower].set_pos((o2 + old_pos[lower]*10)/11.f);
    parts[hand].set_pos(o3);
}

void fighter::linear_move(int hand, vec3f pos, float time, bodypart_t b)
{
    movement m;

    m.load(hand, pos, time, 0, b, mov::MOVES);

    moves.push_back(m);
}

void fighter::spherical_move(int hand, vec3f pos, float time, bodypart_t b)
{
    movement m;

    m.load(hand, pos, time, 1, b, mov::MOVES);

    moves.push_back(m);
}

///we want the hands to be slightly offset on the sword
void fighter::tick()
{
    using namespace bodypart;

    std::vector<bodypart_t> busy_list;

    for(auto& i : moves)
    {
        if(std::find(busy_list.begin(), busy_list.end(), i.limb) != busy_list.end())
        {
            continue;
        }

        action_map[i.limb] = i;

        if(!i.going)
        {
            i.fire();

            i.start = parts[i.limb].pos;
        }

        busy_list.push_back(i.limb);

        float frac = i.get_frac();

        frac = clamp(frac, 0.f, 1.f);

        vec3f current_pos;

        ///need to use a bitfield really, thisll get unmanageable
        if(i.type == 0)
        {
            ///apply a bit of smoothing
            frac = - frac * (frac - 2);
            current_pos = mix(i.start, i.fin, frac);
        }
        else if(i.type == 1)
        {
            ///need to define this manually to confine it to one axis, slerp is not what i want
            frac = - frac * (frac - 2);
            current_pos = slerp(i.start, i.fin, frac);
        }
        else
        {
            current_pos = slerp(i.start, i.fin, frac);
        }

        if(i.limb == LHAND || i.limb == RHAND)
        {
            IK_hand(i.hand, current_pos);
            IK_hand((i.hand + 1) % 2, parts[i.limb].pos); ///for the moment we just bruteforce IK both hands

            ///if the sword hits something, not again until the next move
            ///make me a function?
            if(i.hit_id == -1 && i.does(mov::DAMAGING))
            {
                ///returns -1 on miss
                i.hit_id = phys->sword_collides(weapon, this);

                ///if hit, need to signal the other fighter that its been hit with its hit id, relative to part num
                if(i.hit_id != -1)
                {
                    fighter* their_parent = phys->bodies[i.hit_id].parent;

                    their_parent->damage((bodypart_t)(i.hit_id % COUNT), 0.4f);

                    printf("%s\n", names[i.hit_id % COUNT].c_str());

                }
            }
        }

        if(i.limb == LFOOT || i.limb == RFOOT)
        {
            IK_foot(i.hand, current_pos);

            if(i.does(mov::MOVES))
            {
                ///bye bye physics :[
                vec3f diff = parts[i.limb].pos - old_pos[i.limb];

                diff = diff.rot({0,0,0}, rot);

                pos.v[0] -= diff.v[0];
                pos.v[2] -= diff.v[2];

                /*vec2f real_dir = move_dir.rot(-rot.v[1] + M_PI/2);

                pos.v[0] -= real_dir.v[0];
                pos.v[2] -= real_dir.v[1];*/

            }
        }

    }

    for(auto it = moves.begin(); it != moves.end();)
    {
        if(it->finished())
        {
            action_map.erase(it->limb);

            it = moves.erase(it);
        }
        else
            it++;
    }

    weapon.set_pos(parts[bodypart::LHAND].pos);

    update_sword_rot();

    parts[BODY].set_pos((parts[LUPPERARM].pos + parts[RUPPERARM].pos + rest_positions[BODY]*3.f)/5.f);

    //parts[BODY].set_pos((parts[BODY].pos * 20 + parts[RUPPERLEG].pos + parts[LUPPERLEG].pos)/(20 + 2));

    parts[HEAD].set_pos((parts[BODY].pos*2 + rest_positions[HEAD] * 32.f) / (32 + 2));



    /*int collide_id = phys->sword_collides(weapon);

    if(collide_id != -1)
        printf("%s %i\n", bodypart::names[collide_id % bodypart::COUNT].c_str(), collide_id);*/
}

#if 0
void fighter::walk(int which)
{
    using namespace bodypart;

    float total_time = 1000.f;

    static sf::Clock clk;

    float frac = total_time / clk.getElapsedTime().asMilliseconds();

    auto foot = which ? RFOOT : LFOOT;

    std::vector<vec3f> positions =
    {
        {rest_positions[foot].v[0], rest_positions[foot].v[1], rest_positions[foot].v[2] + 100},
        {rest_positions[foot].v[0], rest_positions[foot].v[1] + 50, rest_positions[foot].v[2] + 100},
        {rest_positions[foot].v[0], rest_positions[foot].v[1] + 50, rest_positions[foot].v[2] - 100},
        {rest_positions[foot].v[0], rest_positions[foot].v[1], rest_positions[foot].v[2] - 100}
    };

    float air_time = 400.f;
    float short_time = 200.f;

    movement m;
    m.load(which, positions[0], short_time, 1, foot);
    moves.push_back(m);

    m.load(which, positions[1], air_time, 1, foot);
    moves.push_back(m);

    m.load(which, positions[2], short_time, 1, foot);
    moves.push_back(m);

    m.load(which, positions[3], air_time, 1, foot);
    moves.push_back(m);


    if(clk.getElapsedTime().asMilliseconds() > total_time)
        clk.restart();
}
#endif

int modulo_distance(int a, int b, int m)
{
    return std::min(abs(b - a), abs(m - b + a));
}

///gunna need to pass in max len here or will break if foot tries to move outside range
/*std::vector<movement> mov(vec3f current_pos, vec3f start, vec3f fin, float time, int side, bodypart_t b)
{
    float up_dist = 50.f;

    float stroke_time = 200.f;
    float lift_time = 200.f;

    float dist = (start - current_pos).length();

    const float tol = 40.f;

    if(dist < tol)
    {
        movement m;
        m.load(side, fin, time, 1, b);

        return {m};
    }

    std::vector<movement> ret;

    vec3f up = {0, up_dist, 0};

    movement m;

    m.load(side, current_pos + up, lift_time, 1, b);
    ret.push_back(m);

    m.load(side, start + up, stroke_time, 1, b);
    ret.push_back(m);

    m.load(side, start, lift_time, 1, b);
    ret.push_back(m);

    return ret;
}*/

///skip a stride if its blatantly really much further than step, will fix side to side


///this function is a piece of crap and it knows it
bool fighter::skip_stride(vec3f dest, vec3f current, bodypart_t high, bodypart_t low)
{
    vec3f hip = parts[high].pos;
    vec3f foot = parts[low].pos;

    vec3f def_hip = rest_positions[high];
    vec3f def_foot = rest_positions[low];

    float ndist = (def_hip - def_foot).length();

    float req_dist = (def_hip - dest).length();

    if(req_dist >= ndist * 1.2f)
        return true;

    if((current - dest).length() < 45.f)
        return true;

    return false;
}

int fighter::process_foot(bodypart_t foot, int stage, vec2f dir, float d, std::vector<vec3f> positions, float time_wasted, bool can_skip, movement_t extra_tags)
{
    float front_dist = -100.f;

    float up_dist = 50.f;

    float stroke_time = 350.f;
    float lift_time = 100.f;

    float sidestep_dist = 40.f;

    vec2f sidestep = {0.f, sidestep_dist};

    vec3f side = {sidestep.v[0], 0.f, sidestep.v[1]};

    float skip_dist = 81.f;

    if(!can_skip)
        skip_dist = -1.f;

    vec3f up = {0, up_dist, 0};

    int foot_side = foot == bodypart::LFOOT ? 0 : 1;

    movement m;

    if(stage == 0)
    {
        vec3f dest = positions[0] + rest_positions[foot];

        if(dir.v[0] == 0)
            dest = dest + d*side;

        float to_dest = (parts[foot].pos - dest).length();

        if(to_dest > skip_dist)
        {
            m.load(foot_side, dest, stroke_time, 2, foot, mov::NONE);
            m.set(extra_tags);
            m.set(mov::MOVES);

            m.end_time -= time_wasted;
            m.end_time = std::max(m.end_time, 10.f);

            moves.push_back(m);

            return moves.back().id;
        }
    }
    if(stage == 1)
    {
        vec3f dest = positions[1] + rest_positions[foot];

        if(dir.v[0] == 0)
            dest = dest + d*side;

        float to_dest = (parts[foot].pos - dest).length();

        if(to_dest > skip_dist)
        {
            m.load(foot_side, parts[foot].pos + up, lift_time, 1, foot, mov::NONE);
            m.set(extra_tags);
            moves.push_back(m);

            m.load(foot_side, dest, stroke_time - lift_time, 1, foot, mov::NONE);
            m.set(extra_tags);
            m.end_time -= time_wasted;
            m.end_time = std::max(m.end_time, 10.f);
            moves.push_back(m);

            return moves.back().id;
        }
    }
}


void fighter::process_foot_g2(bodypart_t foot, vec2f dir, int& stage, float& frac, vec3f seek, vec3f prev, float seek_time, float elapsed_time)
{
    int which_foot = foot == bodypart::RFOOT ? 1 : 0;

    vec3f cur = parts[foot].pos;

    float distance = (seek - prev).length();

    float remaining = (seek - cur).length();

    float speed = elapsed_time * distance / seek_time;

    if(remaining < speed)
    {
        speed = remaining;
    }

    //printf("%f\n", distance);

    vec3f d = (seek - cur).norm();

    IK_foot(which_foot, cur + speed * d);

    //float acceptable_dist = 20.f;

    //float len = (seek - cur).length();

    //printf("%i %f %f\n", stage, frac, len);

    if(frac >= 1)//  && len < acceptable_dist)
    {
        frac = 0;
        stage = (stage + 1) % 3;

        //printf("fin\n");
    }
}

vec3f seek(vec3f cur, vec3f dest, float dist, float seek_time, float elapsed_time)
{
    float speed = elapsed_time * dist / seek_time;

    vec3f dir = (dest - cur).norm();

    float remaining = (cur - dest).length();

    if(remaining < speed)
        return dest;

    return cur + speed * dir;
}

///do I want to do a proper dynamic timing synchronisation thing?
void fighter::walk_dir(vec2f dir)
{
    #if 0
    using namespace bodypart;

    static float old_time;
    static sf::Clock clk;

    static float idle_time;

    float time = clk.getElapsedTime().asMicroseconds() / 1000.f;

    time -= idle_time;

    float dt = time - old_time;
    old_time = time;


    if(dir.v[0] == 0 && dir.v[1] == 0)
    {
        idle_time += dt;
        return;
    }

    float dist = 100.f;
    float up = 50.f;

    vec2f f = {0, dist};
    f = f.rot(dir.angle());

    float stroke_time = 400.f;
    float up_time = 100.f;

    std::vector<vec3f> leg_positions
    {
        -(vec3f){f.v[0], 0.f, f.v[1]},
        -(vec3f){f.v[0], up, f.v[1]}, ///actually wants to just be current position
        (vec3f){f.v[0], 0.f, f.v[1]}
    };

    std::vector<int> stage_times
    {
        stroke_time,
        up_time,
        stroke_time - up_time
    };

    static std::map<bodypart_t, float> end_time;
    static std::map<bodypart_t, float> travel_dist;
    static std::map<bodypart_t, vec3f> internal_positions = {{LFOOT, bodypart::default_position[LFOOT]}, {RFOOT, bodypart::default_position[RFOOT]}}; ///their positions as found by me!
    static std::map<bodypart_t, int> stages = {{LFOOT, 0}, {RFOOT, 2}};
    static std::map<bodypart_t, vec3f> end_pos;
    static std::map<bodypart_t, bool> going;

    bodypart_t bs[2] = {bodypart::LFOOT, bodypart::RFOOT};


    if(stages[LFOOT] == 0 || stages[RFOOT] == 0)
    {
        vec2f ldir = dir.norm();

        ldir.v[1] = -ldir.v[1];

        pos.v[0] += ldir.rot(- rot.v[1]).v[1];
        pos.v[2] += ldir.rot(- rot.v[1]).v[0];
    }


    int num = leg_positions.size();

    for(auto& foot : bs)
    {
        //if(time < end_time[foot])
        if(going[foot])
            continue;

        going[foot] = true;

        ///we can fire another action off now
        int stage = (stages[foot] + 1) % num;

        vec3f goal = leg_positions[stage] + rest_positions[foot];

        if(stage == 1)
            goal = parts[foot].pos + (vec3f){0, up, 0};

        float dist = (goal - internal_positions[foot]).length();

        float fin_time = time + stage_times[stage];

        stages[foot] = stage;
        travel_dist[foot] = dist;
        end_time[foot] = fin_time;
        end_pos[foot] = goal;
    }

    for(auto& foot : bs)
    {
        vec3f ret = seek(internal_positions[foot], end_pos[foot], travel_dist[foot], stage_times[stages[foot]], dt);

        internal_positions[foot] = ret;

        int which_foot = foot == RFOOT ? 1 : 0;

        IK_foot(which_foot, ret);

        float internal_dist = (ret - end_pos[foot]).length();

        if(internal_dist < 1.f)
           going[foot] = false;
        //    end_time[foot] = 0;

        ///regulator
        if(foot == RFOOT && stages[RFOOT] == 2 && stages[LFOOT] == 0 && end_time[LFOOT] != 0)
        {
            //end_time[foot] = end_time[LFOOT] + stage_times[2];
        }

        /*if(foot == RFOOT && (((stages[LFOOT] + 2) % num) != stages[RFOOT]) ||  && end_time[LFOOT] != 0)
        {
            end_time[foot] = 0;
        }*/

        ///something gone wrong, fix
        if(foot == RFOOT && stages[RFOOT] == stages[LFOOT] && end_time[LFOOT] != 0)
        {
            //end_time[foot] = 0;
            going[foot] = false;
        }
    }

    //printf("%i %i\n", stages[LFOOT], stages[RFOOT]);


    ///do seeking behaviour here
}
    #endif
    ///try and fix the lex stiffening up a bit, but who cares
    #if 1
    static sf::Clock clk;

    if(dir.v[0] == 0 && dir.v[1] == 0)
    {
        clk.restart();
        return;
    }

    ///in ms
    float time_elapsed = clk.getElapsedTime().asMicroseconds() / 1000.f;

    ///prevent feet going out of sync if there's a pause
    time_elapsed = clamp(time_elapsed, 0.f, 67.f);

    float dist = 100.f;

    vec2f f = {0, dist};
    f = f.rot(dir.angle());

    float up = 50.f;


    float stroke_time = 400.f;
    float up_time = 100.f;

    std::vector<vec3f> leg_positions
    {
        -(vec3f){f.v[0], 0.f, f.v[1]},
        -(vec3f){f.v[0], up, f.v[1]}, ///actually wants to just be current position
        (vec3f){f.v[0], 0.f, f.v[1]}
    };

    std::vector<int> stage_times
    {
        stroke_time,
        up_time,
        stroke_time - up_time
    };

    int num = 3;

    if(left_stage == 0 || right_stage == 0)
    {
        vec2f ldir = dir.norm();

        ldir.v[1] = -ldir.v[1];

        pos.v[0] += ldir.rot(- rot.v[1]).v[1];
        pos.v[2] += ldir.rot(- rot.v[1]).v[0];
    }

    static std::map<bodypart_t, vec3f> up_pos;

    int prev_stage = left_stage;

    //if(left_stage == 0)
    {
        int& stage = left_stage;
        float& frac = left_frac;

        auto foot = bodypart::LFOOT;

        vec3f seek = leg_positions[stage] + rest_positions[foot];

        if(stage == 1)
            seek = up_pos[foot];

        vec3f prev = leg_positions[(stage + num - 1) % num] + rest_positions[foot];

        process_foot_g2(foot, dir, stage, frac, seek, prev, stage_times[stage], time_elapsed);

        if(stage == 1 && frac == 0)
            up_pos[foot] = parts[foot].pos + (vec3f){0, up, 0.f};
    }

    {
        int& stage = right_stage;
        float& frac = right_frac;

        auto foot = bodypart::RFOOT;

        vec3f seek = leg_positions[stage] + rest_positions[foot];

        if(stage == 1)
            seek = up_pos[foot];

        vec3f prev = leg_positions[(stage + num - 1) % num] + rest_positions[foot];

        float time = stage_times[stage];

        process_foot_g2(foot, dir, stage, frac, seek, prev, time, time_elapsed);

        if(stage == 1 && frac == 0)
            up_pos[foot] = parts[foot].pos + (vec3f){0, up, 0.f};
    }

    /*bool already_done = false;

    {
        int& stage = right_stage;
        float& frac = right_frac;

        auto foot = bodypart::RFOOT;

        vec3f seek = leg_positions[stage] + rest_positions[foot];

        vec3f cur = parts[foot].pos;

        float len = (seek - cur).length();

        if(frac > 0.8 && len < 1.f)
        {
            left_stage = (left_stage + 1) % num;
            left_frac = 0;

            right_stage = (right_stage + 1) % num;
            right_frac = 0;
        }
    }*/



    float l_r = 1 - left_frac;
    l_r *= stage_times[left_stage];

    float r_r = 1 - right_frac;
    r_r *= stage_times[right_stage];

    float max_time = std::min(l_r, r_r) - time_elapsed;
    max_time = std::max(max_time, 0.f);

    max_time *= 0.1f;

    float l_a = max_time / stage_times[left_stage];
    float r_a = max_time / stage_times[right_stage];


    if(left_stage == 0)
    {
        float acceptable_dist = 0.f;

        int& stage = left_stage;
        float& frac = left_frac;

        auto foot = bodypart::LFOOT;

        vec3f seek = leg_positions[stage] + rest_positions[foot];

        vec3f cur = parts[foot].pos;

        float len = (seek - cur).length();

        if(len <= acceptable_dist)
        {
            //left_stage = (left_stage + 1) % num;
            //left_frac = 1;

            //right_stage = (right_stage + 1) % num;
            //right_frac = 1;

            left_frac += l_a;
            right_frac += r_a;
        }
    }

    if(right_stage == 0)
    {
        float acceptable_dist = 0.f;

        int& stage = right_stage;
        float& frac = right_frac;

        auto foot = bodypart::RFOOT;

        vec3f seek = leg_positions[stage] + rest_positions[foot];

        vec3f cur = parts[foot].pos;

        float len = (seek - cur).length();

        if(len <= acceptable_dist)
        {
            //left_stage = (left_stage + 1) % num;
            //left_frac = 1;

            //right_stage = (right_stage + 1) % num;
            //right_frac = 1;

            left_frac += l_a;
            right_frac += r_a;
        }
    }


    left_frac += (time_elapsed) / stage_times[left_stage];
    right_frac += (time_elapsed) / stage_times[right_stage];


    /*if(left_frac >= 1)
    {
        left_frac = 0;
        left_stage = (left_stage + 1) % num;
    }*/

    /*if(right_frac >= 1)
    {
        right_frac = 0;
        right_stage = (right_stage + 1) % num;
    }*/

    clk.restart();
}
#endif

#if 0

///gen 1 foot tech
    move_dir = dir;

    using namespace bodypart;

    const float idle_time = 5000.f;

    bool currently_idle = false;

    if(idle.getElapsedTime().asMilliseconds() > idle_time)
    {
        currently_idle = true;
    }

    if((dir.v[0] != 0 || dir.v[1] != 0) && idling)
    {
        if(left_fired)
            left_stage = (left_stage + 1) % 2;
        if(right_fired)
            right_stage = (right_stage + 1) % 2;

        if(left_fired && right_fired)
        {
            if(dir.v[0] > 0)
            {
                left_stage = 0;
                right_stage = 1;
            }
            else
            {
                left_stage = 1;
                right_stage = 0;
            }
        }
    }

    if(dir.v[0] != 0 || dir.v[1] != 0)
    {
        left_fired = false;
        right_fired = false;

        idle_fired_first = -1;

        idling = false;

        currently_idle = false;

        idle.restart();
    }

    if((!currently_idle || idling) && dir.v[0] == 0 && dir.v[1] == 0)
        return;

    if(!currently_idle)
        idle.restart();

    float front_dist = -100.f;

    float up_dist = 50.f;

    float stroke_time = 400.f;
    float lift_time = 100.f;

    float sidestep_dist = 40.f;

    vec2f sidestep = {0.f, sidestep_dist};
    //sidestep = sidestep.rot(dir.angle());

    vec3f side = {sidestep.v[0], 0.f, sidestep.v[1]};

    vec2f forwards = {0, -front_dist};
    forwards = forwards.rot(dir.angle());

    vec3f along = {forwards.v[0], 0, forwards.v[1]};
    vec3f behind = -along;

    //vec3f behind_up = behind + (vec3f){0, up_dist, 0};

    vec3f up = {0, up_dist, 0};


    std::vector<vec3f> positions =
    {
        behind,
        along
    };


    std::map<bodypart_t, bool> busy;


    movement* m1, *m2;

    m1 = get_movement(left_id);
    m2 = get_movement(right_id);

    std::map<bodypart_t, float> time_wasted;

    std::map<bodypart_t, bool> skippable;

    if(m1 && !currently_idle)
    {
        if(m1->does(mov::CAN_STOP))
        {
            time_wasted[m1->limb] = m1->clk.getElapsedTime().asMilliseconds();

            skippable[m1->limb] = true;

            cancel(m1->limb);
        }
    }

    if(m2 && !currently_idle)
    {
        if(m2->does(mov::CAN_STOP))
        {
            time_wasted[m2->limb] = m2->clk.getElapsedTime().asMilliseconds();

            skippable[m2->limb] = true;

            cancel(m2->limb);
        }
    }


    busy[LFOOT] = get_movement(left_id) != nullptr;
    busy[RFOOT] = get_movement(right_id) != nullptr;


    float skip_dist = 81.f;

    int num = 2;

    if(!currently_idle && !idling && !busy[LFOOT] && !busy[RFOOT])
    {
        auto foot = LFOOT;

        left_id = process_foot(LFOOT, left_stage, dir, -1, positions);

        foot = RFOOT;

        movement m;

        if(left_stage != right_stage)
        {
            right_id = process_foot(RFOOT, right_stage, dir, 1, positions);

            right_stage = (right_stage + 1) % num;
        }

        if(!busy[LFOOT])
            left_stage = (left_stage + 1) % num;
    }

    std::vector<vec3f> idle_pos_l =
    {
        {0,0,0}, ///dont care about this one
        along/2.f
    };

    std::vector<vec3f> idle_pos_r =
    {
        {0,0,0}, ///dont care about this one
        -along/2.f
    };

    if(currently_idle && !idling && get_movement(left_id) == nullptr && get_movement(right_id) == nullptr)
    {
        if(left_stage != right_stage)
        {
            left_id = process_foot(LFOOT, 1, {1, 1}, -1, idle_pos_l, 200, false, mov::CAN_STOP);
        }

        if(right_stage == left_stage)
        {
            right_id = process_foot(RFOOT, 1, {1, 1}, -1, idle_pos_r, 200, false, mov::CAN_STOP);
        }

        if(left_stage != right_stage)
        {
            if(!(idle_fired_first == 1))
                idle_fired_first = 0;

            left_fired = true;

            left_stage = (left_stage + 1) % num;
        }
        else
        {
            if(!(idle_fired_first == 0))
                idle_fired_first = 1;

            right_fired = true;

            right_stage = (right_stage + 1) % num;
        }
    }

    bool c1 = idle_fired_first == 0 && get_movement(right_id) != nullptr;
    bool c2 = idle_fired_first == 1 && get_movement(left_id) != nullptr;

    if(currently_idle && (c1 || c2))
    {
        idling = true;
    }
}
#endif

#if 0
///gen 0 foot tech
void fighter::walk_dir(vec2f dir)
{
    using namespace bodypart;

    static sf::Clock clk;

    float front_dist = 100.f;
    float back_dist = -100.f;

    float up_dist = 50.f;

    float stroke_time = 400.f;
    float lift_time = 100.f;

    vec2f forwards = {0, front_dist};
    forwards = forwards.rot(dir.angle());

    vec2f backwards = -forwards;

    std::vector<vec3f> l_pos =
    {
        {rest_positions[LFOOT].v[0], rest_positions[LFOOT].v[1], rest_positions[LFOOT].v[2]},
        {rest_positions[LFOOT].v[0], rest_positions[LFOOT].v[1] + up_dist, rest_positions[LFOOT].v[2]},
        {rest_positions[LFOOT].v[0], rest_positions[LFOOT].v[1] + up_dist, rest_positions[LFOOT].v[2]},
        {rest_positions[LFOOT].v[0], rest_positions[LFOOT].v[1], rest_positions[LFOOT].v[2]}
    };

    std::vector<vec3f> r_pos =
    {
        {rest_positions[RFOOT].v[0], rest_positions[RFOOT].v[1], rest_positions[RFOOT].v[2]},
        {rest_positions[RFOOT].v[0], rest_positions[RFOOT].v[1] + up_dist, rest_positions[RFOOT].v[2]},
        {rest_positions[RFOOT].v[0], rest_positions[RFOOT].v[1] + up_dist, rest_positions[RFOOT].v[2]},
        {rest_positions[RFOOT].v[0], rest_positions[RFOOT].v[1], rest_positions[RFOOT].v[2]}
    };

    int num = l_pos.size();

    std::vector<int> times
    {
        stroke_time,
        lift_time,
        stroke_time - lift_time*2, ///not *2, is not accurate, just shorter
        lift_time
    };

    l_pos[0].v[0] += forwards.v[0];
    l_pos[0].v[2] += forwards.v[1];

    l_pos[1].v[0] += forwards.v[0];
    l_pos[1].v[2] += forwards.v[1];

    l_pos[2].v[0] += backwards.v[0];
    l_pos[2].v[2] += backwards.v[1];

    l_pos[3].v[0] += backwards.v[0];
    l_pos[3].v[2] += backwards.v[1];


    r_pos[0].v[0] += forwards.v[0];
    r_pos[0].v[2] += forwards.v[1];

    r_pos[1].v[0] += forwards.v[0];
    r_pos[1].v[2] += forwards.v[1];

    r_pos[2].v[0] += backwards.v[0];
    r_pos[2].v[2] += backwards.v[1];

    r_pos[3].v[0] += backwards.v[0];
    r_pos[3].v[2] += backwards.v[1];

    ///foot stages 0 1 2 3 as defined by the positions vector

    ///feet want to be 2 stages out
    ///lag of two from left foot, then right starts
    ///aah! we're not moving, do something!

    ///need to go idle
    if(get_movement(left_id) == nullptr && get_movement(right_id) == nullptr)
    {
        int d = modulo_distance(left_stage, right_stage, num);

        bool left = get_movement(left_id) == nullptr;
        bool right = get_movement(right_id) == nullptr;

        //printf("%i %i\n", left_stage, right_stage);

        if(skip_stride(l_pos[left_stage], parts[LFOOT].pos, LUPPERLEG, LFOOT))
        {
            left_stage = (left_stage + 1) % num;
        }
        else
        if(left)
        {
            movement m;
            m.load(0, l_pos[left_stage], times[left_stage], 1, LFOOT, mov::NONE);

            if(left_stage == 0)
                m.set(mov::MOVES);

            moves.push_back(m);

            left_stage = (left_stage + 1) % num;

            left_id = moves.back().id;
        }

        if(skip_stride(r_pos[right_stage], parts[RFOOT].pos, RUPPERLEG, RFOOT))
        {
            right_stage = (right_stage + 1) % num;
        }
        else
        if(right && d == 2)
        {
            movement m;
            m.load(1, r_pos[right_stage], times[right_stage], 1, RFOOT, mov::NONE);

            if(right_stage == 0)
                m.set(mov::MOVES);

            moves.push_back(m);

            right_stage = (right_stage + 1) % num;

            right_id = moves.back().id;
        }


        //right = right && left_full;

        /*if(left)
        {
            if(times[left_stage] != lift_time)
            {
                auto vec = mov(parts[LFOOT].pos, l_pos[(left_stage + 3) % 4], l_pos[left_stage], times[left_stage], 0, LFOOT);

                moves.insert(moves.end(), vec.begin(), vec.end());

                if(vec.size() == 1)
                {
                    left_stage = (left_stage + 1) % 4;

                    left_full = true;
                }
                else
                {
                    left_full = false;
                }
            }
            else
            {
                movement m;
                m.load(0, l_pos[left_stage], times[left_stage], 1, LFOOT);

                moves.push_back(m);

                left_stage = (left_stage + 1) % 4;

                left_full = true;
            }


            left_id = moves.back().id;
        }

        printf("%i\n", d);


        if(d == 2 && right)
        {

            if(times[right_stage] != lift_time)
            {
                auto vec = mov(parts[RFOOT].pos, r_pos[(right_stage + 3) % 4], r_pos[right_stage], times[right_stage], 1, RFOOT);

                moves.insert(moves.end(), vec.begin(), vec.end());

                if(vec.size() == 1)
                {
                    right_stage = (right_stage + 1) % 4;
                }
            }
            else
            {
                movement m;
                m.load(1, r_pos[right_stage], times[right_stage], 1, RFOOT);

                moves.push_back(m);

                right_stage = (right_stage + 1) % 4;
            }


            right_id = moves.back().id;

        }*/
    }

}
#endif

void fighter::set_stance(int _stance)
{
    stance = _stance;
}

bool fighter::can_attack(bodypart_t type)
{
    ///find the last attack of the current type
    ///if its going, and within x ms of finishing, then fine

    int last_pos = -1;

    for(int i=0; i<moves.size(); i++)
    {
        if(moves[i].limb == type)
            last_pos = i;
    }

    if(last_pos == -1)
        return true;

    ///final move of this type not executed, probably cant attack
    ///in the future we need to sum all the times, etc. Ie do it properly
    if(!moves[last_pos].going)
        return false;

    float time_left = moves[last_pos].time_remaining();


    const float threshold = 500;

    if(time_left < threshold)
    {
        return true;
    }

    return false;
}

///this function assumes that an attack movelist keeps a consistent bodypart
void fighter::queue_attack(attack_t type)
{
    attack a = attack_list[type];

    if(!can_attack(a.moves.front().limb))
        return;

    for(auto& i : a.moves)
    {
        add_move(i);
    }
}

void fighter::add_move(const movement& m)
{
    moves.push_back(m);
}

void fighter::update_sword_rot()
{
    using namespace bodypart;

    if(stance == 0)
    {
        ///use elbow -> hand vec
        vec3f lvec = parts[LHAND].pos - parts[LUPPERARM].pos;
        vec3f rvec = parts[RHAND].pos - parts[RUPPERARM].pos;

        float l_weight = 1.0f;
        float r_weight = 0.0f;

        float total = l_weight + r_weight;

        vec3f avg = (lvec*l_weight + rvec*r_weight) / total;

        weapon.dir = avg.norm();


        float x = atan2(avg.v[1], avg.v[2]) - M_PI/2.f;
        float y = atan2(avg.v[2], avg.v[0]);
        float z = atan2(avg.v[1], avg.v[0]);


        //static float xy = 0.f;

        //xy += 0.01f;

        weapon.set_rot({0, y, x});

        parts[LHAND].set_rot({0, y, x});
        parts[RHAND].set_rot({0, y, x});

        /*mat3f m;

        m.from_dir(avg.norm());

        float a11, a12, a13, a21, a22, a23, a31, a32, a33;

        a11 = m.v[0][0];
        a12 = m.v[0][1];
        a13 = m.v[0][2];

        a21 = m.v[1][0];
        a22 = m.v[1][1];
        a23 = m.v[1][2];

        a31 = m.v[2][0];
        a32 = m.v[2][1];
        a33 = m.v[2][2];


        float x = atan2(a32, a33);
        float y = atan2(-a31, sqrtf(a32*a32 + a33*a33));
        float z = atan2(a21, a11);*/

        //weapon.set_rot({z, y, x});
    }
}

///flush HERE?
void fighter::set_pos(vec3f _pos)
{
    pos = _pos;
}

void fighter::set_rot(vec3f _rot)
{
    rot = _rot;
}

movement* fighter::get_movement(size_t id)
{
    for(auto& i : moves)
    {
        if(id == i.id)
            return &i;
    }

    return nullptr;
}


pos_rot to_world_space(vec3f world_pos, vec3f world_rot, vec3f local_pos, vec3f local_rot)
{
    vec3f relative_pos = local_pos.rot({0,0,0}, world_rot);

    vec3f total_rot = world_rot + local_rot;

    vec3f n_pos = relative_pos + world_pos;

    return {n_pos, total_rot};
}

///note to self, make this not full of shit
void fighter::update_render_positions()
{
    using namespace bodypart;

    std::map<int, float> foot_heights;

    ///bob OPPOSITE side of body
    float r_bob = parts[LFOOT].pos.v[1] - rest_positions[LFOOT].v[1];
    float l_bob = parts[RFOOT].pos.v[1] - rest_positions[RFOOT].v[1];

    foot_heights[0] = l_bob * 0.7 + r_bob * 0.3;
    foot_heights[1] = l_bob * 0.3 + r_bob * 0.7;
    foot_heights[2] = (foot_heights[0] + foot_heights[1]) / 2.f;

    for(part& i : parts)
    {
        vec3f t_pos = i.pos;

        t_pos.v[1] += foot_heights[which_side[i.type]] * foot_modifiers[i.type];

        auto r = to_world_space(pos, rot, t_pos, i.rot);

        i.model.set_pos({r.pos.v[0], r.pos.v[1], r.pos.v[2]});
        i.model.set_rot({r.rot.v[0], r.rot.v[1], r.rot.v[2]});
        i.model.g_flush_objects();
    }

    auto r = to_world_space(pos, rot, weapon.pos, weapon.rot);

    weapon.model.set_pos({r.pos.v[0], r.pos.v[1], r.pos.v[2]});
    weapon.model.set_rot({r.rot.v[0], r.rot.v[1], r.rot.v[2]});
    weapon.model.g_flush_objects();


    for(int i=0; i<bodypart::COUNT; i++)
    {
        old_pos[i] = parts[i].pos;
    }
}

void fighter::set_team(int _team)
{
    team = _team;

    for(auto& i : parts)
    {
        i.set_team(team);
    }

    weapon.set_team(team);
}

void fighter::set_physics(physics* _phys)
{
    phys = _phys;

    for(part& i : parts)
    {
        phys->add_objects_container(&i.model, &i, i.team, this);
    }
}

void fighter::cancel(bodypart_t type)
{
    for(auto it = moves.begin(); it != moves.end();)
    {
        if(it->limb == type)
        {
            action_map.erase(it->limb);

            it = moves.erase(it);
        }
        else
            it++;
    }
}

///i've taken damage. If im during the windup phase of an attack, recoil
void fighter::damage(bodypart_t type, float d)
{
    using namespace bodypart;

    parts[type].damage(d);

    movement lhand = action_map[LHAND];
    movement rhand = action_map[RHAND];

    if(lhand.does(mov::WINDUP) || rhand.does(mov::WINDUP))
    {
        cancel(LHAND);
        cancel(RHAND);

        queue_attack(attacks::RECOIL);
    }
}
