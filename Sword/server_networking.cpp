#include "server_networking.hpp"
#include "fighter.hpp"

std::string respawn_info::get_display_string()
{
    float remaining = spawn_time - time_elapsed;

    remaining = std::max(remaining / 1000.f, 0.f);

    remaining = round(remaining);

    return std::to_string((int)remaining) + "s remaining";
}

server_networking::server_networking()
{
    ///this is a hack
    master_info.timeout_delay = 1000.f;
    //game_info.timeout_delay = 1000.f;
}

void server_networking::join_master()
{
    ///timeout 1 second
    if(master_info.owns_socket() && !master_info.within_timeout())
    {
        master_info = tcp_connect(MASTER_IP, MASTER_PORT, 1, 0);

        printf("master tcp\n");
    }

    if(master_info.valid())
    {
        to_master.close();

        to_master = tcp_sock(master_info.get());

        printf("joined master server\n");
    }
}


///we want to make this reconnect if to_game is invalid, ie we've lost connection
///?
void server_networking::join_game_tick(const std::string& address, const std::string& port)
{
    //if(game_info.owns_socket() && !game_info.within_timeout())
    //    game_info = tcp_connect(address, GAMESERVER_PORT, 5, 0);

    //if(game_info.valid())
    if(!to_game.valid())
    {
        to_game.close();

        to_game = udp_connect(address, port);

        ///send request to join
        byte_vector vec;
        vec.push_back(canary_start);
        vec.push_back(message::CLIENTJOINREQUEST);
        vec.push_back(canary_end);

        udp_send(to_game, vec.ptr);

        trying_to_join_game = false;
        joined_game = true; ///uuh. ok then, sure i guess
        have_id = false;
        my_id = -1;
        discovered_fighters.clear();

        printf("Connected gameserver %s:%s\n", address.c_str(), port.c_str());
    }
}

void server_networking::join_game_by_local_id_tick(int id)
{
    if(id < 0 || id >= (int)server_list.size())
    {
        printf("invalid joingame id %i\n", id);
        return;
    }

    game_server serv = server_list[id];

    return join_game_tick(serv.address, serv.their_host_port);
}

void server_networking::set_game_to_join(const std::string& address, const std::string& port)
{
    ///edge
    if(trying_to_join_game == false)
    {
        //game_info.retry();
        joined_game = false;
    }

    trying_to_join_game = true;
    trying_to_join_address = address;
    trying_to_join_port = port;
}

void server_networking::set_game_to_join(int id)
{
    if(id < 0 || id >= (int)server_list.size())
    {
        //printf("invalid setgame id %i\n", id);
        return;
    }

    game_server serv = server_list[id];

    return set_game_to_join(serv.address, serv.their_host_port);
}

std::vector<game_server> server_networking::get_serverlist(byte_fetch& fetch)
{
    int32_t server_num = fetch.get<int32_t>();

    std::vector<game_server> lservers;

    for(int i=0; i<server_num; i++)
    {
        int32_t len = fetch.get<int32_t>();

        std::string ip;

        for(int n=0; n<len; n++)
        {
            ip = ip + fetch.get<char>();
        }

        uint32_t port = fetch.get<uint32_t>();

        std::string their_host_port = std::to_string(port);

        game_server serv;
        serv.address = ip;
        serv.their_host_port = their_host_port;

        lservers.push_back(serv);
    }

    int32_t found_end = fetch.get<int32_t>();

    if(found_end == canary_end)
    {
        return lservers;
    }

    return std::vector<game_server>();
}

void server_networking::ping_master()
{
    if(!to_master.valid())
        return;

    pinged = true;

    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::CLIENT);
    vec.push_back(canary_end);

    tcp_send(to_master, vec.ptr);
}

template<typename T>
ptr_info get_inf(T* ptr)
{
    return {(void*)ptr, sizeof(T)};
}

