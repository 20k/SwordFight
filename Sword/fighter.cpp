#include "fighter.hpp"

const vec3f* bodypart::init_default()
{
    using namespace bodypart;

    static vec3f pos[COUNT];

    //vec3f* pos = new vec3f[COUNT];

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

    model.set_active(true);
}

part::part()
{
    model.set_file("../openclrenderer/objects/cylinder.obj");
    set_pos({0,0,0});
    set_rot({0,0,0});
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

    model.set_pos({pos.v[0], pos.v[1], pos.v[2]});

    model.g_flush_objects();
}

void part::set_rot(vec3f _rot)
{
    rot = _rot;

    model.set_rot({rot.v[0], rot.v[1], rot.v[2]});

    model.g_flush_objects();
}

void movement::load(int _hand, vec3f _end_pos, float _time, int _type)
{
    end_time = _time;
    fin = _end_pos;
    type = _type;
    hand = _hand;

    limb = hand ? bodypart::RHAND : bodypart::LHAND;
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
    end_time = 0.f;
    start = {0,0,0};
    fin = {0,0,0};
    type = 0;
    hand = 0;
    going = false;
}

movement::movement(int hand, vec3f end_pos, float time, int type) : movement()
{
    load(hand, end_pos, time, type);
}

sword::sword()
{
    model.set_file("res/sword.obj");
    model.set_pos({0, 0, -100});
    model.set_active(true);
}

void sword::scale()
{
    model.scale(50.f);
}

void sword::set_pos(vec3f _pos)
{
    pos = _pos;

    model.set_pos({pos.v[0], pos.v[1], pos.v[2]});

    model.g_flush_objects();
}

void sword::set_rot(vec3f _rot)
{
    rot = _rot;

    model.set_rot({rot.v[0], rot.v[1], rot.v[2]});

    model.g_flush_objects();
}

fighter::fighter()
{
    stance = 0;

    rest_positions = bodypart::init_default();

    for(size_t i=0; i<bodypart::COUNT; i++)
    {
        parts[i].set_type((bodypart_t)i);
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
        parts[i].model.scale(bodypart::scale/4.f);

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

void fighter::linear_move(int hand, vec3f pos, float time)
{
    movement m;

    m.load(hand, pos, time, 0);

    moves.push_back(m);
}

void fighter::spherical_move(int hand, vec3f pos, float time)
{
    movement m;

    m.load(hand, pos, time, 1);

    moves.push_back(m);
}

///we want the hands to be slightly offset on the sword
void fighter::tick()
{
    std::vector<bodypart_t> busy_list;

    for(auto& i : moves)
    {
        if(std::find(busy_list.begin(), busy_list.end(), i.limb) != busy_list.end())
        {
            continue;
        }

        if(!i.going)
        {
            i.fire();

            i.start = parts[i.limb].pos;
        }

        busy_list.push_back(i.limb);

        float frac = i.get_frac();

        frac = clamp(frac, 0.f, 1.f);

        ///apply a bit of smoothing
        frac = - frac * (frac - 2);

        vec3f current_pos;

        if(i.type == 0)
        {
            current_pos = mix(i.start, i.fin, frac);
        }
        if(i.type == 1)
        {
            ///need to define this manually to confine it to one axis, slerp is not what i want
            current_pos = slerp(i.start, i.fin, parts[bodypart::BODY].pos, frac);
        }

        IK_hand(i.hand, current_pos);
        IK_hand((i.hand + 1) % 2, parts[i.limb].pos); ///for the moment we just bruteforce IK both hands
    }

    for(auto it = moves.begin(); it != moves.end();)
    {
        if(it->finished() >= 1.f)
        {
            it = moves.erase(it);
        }
        else
            it++;
    }

    weapon.set_pos(parts[bodypart::LHAND].pos);

    update_sword_rot();

    parts[bodypart::BODY].set_pos((parts[bodypart::LUPPERARM].pos + parts[bodypart::RUPPERARM].pos + rest_positions[bodypart::BODY]*3.f)/5.f);
}

void fighter::set_stance(int _stance)
{
    stance = _stance;
}

void fighter::queue_attack(attack_t type)
{
    attack a = attack_list[type];

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

struct pos_rot
{
    vec3f pos;
    vec3f rot;
};

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
    for(part& i : parts)
    {
        auto r = to_world_space(pos, rot, i.pos, i.rot);

        i.model.set_pos({r.pos.v[0], r.pos.v[1], r.pos.v[2]});
        i.model.set_rot({r.rot.v[0], r.rot.v[1], r.rot.v[2]});
        i.model.g_flush_objects();
    }

    auto r = to_world_space(pos, rot, weapon.pos, weapon.rot);

    weapon.model.set_pos({r.pos.v[0], r.pos.v[1], r.pos.v[2]});
    weapon.model.set_rot({r.rot.v[0], r.rot.v[1], r.rot.v[2]});
    weapon.model.g_flush_objects();
}
