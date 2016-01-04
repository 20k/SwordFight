#include "fighter.hpp"
#include "physics.hpp"
#include "../openclrenderer/obj_mem_manager.hpp"
#include <unordered_map>
#include "../openclrenderer/network.hpp"

#include "object_cube.hpp"

#include "particle_effect.hpp"

#include "../openclrenderer/light.hpp"

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

part::part(object_context& context)
{
    //performed_death = false;

    cpu_context = &context;
    model = context.make_new();

    is_active = false;

    hp = 1.f;

    set_pos({0,0,0});
    set_rot({0,0,0});

    set_global_pos({0,0,0});
    set_global_rot({0,0,0});

    model->set_file("./Res/bodypart_red.obj");

    team = -1;

    quality = 0;
}

part::part(bodypart_t t, object_context& context) : part(context)
{
    set_type(t);
}

part::~part()
{
    model->set_active(false);
}

void part::set_active(bool active)
{
    model->set_active(active);

    is_active = active;
}

void part::scale()
{
    float amount = bodypart::scale/3.f;

    if(type != bodypart::HEAD)
        model->scale(amount);
    else
        model->scale(amount);
}

objects_container* part::obj()
{
    return model;
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
    model->set_pos({global_pos.v[0], global_pos.v[1], global_pos.v[2]});
    model->set_rot({global_rot.v[0], global_rot.v[1], global_rot.v[2]});
}

void part::set_team(int _team)
{
    int old = team;

    team = _team;

    if(old != team)
        load_team_model();
}

void part::load_team_model()
{
    ///this is not the place to define these
    const std::string low_red = "res/low/bodypart_red.obj";
    const std::string high_red = "res/high/bodypart_red.obj";
    const std::string low_blue = "res/low/bodypart_blue.obj";
    const std::string high_blue = "res/high/bodypart_blue.obj";

    std::string to_load = low_red;

    if(quality == 0)
    {
        if(team == 0)
            to_load = low_red;
        else
            to_load = low_blue;
    }
    else
    {
        if(team == 0)
            to_load = high_red;
        else
            to_load = high_blue;
    }

    model->set_file(to_load);

    model->set_normal("res/norm_body.png");

    model->unload();

    set_active(true);

    cpu_context->load_active();

    scale();
}

void part::set_quality(int _quality)
{
    int old_quality = quality;

    quality = _quality;

    if(quality != old_quality)
    {
        load_team_model();
    }
}

///a network transmission of damage will get swollowed if you are hit between the time you spawn, and the time it takes to transmit
///the hp stat to the destination. This is probably acceptable

///temp error as this class needs gpu access
void part::damage(float dam, bool do_effect)
{
    hp -= dam;

    //printf("%f\n", hp);

    if(is_active && hp < 0.0001f)
    {
        //printf("I blowed up %s\n", bodypart::names[type].c_str());

        if(do_effect)
        {
            cube_effect e;

            e.make(1300, global_pos, 100.f, team, 10, *cpu_context);
            particle_effect::push(e);
        }

        //set_pos({0, -1000000000, 0});
        set_active(false);

        cpu_context->load_active();
        cpu_context->build();
        //model->hide();

        //cpu_context.load_active();
    }

    network_hp();
}

/*void part::tick()
{
    if(hp < 0.0001f && !performed_death)
    {



        performed_death = true;
    }
}*/

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
    int old = team;

    team = _team;

    if(old != team)
        load_team_model();
}

void sword::load_team_model()
{
    if(team == 0)
    {
        model->set_file("./Res/sword_red.obj");
    }
    else
    {
        model->set_file("./Res/sword_blue.obj");
    }

    model->unload();

    model->set_active(true);

    //model->cache = false;

    cpu_context->load_active();
    scale();
}

sword::sword(object_context& cpu)
{
    cpu_context = &cpu;

    model = cpu.make_new();

    model->set_pos({0, 0, -100});
    dir = {0,0,0};
    model->set_file("./Res/sword_red.obj");
    team = -1;
}

