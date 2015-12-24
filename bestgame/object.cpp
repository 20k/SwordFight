#include "object.hpp"

void game_entity::set_dim(vec2f _dim)
{
    dim = _dim;
}

void game_entity::set_pos(vec2f _pos)
{
    pos = _pos;
}

void game_entity::translate(vec2f _dir)
{
    pos = pos + _dir;
}

void game_entity::move_at_speed(vec2f _dir, float speed, float dt)
{
    pos = pos + _dir.norm() * speed * dt;
}

///need to do a explode die effect
bool game_entity::is_dead()
{
    return hp <= 0.f;
}

void game_entity::do_damage(float frac)
{
    hp -= frac;

    ///i am dead
    if(do_death_effect)
        return;

    ///do hit effect
    hit_timer.restart();
    do_hit_effect = true;

    if(is_dead())
    {
        death_timer.restart();
        do_death_effect = true;
    }
}

void game_entity::set_team(team_t _team)
{
    team_collision = _team;
    team_colour = _team;
}

void game_entity::tick(state& s, float dt) {}

game_entity::game_entity()
{
    hp = 1.f;
    to_remove = false;
    team_colour = team::NONE;
    team_collision = team::NONE;
    do_hit_effect = false;
    do_death_effect = false;
    dim = {5, 5};
}

game_entity::~game_entity() {}

int collider::get_collider_id(game_entity* me, const std::vector<game_entity*>& entities)
{
    vec2f pos = me->pos;

    team_t my_team = me->team_collision;

    for(int i=0; i<entities.size(); i++)
    {
        game_entity* en = entities[i];

        team_t their_team = en->team_collision;

        ///im within his rectangle (temp)
        ///my team is not his team (no ff)
        ///and his team isn't none
        if(pos >= (en->pos - en->dim/2.f) && pos < (en->pos + en->dim/2.f) &&
           my_team != their_team && their_team != team::NONE)
        {
            return i;
        }
    }

    return -1;
}

void projectile::set_dir(vec2f _dir)
{
    dir = _dir.norm();
}

void projectile::set_speed(float _speed)
{
    speed = _speed;
}

void projectile::set_damage(float _damage)
{
    damage = _damage;
}

void projectile::tick(state& s, float dt)
{
    if(clk.getElapsedTime().asMilliseconds() / 1000.f > time_to_live_s)
        to_remove = true;

    move_at_speed(dir.norm(), speed, dt);

    render_square sq(pos, dim, {1, 1, 0.3});

    s.render_2d->add(sq);

    collider collide;

    int id = collide.get_collider_id(this, *s.entities);

    if(id == -1)
        return;

    game_entity* en = (*s.entities)[id];

    en->do_damage(damage);

    to_remove = true;
}

projectile::projectile()
{
    speed = 1.f;
    damage = 0.35f;
    time_to_live_s = 5;
}

character::character(){time_fired_s = 0.f; current_time = 0.f;}

vec3f character::get_current_col()
{
    vec3f normal_col = team::normal_cols[team_colour];
    vec3f hurt_col = team::hit_cols[team_colour];

    vec3f set_col = normal_col;

    if(hit_timer.getElapsedTime().asMilliseconds() / 1000.f < game::hit_timer_s && do_hit_effect)
    {
        set_col = hurt_col;
        death_timer.restart();
    }
    else
    {
        do_hit_effect = false;
    }

    if(!do_hit_effect && do_death_effect)
    {
        ///bullets pass through me
        team_collision = team::NONE;

        if(death_timer.getElapsedTime().asMilliseconds() / 1000.f < game::death_fade_s)
        {
            float frac = (death_timer.getElapsedTime().asMilliseconds() / 1000.f) / game::death_fade_s;

            frac = clamp(1.f - frac, 0.f, 1.f);

            set_col = normal_col * frac;
        }
        else
        {
            to_remove = true;
        }
    }

    return set_col;
}

void character::tick(state& s, float dt)
{
    vec3f set_col = get_current_col();

    render_square sq(pos, dim, set_col);

    s.render_2d->add(sq);

    current_time += dt / 1000.f;
}

projectile* character::fire(vec2f dir, float time_between_shots, float projectile_speed)
{
    if(time_fired_s + time_between_shots > current_time)
        return nullptr;

    time_fired_s = current_time;

    projectile* new_projectile = new projectile;

    new_projectile->set_dir(dir);
    new_projectile->set_pos(pos);
    new_projectile->set_dim({3, 3});
    new_projectile->set_team(team_collision);
    new_projectile->set_speed(projectile_speed);

    return new_projectile;
}