std::map<int, ptr_info> build_fighter_network_stack(fighter* fight)
{
    std::map<int, ptr_info> fighter_stack;

    int c = 0;

    for(int i=0; i<fight->parts.size(); i++)
    {
        fighter_stack[c++] = get_inf(&fight->parts[i].obj()->pos);
        fighter_stack[c++] = get_inf(&fight->parts[i].obj()->rot);
        fighter_stack[c++] = get_inf(&fight->parts[i].hp);
        fighter_stack[c++] = get_inf(&fight->parts[i].net.hp_delta);
    }

    fighter_stack[c++] = get_inf(&fight->weapon.model->pos);
    fighter_stack[c++] = get_inf(&fight->weapon.model->rot);

    fighter_stack[c++] = get_inf(&fight->net.is_blocking);
    fighter_stack[c++] = get_inf(&fight->net.recoil);

    fighter_stack[c++] = get_inf(&fight->net.reported_dead);

    return fighter_stack;
}

template<typename T>
void set_map_element(std::map<int, ptr_info>& change, std::map<int, ptr_info>& stk, T* elem)
{
    int pos = get_position_of(stk, elem);

    change[pos] = get_inf(elem);
}

std::map<int, ptr_info> build_host_network_stack(fighter* fight)
{
    std::map<int, ptr_info> total_stack = build_fighter_network_stack(fight);

    std::map<int, ptr_info> to_send;

    for(int i=0; i<fight->parts.size(); i++)
    {
        set_map_element(to_send, total_stack, &fight->parts[i].obj()->pos);
        set_map_element(to_send, total_stack, &fight->parts[i].obj()->rot);
        set_map_element(to_send, total_stack, &fight->parts[i].hp);
    }

    set_map_element(to_send, total_stack, &fight->weapon.model->pos);
    set_map_element(to_send, total_stack, &fight->weapon.model->rot);

    set_map_element(to_send, total_stack, &fight->net.is_blocking);

    return to_send;
}

