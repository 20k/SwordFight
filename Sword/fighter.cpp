#include "fighter.hpp"
#include "physics.hpp"
#include <unordered_map>

#include "object_cube.hpp"

#include "particle_effect.hpp"

#include "../openclrenderer/light.hpp"

#include "../openclrenderer/obj_load.hpp"

#include "text.hpp"

#include "../sword_server/teaminfo_shared.hpp"

#include "../openclrenderer/logging.hpp"

#include "map_tools.hpp"
#include "network_fighter_model.hpp"

#include "../openclrenderer/texture.hpp"

///separate jumping and falling eventually
vec3f jump_descriptor::get_relative_jump_displacement_tick(float dt, fighter* fight)
{
    /*if(current_time > time_ms)
    {
        should_play_foot_sounds = is_jumping;

        current_time = 0;
        is_jumping = false;
        return {0,0,0};
    }*/

    if(!is_jumping)
    {
        return {0,0,0};
    }

    should_play_foot_sounds = false;

    vec3f offset = {0, 0, 0};

    offset.v[0] = dir.v[0];
    offset.v[2] = dir.v[1];

    ///this because im an idiot in walk dir
    ///and movement speed is dt/2
    vec3f dt_struct = {dt/2.f, 0, dt/2.f};

    offset = offset * dt_struct * last_speed;

    float frac = current_time / time_ms;

    if(frac > 1)
        frac = 1;

    ///move in a sine curve, cos is the differential of sin
    float dh = cos(frac * M_PI);

    offset.v[1] = dh * dt;

    vec2f clamped = fight->get_wall_corrected_move({fight->pos.v[0], fight->pos.v[2]}, {offset.v[0], offset.v[2]});
    clamped = fight->get_external_fighter_corrected_move(s_xz(fight->pos), clamped, fight->fighter_list);
    clamped = fight->get_wall_corrected_move(s_xz(fight->pos), clamped);

    offset.v[0] = clamped.v[0];
    offset.v[2] = clamped.v[1];

    current_time += dt;

    return offset;
}

void jump_descriptor::terminate_early()
{
    should_play_foot_sounds = is_jumping;

    current_time = 0;
    is_jumping = false;
}

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

///need to make textures unique optionally
part::part(object_context& context)
{
    cpu_context = &context;
    model = context.make_new();

    is_active = false;

    hp = 1.f;

    set_pos({0,0,0});
    set_rot({0,0,0});

    set_global_pos({0,0,0});
    set_global_rot({0,0,0});

    model->set_file("./Res/high/bodypart_red.obj");

    model->set_unique_textures(true);

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
    model->hide();

    ///have to fix some of the network hodge-podgery first before this will work
    ///as the active state of network fighters is overwritten by the model state
    /*if(!model->isactive && active)
    {
        model->set_active(true);
    }

    if(model->isactive && !active)
    {
        model->hide();
    }*/

    is_active = active;
}