void sword::scale()
{
    model->scale(50.f);
    model->set_specular(0.4f);

    bound = get_bbox(model);

    float sword_height = 0;

    for(triangle& t : model->objs[0].tri_list)
    {
        for(vertex& v : t.vertices)
        {
            vec3f pos = xyz_to_vec(v.get_pos());

            if(pos.v[1] > sword_height)
                sword_height = pos.v[1];
        }
    }

    length = sword_height;
}

void sword::set_pos(vec3f _pos)
{
    pos = _pos;
}

void sword::set_rot(vec3f _rot)
{
    rot = _rot;
}

link make_link(part* p1, part* p2, int team, float squish = 0.0f, float thickness = 18.f, vec3f offset = {0,0,0})
{
    vec3f dir = (p2->pos - p1->pos);

    std::string tex = "./res/red.png";

    ///should really define this in a header somewhere, rather than here in shit code
    if(team == 1)
        tex = "./res/blue.png";

    objects_container* o = p1->cpu_context->make_new();
    o->set_load_func(std::bind(load_object_cube, std::placeholders::_1, p1->pos + dir * squish, p2->pos - dir * squish, thickness, tex));
    o->cache = false;
    //o.set_normal("res/norm_body.png");

    link l;

    l.obj = o;

    l.p1 = p1;
    l.p2 = p2;

    l.offset = offset;

    l.squish_frac = squish;

    return l;
}

///need to only maintain 1 copy of this, I'm just a muppet
fighter::fighter(object_context& _cpu_context, object_context_data& _gpu_context) : weapon(_cpu_context), my_cape(_cpu_context, _gpu_context)
{
    cpu_context = &_cpu_context;
    gpu_context = &_gpu_context;

    for(int i=0; i<bodypart::COUNT; i++)
    {
        parts.push_back(part(_cpu_context));
    }

    quality = 0;

    light l1;

    my_lights.push_back(light::add_light(&l1));
    my_lights.push_back(light::add_light(&l1));
    my_lights.push_back(light::add_light(&l1));

    load();

    pos = {0,0,0};
    rot = {0,0,0};

    game_state = nullptr;
}

void fighter::load()
{
    performed_death = false;

    net.recoil = false;
    net.is_blocking = false;

    rot_diff = {0,0,0};

    look_displacement = {0,0,0};

    frametime = 0;

    my_time = 0;

    look = {0,0,0};

    left_frac = 0.f;
    right_frac = 0.f;

    idle_fired_first = -1;

    idling = false;

    team = -1;

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
}

void fighter::respawn(vec2f _pos)
{
    int old_team = team;

    load();

    team = old_team;

    ///need to randomise this really
    pos = {_pos.v[0],0,_pos.v[1]};
    rot = {0,0,0};

    for(auto& i : parts)
    {
        i.set_active(true);
    }

    weapon.model->set_active(true);

    for(auto& i : joint_links)
    {
        i.obj->set_active(true);
    }

    set_team(team);

    cpu_context->load_active();

    cpu_context->build();
    gpu_context = cpu_context->fetch();

    //obj_mem_manager::g_arrange_mem();
    //obj_mem_manager::g_changeover();

    //my_cape.make_stable(this);

    //network::host_update(&net.dead);
}