void server_networking::tick(object_context* ctx, gameplay_state* st, physics* phys)
{
    ///tries to join
    join_master();

    if(sock_readable(to_master))
    {
        auto data = tcp_recv(to_master);

        ///actually, we only want to ping master once
        ///unless we set a big timeout and repeatedly ping master
        ///eh
        /*if(!to_master.valid())
        {
            to_master.close();
            master_info.retry();

            printf("why\n");
        }*/

        ///uuh. We're currently just leaving a dangling connection on our end
        ///that seems like poor form

        byte_fetch fetch;
        fetch.ptr.swap(data);

        if(!fetch.finished())
        {
            int32_t found_canary = fetch.get<int32_t>();

            while(found_canary != canary_start && !fetch.finished())
            {
                found_canary = fetch.get<int32_t>();
            }

            int32_t type = fetch.get<int32_t>();

            if(type == message::CLIENTRESPONSE)
            {
                auto servs = get_serverlist(fetch);

                if(servs.size() > 0)
                {
                    server_list = servs;
                    has_serverlist = true;

                    print_serverlist();

                    ///we're done here
                    to_master.close();
                }
                else
                {
                    printf("Network error or 0 gameservers available\n");

                    pinged = false; ///invalid response, ping again
                }
            }
        }
    }

    bool any_recv = true;

    while(any_recv && sock_readable(to_game))
    {
        auto data = udp_receive_from(to_game, &to_game_store);
        is_init = true;

        any_recv = data.size() > 0;

        byte_fetch fetch;
        fetch.ptr.swap(data);

        while(!fetch.finished() && any_recv)
        {
            int32_t found_canary = fetch.get<int32_t>();

            while(found_canary != canary_start && !fetch.finished())
            {
                found_canary = fetch.get<int32_t>();
            }

            int32_t type = fetch.get<int32_t>();

            if(type == message::FORWARDING)
            {
                int32_t player_id = fetch.get<int32_t>();

                int32_t component_id = fetch.get<int32_t>();

                int32_t len = fetch.get<int32_t>();

                void* payload = fetch.get(len);

                int found_end = fetch.get<int32_t>();

                if(canary_end != found_end)
                {
                    printf("bad forward canary\n");

                    return;
                }

                if(discovered_fighters[player_id].id == -1)
                {
                    discovered_fighters[player_id] = make_networked_player(player_id, ctx, st, phys);

                    printf("made a new networked player\n");
                }

                network_player play = discovered_fighters[player_id];

                std::map<int, ptr_info> arg_map = build_fighter_network_stack(play.fight);

                if(component_id < 0 || component_id >= arg_map.size())
                {
                    printf("err in forwarding\n");
                    return; ///?
                }

                ptr_info comp = arg_map[component_id];

                if(len != comp.size)
                {
                    printf("err in argument size\n");
                    return;
                }

                memmove(comp.ptr, payload, comp.size);

                ///done for me now
                if(player_id == my_id)
                    continue;

                for(auto& i : play.fight->parts)
                {
                    ///???
                    i.obj()->set_pos(i.obj()->pos);
                    i.obj()->set_rot(i.obj()->rot);
                }

                play.fight->weapon.model->set_pos(play.fight->weapon.model->pos);
                play.fight->weapon.model->set_rot(play.fight->weapon.model->rot);

                play.fight->overwrite_parts_from_model();
                play.fight->network_update_render_positions();
            }

            if(type == message::CLIENTJOINACK)
            {
                int32_t recv_id = fetch.get<int32_t>();

                int32_t canary_found = fetch.get<int32_t>();

                if(canary_found == canary_end)
                {
                    have_id = true;
                    my_id = recv_id;

                    printf("Got joinack, I have id: %i\n", my_id);
                }
            }

            if(type == message::TEAMASSIGNMENT)
            {
                int32_t client_id = fetch.get<int32_t>();
                int32_t team = fetch.get<int32_t>();

                int32_t canary_found = fetch.get<int32_t>();

                //printf("teamassign\n");

                if(canary_found == canary_end)
                {
                    if(discovered_fighters.find(client_id) != discovered_fighters.end() && discovered_fighters[client_id].fight)
                        discovered_fighters[client_id].fight->set_team(team);
                    else
                        printf("teamassignskip\n");
                }
                else
                {
                    printf("Team assign canary err\n");
                }
            }

            if(type == message::GAMEMODEUPDATE)
            {
                just_new_round = false;

                bool prev_game = game_info.game_over();

                game_info.process_gamemode_update(fetch);

                ///just swapped from not game over to it is game over!
                if(prev_game == true && !game_info.game_over())
                {
                    just_new_round = true;
                }
            }

            if(type == message::RESPAWNRESPONSE)
            {
                vec2f new_pos = fetch.get<vec2f>();

                int32_t canary_found = fetch.get<int32_t>();

                if(canary_found == canary_end)
                {
                    if(discovered_fighters[my_id].fight != nullptr)
                        discovered_fighters[my_id].fight->respawn(new_pos);
                }
                else
                {
                    printf("canary error in respawnresponse\n");
                }
            }

            if(type == message::RESPAWNINFO)
            {
                /*byte_vector vec;
                vec.push_back(canary_start);
                vec.push_back(message::RESPAWNINFO);
                vec.push_back(time_elapsed);
                vec.push_back(i.time_to_respawn_ms);
                vec.push_back(canary_end);*/

                float time_elapsed = fetch.get<float>();
                float respawn_time = fetch.get<float>();

                int32_t canary_found = fetch.get<int32_t>();

                if(canary_found == canary_end)
                {
                    respawn_inf = {time_elapsed, respawn_time};
                }
                else
                {
                    printf("canary err in respawninfo\n");
                }
            }
        }
    }

    if(is_init && to_game.valid() && (time_since_last_send.getElapsedTime().asMicroseconds() / 1000.f) > time_between_sends_ms)
    {
        if(have_id && discovered_fighters[my_id].fight != nullptr)
        {
            std::map<int, ptr_info> host_stack = build_host_network_stack(discovered_fighters[my_id].fight);

            //for(int i=0; i<host_stack.size(); i++)
            for(auto& i : host_stack)
            {
                ptr_info inf = i.second;

                int32_t id = i.first;

                byte_vector vec;
                vec.push_back(canary_start);
                vec.push_back(message::FORWARDING);
                vec.push_back<int32_t>(my_id);
                vec.push_back<int32_t>(id);

                vec.push_back<int32_t>(inf.size);
                vec.push_back((uint8_t*)inf.ptr, inf.size);

                vec.push_back(canary_end);

                udp_send_to(to_game, vec.ptr, (const sockaddr*)&to_game_store);
            }

            ///uuh. Looking increasingly like we should just include the home fighter in this one, eh?
            for(auto& net_fighter : discovered_fighters)
            {
                //if(my_id == net_fighter.first)
                //   continue;

                fighter* fight = net_fighter.second.fight;

                ///? should be impossibru
                if(!fight)
                    continue;

                if(fight->net.recoil_dirty)
                {
                    network_update_element<int32_t>(this, &fight->net.recoil, fight);

                    fight->net.recoil_dirty = false;
                }

                ///ok so this still doesnt work for quite a few reasons
                ///mainly we send the delta and then 0 it, which makes parts respawn
                ///ok so under the delta scheme, tha player never actually dies
                ///which means that under the current method
                ///they'll just be teleported across the map
                ///parts don't reinitialise properly
                if(my_id != net_fighter.first)
                {
                    for(auto& i : fight->parts)
                    {
                        if(i.net.hp_dirty)
                        {
                            network_update_element<float>(this, &i.net.hp_delta, fight);

                            i.net.hp_delta = 0.f;

                            i.net.hp_dirty = false;
                        }
                    }
                }

                if(fight->net.reported_dead)
                {
                    int32_t player_id = get_id_from_fighter(fight);

                    if(player_id == -1)
                    {
                        printf("Fighter reported dead, but does not exist on networking\n");
                    }

                    byte_vector vec;
                    vec.push_back<int32_t>(canary_start);
                    vec.push_back<int32_t>(message::REPORT);
                    vec.push_back<int32_t>(report::DEATH);
                    vec.push_back<int32_t>(player_id);
                    vec.push_back<int32_t>(0); ///no extra data
                    vec.push_back<int32_t>(canary_end);

                    udp_send(to_game, vec.ptr);

                    fight->net.reported_dead = 0;
                }
            }

            ///spam server with packets until it respawns us
            if(discovered_fighters[my_id].fight->dead())
            {
                byte_vector vec;
                vec.push_back<int32_t>(canary_start);
                vec.push_back<int32_t>(message::RESPAWNREQUEST);
                vec.push_back<int32_t>(canary_end);

                udp_send(to_game, vec.ptr);
            }

            for(auto& i : discovered_fighters[my_id].fight->parts)
            {
                ///I set my own HP, don't update my hp with the delta
                if(i.net.hp_dirty)
                {
                    i.net.hp_dirty = false;
                    i.net.hp_delta = 0.f;
                }

                if(i.net.hp_delta != 0.f)
                {
                    i.hp += i.net.hp_delta;

                    i.net.hp_delta = 0.f;
                }
            }

            time_since_last_send.restart();
        }
    }

    if(!has_serverlist && !pinged)
    {
        ping_master();
    }

    if(trying_to_join_game)
    {
        join_game_tick(trying_to_join_address, trying_to_join_port);
    }

    for(auto& i : discovered_fighters)
    {
        if(i.first == my_id)
            continue;

        //if(i.second.id < 0)
        //    continue;

        i.second.fight->manual_check_part_alive();

        i.second.fight->respawn_if_appropriate();

        i.second.fight->overwrite_parts_from_model();
        i.second.fight->manual_check_part_death();

        i.second.fight->my_cape.tick(i.second.fight);

        ///death is dynamically calculated from part health
        if(!i.second.fight->dead())
            i.second.fight->update_lights();
    }

    game_info.tick();
}