void part::scale()
{
    float amount = bodypart::scale/3.f;

    model->set_dynamic_scale(amount);
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

///wait, if we've got dynamic texture update
///I can just go from red -> dark red -> black
///yay!
void part::update_model()
{
    model->set_pos({global_pos.v[0], global_pos.v[1], global_pos.v[2]});
    model->set_rot({global_rot.v[0], global_rot.v[1], global_rot.v[2]});

    ///in preparation for 0 cost killing
    if(!is_active)
    {
        model->hide();
    }
}

void part::set_team(int _team)
{
    int old = team;

    team = _team;

    if(old != team)
        load_team_model();
}

///might currently leak texture memory
void part::load_team_model()
{
    const std::string low = "res/low/bodypart_red.obj";
    const std::string high = "res/high/bodypart_red.obj";

    std::string to_load = low;

    if(quality == 1)
        to_load = high;

    model->set_file(to_load);

    model->unload();

    set_active(true);

    cpu_context->load_active();

    texture_context* tex_ctx = &cpu_context->tex_ctx;

    texture* tex = tex_ctx->id_to_tex(model->objs[0].tid);

    if(tex)
    {
        vec3f col = team_info::get_team_col(team);

        int res = 256;

        if(quality == 1)
            res = 1024;

        tex->set_create_colour(sf::Color(col.x(), col.y(), col.z(), 255), res, res);
    }

    model->set_specular(bodypart::specular);

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
void part::damage(fighter* parent, float dam, bool do_effect, int32_t network_id_hit_by)
{
    //set_hp(hp - dam);

    //net.damage_info.id_hit_by = network_id_hit_by;

    request_network_hp_delta(-dam, parent);

    damage_info current_info = parent->net_fighter_copy->network_parts[type].requested_damage_info.networked_val;

    current_info.id_hit_by = network_id_hit_by;

    parent->net_fighter_copy->network_parts[type].requested_damage_info.set_transmit_val(current_info);

    if(is_active && hp < 0.0001f)
    {
        perform_death(do_effect);
    }

    ///so, lets do this elsewhere
}

///so delta gets reset to 0 post networking, ie per frame
void part::request_network_hp_delta(float delta, fighter* parent)
{
    network_part_info& net_p = parent->net_fighter_copy->network_parts[type];

    damage_info current_info = parent->net_fighter_copy->network_parts[type].requested_damage_info.networked_val;

    current_info.hp_delta = delta;

    parent->net_fighter_copy->network_parts[type].requested_damage_info.set_transmit_val(current_info);
}

void part::update_texture_by_hp()
{
    if(old_hp != hp)
    {
        float hp_store = old_hp;

        old_hp = hp;

        vec3f col = team_info::get_team_col(team);

        cl_float4 pcol = {col.v[0], col.v[1], col.v[2], 255.f};

        if(!model->isactive || !model->isloaded)
            return;

        ///if this is async this might break
        if(hp > 0 && hp != 1.f && hp < hp_store)
        {
            cl_float4 dcol = {20, 20, 20};

            dcol.x /= 255.f;
            dcol.y /= 255.f;
            dcol.z /= 255.f;

            dcol.w = 1;

            cl_uint tid = model->objs[0].tid;

            texture* tex = model->parent->tex_ctx.id_to_tex(tid);

            int rnum = 10;

            for(int i=0; i<rnum; i++)
            {
                float width = tex->get_largest_dimension();

                vec2f rpos = randf<2, float>(width * 0.2f, width * 0.8f) * (vec2f){tex->c_image.getSize().x, tex->c_image.getSize().y};

                rpos = rpos / width;

                float angle = randf_s() * 2 * M_PI;

                vec2f dir = {cos(angle), sin(angle)};

                tex->update_random_lines(40, dcol, {rpos.v[0], rpos.v[1]}, {dir.v[0], dir.v[1]}, cpu_context->fetch()->tex_gpu_ctx);
            }

            tex->update_gpu_mipmaps(cpu_context->fetch()->tex_gpu_ctx);
        }

        if(hp == 1.f)
        {
            cl_uint tid = model->objs[0].tid;

            texture* tex = model->parent->tex_ctx.id_to_tex(tid);

            tex->update_gpu_texture_col(pcol, cpu_context->fetch()->tex_gpu_ctx);
        }
    }
}

void part::perform_death(bool do_effect)
{
    //printf("%f hp %i isactive\n", hp, is_active);

    if(do_effect)
    {
        cube_effect e;

        e.make(1300, global_pos, 100.f, team, 10, *cpu_context);
        particle_effect::push(e);
    }

    set_active(false);
    obj()->hide();

    //cpu_context->load_active();
    //cpu_context->build_request();

    lg::log("Perform death");
}

void part::set_hp(float h)
{
    float delta = h - hp;

    hp = h;

    //network_hp(delta);
}


/*void part::network_hp(float delta)
{
    net.hp_dirty = true;
    net.damage_info.hp_delta += delta;
}*/

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
    model->unload();

    model->set_active(true);

    cpu_context->load_active();


    texture_context* tex_ctx = &cpu_context->tex_ctx;

    texture* ntex = tex_ctx->make_new_cached(team_info::get_texture_cache_name(team));

    vec3f col = team_info::get_team_col(team);

    ntex->set_create_colour({col.v[0], col.v[1], col.v[2]}, 128, 128);

    model->patch_non_square_texture_maps();

    ///this is to make everything red
    for(auto& i : model->objs)
        i.tid = ntex->id;

    ///make this a default action
    scale();

    model->set_specular(bodypart::specular);
}

sword::sword(object_context& cpu)
{
    cpu_context = &cpu;

    model = cpu.make_new();

    model->set_pos({0, 0, -100});
    dir = {0,0,0};
    model->set_file("./Res/sword_red.obj");
    //model->set_file("./Res/trombone_cutdown_nomod.obj");
    team = -1;
}

void sword::scale()
{
    model->set_dynamic_scale(50.f);
    model->set_specular(0.4f);

    bound = get_bbox(model);

    float sword_height = 0;

    for(auto& i : model->objs)
    {
        for(triangle& t : i.tri_list)
        {
            for(vertex& v : t.vertices)
            {
                vec3f pos = xyz_to_vec(v.get_pos());

                if(pos.v[1] > sword_height)
                    sword_height = pos.v[1];
            }
        }
    }

    length = sword_height * model->dynamic_scale;
}

objects_container* sword::obj()
{
    return model;
}

void sword::update_model()
{
    model->set_pos({pos.v[0], pos.v[1], pos.v[2]});
    model->set_rot({rot.v[0], rot.v[1], rot.v[2]});
}

void sword::set_pos(vec3f _pos)
{
    pos = _pos;
}

void sword::set_rot(vec3f _rot)
{
    rot = _rot;
}

void sword::set_active(bool active)
{
    is_active = active;

    if(!is_active)
    {
        model->hide();
    }
}

link make_link(part* p1, part* p2, int team, float squish = 0.0f, float thickness = 18.f, vec3f offset = {0,0,0})
{
    vec3f dir = (p2->pos - p1->pos);

    texture_context* tex_ctx = &p1->cpu_context->tex_ctx;

    texture* ntex = tex_ctx->make_new_cached(team_info::get_texture_cache_name(team));

    vec3f col = team_info::get_team_col(team);

    ntex->set_create_colour({col.v[0], col.v[1], col.v[2]}, 128, 128);

    vec3f start = p1->pos + dir * squish;
    vec3f finish = p2->pos - dir * squish;

    objects_container* o = p1->cpu_context->make_new();
    o->set_load_func(std::bind(load_object_cube_tex, std::placeholders::_1, start, finish, thickness, *ntex, false));
    o->cache = false;

    link l;

    l.obj = o;

    l.p1 = p1;
    l.p2 = p2;

    l.offset = offset;

    l.squish_frac = squish;

    l.length = (finish - start).length();

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

    cosmetic.tophat = cpu_context->make_new();

    quality = 0;

    light l1;

    my_lights.push_back(light::add_light(&l1));
    my_lights.push_back(light::add_light(&l1));
    my_lights.push_back(light::add_light(&l1));

    load();

    pos = {0,0,0};
    rot = {0,0,0};

    game_state = nullptr;

    net_fighter_copy = new network_fighter;
}

fighter::~fighter()
{
    delete net_fighter_copy;
}

void fighter::load()
{
    trombone_manage.init(cpu_context);

    sprint_frac = 0.f;

    is_sprinting = false;

    rand_offset_ms = randf_s(0.f, 991.f);

    gpu_name_dirty = 0;

    last_walk_dir = {0,0};

    last_walk_dir_diff = {0,0};

    camera_bob_mult = 0;

    camera_bob = {0,0,0};

    player_id_i_was_last_hit_by = -1;

    crouch_frac = 0.f;

    momentum = {0,0};

    jump_info = jump_descriptor();

    sword_rotation_offset = {0,0,0};

    net.reported_dead = 0;

    performed_death = false;

    rot_diff = {0,0,0};

    look_displacement = {0,0,0};

    frametime = 0;

    my_time = 0;

    look = {0,0,0};

    idling = false;

    team = -1;

    stance = 0;

    ///im not sure why this is a duplicate of default_position
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

    shoulder_rotation = 0.f;

    left_foot_sound = true;
    right_foot_sound = true;

    just_spawned = true;

    cosmetic.set_active(true);
}

void fighter::respawn(vec2f _pos)
{
    int old_team = team;

    load();

    team = old_team;

    ///this doesn't work properly for some reason
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

    cpu_context->build_request();

    lg::log("Respawn");

    //network::host_update(&net.dead);
}

void fighter::die()
{
    net.reported_dead = true;

    performed_death = true;

    //net.dead = true;

    for(auto& i : parts)
    {
        i.set_hp(-1.f);
        i.set_active(false);
        i.obj()->hide(); ///so we can do this alloclessly
    }

    weapon.model->set_active(false);
    weapon.model->hide();

    for(auto& i : joint_links)
    {
        i.obj->set_active(false);
        i.obj->hide();
    }

    cosmetic.set_active(false);
    cosmetic.tophat->hide();

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

        weapon.obj()->hide();
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

    cpu_context->build_request();

    //lg::log("Die");

    ///pipe out hp here, just to check
}

int fighter::num_dead()
{
    int num_destroyed = 0;

    for(auto& p : parts)
    {
        ///hp_delta is predicted
        ///this is so that network stuff wont trigger multiple deaths
        ///by mistake
        if(p.hp <= 0)
        {
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
    if(num_dead() >= num_needed_to_die() && !performed_death)
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

///so, this seems like a bug?
bool fighter::dead()
{
    return (num_dead() > num_needed_to_die()) || performed_death;
}

void fighter::set_weapon(int weapon_id)
{
    if(weapon_id == current_weapon)
        return;

    if(weapon_id == 0)
    {
        cancel_hands();
        queue_attack(attacks::FAST_REST);

        trombone_manage.set_active(false);
    }

    ///trombone is already managing its shizzle because it has different requirements to the above
    ///its a bit silly. OK so when you're sprinting with the trombone that's not performing any kind of 'move'
    ///but it does with the sword
    ///so going from trombone -> sword we can just cancel and queue attack
    ///whereas as from sword -> trombone we have to ensure that it will always assume the trombone position
    ///which may not work if we're sprinting. A cleaner solution would be to cancel any ongoing moves
    ///but this may break future functionality, so really this is a clustertruck all around
    if(weapon_id == 1)
    {
        trombone_manage.set_active(true);
    }

    current_weapon = weapon_id;
}

void fighter::tick_cape()
{
    return;

    if(quality == 0)
        return;

    int ticks = 1;

    for(int i=0; i<ticks; i++)
    {
        my_cape.tick(this);
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

    vec3f clamps = {M_PI/6.f, M_PI/12.f, M_PI/8.f};
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

    old_look_displacement = look_displacement;
    look_displacement = (vec3f){width, height, 0.f};

    look = new_look;
}

///s2 and s3 define the shoulder -> elbow, and elbow -> hand length
///need to handle double cos problem
float get_joint_angle(vec3f end_pos, vec3f start_pos, float s2, float s3)
{
    float s1 = (end_pos - start_pos).length();

    s1 = clamp(s1, 0.f, s2 + s3);

    float ic = (s2 * s2 + s3 * s3 - s1 * s1) / (2 * s2 * s3);

    ic = clamp(ic, -1, 1);

    float angle = acos ( ic );

    return angle;
}

float get_joint_angle_foot(vec3f end_pos, vec3f start_pos, float s2, float s3)
{
    float s1 = (end_pos - start_pos).length();

    s1 = clamp(s1, 0.f, s2 + s3);

    float ic = (s2 * s2 + s3 * s3 - s1 * s1) / (2 * s2 * s3);

    ic = clamp(ic, -1, 1);

    float angle = acos ( ic );

    return angle;
}

///p1 shoulder, p2 elbow, p3 hand
void inverse_kinematic(vec3f pos, vec3f p1, vec3f p2, vec3f p3, vec3f& o_p1, vec3f& o_p2, vec3f& o_p3)
{
    float s1 = (p3 - p1).length();
    float s2 = (p2 - p1).length();
    float s3 = (p3 - p2).length();

    float joint_angle = get_joint_angle(pos, p1, s2, s3);

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

///p1 shoulder, p2 elbow, p3 hand/foot
void inverse_kinematic_foot(vec3f pos, vec3f p1, vec3f p2, vec3f p3, vec3f off1, vec3f off2, vec3f off3, vec3f& o_p1, vec3f& o_p2, vec3f& o_p3)
{
    float s1 = (p3 - p1 - off1).length();
    float s2 = (p2 - p1).length();
    float s3 = (p3 - p2).length();


    float joint_angle = M_PI + get_joint_angle_foot(pos, p1 + off1, s2, s3);

    o_p3 = pos;

    float area = 0.5f * s2 * s3 * sin(joint_angle);

    ///height of scalene triangle is 2 * area / base

    float height = 2 * area / s1;

    vec3f d1 = (o_p3 - p1 - off1);
    vec3f d2 = {1, 0, 0};

    vec3f d3 = cross(d1, d2);

    d3 = d3.norm();

    vec3f half = (p1 + off1 + o_p3)/2.f;

    ///set this to std::max(height, 30.f) if you want beelzebub strolling around
    o_p2 = half + std::min(height, -5.f) * d3;

    vec3f d = (o_p3 - p1 - off1).norm();

    ///no wait, this is defining hip wiggle
    o_p1 = p1 + off1 + d.norm() * (vec3f){20.f, 4.f, 20.f};
}

void fighter::IK_hand(int which_hand, vec3f pos, float upper_rotation, bool arms_are_locked, bool force_positioning)
{
    using namespace bodypart;

    auto upper = which_hand ? RUPPERARM : LUPPERARM;
    auto lower = which_hand ? RLOWERARM : LLOWERARM;
    auto hand = which_hand ? RHAND : LHAND;

    vec3f i1, i2, i3;

    i1 = rest_positions[upper];
    i2 = rest_positions[lower];
    i3 = rest_positions[hand];

    i1 = i1.rot({0,0,0}, {0, upper_rotation, 0});
    i2 = i2.rot({0,0,0}, {0, upper_rotation, 0});
    i3 = i3.rot({0,0,0}, {0, upper_rotation, 0});

    vec3f o1, o2, o3;

    inverse_kinematic(pos, i1, i2, i3, o1, o2, o3);

    if(arms_are_locked)
    {
        o2 = (o1 + o3) / 2.f;
    }

    if(force_positioning && (o3 - pos).length() > 1)
    {
        o3 = pos;

        o2 = (o1 + o3) / 2.f;
    }

    parts[upper].set_pos(o1);
    parts[lower].set_pos(o2);
    parts[hand].set_pos(o3);
}

///ergh. merge offsets into inverse kinematic foot as separate arguments
///so we dont mess up length calculations
void fighter::IK_foot(int which_foot, vec3f pos, vec3f off1, vec3f off2, vec3f off3)
{
    using namespace bodypart;

    auto upper = which_foot ? RUPPERLEG : LUPPERLEG;
    auto lower = which_foot ? RLOWERLEG : LLOWERLEG;
    auto hand = which_foot ? RFOOT : LFOOT;

    vec3f o1, o2, o3;

    ///put offsets into this function
    inverse_kinematic_foot(pos, rest_positions[upper], rest_positions[lower], rest_positions[hand], off1, off2, off3, o1, o2, o3);

    parts[upper].set_pos(o1);
    parts[lower].set_pos((o2 + old_pos[lower]*5.f)/6.f);
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

float frac_smooth(float in)
{
    return - in * (in - 2);
}

///if we exaggerate the skew, this is workable
float erf_smooth(float x)
{
    if(x < 0 || x >= 1)
        return x;

    return powf((- x * (x - 2) * (erf((x - 0.5f) * 3) + 1) / 2.f), 1.5f);
}

float rerf_smooth(float x)
{
    return (erf((x - 0.5f) * 3) + 1) / 2.f;
}

float derf_smooth(float x)
{
    if(x < 0 || x >= 1)
        return x;

    float erf1 = rerf_smooth(x);
    float erf2 = 1.f - rerf_smooth(1.f - x);

    return erf1 * (x) + erf2 * (1.f - x);
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

    network_fighter& my_network = *net_fighter_copy;

    ///updated from networking, but we can modify it
    ///Itll stay the same until it gets updated by the networking again
    bool should_recoil = my_network.network_fighter_inf.recoil_requested.get_local_val();

    if(should_recoil)
    {
        bool forced_recoil = my_network.network_fighter_inf.recoil_forced.get_local_val();

        if(can_windup_recoil() || forced_recoil)
            recoil();

        my_network.network_fighter_inf.recoil_requested.set_local_val(0);
        my_network.network_fighter_inf.recoil_forced.set_local_val(0);

        my_network.network_fighter_inf.recoil_requested.network_local();
        my_network.network_fighter_inf.recoil_forced.network_local();


        //net.force_recoil = 0;
        //net.recoil = 0;
        //net.recoil_dirty = true;
    }

    ///use sword rotation offset to make sword 90 degrees at blocking

    net.is_damaging = 0;

    bool arms_are_locked = false;

    ///only the first move is ever executed PER LIMB
    ///the busy list is used purely to stop all the rest of the moves from activating
    ///because different limbs can have different moves activating concurrently
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

        ///really i need to set this per... limb?
        if(i.does(mov::DAMAGING))
        {
            net.is_damaging = 1;
        }

        float arm_len = (default_position[LHAND] - default_position[LUPPERARM]).length();

        ///this is the head vector, but we want the tip of the sword to go through the centre
        ///time for maths
        ///really this wants to dynamically find the player's LOOK vector
        ///but this combined with look displacement does an alright job
        vec3f head_vec = {0, default_position[HEAD].v[1], -arm_len};
        float sword_len = weapon.length;

        ///current sword tip
        vec3f sword_vec = (vec3f){0, sword_len, 0}.rot({0,0,0}, weapon.rot) + weapon.pos;

        vec3f desired_sword_vec = {sword_vec.v[0], head_vec.v[1], sword_vec.v[2]};

        if(i.does(mov::OVERHEAD_HACK))
            desired_sword_vec = desired_sword_vec.norm() * sword_len;

        ///this worked fine before with body because they're on the same height
        ///hmm. both are very similar in accuracy, but they're slightly stylistically different
        ///comebacktome ???
        ///????
        vec3f desired_hand_relative_sword = desired_sword_vec - (desired_sword_vec - default_position[LUPPERARM]).norm() * sword_len;

        ///really we only want the height
        float desired_hand_height = desired_hand_relative_sword.v[1];


        vec3f cx_sword_vec = {0.f, head_vec.v[1], sword_vec.v[2]};

        vec3f cx_sword_rel = cx_sword_vec - (cx_sword_vec - default_position[LUPPERARM]).norm() * sword_len;

        float desired_hand_x = cx_sword_rel.v[0];

        vec3f half_hand_vec = ((vec3f){desired_hand_x, desired_hand_height, i.fin.v[2]} + i.start)/2.f;


        busy_list.push_back(i.limb);

        float frac = i.get_frac();

        frac = clamp(frac, 0.f, 1.f);

        vec3f current_pos;

        vec3f actual_finish = i.fin;
        vec3f actual_start = i.start;

        if(i.does(mov::FINISH_INDEPENDENT))
        {
            actual_finish = actual_finish - look_displacement;
        }

        if(i.does(mov::LOCKS_ARMS))
        {
            //arms_are_locked = true;
        }

        vec3f actual_avg = (actual_start + actual_finish) / 2.f;

        ///lets use mix3
        if(i.does(mov::PASS_THROUGH_SCREEN_CENTRE))
        {
            actual_avg = head_vec;

            actual_avg.v[1] = desired_hand_height;
        }

        if(i.does(mov::FINISH_AT_SCREEN_CENTRE))
        {
            actual_avg = half_hand_vec;

            actual_finish.v[1] = desired_hand_height;
            actual_finish.v[0] = desired_hand_x;
        }

        if(i.does(mov::FINISH_AT_90))
        {
            float ffrac = frac_smooth(frac);

            sword_rotation_offset.v[1] = M_PI/2.f * pow(ffrac, 2.f);
        }
        else
        {
            float ffrac = frac_smooth(frac);

            if(fabs(sword_rotation_offset.v[1]) > 0.0001f)
            {
                sword_rotation_offset.v[1] = M_PI/2.f * pow((1.f - ffrac), 1.7f);
            }
        }

        ///scrap mix3 and slerp3 stuff, need to interpolate properly
        ///need to use a bitfield really, thisll get unmanageable
        if(i.type == 0)
        {
            ///apply a bit of smoothing
            frac = frac_smooth(frac);
            current_pos = mix(actual_start, actual_finish, frac); ///do a slerp3
        }
        else if(i.type == 1)
        {
            ///need to define this manually to confine it to one axis, slerp is not what i want
            frac = frac_smooth(frac);
            current_pos = slerp(actual_start, actual_finish, frac);

            ///so the reason this flatttens it out anyway
            ///is because we're swappign from slerping to cosinterpolation
            if(i.does(mov::PASS_THROUGH_SCREEN_CENTRE))
                current_pos.v[1] = mix3(actual_start, actual_avg, actual_finish, frac).v[1];

        }
        else if(i.type == 2)
        {
            current_pos = slerp(actual_start, actual_finish, frac);
        }
        else if(i.type == 3)
        {
            frac = erf_smooth(frac);

            current_pos = slerp(actual_start, actual_finish, frac);
        }
        else if(i.type == 4)
        {
            frac = 1.f - erf_smooth(1.f - frac);

            current_pos = slerp(actual_start, actual_finish, frac);
        }
        else if(i.type == 5)
        {
            frac = derf_smooth(frac);

            current_pos = slerp(actual_start, actual_finish, frac);
        }

        if(i.limb == LHAND || i.limb == RHAND)
        {
            ///focus pos is relative to player, but does NOT include look_displacement OR world anything
            vec3f old_pos = focus_pos;

            focus_pos = current_pos;

            ///losing a frame currently, FIXME
            ///if the sword hits something, not again until the next move
            ///make me a function?
            if(i.hit_id < 0 && i.does(mov::DAMAGING))
            {
                ///this is the GLOBAL move dir, current_pos could be all over the place due to interpolation, lag etc
                vec3f move_dir = (focus_pos - old_pos).norm();

                #define ATTACKER_PHYS
                #ifdef ATTACKER_PHYS
                ///pass direction vector into here, then do the check
                ///returns -1 on miss
                i.hit_id = phys->sword_collides(weapon, this, move_dir, is_player);

                ///if hit, need to signal the other fighter that its been hit with its hit id, relative to part num
                if(i.hit_id >= 0)
                {
                    fighter* their_parent = phys->bodies[i.hit_id].parent;

                    ///this is the only time damage is applied to anything, ever
                    their_parent->damage((bodypart_t)(i.hit_id % COUNT), i.damage, this->network_id, this->is_offline_client && their_parent->is_offline_client);

                    ///this is where the networking fighters get killed
                    ///this is no longer true, may happen here or in server_networking
                    ///probably should remove this
                    their_parent->checked_death();

                    //printf("%s\n", names[i.hit_id % COUNT].c_str());
                }
                #endif
            }

            if(i.does(mov::BLOCKING))
            {
                net.is_blocking = 1;
            }
        }
    }

    for(auto it = moves.begin(); it != moves.end();)
    {
        ///if we've finished and we're not a continuous action, erase
        ///if we're a continuous action and we haven't been set this frame, terminate
        if((it->finished() && !it->does(mov::CONTINUOUS_SPRINT)) || (it->does(mov::CONTINUOUS_SPRINT) && !it->was_set_this_frame))
        {
            action_map.erase(it->limb);

            it = moves.erase(it);
        }
        else
            it++;
    }

    for(auto& i : moves)
    {
        if(i.does(mov::CONTINUOUS_SPRINT))
        {
            i.was_set_this_frame = 0;
        }
    }

    vec3f jump_displacement = jump_info.get_relative_jump_displacement_tick(frametime, this);

    ///0 is normal, paths are about -60
    float ground_height = 0;

    if(game_state != nullptr)
        ground_height = -game_state->current_map.get_ground_height(pos);

    float current_height = pos.v[1];

    if(current_height < ground_height - 1.f)
    {
        jump_info.terminate_early();
    }

    ///still jumping
    if(jump_info.is_jumping)
    {
        pos = pos + jump_displacement;
    }
    else
    {
        pos.v[1] = ground_height;
    }

    ///again needs to be made frametime independent
    const float displacement = (rest_positions[bodypart::LHAND] - rest_positions[bodypart::LUPPERARM]).length();
    float focus_rotation = 0.f;
    shoulder_rotation = shoulder_rotation * 6 + atan2((old_look_displacement.v[0] - look_displacement.v[0]) * 10.f, displacement);
    shoulder_rotation /= 7.f;

    vec3f rot_focus = (focus_pos + look_displacement).rot((vec3f){0,0,0}, (vec3f){0, focus_rotation, 0});

    vec2f weapon_pos = {weapon.pos.v[0], weapon.pos.v[2]};

    float wangle = weapon_pos.angle() + M_PI/2.f;

    shoulder_rotation += (wangle - shoulder_rotation) + shoulder_rotation * 5.f;
    shoulder_rotation /= 6;


    IK_hand(0, rot_focus, shoulder_rotation, arms_are_locked);

    if(!rhand_overridden)
        IK_hand(1, parts[LHAND].pos, shoulder_rotation, arms_are_locked, true);
    else
    {
        IK_hand(1, rhand_override_pos, shoulder_rotation, arms_are_locked, true);
    }

    vec3f slave_to_master = parts[LHAND].pos - parts[RHAND].pos;

    ///dt smoothing doesn't work because the shoulder position is calculated
    ///dynamically from the focus position
    ///this means it probably wants to be part of our IK step?
    ///hands disconnected
    if(slave_to_master.length() > 1.f && !rhand_overridden)
    {
        parts[RHAND].pos = parts[LHAND].pos;
    }

    ///sword render stuff updated here
    update_sword_rot();

    parts[BODY].set_pos((parts[LUPPERARM].pos + parts[RUPPERARM].pos + rest_positions[BODY]*5.f)/7.f);

    vec3f head_rest = rest_positions[HEAD];
    vec3f body_rest = rest_positions[BODY];

    parts[HEAD].set_pos((head_rest - body_rest) + parts[BODY].pos);


    float cdist = 2 * bodypart::scale / 2.f;

    std::vector<bodypart_t> valid_parts_for_crouching = {HEAD, BODY, LUPPERARM, RUPPERARM, LLOWERARM, RLOWERARM, LHAND};

    if(!rhand_overridden)
    {
        valid_parts_for_crouching.push_back(RHAND);
    }

    for(auto& i : valid_parts_for_crouching)
    {
        auto type = i;

        vec3f pos = parts[type].pos;

        if(i != LHAND && i != RHAND)
            pos.v[1] -= cdist * crouch_frac;
        else
            pos.v[1] += smoothed_crouch_offset;

        parts[type].set_pos(pos);
    }

    smoothed_crouch_offset = - cdist * crouch_frac;

    IK_foot(0, parts[LFOOT].pos, {0, -cdist * crouch_frac, 0}, {0, -cdist * crouch_frac, 0}, {0,0,0});
    IK_foot(1, parts[RFOOT].pos, {0, -cdist * crouch_frac, 0}, {0, -cdist * crouch_frac, 0}, {0,0,0});

    weapon.set_pos(parts[bodypart::LHAND].pos);


    ///process death

    ///rip
    checked_death();
    manual_check_part_death();

    rhand_overridden = false;
}

///conflicts with manual check part alive
void fighter::shared_tick(server_networking* networking)
{
    /*if(parts[bodypart::RHAND].obj()->isactive)
    {
        parts[bodypart::RHAND].obj()->set_active(false);

        cpu_context->build_request();

        lg::log("Setactive shared");
    }*/

    trombone_manage.position_model(this);
    trombone_manage.register_server_networking(this, networking);
}

void fighter::manual_check_part_death()
{
    ///num needed to die = 3
    ///if 1, one dead, fine
    ///if 2, two dead, fine
    ///if 3, 3 dead bad
    ///therefore 2 < 3
    ///so that's fine
    ///i dunno why there was a -1 here originally
    bool do_explode_effect = num_dead() < num_needed_to_die();

    for(auto& i : parts)
    {
        if(i.hp < 0.0001f && i.is_active)
        {
            i.perform_death(do_explode_effect);
        }
    }
}

void fighter::manual_check_part_alive()
{
    ///do not respawn parts if the fighter is dead!
    if(dead())
        return;

    bool any = false;

    for(auto& i : parts)
    {
        if(i.hp > 0.0001f && !i.is_active)
        {
            i.set_active(true);

            any = true;
        }
    }

    if(any)
    {
        cpu_context->load_active();
        cpu_context->build_request();

        lg::log("Check alive");
    }
}

vec2f fighter::get_wall_corrected_move(vec2f pos, vec2f move_dir)
{
    bool xw = false;
    bool yw = false;

    vec2f dir_move = move_dir;
    vec2f lpos = pos;

    if(game_state->current_map.rectangle_in_wall(lpos + (vec2f){dir_move.v[0], 0.f}, get_approx_dim(), game_state))
    {
        dir_move.v[0] = 0.f;
        xw = true;
    }
    if(game_state->current_map.rectangle_in_wall(lpos + (vec2f){0.f, dir_move.v[1]}, get_approx_dim(), game_state))
    {
        dir_move.v[1] = 0.f;
        yw = true;
    }

    ///if I move into wall, but yw and xw aren't true, stop
    ///there are some diagonal cases here which might result in funky movement
    ///but largely should be fine
    if(game_state->current_map.rectangle_in_wall(lpos + dir_move, get_approx_dim(), game_state) && !xw && !yw)
    {
        dir_move = 0.f;
    }

    return dir_move;
}

vec2f fighter::get_external_fighter_corrected_move(vec2f pos, vec2f move_dir, const std::vector<fighter*>& fighter_list)
{
    vec2f dim = get_approx_dim();

    float bubble_rad = dim.v[0] * fighter_stats::bubble_modifier_relative_to_approx_dim;

    for(auto& i : fighter_list)
    {
        if(i == this)
            continue;

        if(i->dead())
            continue;

        vec2f their_pos = s_xz(i->pos);

        float cur_len = (pos - their_pos).length();
        float predict_len = (pos + move_dir - their_pos).length();

        ///write a projection and rejection function
        if(cur_len < bubble_rad && predict_len < cur_len)
        {
            vec2f me_to_them = their_pos - pos;

            me_to_them = me_to_them.norm();

            float scalar_proj = dot(move_dir, me_to_them);

            vec2f to_them_relative = scalar_proj * me_to_them;

            vec2f perp = move_dir - to_them_relative;

            return perp;
        }
    }

    return move_dir;
}

///do I want to do a proper dynamic timing synchronisation thing?
void fighter::walk_dir(vec2f dir, bool sprint)
{
    last_walk_dir = dir;

    if(game_state == nullptr)
    {
        lg::log("Warning: No gameplay state for fighter");
    }

    if(jump_info.is_jumping)
    {
        walk_clock.restart();

        return;
    }

    jump_info.dir = {0,0};

    ///try and fix the lex stiffening up a bit, but who cares
    ///make feet average out with the ground
    bool idle = dir.v[0] == 0 && dir.v[1] == 0;

    ///Make me a member variable?
    ///the last valid direction
    static vec2f valid_dir = {-1, 0};

    if(idle)
        dir = valid_dir;
    else
        valid_dir = dir;

    ///in ms
    ///replace this with a dt
    float time_elapsed = walk_clock.getElapsedTime().asMicroseconds() / 1000.f;

    float speed_mult = fighter_stats::speed;
    time_elapsed *= speed_mult;

    jump_info.last_speed = speed_mult;

    float h = 120.f;

    is_sprinting = false;

    if(dir.v[0] == -1 && sprint)
    {
        sprint_frac += time_elapsed / bodypart::sprint_acceleration_time_ms;

        sprint_frac = clamp(sprint_frac, 0.f, 1.f);

        time_elapsed = time_elapsed + time_elapsed * (fighter_stats::sprint_speed - 1) * sprint_frac;
        h *= 1.2f;

        jump_info.last_speed = fighter_stats::speed + (fighter_stats::speed - 1.f) * fighter_stats::sprint_speed * sprint_frac;

        is_sprinting = true;
    }
    else
    {
        sprint_frac -= time_elapsed / bodypart::sprint_acceleration_time_ms;
    }

    sprint_frac = clamp(sprint_frac, 0.f, 1.f);

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
        ///this portion of code handles collision detection
        ///as well as ensuring the leg animations don't do anything silly
        ///this code contains a lot of redundant variables
        ///particularly annoying due to a lack of swizzling

        vec2f dir_move = {global_dir.v[0], global_dir.v[2]};

        vec2f lpos = {pos.v[0], pos.v[2]};

        float move_amount = dir_move.length();

        dir_move = get_wall_corrected_move(lpos, dir_move);

        dir_move = get_external_fighter_corrected_move(lpos, dir_move, fighter_list);

        ///just in case!
        ///disappearing may be because the pos is being destroyed by this
        ///hypothetical
        if(!game_state->current_map.rectangle_in_wall(lpos + dir_move, get_approx_dim(), game_state))
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

                ///if we cant move anywhere, prevent division by 0 and set movedir to 0
                if(time_elapsed > 0.0001f)
                    inv = inv / (time_elapsed / 2.f);
                else
                    inv = {0,0};

                ldir = inv;
            }

            ///time independent movement direction
            ///however the movement direction does not account for speed variances
            jump_info.dir = dir_move / (time_elapsed / 2.f);
            ///the animation fraction is the fraction of our speed that we keep,
            ///due to wall intersection. Eg, a very high degree of our velocity vector
            ///through the wall, results in a low anim frac
            ///really_moved_distance / wanting_to_move_distances
            jump_info.last_speed *= anim_frac;

            ///ok, so we update fighter pos here
            pos = pos + (vec3f){dir_move.v[0], 0.f, dir_move.v[1]};

            last_walk_dir_diff = dir_move;
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

void fighter::crouch_tick(bool do_crouch)
{
    using namespace bodypart;

    ///milliseconds
    float fdiff = frametime / 1000.f;

    const float time_to_crouch_s = 0.1f;

    if(do_crouch)
        crouch_frac += fdiff / time_to_crouch_s;
    else
        crouch_frac -= fdiff / time_to_crouch_s;

    crouch_frac = clamp(crouch_frac, 0.f, 1.f);
}

#include "sound.hpp"

void fighter::do_foot_sounds(bool is_player)
{
    if(dead())
        return;

    if(suppress_foot_sounds && foot_supression_timer.getElapsedTime().asMilliseconds() / 1000.f < time_to_suppress_foot_sounds_s)
        return;

    suppress_foot_sounds = false;

    const int asphalt_start = 2;
    const int foot_nums = 8;

    static int current_num = 0;
    current_num %= foot_nums;

    bool is_relative = is_player;

    using namespace bodypart;

    part& lfoot = parts[LFOOT];
    part& rfoot = parts[RFOOT];

    float ldiff = fabs(lfoot.pos.v[1] - rest_positions[LFOOT].v[1]);
    float rdiff = fabs(rfoot.pos.v[1] - rest_positions[RFOOT].v[1]);

    ///so, because the sound isn't tracking
    ///after its played, itll then be exposed to the 3d audio system
    vec3f centre = (lfoot.pos + rfoot.pos) / 2.f;

    centre = parts[BODY].global_pos;

    float cutoff = 1.f;

    ///need to exclude jumping in some other way
    if(ldiff < cutoff)
    {
        if(!left_foot_sound)
        {
            sound::add(current_num + asphalt_start, centre, is_relative);

            current_num = (current_num + 1) % foot_nums;

            left_foot_sound = true;
        }
    }
    else
    {
        left_foot_sound = false;
    }

    if(rdiff < cutoff)
    {
        if(!right_foot_sound)
        {
            sound::add(current_num + asphalt_start, centre, is_relative);

            current_num = (current_num + 1) % foot_nums;

            right_foot_sound = true;
        }
    }
    else
    {
        right_foot_sound = false;
    }

    if(jump_info.should_play_foot_sounds)
    {
        left_foot_sound = true;
        right_foot_sound = true;

        sound::add(current_num + asphalt_start, centre, is_relative);

        current_num = (current_num + 1) % foot_nums;

        sound::add(current_num + asphalt_start, centre, is_relative);

        current_num = (current_num + 1) % foot_nums;

        jump_info.should_play_foot_sounds = false;

        suppress_foot_sounds = true;
        time_to_suppress_foot_sounds_s = 0.2f;
        foot_supression_timer.restart();
    }
}

void fighter::set_stance(int _stance)
{
    stance = _stance;
}

///so type is limb
bool fighter::can_attack(bodypart_t type)
{
    ///find the last attack of the current type
    ///if its going, and within x ms of finishing, then fine

    int going_pos = -1;
    int any_queued = -1;

    for(int i=0; i<moves.size(); i++)
    {
        if(moves[i].limb == type && moves[i].going)
        {
            going_pos = i;
        }

        if(moves[i].limb == type)
            any_queued = i;

        if(moves[i].limb && moves[i].does(mov::NO_POST_QUEUE))
            return false;
    }

    ///nothing going, we can queue attack
    if(any_queued == -1)
        return true;

    if(going_pos == -1)
        return false;

    float queue_time = moves[going_pos].time_remaining();

    for(int i=going_pos + 1; i<moves.size(); i++)
    {
        if(moves[i].limb == type)
        {
            queue_time += moves[i].end_time;
        }
    }

    const float threshold = 500;

    if(queue_time < threshold)
        return true;

    return false;
}

movement* fighter::get_current_move(bodypart_t type)
{
    for(int i=0; i < moves.size(); i++)
    {
        if(moves[i].limb == type && moves[i].going)
        {
            return &moves[i];
        }
    }

    return nullptr;
}

///this function assumes that an attack movelist keeps a consistent bodypart
void fighter::queue_attack(attack_t type)
{
    if(dead())
        return;

    attack a = attack_list[type];

    if(a.moves.size() > 0 && !can_attack(a.moves.front().limb))
    {
        movement* cmove = get_current_move(a.moves.front().limb);

        if(cmove && cmove->does(mov::CONTINUOUS_SPRINT))
        {
            cmove->was_set_this_frame = 1;
        }

        return;
    }

    for(auto i : a.moves)
    {
        ///this is probably a mistake to initialise this here
        i.damage = attacks::damage_amounts[type];

        i.was_set_this_frame = 1;

        add_move(i);
    }
}

void fighter::add_move(const movement& m)
{
    moves.push_back(m);
}

void fighter::try_jump()
{
    if(!jump_info.is_jumping)
    {
        jump_info.is_jumping = true;

        jump_info.current_time = 0;

        jump_info.pre_jump_pos = pos;
    }
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

        vec3f rot = avg.get_euler() + sword_rotation_offset;

        weapon.set_rot(rot);

        parts[LHAND].set_rot(rot);
        parts[RHAND].set_rot(rot);

        #ifdef TPOSE
        parts[bodypart::RHAND].rot = (parts[bodypart::RHAND].pos - parts[bodypart::RLOWERARM].pos).get_euler();
        parts[bodypart::LHAND].rot = (parts[bodypart::LHAND].pos - parts[bodypart::LLOWERARM].pos).get_euler();
        #endif // TPOSE
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

///need to clamp this while attacking
void fighter::set_rot_diff(vec3f diff)
{
    ///check movement list
    ///if move going
    ///and does damage
    ///clamp
    float max_angle_while_damaging = 6.f * frametime / 1000.f;

    float yangle_diff = diff.v[1];

    for(auto& i : moves)
    {
        if(!i.going)
            continue;

        if((i.does(mov::DAMAGING) || i.does(mov::WINDUP)) && !i.does(mov::NO_CAMERA_LIMIT))
        {
            yangle_diff = clamp(yangle_diff, -max_angle_while_damaging, max_angle_while_damaging);
        }
    }

    diff.v[1] = yangle_diff;

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

    return {n_pos, total_rot};
}

void smooth(vec3f& in, vec3f old, float dt)
{
    float diff = (in - old).length();

    if(diff > 100.f)
    {
        return;
    }

    vec3f vec_frac = {40.f, 35.f, 40.f};

    vec3f diff_indep = (in - old);

    diff_indep = (diff_indep / vec_frac) * dt;

    in = old + diff_indep;
}

void smooth(float& in, float old, float dt)
{
    float diff = fabs(in - old);

    if(diff > 100.f)
    {
        return;
    }

    float frac = 35;

    float diff_indep = (in - old);

    diff_indep = (diff_indep / frac) * dt;

    in = old + diff_indep;
}

///note to self, make this not full of shit
///smoothing still isn't good
void fighter::update_render_positions()
{
    using namespace bodypart;

    if(!just_spawned)
    {
        smooth(parts[LLOWERARM].pos, old_pos[LLOWERARM], frametime);
        smooth(parts[RLOWERARM].pos, old_pos[RLOWERARM], frametime);

        smooth(parts[RUPPERARM].pos, old_pos[RUPPERARM], frametime);
        smooth(parts[LUPPERARM].pos, old_pos[LUPPERARM], frametime);

        smooth(parts[BODY].pos, old_pos[BODY], frametime);

        float s1 = smoothed_crouch_offset;

        smooth(s1, smoothed_crouch_offset_old, frametime);

        smooth(smoothed_crouch_offset, s1, frametime);
    }


    just_spawned = false;

    float lfoot_extra_bob = 0;
    float rfoot_extra_bob = 0;

    ///ehh.... makes everything a bit too wobby. Review this when I'm less tired
    /*if(game_state != nullptr)
    {
        lfoot_extra_bob = -game_state->current_map.get_ground_height(parts[LFOOT].global_pos) - pos.v[1];
        rfoot_extra_bob = -game_state->current_map.get_ground_height(parts[RFOOT].global_pos) - pos.v[1];
    }*/

    std::map<int, float> foot_heights;

    ///bob OPPOSITE side of body
    ///we're getting some pretty aggressive popping on strafe, needs fixing possibly with smoothing
    float l_bob = parts[RFOOT].pos.v[1] - rest_positions[RFOOT].v[1];
    float r_bob = parts[LFOOT].pos.v[1] - rest_positions[LFOOT].v[1];

    l_bob += lfoot_extra_bob;
    r_bob += rfoot_extra_bob;

    foot_heights[0] = l_bob * 0.7 + r_bob * 0.3;
    foot_heights[1] = l_bob * 0.3 + r_bob * 0.7;
    foot_heights[2] = (foot_heights[0] + foot_heights[1]) / 2.f;

    std::map<int, float> foot_displacements;

    float l_f = parts[LFOOT].pos.v[2] - rest_positions[LFOOT].v[2];
    float r_f = parts[RFOOT].pos.v[2] - rest_positions[RFOOT].v[2];

    foot_displacements[0] = l_f;
    foot_displacements[1] = r_f;
    foot_displacements[2] = (l_f + r_f) / 2.f;

    //for(part& i : parts)
    for(int kk=0; kk<parts.size(); kk++)
    {
        part& i = parts[kk];

        /*if(kk == RHAND && rhand_overridden)
        {
            rhand_overridden = false;
            continue;
        }*/

        vec3f t_pos = i.pos;

        t_pos.v[1] += foot_heights[which_side[i.type]] * foot_modifiers[i.type] * overall_bob_modifier;

        float idle_parameter = sin(idle_offsets[i.type] + idle_speed * my_time / 1000.f);

        t_pos.v[1] += idle_modifiers[i.type] * idle_height * idle_parameter;

        float twist_extra = shoulder_rotation * waggle_modifiers[i.type];

        t_pos = t_pos.rot({0,0,0}, {0, twist_extra, 0});

        //if(is_sprinting)
        float zsprint = foot_displacements[which_side[i.type]] * forward_thrust_hip_relative[i.type] * overall_forward_thrust_mod;
        float znsprint = foot_displacements[which_side[i.type]] * forward_thrust_hip_relative[i.type] * overall_forward_thrust_mod_nonsprint;

        float zextra = zsprint * sprint_frac + znsprint * (1.f - sprint_frac);

        t_pos.v[2] += zextra;

        auto r = to_world_space(pos, rot, t_pos, i.rot);

        i.set_global_pos({r.pos.v[0], r.pos.v[1], r.pos.v[2]});
        i.set_global_rot({r.rot.v[0], r.rot.v[1], r.rot.v[2]});

        i.update_model();
    }

    ///the above headlook never worked

    {
        part& p = parts[HEAD];

        vec3f rvec = default_position[HEAD] - default_position[BODY];

        vec3f wvec = (p.pos - parts[BODY].pos);

        ///camera bob
        float vanilla_v1 = (default_position[HEAD] - default_position[BODY]).v[1];
        float with_bob_v1 = (parts[HEAD].global_pos - parts[BODY].global_pos).v[1];
        float extra = with_bob_v1 - vanilla_v1;
        vec3f extra_bob = {0, -extra, 0};
        camera_bob = extra_bob;

        rvec = wvec;

        vec3f base = parts[BODY].global_pos;


        float look_angle = look.v[0];

        ///-look_angle/2.f

        vec3f r1 = rvec.rot({0,0,0}, {-look_angle/2.f, 0, 0});

        vec3f total_rot = {rot.v[0], rot.v[1], rot.v[2]};

        vec3f rotated_vec = r1.rot({0,0,0}, total_rot);

        mat3f a1;
        a1.load_rotation_matrix((vec3f){-look_angle/2.f, 0, 0});

        mat3f a2;
        a2.load_rotation_matrix(rot);

        mat3f a3 = a2 * a1;

        vec3f angles = a3.get_rotation();

        p.set_global_pos(rotated_vec + base + extra_bob);
        p.set_global_rot(angles);

        p.update_model();
    }

    if(game_state != nullptr)
    {
        ///take normal, turn normal to axis angle, rotate foot about that. Ez
        float lfoot_extra = -game_state->current_map.get_ground_height(parts[LFOOT].global_pos);
        float rfoot_extra = -game_state->current_map.get_ground_height(parts[RFOOT].global_pos);

        vec3f lglobal = parts[LFOOT].global_pos;
        vec3f rglobal = parts[RFOOT].global_pos;

        lglobal.v[1] += lfoot_extra - pos.v[1];
        rglobal.v[1] += rfoot_extra - pos.v[1];

        parts[LFOOT].set_global_pos(lglobal);
        parts[RFOOT].set_global_pos(rglobal);

        parts[LFOOT].update_model();
        parts[RFOOT].update_model();
    }

    auto r = to_world_space(pos, rot, weapon.pos, weapon.rot);


    weapon.model->set_pos({r.pos.v[0], r.pos.v[1], r.pos.v[2]});
    weapon.model->set_rot({r.rot.v[0], r.rot.v[1], r.rot.v[2]});

    ///calculate distance between links, dynamically adjust positioning
    ///so there's equal slack on both sides
    recalculate_link_positions_from_parts();

    ///sent rhand to point towards the weapon centre from its centre

    for(int i=0; i<bodypart::COUNT; i++)
    {
        old_pos[i] = parts[i].pos;
    }

    smoothed_crouch_offset_old = smoothed_crouch_offset;
}

void fighter::recalculate_link_positions_from_parts()
{
    for(auto& i : joint_links)
    {
        objects_container* obj = i.obj;

        /*vec3f forw = (vec3f){0, 0, -1}.rot(0.f, rot);
        vec3f perp = forw.rot(0.f, {0, M_PI/2, 0});

        vec4f axis_angle = (vec4f){perp.x(), perp.y(), perp.z(), M_PI/2.f};

        quaternion q;
        q.load_from_axis_angle(axis_angle);

        mat3f m = q.get_rotation_matrix();

        vec3f up = m * forw;*/

        vec3f start = i.p1->global_pos;
        vec3f fin = i.p2->global_pos;

        float desired_len = (fin - start).length();
        float real_len = i.length;

        float diff = 0.f;

        if(desired_len > real_len)
            diff = desired_len - real_len;

        start = start + i.offset;
        fin = fin + i.offset;

        vec3f rot = (fin - start).get_euler();

        vec3f dir = (fin - start);

        start = start + dir.norm() * diff/2.f;

        obj->set_pos({start.v[0], start.v[1], start.v[2]});
        obj->set_rot({rot.v[0], rot.v[1], rot.v[2]});

        //obj->set_rot_quat(look_at_quat(dir, up));

        ///-rot here wrong because we redefined!
        //quaternion izquat;
        //izquat.load_from_axis_angle({0, 1, 0, -rot.v[1]});

        /*vec4f rev = {dir.v[0], dir.v[1], dir.v[2], -rot.v[1]};

        quaternion izquat;
        izquat.load_from_axis_angle(rev);

        quaternion nq = look_at_quat(dir, up);

        obj->set_rot_quat(izquat * nq);*/

        /*vec3f up = {0, 1, 0};

        float angle = acos(dot(dir.norm(), up));

        float yangle = atan2(dir.v[2], dir.v[0]);

        mat3f yrot = mat3f().YRot(yangle);
        //mat3f yrot = mat3f().identity();

        vec3f axis = cross(dir.norm(), {0, 1, 0}).norm();

        vec4f aa = {axis.x(), axis.y(), axis.z(), -angle};

        quaternion ib;

        if(axis.length() > 0.001f);
            ib.load_from_axis_angle(aa);

        quaternion nq;
        nq.load_from_matrix(yrot);*/

        //obj->set_rot_quat(ib * nq);
    }
}

void fighter::update_headbob_if_sprinting(bool sprinting)
{
    float dir = sprinting ? 1 : -1;

    if(last_walk_dir.v[0] >= 0)
        dir = -1;

    float modif = 100.f;
    float max_camera = 1.5f;

    camera_bob_mult += dir * frametime / modif;
    camera_bob_mult = clamp(camera_bob_mult, 0.f, max_camera);
}

void fighter::position_cosmetics()
{
    vec3f hpos = parts[bodypart::HEAD].global_pos;

    ///partscale = scale/3.f

    vec3f offset = hpos + (vec3f){0, bodypart::scale/3.f, 0};

    cosmetic.tophat->set_pos({offset.v[0], offset.v[1], offset.v[2]});
}

void fighter::network_update_render_positions()
{
    using namespace bodypart;

    for(auto& i : joint_links)
    {
        bool active = i.p1->is_active && i.p2->is_active;

        objects_container* obj = i.obj;

        vec3f start = i.p1->global_pos;
        vec3f fin = i.p2->global_pos;

        float desired_len = (fin - start).length();
        float real_len = i.length;

        float diff = 0.f;

        if(desired_len > real_len)
            diff = desired_len - real_len;

        start = start + i.offset;
        fin = fin + i.offset;

        vec3f rot = (fin - start).get_euler();

        vec3f dir = (fin - start);

        //start = start + dir * i.squish_frac;
        start = start + dir.norm() * diff/2.f;

        obj->set_pos({start.v[0], start.v[1], start.v[2]});
        obj->set_rot({rot.v[0], rot.v[1], rot.v[2]});
    }
}

///need to fixme
void fighter::update_lights()
{
    vec3f lpos = (parts[bodypart::LFOOT].global_pos + parts[bodypart::RFOOT].global_pos) / 2.f;

    ///+40 is wrong but... eh
    my_lights[0]->set_pos({lpos.v[0], lpos.v[1] + 40.f, lpos.v[2]});

    ///global_rot and a global pos here?
    ///maybe we should transform externally as well
    ///this one incorrect for on walls due to pos
    ///take head->body vector and use that
    vec3f body = parts[bodypart::BODY].global_pos;
    vec3f head = parts[bodypart::HEAD].global_pos;
    vec3f rarm = parts[bodypart::RUPPERARM].global_pos;

    vec3f body_to_head = head - body;
    vec3f body_to_right = rarm - body;

    vec3f in_front = cross(body_to_head.norm(), body_to_right.norm());

    //vec3f bpos = in_front.norm() * 150.f + body;
    vec3f bpos = body_to_head.norm() * 50.f + body + in_front.norm() * 300;

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

    my_lights[1]->set_radius(2000.f);
    my_lights[2]->set_radius(400.f);

    for(auto& i : my_lights)
    {
        vec3f col = team_info::get_team_col(team) / 255.f;

        i->set_col({col.v[0], col.v[1], col.v[2]});
    }

    for(auto& i : my_lights)
    {
        vec3f pos = xyz_to_vec(i->pos);

        ///dirty hack of course
        ///ideally we'd use the alive status for this
        ///but that'd break the death effects
        if(pos.length() > 100000.f)
        {
            i->set_active(false);
        }
        else
        {
            i->set_active(true);
        }
    }
}

void fighter::respawn_if_appropriate()
{
    //printf("respawn %i %i\n", num_dead(), num_needed_to_die());

    ///only respawns if on full health
    ///easiest way to get around some of the delayed dying bugs
    ///could probably still cause issues if we go from full health to 0
    ///in almost no time
    for(auto& i : parts)
    {
        if(i.hp < 0.9999f)
        {
            return;
        }
    }

    //if(num_dead() < num_needed_to_die())
    {
        if(performed_death)
        {
            ///this will still bloody network all the hp changes
            respawn();

            ///hack to stop it from networking the hp change
            ///from respawning the network fighter
            ///I think we no longer need to do this
            ///we probably want to clear any delayed deltas etc
            ///although the respawn time means its probably not applicable
            /*for(auto& i : parts)
            {
                i.net.damage_info.hp_delta = 0.f;
                i.net.hp_dirty = false;
            }*/



            //printf("respawning other playern\n");
        }
    }
}

void fighter::save_old_pos()
{
    for(int i=0; i<bodypart::COUNT; i++)
    {
        if(parts[i].global_pos != old_pos[i])
            old_pos[i] = parts[i].global_pos;
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

    ///pos.v[1] is not correct I don't think
    ///but im not sure that matters
    //pos = parts[bodypart::BODY].global_pos;
    //rot = parts[bodypart::BODY].global_rot;

    ///this is so that if the remote fighter is actually alive but we don't know
    ///it can die again
    ///we could avoid this by networking the death state
    ///but that requries more networking. We already have the info, so do it clientside
    ///hp for parts are also 'reliably' delivered (every frame)
    /*if(!should_die())
    {
        performed_death = false;
    }*/

    ///do not need to update weapon because it does not currently have a global position stored (not a part)
}

void fighter::update_texture_by_part_hp()
{
    for(auto& i : parts)
    {
        i.update_texture_by_hp();
    }
}

void fighter::update_last_hit_id()
{
    int32_t last_id = -1;

    //for(auto& i : parts)
    for(int ii=0; ii<parts.size(); ii++)
    {
        part& local_part = parts[ii];
        network_part_info& net_part = net_fighter_copy->network_parts[ii];

        damage_info& dinfo = net_part.requested_damage_info.networked_val;

        //if(i.net.damage_info.hp_delta != 0.f)
        if(dinfo.hp_delta != 0)
        {
            //i.net.damage_info.hp_delta = 0.f;

            if(last_id != -1)
                lg::log("potential conflict in last id hit, update_last_hit_id()");

            if(dinfo.id_hit_by == -1)
            {
                ///continue
                ///but this might be broken currently
                ///wait
                ///this does not affect anything other than kill reporting!
                ///we can safely skip and 0-ify hp loss?
                ///or maybe lets just skip

                continue;
            }

            last_id = dinfo.id_hit_by;

            player_id_i_was_last_hit_by = last_id;

            lg::log("updated last hit id");
        }
    }
}

///we need to check the case where i've definitely been stabbed on my game
///check if this fighter has hit the non networked fighter
void fighter::check_clientside_parry(fighter* non_networked_fighter)
{
    ///this needs to be a more complex check if the non networked fighter has any pending hits from this client
    ///but its still crap, we might miss a frame or two in the interim if some of the packets are jittered
    //if(net.is_damaging)

    bool any_pending_hits = net.is_damaging;

    for(auto& p : non_networked_fighter->parts)
    {
        for(delayed_delta& d : p.net.delayed_delt)
        {
            if(d.delayed_info.id_hit_by == this->network_id)
            {
                any_pending_hits = true;

                lg::log("Pending!");
            }
        }
    }

    ///ok, so if we've got a pending hit, we need to override the block config
    if(any_pending_hits)
    {
        //printf("we're damaging network\n");

        vec3f move_dir = parts[bodypart::LHAND].global_pos - old_pos[bodypart::LHAND];

        //printf("MD %f %f %f\n", EXPAND_3(move_dir));

        ///this is currently checking clientside parries against all players, whereas we only want to check
        ///this networked fighter against the local player
        ///recoil and stuff needs to be disabled unless its the correct fighter we're hitting :[
        int hit_id = phys->sword_collides(weapon, this, move_dir.norm().back_rot({0,0,0}, rot), false, false, non_networked_fighter);

        ///technically we've detected a clientside hit, but we're actually ONLY looking for parries
        ///because attacks want to be attacker authoratitive, and parries want to be client authoratitive
        ///a successful clientside parry should make you invincible from one players damage for the ping latency * 2  + jitter + fudge
        ///but this can be up to 400ms, which is unfortunate
        ///so how to handle overriding the damage? wait for the attacker to confirm? on the client? delay the damage for a fixed amount and negotiate
        ///????????
        ///if the hit ID == bodypart id hit in pending hits up above, then we've definitely been hit
        ///unless we've got a clientside parry FIRST
        ///add this as a tag in the delayed delta, so we can delayed_delta.is_definitely_hit = 1; then override
        if(hit_id == -2)
        {

            ///fighter* their_parent = phys->bodies[i.hit_id].parent

            ///int bodypart_id = (bodypart_t)(i.hit_id % COUNT)

            bool success = false;

            for(auto& p : non_networked_fighter->parts)
            {
                for(auto& d : p.net.delayed_delt)
                {
                    if(d.delayed_info.id_hit_by == network_id)
                    {
                        if(d.part_hit_before_block)
                            continue;

                        ///no need to go on about it
                        ///ensure we don't generate multiple clientside parries
                        ///for one delayed delta+
                        if(d.part_blocked)
                            continue;

                        d.part_blocked = true;

                        success = true;
                    }
                }
            }

            if(success)
            {
                lg::log("clientside parry");

                local.play_clang_audio = 1;
                local.send_clang_audio = 1;

                clientside_parry_info inf;
                inf.player_id_i_parried = this->network_id;

                non_networked_fighter->clientside_parry_inf.push_back(inf);
            }
            else
            {
                lg::log("We've detected a hit already, too late");
            }
       }

        //int bodypart_id = hit_id % COUNT;

        if(hit_id >= 0)
        {
            //for(auto& p : non_networked_fighter->parts)
            for(int ii = 0; ii < non_networked_fighter->parts.size(); ii++)
            {
                ///we don't care WHICH part is hit, just that the local client detected a block
                part& p = non_networked_fighter->parts[ii];

                for(int kk=0; kk < p.net.delayed_delt.size(); kk++)
                {
                    delayed_delta& d = p.net.delayed_delt[kk];

                    if(d.delayed_info.id_hit_by == network_id)
                    {
                        if(d.part_blocked)
                            continue;

                        d.part_hit_before_block = true;
                    }
                }
            }
        }
    }
}

///this is on my client
///we need to recoil the hitter client
void fighter::process_delayed_deltas()
{
    //for(auto& i : parts)
    for(int kk=0; kk<parts.size(); kk++)
    {
        part& local_part = parts[kk];
        //network_part_info& net_part = net_fighter_copy->network_parts[kk];

        for(int j=0; j<local_part.net.delayed_delt.size(); j++)
        {
            delayed_delta& delt = local_part.net.delayed_delt[j];

            float RTT = delt.delay_time_ms;

            float half_time = RTT / 2.f;

            ///time to apply delayed delta
            if(delt.clk.getElapsedTime().asMicroseconds() / 1000.f >= half_time)
            {
                lg::log("Processing delayed hit with RTT/2 ", half_time, " and time clock ", delt.clk.getElapsedTime().asMicroseconds() / 1000.f);

                lg::log("Damage ", delt.delayed_info.hp_delta);

                bool apply_damage = true;

                for(int k=0; k<clientside_parry_inf.size(); k++)
                {
                    clientside_parry_info& info = clientside_parry_inf[k];

                    ///on the clientside, i have parried this attack
                    ///do not apply damage
                    if(info.player_id_i_parried == delt.delayed_info.id_hit_by)
                    {
                        apply_damage = false;
                    }
                }

                ///ok, so I'm pretty sure this only gets called once, on the hit client
                ///so flinch should simply set the broadcast sound noise, not clientside parry!
                ///play sounds here, as this is the ultimate arbiter of what happened?
                ///except for an agreed block!
                if(apply_damage)
                {
                    local_part.hp += delt.delayed_info.hp_delta;
                    //net_part.hp += delt.delayed_info.hp_delta;

                    ///hmm, so this is delayed
                    ///so flinch will apply after the ping delay
                    flinch(fighter_stats::flinch_time_ms);

                    lg::log("Applied delayed damage");
                }
                else
                {
                    lg::log("Did not apply delayed damage due to client parry");
                }

                local_part.net.delayed_delt.erase(local_part.net.delayed_delt.begin() + j);
                j--;
            }
        }
    }
}

///eliminate damage taken beacuse I am a clientside parry man now
void fighter::eliminate_clientside_parry_invulnerability_damage()
{
    //for(auto& i : parts)
    for(int i=0; i<parts.size(); i++)
    {
        part& p = parts[i];

        for(int j=0; j<clientside_parry_inf.size(); j++)
        {
            clientside_parry_info inf = clientside_parry_inf[j];

            float time = inf.clk.getElapsedTime().asMicroseconds() / 1000.f;

            if(time >= inf.max_invuln_time_ms)
            {
                lg::log("clientside parry expired");

                clientside_parry_inf.erase(clientside_parry_inf.begin() + j);
                j--;
                continue;
            }

            ///set i.local.play_hit_audio to false
            ///but ignore that for the moment, useful for testing
            ///uuh. We really need to stash hp deltas into an array of damage_infos
            ///and then check and apply them later
            ///atm this might break any damage received in a parry window + RTT/2, which is unacceptable
            /*if(inf.player_id_i_parried == p.net.damage_info.id_hit_by)
            {
                p.net.damage_info.hp_delta = 0.f;

                lg::log("eliminated hit damage due to clientside parry from playerid ", p.net.damage_info.id_hit_by);
            }*/

            ///current part is part[i]
            for(int kk=0; kk<p.net.delayed_delt.size(); kk++)
            {
                if(p.net.delayed_delt[kk].delayed_info.id_hit_by == inf.player_id_i_parried)
                {
                    p.net.delayed_delt.erase(p.net.delayed_delt.begin() + kk);
                    kk--;

                    lg::log("eliminated hit damage due to clientside parry from playerid ", inf.player_id_i_parried);

                    continue;
                }
            }
        }
    }
}

void fighter::set_network_id(int32_t net_id)
{
    network_id = net_id;
}

///only for the local player eh
void fighter::save_network_representation(network_fighter& net_fight)
{
    *net_fighter_copy = net_fight;

    for(int i=0; i<bodypart::COUNT; i++)
    {
        net_fighter_copy->network_parts[i].requested_damage_info.update_internal();
    }

    net_fighter_copy->network_fighter_inf.recoil_forced.update_internal();
    net_fighter_copy->network_fighter_inf.recoil_requested.update_internal();
}

///remember, we have to copy my values into the other struct before we can send them!
network_fighter fighter::get_modified_network_fighter()
{
    return *net_fighter_copy;
}

///me to my network representation
///this doesnt include hp deltas
network_fighter fighter::construct_network_fighter()
{
    network_fighter ret;

    for(int i=0; i<bodypart::COUNT; i++)
    {
        network_part_info& current = ret.network_parts[i];

        current.global_pos = parts[i].global_pos;
        current.global_rot = parts[i].global_rot;

        current.hp = parts[i].hp;
        current.play_hit_audio = parts[i].net.play_hit_audio;
    }

    network_sword_info& sword_info = ret.network_sword;

    sword_info.global_pos = xyz_to_vec(weapon.obj()->pos);
    sword_info.global_rot = xyz_to_vec(weapon.obj()->rot);

    sword_info.is_blocking = net.is_blocking;
    sword_info.is_damaging = net.is_damaging;
    //sword_info.recoil_requested = net.recoil;
    //sword_info.recoil_forced = net.force_recoil;

    network_fighter_info& fighter_info = ret.network_fighter_inf;

    fighter_info.is_dead = dead();
    fighter_info.pos = pos;
    fighter_info.rot = rot;

    int nlength = std::min((int)local_name.size(), MAX_NAME_LENGTH-1);

    memset(&fighter_info.name.v[0], 0, MAX_NAME_LENGTH);

    if(local_name.size() > 0)
        memcpy(&fighter_info.name.v[0], &local_name[0], local_name.size());

    return ret;
}

///network representation constructed to meet the requirements of a client actor
///I think we should store the other network representations in the fighter itself
///so that we can have one we can mod, one raw, and one authoritative
///or store it in the network fighter and keep a ptr?
///Or, keep it in a clunky struct and access through getters/setters to make it easier
///to replace?
void fighter::construct_from_network_fighter(network_fighter& net_fight)
{
    save_old_pos();

    ///we'll need to construct quite a few of these into net. for the time being, including name
    for(int i=0; i<bodypart::COUNT; i++)
    {
        network_part_info& current = net_fight.network_parts[i];

        parts[i].set_global_pos(current.global_pos);
        parts[i].set_global_rot(current.global_rot);
        parts[i].hp = current.hp;

        parts[i].update_model();

        ///reset the to_send, reset internal values to either network, or 0
        net_fight.network_parts[i].requested_damage_info.update_internal();
    }

    network_sword_info& sword_info = net_fight.network_sword;

    weapon.set_pos(sword_info.global_pos);
    weapon.set_rot(sword_info.global_rot);

    weapon.update_model();

    net.is_blocking = sword_info.is_blocking;
    net.is_damaging = sword_info.is_damaging;

    network_fighter_info& fighter_info = net_fight.network_fighter_inf;

    pos = fighter_info.pos;
    rot = fighter_info.rot;

    ///this is of length MAX_NAME_LENGTH + 1
    fighter_info.name.v[MAX_NAME_LENGTH] = 0;

    int str_length = strlen((const char*)&fighter_info.name.v[0]);

    ///is this just paranoia?
    str_length = std::min(str_length, MAX_NAME_LENGTH);

    local_name.clear();

    for(int i=0; i<str_length; i++)
    {
        local_name.push_back(fighter_info.name.v[i]);
    }

    *net_fighter_copy = net_fight;

    net_fighter_copy->network_fighter_inf.recoil_forced.update_internal();
    net_fighter_copy->network_fighter_inf.recoil_requested.update_internal();
}

void fighter::modify_existing_network_fighter_with_local(network_fighter& net_fight)
{
    net_fight.network_fighter_inf.recoil_forced = net_fighter_copy->network_fighter_inf.recoil_forced;
    net_fight.network_fighter_inf.recoil_requested = net_fighter_copy->network_fighter_inf.recoil_requested;

    for(int i=0; i<parts.size(); i++)
    {
        net_fight.network_parts[i].requested_damage_info = net_fighter_copy->network_parts[i].requested_damage_info;
    }
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

    float squish = 0.1f;

    ///now we know the team, we can add the joint parts
    using namespace bodypart;

    ///I think the problem with this is that make_link makes between the real positions
    ///not the default positions
    ///which is pretty tard
    joint_links.push_back(make_link(&parts[LUPPERARM], &parts[LLOWERARM], team, squish));
    joint_links.push_back(make_link(&parts[LLOWERARM], &parts[LHAND], team, squish));

    joint_links.push_back(make_link(&parts[RUPPERARM], &parts[RLOWERARM], team, squish));
    joint_links.push_back(make_link(&parts[RLOWERARM], &parts[RHAND], team, squish));

    joint_links.push_back(make_link(&parts[LUPPERARM], &parts[BODY], team, squish));
    joint_links.push_back(make_link(&parts[RUPPERARM], &parts[BODY], team, squish));

    joint_links.push_back(make_link(&parts[LUPPERLEG], &parts[LLOWERLEG], team, squish));
    joint_links.push_back(make_link(&parts[RUPPERLEG], &parts[RLOWERLEG], team, squish));

    joint_links.push_back(make_link(&parts[LLOWERLEG], &parts[LFOOT], team, squish));
    joint_links.push_back(make_link(&parts[RLOWERLEG], &parts[RFOOT], team, squish));

    for(auto& i : joint_links)
    {
        i.obj->set_active(true);
    }

    cpu_context->load_active();

    for(auto& i : joint_links)
    {
        i.obj->set_specular(bodypart::specular);
    }

    cpu_context->build_request();

    lg::log("set_team");
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

bool fighter::can_windup_recoil()
{
    movement lhand = action_map[bodypart::LHAND];
    movement rhand = action_map[bodypart::RHAND];

    if(lhand.does(mov::WINDUP) || rhand.does(mov::WINDUP))
    {
        return true;
    }

    return false;
}

bool fighter::currently_recoiling()
{
    movement lhand = action_map[bodypart::LHAND];
    movement rhand = action_map[bodypart::RHAND];

    if(!lhand.does(mov::IS_RECOIL) && !rhand.does(mov::IS_RECOIL))
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
    const float unfeintable_time = attacks::unfeintable_time;

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

void fighter::override_rhand_pos(vec3f global_position)
{
    rhand_overridden = true;
    rhand_override_pos = global_position;
}

///i've taken damage. If im during the windup phase of an attack, recoil
void fighter::damage(bodypart_t type, float d, int32_t network_id_hit_by, bool hit_by_offline_client)
{
    using namespace bodypart;

    bool do_explode_effect = num_dead() < num_needed_to_die() - 1;

    parts[type].damage(this, d, do_explode_effect, network_id_hit_by);

    if(hit_by_offline_client)
    {
        parts[type].set_hp(parts[type].hp - d);
        flinch(fighter_stats::flinch_time_ms);
    }

    lg::log("network hit id", network_id_hit_by);

    player_id_i_was_last_hit_by = network_id_hit_by;

    net_fighter_copy->network_fighter_inf.recoil_requested.set_local_val(1);
    net_fighter_copy->network_fighter_inf.recoil_requested.network_local();

    //net.recoil = 1;
    //net.recoil_dirty = true;
}

///implement camera shake effect here
void fighter::flinch(float time_ms)
{
    reset_screenshake_flinch = true;
}

void fighter::set_contexts(object_context* _cpu, object_context_data* _gpu)
{
    if(_cpu)
        cpu_context = _cpu;

    if(_gpu)
        gpu_context = _gpu;
}

///problem is, whenever someone's name gets updated, everyone else's old name gets overwritten
///we need to update all fighters whenever there's an update, or a new fighter created
void fighter::set_name(std::string name)
{
    if(!name_tex_gpu)
        return;

    std::string ping_info = "\nPing: " + std::to_string((int)net.ping);

    local_name = name;

    vec2f fname = {name_dim.x, name_dim.y};

    name_tex.clear(sf::Color(0,0,0,0));
    name_tex.display();

    name_tex.setActive(true);

    text::immediate(&name_tex, local_name + ping_info, fname/2.f, 16, true);

    name_tex.display();

    name_tex.setActive(false);

    gpu_name_dirty = 0;
}

void fighter::update_gpu_name()
{
    if(!name_tex_gpu)
        return;

    if(gpu_name_dirty >= 1)
        return;

    name_tex.setActive(true);

    name_tex_gpu->update_gpu_texture(name_tex.getTexture(), transparency_context->fetch()->tex_gpu_ctx, false, cl::cqueue2);
    name_tex_gpu->update_gpu_mipmaps(transparency_context->fetch()->tex_gpu_ctx, cl::cqueue2);

    cl::cqueue2.flush();

    name_tex.setActive(false);

    gpu_name_dirty++;
}

void fighter::set_secondary_context(object_context* _transparency_context)
{
    if(_transparency_context == nullptr)
    {
        lg::log("massive error in set_secondary_context");

        exit(44334);
    }

    if(name_container)
    {
        name_container->set_active(false);
    }

    transparency_context = _transparency_context;

    name_tex.create(name_dim.x, name_dim.y);
    name_tex.clear(sf::Color(0, 0, 0, 0));
    name_tex.display();

    ///destroy name_tex_gpu

    name_tex_gpu = _transparency_context->tex_ctx.make_new();
    name_tex_gpu->set_location("Res/128x128.png");

    name_container = transparency_context->make_new();
    name_container->set_load_func(std::bind(obj_rect, std::placeholders::_1, *name_tex_gpu, name_obj_dim));

    name_container->set_unique_textures(true);
    name_container->cache = false;
    name_container->set_active(true);

    transparency_context->load_active();

    name_container->set_two_sided(true);
    name_container->set_diffuse(10.f);

    transparency_context->build();
}

void fighter::update_name_info(bool networked_fighter)
{
    if(!name_container)
        return;

    vec3f head_pos = parts[bodypart::HEAD].global_pos;

    float offset = bodypart::scale;

    name_container->set_pos({head_pos.v[0], head_pos.v[1] + offset, head_pos.v[2]});
    name_container->set_rot({rot.v[0], rot.v[1], rot.v[2]});

    if(!name_tex_gpu)
        return;

    if(name_reset_timer.getElapsedTime().asMilliseconds() > (4000.f + rand_offset_ms) || !name_info_initialised)
    {
        ///we've got the correct local name, but it wont blit for some reason
        std::string str = local_name;

        if(str == "")
            str = "Invalid Name";

        set_name(str);

        //if(networked_fighter)
        //    lg::log("Fighter network name", str);

        name_reset_timer.restart();

        name_info_initialised = true;
    }
}

///I should probably stop the hand hits playing audio, or at least make it special
///just sounds wrong atm
void fighter::check_and_play_sounds(bool player)
{
    vec3f hpos = parts[bodypart::HEAD].global_pos;

    for(auto& i : parts)
    {
        ///we're going to need a toggle to say we've played this sound already, dont repeat
        if(i.local.play_hit_audio || i.net.play_hit_audio)
        {
            vec3f pos = i.global_pos;

            sound::add(0, pos);

            i.local.play_hit_audio = 0;

            i.net.play_hit_audio = 0;

            clang_light_effect hit_effect;
            hit_effect.make(400.f, pos, (vec3f){10.f, 10.f, 10.f}, 200.f);

            particle_effect::push(hit_effect);
        }
    }

    if(local.play_clang_audio || net.play_clang_audio)
    {
        vec3f pos = xyz_to_vec(weapon.model->pos);

        if(player)
        {
            ///disable locational clang as its a bit loud and annoying. Could add a bit of locational, but later
            sound::add(1, hpos + (vec3f){0, 0, -100}.rot(0, rot), true);
        }
        else
        {
            sound::add(1, pos);
        }

        local.play_clang_audio = 0;
        net.play_clang_audio = 0;

        clang_light_effect clang_effect;
        clang_effect.make(1000.f, pos, {1.f, 1.f, 1.f}, 1000.f);

        particle_effect::push(clang_effect);
    }
}

void fighter::set_other_fighters(const std::vector<fighter*>& other_fight)
{
    fighter_list = other_fight;
}