void fighter::die()
{
    performed_death = true;

    //net.dead = true;

    for(auto& i : parts)
    {
        i.set_active(false);
    }

    weapon.model->set_active(false);

    for(auto& i : joint_links)
    {
        i.obj->set_active(false);
    }

    ///spawn in some kind of swanky effect here

    /*particle_effect e;
    e.make(1300, parts[bodypart::BODY].global_pos, 250.f);
    e.push();
    e.make(1300, parts[bodypart::BODY].global_pos, 150.f);
    e.push();
    e.make(1300, parts[bodypart::BODY].global_pos, 100.f);
    e.push();
    e.make(1300, parts[bodypart::BODY].global_pos, 50.f);
    e.push();*/

    /*particle_effect e;
    e.make(1300, parts[bodypart::BODY].global_pos, 50.f);
    e.push();*/

    const float death_time = 2000;

    for(auto& i : parts)
    {
        cube_effect e;

        e.make(death_time, i.global_pos, 50.f, team, 10, *cpu_context);
        particle_effect::push(e);
    }

    {
        vec3f weapon_pos = xyz_to_vec(weapon.model->pos);
        vec3f weapon_rot = xyz_to_vec(weapon.model->rot);

        vec3f weapon_dir = (vec3f){0, 1, 0}.rot({0,0,0}, weapon_rot);

        float sword_height = weapon.length;

        for(float i = 0; i < sword_height; i += 40.f)
        {
            vec3f pos = weapon_pos + i * weapon_dir.norm();

            cube_effect e;

            e.make(death_time, pos, 50.f, team, 5, *cpu_context);
            particle_effect::push(e);
        }
    }

    update_lights();

    for(auto& i : my_lights)
    {
        light_effect l;
        l.make(5000.f, i);
        particle_effect::push(l);
    }

    //network::host_update(&net.dead);

    cpu_context->load_active();

    cpu_context->build();
    gpu_context = cpu_context->fetch();
}

int fighter::num_dead()
{
    int num_destroyed = 0;

    for(auto& p : parts)
    {
        if(p.hp <= 0)
        {
            //printf("%s\n", names[p.type].c_str());

            num_destroyed++;
        }
    }

    return num_destroyed;
}

///this is an awful piece of i dont even know what
int fighter::num_needed_to_die()
{
    return 3;
}

bool fighter::should_die()
{
    const int num_destroyed_to_die = num_needed_to_die();

    int num_destroyed = num_dead();

    //printf("%i\n", num_destroyed);

    if(num_destroyed >= num_destroyed_to_die && !performed_death)
        return true;

    return false;
}

void fighter::checked_death()
{
    if(fighter::should_die())
    {
        die();
    }
}

bool fighter::dead()
{
    return (num_dead() > num_needed_to_die()) || performed_death;
}

void fighter::tick_cape()
{
    if(dead())
        return;

    int ticks = 1;

    for(int i=0; i<ticks; i++)
    {
        this->my_cape.tick(this->parts[bodypart::LUPPERARM].obj(),
                               this->parts[bodypart::BODY].obj(),
                               this->parts[bodypart::RUPPERARM].obj(),
                               this
                               );
    }
}

void fighter::set_quality(int _quality)
{
    quality = _quality;

    for(auto& i : parts)
    {
        i.set_quality(quality);
    }
}

void fighter::set_gameplay_state(gameplay_state* st)
{
    game_state = st;
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

    //float len = std::min(max_len, to_target);

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

    ///set this to std::max(height, 30.f) if you want beelzebub strolling around
    o_p2 = half + std::min(height, -5.f) * d3;

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
    parts[lower].set_pos((o2 + old_pos[lower]*5)/6.f);
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
        if(can_recoil())
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

                    ///this is the only time damage is applied to anything, ever
                    their_parent->damage((bodypart_t)(i.hit_id % COUNT), 0.4f);

                    their_parent->checked_death();

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

    parts[HEAD].set_pos((parts[BODY].pos*2.f + rest_positions[HEAD] * 32.f) / (32 + 2));


    ///process death

    ///rip
    checked_death();

    /*int collide_id = phys->sword_collides(weapon);

    if(collide_id != -1)
        printf("%s %i\n", bodypart::names[collide_id % bodypart::COUNT].c_str(), collide_id);*/
}