int32_t server_networking::get_id_from_fighter(fighter* f)
{
    for(auto& i : discovered_fighters)
    {
        if(i.second.fight == f)
            return i.first;
    }

    return -1;
}

void server_networking::print_serverlist()
{
    for(int i=0; i<(int)server_list.size(); i++)
    {
        printf("SN: %i %s:%s\n", i, server_list[i].address.c_str(), server_list[i].their_host_port.c_str());
    }
}

network_player server_networking::make_networked_player(int32_t id, object_context* ctx, gameplay_state* current_state, physics* phys)
{
    fighter* net_fighter = new fighter(*ctx, *ctx->fetch());

    net_fighter->set_team(0);
    net_fighter->set_pos({0, 0, 0});
    net_fighter->set_rot({0, 0, 0});
    //net_fighter->set_quality(s.quality);
    net_fighter->set_quality(1); ///???
    net_fighter->set_gameplay_state(current_state);

    ctx->load_active();

    net_fighter->set_physics(phys);
    net_fighter->update_render_positions();

    ctx->build();

    net_fighter->update_lights();

    network_player play;
    play.fight = net_fighter;
    play.id = id;

    return play;
}

void server_networking::set_my_fighter(fighter* fight)
{
    if(!have_id)
        return;

    discovered_fighters[my_id] = {fight, my_id};
}

