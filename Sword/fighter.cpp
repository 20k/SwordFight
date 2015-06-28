#include "fighter.hpp"

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
}

part::part(bodypart_t t)
{
    model.set_file("../openclrenderer/objects/cylinder.obj");

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


fighter::fighter()
{
    for(size_t i=0; i<bodypart::COUNT; i++)
    {
        parts[i].set_type((bodypart_t)i);
    }
}

void fighter::scale()
{
    for(size_t i=0; i<bodypart::COUNT; i++)
        parts[i].model.scale(bodypart::scale/4.f);
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

    o_p1 = p1;

    float max_len = (p3 - p1).length();

    float to_target = (pos - p1).length();

    float len = std::min(max_len, to_target);

    vec3f dir = (pos - p1).norm();

    //printf("%f\n", len);

    o_p3 = p1 + dir * len;

    float area = 0.5f * s2 * s3 * sin(joint_angle);

    ///height of scalene triangle is 2 * area / base

    float height = 2 * area / s1;

    ///ah just fuck it
    vec3f halfway = (o_p3 + o_p1) / 2.f;

    halfway.v[1] -= height;

    o_p2 = halfway;
}

void fighter::IK_hand(int which_hand, vec3f pos)
{
    using namespace bodypart;

    auto upper = which_hand ? RUPPERARM : LUPPERARM;
    auto lower = which_hand ? RLOWERARM : LLOWERARM;
    auto hand = which_hand ? RHAND : LHAND;

    //printf("%f\n", pos.v[0]);

    vec3f o1, o2, o3;

    inverse_kinematic(pos, default_position[upper], default_position[lower], default_position[hand], o1, o2, o3);

    printf("%f\n", o2.v[2]);

    parts[upper].set_pos(o1);
    parts[lower].set_pos(o2);
    parts[hand].set_pos(o3);
}
