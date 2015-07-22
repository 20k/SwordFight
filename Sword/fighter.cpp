#include "fighter.hpp"
#include "physics.hpp"
#include "../openclrenderer/obj_mem_manager.hpp"
#include <unordered_map>
#include "../openclrenderer/network.hpp"

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

    set_hp(1.f);
}

part::part()
{
    is_active = false;

    hp = 1.f;

    set_pos({0,0,0});
    set_rot({0,0,0});

    set_global_pos({0,0,0});
    set_global_rot({0,0,0});

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

void part::set_active(bool active)
{
    model.set_active(active);

    is_active = active;
}

void part::scale(float amount)
{
    model.scale(amount);
}

objects_container* part::obj()
{
    return &model;
}

void part::set_pos(vec3f _pos)
{
    pos = _pos;
}

void part::set_rot(vec3f _rot)
{
    rot = _rot;
}

void part::set_global_pos(vec3f _global_pos)
{
    global_pos = _global_pos;
}

void part::set_global_rot(vec3f _global_rot)
{
    global_rot = _global_rot;
}

void part::update_model()
{
    model.set_pos({global_pos.v[0], global_pos.v[1], global_pos.v[2]});
    model.set_rot({global_rot.v[0], global_rot.v[1], global_rot.v[2]});

    model.g_flush_objects();
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

    model.unload();

    set_active(true);

    team = _team;
}

///a network transmission of damage will get swollowed if you are hit between the time you spawn, and the time it takes to transmit
///the hp stat to the destination. This is probably acceptable
void part::damage(float dam)
{
    hp -= dam;

    //printf("%f\n", hp);

    if(model.isactive && hp < 0.0001f)
    {
        //printf("I blowed up %s\n", bodypart::names[type].c_str());
        model.set_active(false);

        obj_mem_manager::load_active_objects();
        obj_mem_manager::g_arrange_mem();
        obj_mem_manager::g_changeover();
    }

    network_hp();
}

void part::set_hp(float h)
{
    hp = h;

    network_hp();
}

void part::network_hp()
{
    network::host_update(&hp);
}

bool part::alive()
{
    ///hp now networked, dont need to hodge podge this with model active status
    return (hp > 0);// && model.isactive;
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
    float time = clk.getElapsedTime().asMicroseconds() / 1000.f;

    return std::max(end_time - time, 0.f);
}

float movement::get_frac()
{
    float time = clk.getElapsedTime().asMicroseconds() / 1000.f;

    return (float)time / end_time;
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

    model.unload();

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

    bound = get_bbox(&model);
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

///need to only maintain 1 copy of this, I'm just a muppet
fighter::fighter()
{
    rot_diff = {0,0,0};

    look_displacement = {0,0,0};

    frametime = 0;

    my_time = 0;

    look = {0,0,0};

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

void fighter::respawn(vec2f _pos)
{
    rot_diff = {0,0,0};

    look_displacement = {0,0,0};

    frametime = 0;

    my_time = 0;

    frame_clock.restart();

    look = {0,0,0};

    net.dead = false;

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

    //rest_positions = bodypart::init_default();

    for(size_t i=0; i<bodypart::COUNT; i++)
    {
        parts[i].set_type((bodypart_t)i); ///resets hp, and networks it
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

    ///need to randomise this really
    pos = {_pos.v[0],0,_pos.v[1]};
    rot = {0,0,0};

    for(auto& i : parts)
    {
        i.set_active(true);
    }

    weapon.model.set_active(true);

    obj_mem_manager::load_active_objects();
    obj_mem_manager::g_arrange_mem();
    obj_mem_manager::g_changeover();
}

void fighter::die()
{
    net.dead = true;

    for(auto& i : parts)
    {
        i.set_active(false);
    }

    weapon.model.set_active(false);

    obj_mem_manager::load_active_objects();
    obj_mem_manager::g_arrange_mem();
    obj_mem_manager::g_changeover();
}

void fighter::scale()
{
    for(size_t i=0; i<bodypart::COUNT; i++)
        parts[i].scale(bodypart::scale/3.f);

    weapon.scale();
}

void fighter::set_look(vec3f _look)
{
    vec3f current_look = look;

    vec3f new_look = _look;

    vec3f clamps = {M_PI/8.f, M_PI/12.f, M_PI/8.f};
    new_look = clamp(new_look, -clamps, clamps);

    vec3f origin = parts[bodypart::BODY].pos;

    vec3f c2b = current_look - origin;
    vec3f n2b = new_look - origin;

    float angle_constraint = 0.004f * frametime;

    float angle = dot(c2b.norm(), n2b.norm());

    if(fabs(angle) >= angle_constraint)
    {
        new_look = mix(current_look, new_look, angle_constraint);
    }

    new_look = clamp(new_look, -clamps, clamps); /// just in case for some reason the old current_look was oob

    const float displacement = (rest_positions[bodypart::LHAND] - rest_positions[bodypart::LUPPERARM]).length();

    float height = displacement * sin(new_look.v[0]);
    float width = displacement * sin(new_look.v[1]);

    look_displacement = (vec3f){width, height, 0.f};

    look = new_look;
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

    //o1 = o1 + look_displacement;
    //o2 = o2 + look_displacement;
    //o3 = o3 + look_displacement;

    //printf("%f\n", o2.v[2]);

    //printf("%f\n", look_displacement.v[0]);

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
void fighter::tick(bool is_player)
{
    float cur_time = frame_clock.getElapsedTime().asMicroseconds() / 1000.f;

    frametime = cur_time - my_time;

    my_time = cur_time;


    using namespace bodypart;

    std::vector<bodypart_t> busy_list;

    ///will be set to true if a move is currently doing a blocking action
    net.is_blocking = 0;


    if(net.recoil)
    {
        recoil();

        net.recoil = 0;
    }

    for(movement& i : moves)
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

            if((i.limb == LHAND || i.limb == RHAND) && !i.does(mov::START_INDEPENDENT))
                i.start = focus_pos;

            if((i.limb == LHAND || i.limb == RHAND) && i.does(mov::START_INDEPENDENT))
                i.start = focus_pos;
        }

        busy_list.push_back(i.limb);

        float frac = i.get_frac();

        frac = clamp(frac, 0.f, 1.f);

        vec3f current_pos;

        vec3f actual_finish = i.fin;

        if(i.does(mov::FINISH_INDEPENDENT))
        {
            actual_finish = actual_finish - look_displacement;
        }

        ///need to use a bitfield really, thisll get unmanageable
        if(i.type == 0)
        {
            ///apply a bit of smoothing
            frac = - frac * (frac - 2);
            current_pos = mix(i.start, actual_finish, frac);
        }
        else if(i.type == 1)
        {
            ///need to define this manually to confine it to one axis, slerp is not what i want
            frac = - frac * (frac - 2);
            current_pos = slerp(i.start, actual_finish, frac);
        }
        else
        {
            current_pos = slerp(i.start, actual_finish, frac);
        }

        if(i.limb == LHAND || i.limb == RHAND)
        {
            ///focus pos is relative to player, but does NOT include look_displacement OR world anything
            vec3f old_pos = focus_pos;
            focus_pos = current_pos;

            ///losing a frame currently, FIXME
            ///if the sword hits something, not again until the next move
            ///make me a function?
            if(i.hit_id == -1 && i.does(mov::DAMAGING))
            {
                ///this is the GLOBAL move dir, current_pos could be all over the place due to interpolation, lag etc
                vec3f move_dir = (focus_pos - old_pos).norm();

                ///pass direction vector into here, then do the check
                ///returns -1 on miss
                i.hit_id = phys->sword_collides(weapon, this, move_dir, is_player);

                ///if hit, need to signal the other fighter that its been hit with its hit id, relative to part num
                if(i.hit_id != -1)
                {
                    fighter* their_parent = phys->bodies[i.hit_id].parent;

                    their_parent->damage((bodypart_t)(i.hit_id % COUNT), 0.4f);

                    //printf("%s\n", names[i.hit_id % COUNT].c_str());
                }
            }

            if(i.does(mov::BLOCKING))
            {
                net.is_blocking = 1;
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

    IK_hand(0, focus_pos + look_displacement);
    IK_hand(1, focus_pos + look_displacement);

    weapon.set_pos(parts[bodypart::LHAND].pos);

    update_sword_rot();

    parts[BODY].set_pos((parts[LUPPERARM].pos + parts[RUPPERARM].pos + rest_positions[BODY]*3.f)/5.f);

    //parts[BODY].set_pos((parts[BODY].pos * 20 + parts[RUPPERLEG].pos + parts[LUPPERLEG].pos)/(20 + 2));

    parts[HEAD].set_pos((parts[BODY].pos*2 + rest_positions[HEAD] * 32.f) / (32 + 2));


    ///process death

    ///nope. DO THIS PROPERLY
    /*///propagate network model destruction
    for(auto& p : parts)
    {
        if(p.model.isactive == false)
            p.hp = 0;
    }*/

    const int num_destroyed_to_die = 3;

    int num_destroyed = 0;

    for(auto& p : parts)
    {
        if(p.hp <= 0)
        {
            //printf("%s\n", names[p.type].c_str());

            num_destroyed++;
        }
    }

    //printf("%i\n", num_destroyed);

    if(num_destroyed >= num_destroyed_to_die && !net.dead)
        die();

    /*int collide_id = phys->sword_collides(weapon);

    if(collide_id != -1)
        printf("%s %i\n", bodypart::names[collide_id % bodypart::COUNT].c_str(), collide_id);*/
}


int modulo_distance(int a, int b, int m)
{
    return std::min(abs(b - a), abs(m - b + a));
}

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
    ///try and fix the lex stiffening up a bit, but who cares
    if(dir.v[0] == 0 && dir.v[1] == 0)
    {
        walk_clock.restart();
        return;
    }

    ///in ms
    float time_elapsed = walk_clock.getElapsedTime().asMicroseconds() / 1000.f;

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

    //if(left_stage == 0 || right_stage == 0)
    {
        vec2f ldir = dir.norm();

        ldir.v[1] = -ldir.v[1];

        pos.v[0] += ldir.rot(- rot.v[1]).v[1] * time_elapsed/2.f;
        pos.v[2] += ldir.rot(- rot.v[1]).v[0] * time_elapsed/2.f;
    }

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

    ///I no longer have a complete understanding of how this section works
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

    walk_clock.restart();
}

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
    rot_diff = _rot - rot;

    rot = _rot;
}

void fighter::set_rot_diff(vec3f diff)
{
    rot_diff = diff;
    rot = rot + diff;
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

        i.set_global_pos({r.pos.v[0], r.pos.v[1], r.pos.v[2]});
        i.set_global_rot({r.rot.v[0], r.rot.v[1], r.rot.v[2]});
        i.update_model();
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
        phys->add_objects_container(&i, this);
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

void fighter::cancel_hands()
{
    cancel(bodypart::LHAND);
    cancel(bodypart::RHAND);
}

///wont recoil more than once, because recoil is not a kind of windup
void fighter::checked_recoil()
{
    movement lhand = action_map[bodypart::LHAND];
    movement rhand = action_map[bodypart::RHAND];

    if(lhand.does(mov::WINDUP) || rhand.does(mov::WINDUP))
    {
        recoil();
    }
}

bool fighter::can_recoil()
{
    movement lhand = action_map[bodypart::LHAND];
    movement rhand = action_map[bodypart::RHAND];

    if(lhand.does(mov::WINDUP) || rhand.does(mov::WINDUP))
    {
        return true;
    }

    return false;
}

void fighter::recoil()
{
    cancel(bodypart::LHAND);
    cancel(bodypart::RHAND);
    queue_attack(attacks::RECOIL);
}

void fighter::try_feint()
{
    const float unfeintable_time = 100.f;

    movement lhand = action_map[bodypart::LHAND];
    movement rhand = action_map[bodypart::RHAND];

    bool lfeint = lhand.does(mov::WINDUP) && (lhand.time_remaining() > unfeintable_time);
    bool rfeint = rhand.does(mov::WINDUP) && (rhand.time_remaining() > unfeintable_time);

    if(lfeint || rfeint)
    {
        cancel(bodypart::LHAND);
        cancel(bodypart::RHAND);

        queue_attack(attacks::FEINT);
    }
}


///i've taken damage. If im during the windup phase of an attack, recoil
void fighter::damage(bodypart_t type, float d)
{
    using namespace bodypart;

    parts[type].damage(d);

    if(can_recoil())
    {
        net.recoil = 1;
        network::host_update(&net.recoil);

        cancel_hands();
    }
}