void gamemode_info::process_gamemode_update(byte_fetch& arg)
{
    byte_fetch fetch = arg;

    current_mode = (game_mode_t)fetch.get<int32_t>();

    current_session_state = fetch.get<decltype(current_session_state)>();
    current_session_boundaries = fetch.get<decltype(current_session_boundaries)>();

    /*max_time_elapsed = fetch.get<float>();
    max_kills = fetch.get<int32_t>();*/

    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
    {
        printf("err in proces gamemode updates bad canary\n");
        return;
    }

    arg = fetch;

    clk.restart();
}

void gamemode_info::tick()
{
    current_session_state.time_elapsed += (clk.getElapsedTime().asMicroseconds() / 1000.f);
    clk.restart();
}

bool gamemode_info::game_over()
{
    if(current_session_state.team_1_killed >= current_session_boundaries.max_kills ||
       current_session_state.team_0_killed >= current_session_boundaries.max_kills ||
       current_session_state.time_elapsed > current_session_boundaries.max_time_ms)
    {
        return true;
    }

    return false;
}

///needs to be gamemode specific really
std::string gamemode_info::get_game_over_string()
{
    ///team 0 wins
    ///should really be done by who killed more
    if(current_session_state.team_1_killed >= current_session_boundaries.max_kills)
    {
        return "Team " + team_defs::team_names[0] + " wins!";
    }

    if(current_session_state.team_0_killed >= current_session_boundaries.max_kills)
    {
        return "Team " + team_defs::team_names[1] + " wins!";
    }

    return "Its a draw!";
}

///gamemode?
///centre align?
std::string gamemode_info::get_display_string()
{
    ///the number of kills is the opposite of who killed who
    std::string t0str = std::to_string(current_session_state.team_1_killed);
    std::string t1str = std::to_string(current_session_state.team_0_killed);
    std::string mkstr = std::to_string(current_session_boundaries.max_kills);

    std::string t0remaining = std::to_string(current_session_boundaries.max_kills - current_session_state.team_1_killed);
    std::string t1remaining = std::to_string(current_session_boundaries.max_kills - current_session_state.team_0_killed);

    std::string tstr = std::to_string((int)current_session_state.time_elapsed / 1000);
    std::string mtstr = std::to_string((int)current_session_boundaries.max_time_ms / 1000);

    std::string team_line = "Team 0 kills: " + t0str + " with " + t0remaining + " remaining!"
                        + "\nTeam 1 kills: " + t1str + " with " + t1remaining + " remaining!";

    std::string team_next_line = mkstr + " kills needed total";

    std::string time_str = tstr + " of " + mtstr + " remaining!";

    //return kstr + "/" + mkstr + "\n" + tstr + "/" + mtstr;

    return team_line + "\n" + team_next_line + "\n" + time_str;
}