int modulo_distance(int a, int b, int m)
{
    return std::min(abs(b - a), abs(m - b + a));
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
void fighter::walk_dir(vec2f dir, bool sprint)
{
    if(game_state == nullptr)
    {
        printf("Warning: No gameplay state for fighter\n");
    }

    ///try and fix the lex stiffening up a bit, but who cares
    ///make feet average out with the ground
    bool idle = dir.v[0] == 0 && dir.v[1] == 0;

    ///Make me a member variable?
    static vec2f valid_dir = {-1, 0};

    if(idle)
        dir = valid_dir;
    else
        valid_dir = dir;



    ///in ms
    ///replace this with a dt
    float time_elapsed = walk_clock.getElapsedTime().asMicroseconds() / 1000.f;

    float h = 120.f;

    if(dir.v[0] == -1 && sprint)
    {
        time_elapsed *= 1.3f;
        h *= 1.2f;
    }

    float dist = 125.f;

    float up = 50.f;

    int num = 3;

    vec2f ldir = dir.norm();

    ldir.v[1] = -ldir.v[1];

    vec2f rld = ldir.rot(-rot.v[1]);

    vec3f global_dir = {rld.v[1] * time_elapsed/2.f, 0.f, rld.v[0] * time_elapsed/2.f};

    ///dont move the player if we're really idling
    ///move me into a function
    ///if we're not idling, we want to actually move the player
    ///want to make the movement animation speed based on how far we actually moved
    ///out of how far we tried to move
    if(!idle)
    {
        vec3f predicted = pos + global_dir;

        vec2f lpredict = {predicted.v[0], predicted.v[2]};

        vec2f dir_move = {global_dir.v[0], global_dir.v[2]};

        vec2f lpos = {pos.v[0], pos.v[2]};

        float move_amount = dir_move.length();

        ///x move in wall
        bool xw = false;
        bool yw = false;

        if(rectangle_in_wall(lpos + (vec2f){dir_move.v[0], 0.f}, get_approx_dim(), game_state))
        {
            dir_move.v[0] = 0.f;
            xw = true;
        }
        if(rectangle_in_wall(lpos + (vec2f){0.f, dir_move.v[1]}, get_approx_dim(), game_state))
        {
            dir_move.v[1] = 0.f;
            yw = true;
        }

        ///if I move into wall, but yw and xw aren't true, stop
        ///there are some diagonal cases here which might result in funky movement
        ///but largely should be fine
        if(rectangle_in_wall(lpos + dir_move, get_approx_dim(), game_state) && !xw && !yw)
        {
            dir_move = 0.f;
        }

        ///just in case!
        if(!rectangle_in_wall(lpos + dir_move, get_approx_dim(), game_state))
        {
            float real_move = dir_move.length();

            float anim_frac = 0.f;

            if(move_amount > 0.0001f)
            {
                anim_frac = real_move / move_amount;
                time_elapsed *= anim_frac;

                ///so now global_dir is my new global move direction
                ///lets translate it back into local, and then
                ///update our move estimate by that much

                vec2f inv = {dir_move.v[1], dir_move.v[0]};

                inv = inv.rot(rot.v[1]);

                inv = inv / (time_elapsed / 2.f);

                ldir = inv;
            }

            pos = pos + (vec3f){dir_move.v[0], 0.f, dir_move.v[1]};
        }
    }

    vec3f current_dir = (vec3f){ldir.v[1], 0.f, ldir.v[0]} * time_elapsed/2.f;

    vec3f fin = (vec3f){ldir.v[1], 0.f, ldir.v[0]}.norm() * dist;

    ///1 is push, 0 is air
    static float lmod = 1.f;
    static float frac = 0.f;

    auto foot = bodypart::LFOOT;
    auto ofoot = bodypart::RFOOT;

    vec3f lrp = {parts[foot].pos.v[0], 0.f, parts[foot].pos.v[2]};
    vec3f rrp = {parts[ofoot].pos.v[0], 0.f, parts[ofoot].pos.v[2]};


    float lfrac = (fin - lrp).length() / (dist * 2);
    float rfrac = (fin - rrp).length() / (dist * 2);

    lfrac -= 0.2f;
    rfrac -= 0.2f;

    ///dont move the feet if we're really idling in the direction of last travel
    if(!idle)
    {
        IK_foot(0, parts[foot].pos - lmod * current_dir);
        IK_foot(1, parts[bodypart::RFOOT].pos + lmod * current_dir);
    }

    lfrac /= 0.8f;
    rfrac /= 0.8f;

    lfrac = clamp(lfrac, 0.f, 1.f);
    rfrac = clamp(rfrac, 0.f, 1.f);

    //printf("%f %f\n", lfrac, rfrac);
    //printf("%f %f %f\n", fin.v[0], fin.v[1], fin.v[2]);

    if(lmod < 0 && !idle)
    {
        float xv = -lfrac * (lfrac - 1);

        parts[foot].pos.v[1] = h * xv + rest_positions[foot].v[1];
    }
    if(lmod > 0 && !idle)
    {
        float xv = -rfrac * (rfrac - 1);

        parts[ofoot].pos.v[1] = h * xv + rest_positions[ofoot].v[1];
    }

    current_dir = current_dir.norm();

    //printf("%f %f %f\n", current_dir.v[0], current_dir.v[1], current_dir.v[2]);
    //printf("%f %f %f\n", parts[foot].pos.v[0], parts[foot].pos.v[1], parts[foot].pos.v[2]);

    float real_weight = 5.f;

    ///adjust foot so that it sits on the 'correct' line

    ///works for idling, average in last direction of moving to stick the feet on ground
    ///also keeps feet in place while regular walking
    {
        foot = bodypart::LFOOT;

        vec3f rest = rest_positions[foot];
        vec3f shortest_dir = point2line_shortest(rest_positions[foot], current_dir, parts[foot].pos);

        vec3f line_point = shortest_dir + parts[foot].pos;

        IK_foot(0, (line_point + parts[foot].pos * real_weight) / (1.f + real_weight));
    }

    {
        foot = bodypart::RFOOT;

        vec3f rest = rest_positions[foot];
        vec3f shortest_dir = point2line_shortest(rest_positions[foot], current_dir, parts[foot].pos);

        vec3f line_point = shortest_dir + parts[foot].pos;

        IK_foot(1, (line_point + parts[foot].pos * real_weight) / (1.f + real_weight));
    }

    ///reflects foot when it reaches the destination
    {
        foot = bodypart::LFOOT;

        vec3f without_up = {parts[foot].pos.v[0], 0.f, parts[foot].pos.v[2]};
        vec3f without_up_rest = {rest_positions[foot].v[0], 0.f, rest_positions[foot].v[2]};

        ///current dir is the direction we are going in

        if((without_up - without_up_rest).length() > dist)
        {
            vec3f d = without_up_rest - without_up;
            d = d.norm();

            float excess = (without_up_rest - without_up).length() - dist;

            parts[foot].pos = parts[foot].pos + d * excess;

            lmod = -lmod;
        }
    }


    {
        foot = bodypart::RFOOT;

        vec3f without_up = {parts[foot].pos.v[0], 0.f, parts[foot].pos.v[2]};
        vec3f without_up_rest = {rest_positions[foot].v[0], 0.f, rest_positions[foot].v[2]};

        ///current dir is the direction we are going in

        if((without_up - without_up_rest).length() > dist)
        {
            vec3f d = without_up_rest - without_up;
            d = d.norm();

            float excess = (without_up_rest - without_up).length() - dist;

            parts[foot].pos = parts[foot].pos + d * excess;
        }
    }

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

        vec3f rot = avg.get_euler();

        weapon.set_rot(rot);

        parts[LHAND].set_rot(rot);
        parts[RHAND].set_rot(rot);
    }
}