void ai_character::tick(state& s, float dt)
{
    character::tick(s, dt);

    s.ai->add_hostile_ai(this);
}

vanilla_enemy::vanilla_enemy()
{
    set_dim({10, 10});
}

robot_enemy::robot_enemy()
{
    set_dim({12, 12});

    firing = false;
}

hound_enemy::hound_enemy()
{
    set_dim({7, 7});

    move_clock = 0.f;
    last_move_time = 0.f;

    //ai_active = true;
}

hound_master::hound_master()
{
    set_dim({9, 9});
    hounds_init = false;
}

tank_enemy::tank_enemy()
{
    set_dim({20, 20});

    seeking = false;
}

player::player()
{
    set_dim({10, 10});
}

vec2f seeker::get_seek(vec2f current_pos)
{
    if(seeking)
    {
        return seek_vec - current_pos;
    }

    return {0,0};

    /*if(!seeking)
    {
        vec2f c_pos = pos;

        float seek_distance = 80;

        vec2f seek = c_pos + seek_distance * randf<2, float>(-1.f, 1.f);

        seek_vec = seek;

        seeking = true;
    }

    if((seek_vec - pos).length() < 1)
        seeking = false;*/

}

void seeker::start_seek(vec2f my_pos, float distance)
{
    if(seeking)
        return;

    seek_vec = my_pos + distance * randf<2, float>(-1.f, 1.f);

    seeking = true;
}

bool seeker::is_complete(vec2f current_pos)
{
    return (current_pos - seek_vec).length() < 1.f;
}

void seeker::terminate()
{
    seeking = false;
}

bool seeker::running()
{
    return seeking;
}

vec2f get_move_within_distance(vec2f my_pos, vec2f enemy_pos, float distance)
{
    vec2f dir = enemy_pos - my_pos;

    float ldist = dir.length();

    if(ldist >= distance)
    {
        return dir.norm();
    }

    return {0,0};
}

void ai_character::do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai)
{

}

void vanilla_enemy::do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai)
{
    const float approach_distance = 100;

    const float speed = 0.1f;

    ///from player to character
    vec2f dir = pos - current_enemy->pos;

    ///from character to player
    dir = -dir;

    float distance = dir.length();

    if(distance >= approach_distance)
        move_at_speed(dir.norm(), speed, dt);
    else
        move_at_speed(-dir.norm(), speed/3.f, dt);

    ///this is incorrect because we want to base it off speed really so that they cant ever break
    ///but oh well
    move_at_speed(pad_dir.norm(), 0.1f, dt);

    projectile* proj = fire(dir, 0.5f);

    if(proj != nullptr)
        s.entities->push_back(proj);
}

///laser projectile does way too much damage because each individual piece is a projectile
projectile* robot_enemy::fire_laser(vec2f dir, float time_between_pulses, float duration)
{
    float refire_time = laser_refire_clock.getElapsedTime().asMilliseconds() / 1000.f;

    if(refire_time >= time_between_pulses)
    {
        laser_duration_clock.restart();
        laser_refire_clock.restart();
    }

    float time_since_fired = laser_duration_clock.getElapsedTime().asMilliseconds() / 1000.f;

    if(time_since_fired < duration)
    {
        projectile* proj = fire(dir, 0.01f, 0.7f);

        firing = true;

        return proj;
    }

    firing = false;

    return nullptr;
}

void robot_enemy::do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai)
{
    move_at_speed(pad_dir.norm(), 0.1f, dt);

    const float speed = 0.2f / 3;

    ///distance at which seeking activates
    const float seek_distance = 400;

    ///distance to seek when seeking
    const float distance_to_seek = 100;

    vec2f change = get_move_within_distance(pos, current_enemy->pos, seek_distance);

    if(!seek.running())
        move_at_speed(change, speed, dt);

    vec2f dir = current_enemy->pos - pos;

    float len = dir.length();

    if(len < seek_distance && !seek.running() && !firing)
    {
        seek.start_seek(pos, distance_to_seek);
    }

    if(seek.running() && !seek.is_complete(pos) && !firing)
    {
        vec2f dist = seek.get_seek(pos);

        move_at_speed(dist, speed, dt);
    }

    if(seek.is_complete(pos))
    {
        seek.terminate();
    }

    if(!firing)
        locked_fire_dir = dir;

    if(len < seek_distance && !seek.running())
    {
        projectile* proj = fire_laser(locked_fire_dir.norm(), 3.f, 0.5f);

        if(proj != nullptr)
            s.entities->push_back(proj);
    }
}