///flush HERE?
void fighter::set_pos(vec3f _pos)
{
    pos = _pos;

    ///stash the lights somewhere
    ///pos only gets set if we're being forcobly moved
    for(auto& i : my_lights)
    {
        i->set_pos({pos.v[0], pos.v[1], pos.v[2]});
    }
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

vec2f fighter::get_approx_dim()
{
    vec2f real_size = {bodypart::scale, bodypart::scale};

    real_size = real_size * 2.f;
    real_size = real_size + bodypart::scale * 2.f/5.f;

    return real_size;
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

    /*vec3f relative_pos = local_pos.back_rot({0,0,0}, local_rot);

    vec3f n_pos = relative_pos.back_rot(world_pos, world_rot);

    vec3f total_rot = world_rot + local_rot;*/

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

    weapon.model->set_pos({r.pos.v[0], r.pos.v[1], r.pos.v[2]});
    weapon.model->set_rot({r.rot.v[0], r.rot.v[1], r.rot.v[2]});

    for(auto& i : joint_links)
    {
        bool active = i.p1->is_active && i.p2->is_active;

        objects_container* obj = i.obj;

        vec3f start = i.p1->global_pos;
        vec3f fin = i.p2->global_pos;

        start = start + i.offset;
        fin = fin + i.offset;

        vec3f rot = (fin - start).get_euler();

        vec3f dir = (fin - start);

        start = start + dir * i.squish_frac;

        obj->set_pos({start.v[0], start.v[1], start.v[2]});
        obj->set_rot({rot.v[0], rot.v[1], rot.v[2]});
    }

    for(int i=0; i<bodypart::COUNT; i++)
    {
        old_pos[i] = parts[i].pos;
    }
}

void fighter::update_lights()
{
    vec3f lpos = (parts[bodypart::LFOOT].global_pos + parts[bodypart::RFOOT].global_pos) / 2.f;

    my_lights[0]->set_pos({lpos.v[0], lpos.v[1] + 40.f, lpos.v[2]});

    vec3f bpos = (vec3f){0, parts[bodypart::BODY].global_pos.v[1] + 50.f, -150.f}.rot({0,0,0}, rot) + pos;

    my_lights[1]->set_pos({bpos.v[0], bpos.v[1], bpos.v[2]});

    vec3f sword_tip = xyz_to_vec(weapon.model->pos) + (vec3f){0, weapon.length, 0}.rot({0,0,0}, xyz_to_vec(weapon.model->rot));

    my_lights[2]->set_pos({sword_tip.v[0], sword_tip.v[1], sword_tip.v[2]});

    for(auto& i : my_lights)
    {
        i->set_radius(1000.f);
        i->set_shadow_casting(0);
        i->set_brightness(1.0f);
        i->set_diffuse(1.0f);
    }

    my_lights[1]->set_radius(700.f);
    my_lights[2]->set_radius(400.f);

    for(auto& i : my_lights)
    {
        if(team == 0)
            i->set_col({1.f, 0.f, 0.f, 0.f});
        else
            i->set_col({0.f, 0.f, 1.f, 0.f});

        //i->set_col({1.f, 1.f, 1.f});
    }

    for(auto& i : my_lights)
    {
        vec3f pos = xyz_to_vec(i->pos);

        ///dirty hack of course
        ///ideally we'd use the alive status for this
        ///but that'd break the death effects
        if(pos.length() > 10000.f)
        {
            i->set_active(false);
        }
        else
        {
            i->set_active(true);
        }
    }
}

///net-fighters ONLY
void fighter::overwrite_parts_from_model()
{
    for(part& i : parts)
    {
        cl_float4 pos = i.obj()->pos;
        cl_float4 rot = i.obj()->rot;

        i.set_global_pos(xyz_to_vec(pos));
        i.set_global_rot(xyz_to_vec(rot));

        i.set_active(i.obj()->isactive);

        i.update_model();
    }

    pos = parts[bodypart::BODY].global_pos;
    rot = parts[bodypart::BODY].global_rot;

    ///this is so that if the remote fighter is actually alive but we don't know
    ///it can die again
    ///we could avoid this by networking the death state
    ///but that requries more networking. We already have the info, so do it clientside
    ///hp for parts are also 'reliably' delivered (every frame)
    if(!should_die())
    {
        performed_death = false;
    }

    ///do not need to update weapon because it does not currently have a global position stored (not a part)
}

void fighter::set_team(int _team)
{
    int old = team;

    team = _team;

    for(part& i : parts)
    {
        i.set_team(team);
    }

    weapon.set_team(team);

    if(old == team)
        return;

    for(auto& i : joint_links)
    {
        i.obj->set_active(false);
    }

    joint_links.clear();

    ///now we know the team, we can add the joint parts
    using namespace bodypart;

    joint_links.push_back(make_link(&parts[LUPPERARM], &parts[LLOWERARM], team, 0.1f));
    joint_links.push_back(make_link(&parts[LLOWERARM], &parts[LHAND], team, 0.1f));

    joint_links.push_back(make_link(&parts[RUPPERARM], &parts[RLOWERARM], team, 0.1f));
    joint_links.push_back(make_link(&parts[RLOWERARM], &parts[RHAND], team, 0.1f));

    joint_links.push_back(make_link(&parts[LUPPERARM], &parts[BODY], team, 0.1f));
    joint_links.push_back(make_link(&parts[RUPPERARM], &parts[BODY], team, 0.1f));

    /*joint_links.push_back(make_link(&parts[LUPPERARM], &parts[RUPPERARM], 0.1f, 25.f, {0, -bodypart::scale * 0.2, 0}));
    joint_links.push_back(make_link(&parts[LUPPERARM], &parts[RUPPERARM], 0.1f, 23.f, {0, -bodypart::scale * 0.8, 0}));
    joint_links.push_back(make_link(&parts[LUPPERLEG], &parts[RUPPERLEG], -0.2f, 21.f, {0, bodypart::scale * 0.2, 0}));*/

    //joint_links.push_back(make_link(&parts[LUPPERARM], &parts[BODY], 0.2f, {0, -bodypart::scale * 0.75f, 0}));
    //joint_links.push_back(make_link(&parts[RUPPERARM], &parts[BODY], 0.2f, {0, -bodypart::scale * 0.75f, 0}));

    //joint_links.push_back(make_link(&parts[LUPPERARM], &parts[BODY], 0.2f, {0, -bodypart::scale * 1.5f, 0}));
    //joint_links.push_back(make_link(&parts[RUPPERARM], &parts[BODY], 0.2f, {0, -bodypart::scale * 1.5f, 0}));

    /*joint_links.push_back(make_link(&parts[LUPPERLEG], &parts[RUPPERLEG], 0.2f));*/

    joint_links.push_back(make_link(&parts[LUPPERLEG], &parts[LLOWERLEG], team, 0.1f));
    joint_links.push_back(make_link(&parts[RUPPERLEG], &parts[RLOWERLEG], team, 0.1f));

    joint_links.push_back(make_link(&parts[LLOWERLEG], &parts[LFOOT], team, 0.1f));
    joint_links.push_back(make_link(&parts[RLOWERLEG], &parts[RFOOT], team, 0.1f));


    for(auto& i : joint_links)
    {
        i.obj->set_active(true);
    }
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

    bool do_explode_effect = num_dead() < num_needed_to_die() - 1;

    parts[type].damage(d, do_explode_effect);

    net.recoil = 1;
    network::host_update(&net.recoil);
}

void fighter::set_contexts(object_context* _cpu, object_context_data* _gpu)
{
    if(_cpu)
        cpu_context = _cpu;

    if(_gpu)
        gpu_context = _gpu;
}