void hound_enemy::set_owned()
{
    move_probability = 1.f;
}

void hound_enemy::set_free()
{
    move_probability = 0.3f;
}

///disorganised hound AI
///sometimes just charge randomly, sometimes charge player
///this is the reward ai for killing the hound master
void hound_enemy::do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai)
{
    const float move_duration_s = 0.3f;
    const float move_duration_ms = move_duration_s * 1000.f;

    const float move_distance = 70;
    const float time_between_move_s = 1.f;
    const float time_between_move_ms = time_between_move_s * 1000.f;

    const float move_speed = move_distance / move_duration_ms;

    const float player_offset = 40.f;

    const float projectile_speed = 0.25f;

    move_at_speed(pad_dir, 0.1f, dt);


    move_clock += dt;


    const float chance_to_move_towards_player = move_probability;

    float pchance = randf_s(0.f, 1.f);

    vec2f to_player = current_enemy->pos - pos;

    vec2f my_dir = to_player.norm() * move_distance;

    ///fail
    if(pchance >= chance_to_move_towards_player)
    {
        my_dir = move_distance * randf<2, float>(-1, 1);
    }

    vec2f my_seek = my_dir + pos + randf<2, float>(-player_offset, player_offset);

    my_seek = (my_seek - pos).norm() * move_distance + pos;

    if(!seek.running() && move_clock > last_move_time + time_between_move_ms)
    {
        last_move_time = move_clock;
        seek.start_seek(my_seek, 0.f);

        projectile* proj = fire(my_seek - pos, 0.01f, projectile_speed);

        if(proj != nullptr)
        {
            s.entities->push_back(proj);
        }

        //seek.start_seek(current_enemy->pos + randf<2, float>(-player_offset, player_offset), move_distance);
    }

    if(seek.running())
    {
        vec2f seek_dir = seek.get_seek(pos);

        move_at_speed(seek_dir, move_speed, dt);
    }

    if(seek.is_complete(pos))
    {
        seek.terminate();
    }
}

void hound_master::do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai)
{
    ///should I have some kind of attack?

    const float speed = 0.1f;

    ///wander
    if(hounds.size() == 0)
    {
        if(!seek.running())
        {
            seek.start_seek(pos, 100.f);
        }

        if(seek.running())
        {
            vec2f dir = seek.get_seek(pos);

            move_at_speed(dir, speed/2, dt);
        }

        if(seek.is_complete(pos))
        {
            seek.terminate();
        }
    }
    ///stick with the hounds
    else
    {
        vec2f hound_pos = {0,0};

        for(int i=0; i<hounds.size(); i++)
        {
            hound_pos = hound_pos + hounds[i]->pos;
        }

        ///avg
        hound_pos = hound_pos / hounds.size();

        vec2f dir = hound_pos - pos;

        if(dir.length() < 50.f)
            return;

        move_at_speed(hound_pos - pos, speed, dt);
    }
}

///maybe fire a spiral pattern instead?
std::vector<projectile*> tank_enemy::fire_circle(float time_between_shots, int num, float speed)
{
    std::vector<projectile*> projs;

    if(time_fired_s + time_between_shots > current_time)
        return std::vector<projectile*>();

    time_fired_s = current_time;

    for(int i=0; i<num; i++)
    {
        projectile* new_projectile = new projectile;

        float arc = ((float)i / num) * M_PI * 2.f;

        float y = sin(arc);
        float x = cos(arc);

        vec2f dir = {x, y};

        new_projectile->set_dir(dir.norm());
        new_projectile->set_pos(pos);
        new_projectile->set_dim({3, 3});
        new_projectile->set_speed(speed);
        new_projectile->set_team(team_collision);

        projs.push_back(new_projectile);
    }

    return projs;
}

void tank_enemy::do_ai(state& s, float dt, vec2f pad_dir, character* current_enemy, const std::vector<ai_character*>& friendly_ai)
{
    ///move to the centre of a whole bunch of ai

    /*vec2f friendly_avg = std::accumulate(friendly_ai.begin(), friendly_ai.end(), (vec2f){0,0},
                                         [](vec2f r1, auto r2)
                                         {
                                            return r1 + r2->pos;
                                         });

    friendly_avg = friendly_avg / (float)friendly_ai.size();*/

    if(!seeking)
    {
        vec2f c_pos = pos;

        float seek_distance = 80;

        vec2f seek = c_pos + seek_distance * randf<2, float>(-1.f, 1.f);

        seek_vec = seek;

        seeking = true;
    }

    if((seek_vec - pos).length() < 1)
        seeking = false;

    move_at_speed(pad_dir.norm(), 0.1f, dt);

    const float speed = 0.08f;

    move_at_speed(seek_vec - pos, speed, dt);

    auto projs = fire_circle(2.f, 20, 0.3f);

    s.entities->insert(s.entities->end(), projs.begin(), projs.end());
}

void vanilla_enemy::tick(state& s, float dt)
{
    ai_character::tick(s, dt);
}

void robot_enemy::tick(state& s, float dt)
{
    ai_character::tick(s, dt);
}

void hound_enemy::tick(state& s, float dt)
{
    ///hmm. This is super hacky
    //if(ai_active)
    ai_character::tick(s, dt);
    //else
    //    character::tick(s, dt);
}

void hound_master::tick(state& s, float dt)
{
    ai_character::tick(s, dt);

    if(!hounds_init)
    {
        const float hound_dist = 30.f;

        vec2f positions[4] = {{-1.f, 0.f}, {1.f, 0.f}, {0.f, -1.f}, {0.f, 1.f}};

        for(int i=0; i<4; i++)
        {
            hound_enemy* hound = new hound_enemy;
            hound->set_pos(pos + positions[i]);
            hound->set_team(team_collision);
            hound->set_owned(); ///owned by hound master

            hounds.push_back(hound);
            s.entities->push_back(hound);
        }

        hounds_init = true;
    }

    if(!to_remove)
    {
        for(int i=0; i<hounds.size(); i++)
        {
            if(hounds[i]->to_remove)
            {
                hounds.erase(hounds.begin() + i);
                i--;
            }
        }
    }

    /*///otherwise we might double tick the hounds
    ///a double tick isnt a problem in principle, but im sure it would cause me
    ///no end of joy debugging down the line
    if(!to_remove)
    {
        for(auto& i : hounds)
        {
            i->tick(s, dt);
        }
    }*/

    if(to_remove)
    {
        for(int i=0; i<hounds.size(); i++)
        {
            hounds[i]->set_free();
            //s.entities->push_back(hounds[i]);
        }

        hounds.clear();
    }
}

void tank_enemy::tick(state& s, float dt)
{
    ai_character::tick(s, dt);
}

void player::do_keyboard_controls(state& s, float dt)
{
    vec2f dir = {0,0};

    sf::Keyboard key;

    if(key.isKeyPressed(sf::Keyboard::W))
    {
        dir.v[1] = -1;
    }
    if(key.isKeyPressed(sf::Keyboard::S))
    {
        dir.v[1] = 1;
    }
    if(key.isKeyPressed(sf::Keyboard::A))
    {
        dir.v[0] = -1;
    }
    if(key.isKeyPressed(sf::Keyboard::D))
    {
        dir.v[0] = 1;
    }

    float speed = 0.3;

    move_at_speed(dir.norm(), speed, dt);

    sf::Mouse mouse;

    vec2f mouse_pos;
    mouse_pos.v[0] = mouse.getPosition(*(s.win)).x;
    mouse_pos.v[1] = mouse.getPosition(*(s.win)).y;

    vec2f world_pos;
    world_pos.v[0] = s.win->mapPixelToCoords({mouse_pos.v[0], mouse_pos.v[1]}).x;
    world_pos.v[1] = s.win->mapPixelToCoords({mouse_pos.v[0], mouse_pos.v[1]}).y;

    if(mouse.isButtonPressed(sf::Mouse::Left))
    {
        vec2f dir = world_pos - pos;//mouse_pos - pos;

        game_entity* en = fire(dir, 0.25f);

        if(en != nullptr)
            s.entities->push_back(en);
    }
}

void player::tick(state& s, float dt)
{
    sf::View view = s.win->getView();
    view.setCenter({pos.v[0], pos.v[1]});

    s.win->setView(view);

    character::tick(s, dt);

    if(is_dead())
        return;

    do_keyboard_controls(s, dt);

    s.ai->add_friendly_player(this);
}
