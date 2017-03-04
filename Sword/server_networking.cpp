#include "server_networking.hpp"
#include "fighter.hpp"
#include "sound.hpp"
#include "network_fighter_model.hpp"
#include "../sword_server/master_server/server.hpp"
#include "network_update_wrappers.hpp"

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
    //master_info.timeout_delay = 1000.f;
    //game_info.timeout_delay = 1000.f;
}

void server_networking::join_master()
{
    ///timeout 1 second
    /*if(master_info.owns_socket() && !master_info.within_timeout())
    {
        master_info = tcp_connect(MASTER_IP, MASTER_PORT, 1, 0);

        //printf("master tcp\n");
    }

    if(master_info.valid())
    {
        to_master.close();

        to_master = tcp_sock(master_info.get());

        lg::log("joined master server");
    }*/

    if(!to_master_udp.valid())
    {
        to_master_udp.close();

        to_master_udp = udp_connect(MASTER_IP, MASTER_CLIENT_PORT);

        lg::log("Created master UDP port");
    }
}


///we want to make this reconnect if to_game is invalid, ie we've lost connection
///?
void server_networking::join_game_tick(const std::string& address, const std::string& port)
{
    //if(game_info.owns_socket() && !game_info.within_timeout())
    //    game_info = tcp_connect(address, GAMESERVER_PORT, 5, 0);

    //if(game_info.valid())
    if(!connected_server.to_game.valid())
    {
        connected_server.to_game.close();

        connected_server.to_game = udp_connect(address, port);

        ///send request to join
        byte_vector vec;
        vec.push_back(canary_start);
        vec.push_back(message::CLIENTJOINREQUEST);
        vec.push_back(canary_end);

        udp_send(connected_server.to_game, vec.ptr);

        trying_to_join_game = false;
        joined_game = true; ///uuh. ok then, sure i guess
        connected_server.have_id = false;
        connected_server.my_id = -1;
        connected_server.discovered_fighters.clear();

        lg::log("Connected gameserver ", address.c_str(), ":", port.c_str());
    }
}

void server_networking::join_game_by_local_id_tick(int id)
{
    if(id < 0 || id >= (int)server_list.size())
    {
        lg::log("invalid joingame id ", id);
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

void server_networking::disconnect_game()
{
    connected_server.disconnect();

    joined_game = false;
    trying_to_join_game = false;
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

///gives CLIENTRESPONSE
void server_networking::ping_master()
{
    if(!to_master_udp.valid())
        return;

    pinged = true;

    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::CLIENT);
    vec.push_back(canary_end);

    udp_send(to_master_udp, vec.ptr);
}

///ok so this is all wrong ;_; revert commits
///trombone needs to be part of fighter
std::map<int, ptr_info> build_fighter_network_stack(network_player* net_fight, game_server_session_resources* networking)
{
    fighter* fight = net_fight->fight;
    network_fighter* net = net_fight->net_fighter;
    network_fighter_info* fighter_info = &net->network_fighter_inf;


    std::map<int, ptr_info> fighter_stack;

    constexpr int s_f3 = sizeof(cl_float) * 3;

    int c = 0;

    for(int i=0; i<fight->parts.size(); i++)
    {
        fighter_stack[c++] = get_inf<s_f3>(&net->network_parts[i].global_pos);
        fighter_stack[c++] = get_inf<s_f3>(&net->network_parts[i].global_rot);
        fighter_stack[c++] = get_inf(&net->network_parts[i].hp);

        //fighter_stack[c++] = get_inf(&fight->parts[i].net.damage_info);
        //fighter_stack[c++] = get_inf(&net->network_parts[i].requested_damage_info);
        fighter_stack[c++] = get_inf(net->network_parts[i].requested_damage_info.get_networking_ptr());
        fighter_stack[c++] = get_inf(&fight->parts[i].net.play_hit_audio);
    }

    fighter_stack[c++] = get_inf(&net->network_sword.global_pos);
    fighter_stack[c++] = get_inf(&net->network_sword.global_rot);

    fighter_stack[c++] = get_inf(&net->network_sword.is_blocking);

    fighter_stack[c++] = get_inf(fighter_info->recoil_requested.get_networking_ptr());
    fighter_stack[c++] = get_inf(fighter_info->recoil_forced.get_networking_ptr());

    fighter_stack[c++] = get_inf(&fight->net.reported_dead);
    fighter_stack[c++] = get_inf(&fight->net.play_clang_audio);

    fighter_stack[c++] = get_inf(&net->network_sword.is_damaging);

    fighter_stack[c++] = get_inf(&net->network_fighter_inf.name);

    fighter_stack[c++] = get_inf(&net->network_fighter_inf.pos);
    fighter_stack[c++] = get_inf(&net->network_fighter_inf.rot);

    for(auto& i : networking->registered_network_variable_perplayer[fight->network_id])
    {
        fighter_stack[c++] = i;
    }

    return fighter_stack;
}

template<typename T>
void set_map_element(std::map<int, ptr_info>& change, std::map<int, ptr_info>& stk, T* elem)
{
    int pos = get_position_of(stk, elem);

    change[pos] = get_inf(elem);
}

template<int N>
void set_map_element(std::map<int, ptr_info>& change, std::map<int, ptr_info>& stk, void* elem)
{
    int pos = get_position_of(stk, elem);

    change[pos] = get_inf<N>(elem);
}

/*bool is_damage_info(fighter* fight, void* ptr)
{
    for(auto& i : fight->parts)
    {
        if(&i.net.damage_info == ptr)
            return true;
    }

    return false;
}*/

std::map<int, ptr_info> build_host_network_stack(network_player* net_fight, game_server_session_resources* networking)
{
    fighter* fight = net_fight->fight;
    network_fighter* net = net_fight->net_fighter;

    constexpr int s_f3 = sizeof(cl_float) * 3;

    std::map<int, ptr_info> total_stack = build_fighter_network_stack(net_fight, networking);

    std::map<int, ptr_info> to_send;

    for(int i=0; i<fight->parts.size(); i++)
    {
        set_map_element<s_f3>(to_send, total_stack, &net->network_parts[i].global_pos);
        set_map_element<s_f3>(to_send, total_stack, &net->network_parts[i].global_rot);
        set_map_element(to_send, total_stack, &net->network_parts[i].hp);
    }

    set_map_element(to_send, total_stack, &net->network_sword.global_pos);
    set_map_element(to_send, total_stack, &net->network_sword.global_rot);

    set_map_element(to_send, total_stack, &net->network_sword.is_blocking);

    set_map_element(to_send, total_stack, &net->network_sword.is_damaging);

    set_map_element(to_send, total_stack, &net->network_fighter_inf.pos);
    set_map_element(to_send, total_stack, &net->network_fighter_inf.rot);

    return to_send;
}

/*void server_networking::handle_ping_response(byte_fetch& arg)
{
    byte_fetch fetch = arg;

    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
    {
        lg::log("Canary error in handle ping response");
        return;
    }

    lg::log("Received ping response");
}*/

///wait wait... so the game server gets the ping to itself of the client, and that's considered
///the ping from my client to the other client?
///Ok. We just add our ping to server and their ping to server, makes sense
void server_networking::handle_ping_data(byte_fetch& arg)
{
    byte_fetch fetch = arg;

    int32_t num_players = fetch.get<int32_t>();

    for(int i=0; i<num_players; i++)
    {
        int32_t player_id = fetch.get<int32_t>();
        float player_ping = fetch.get<float>();

        network_player& net_play = connected_server.discovered_fighters[player_id];

        if(net_play.id < 0)
        {
            lg::log("Invalid playerid ", net_play.id);

            connected_server.discovered_fighters.erase(player_id);

            continue;
        }

        ///error
        if(net_play.fight == nullptr)
        {
            lg::log("Oh dear, null fighter");
            continue;
        }

        //lg::log("Got ping ", player_ping, " for player ", player_id);

        net_play.fight->net.ping = player_ping;
    }

    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
    {
        lg::log("Canary error in handle ping data");
        return;
    }

    arg = fetch;
}

void server_networking::handle_ping(byte_fetch& arg)
{
    ///need to handle fetch. It will contain a time, then we just pipe that back?
    ///or should the client handle it?

    byte_fetch fetch = arg;

    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
    {
        lg::log("Canary error in handle ping");
        return;
    }

    byte_vector vec;

    vec.push_back(canary_start);
    vec.push_back(message::PING_RESPONSE);
    vec.push_back(canary_end);

    udp_send(connected_server.to_game, vec.ptr);

    arg = fetch;
}

void server_networking::handle_ping_gameserver_response(byte_fetch& arg)
{
    byte_fetch fetch = arg;

    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
    {
        lg::log("Canary error in handle ping gameserver response");
        return;
    }

    arg = fetch;
}

///don't use this?
void server_networking::ping()
{
    byte_vector vec;

    vec.push_back(canary_start);
    vec.push_back(message::PING);
    vec.push_back(canary_end);

    udp_send(connected_server.to_game, vec.ptr);
}

void server_networking::ping_gameserver(const std::string& address, const std::string& port)
{
    udp_sock nsock = udp_connect(address, port);

    if(!nsock.valid())
        return;

    in_progress_ping cur_ping;

    cur_ping.address = address;
    cur_ping.port = port;
    cur_ping.sock = nsock;

    current_server_pings.push_back(cur_ping);

    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::PING_GAMESERVER);
    vec.push_back(canary_end);

    udp_send(nsock, vec.ptr);
}

void set_gameserver_ping(const std::string& address, const std::string& port, float ping_us, std::vector<game_server>& servers)
{
    for(auto& i : servers)
    {
        if(i.address != address)
            continue;

        if(i.their_host_port != port)
            continue;

        i.ping = ping_us / 1000.f;
    }
}

void server_networking::tick_gameserver_ping_responses()
{
    for(int i=0; i<current_server_pings.size(); i++)
    {
        bool terminate = false;

        udp_sock& sock = current_server_pings[i].sock;

        bool any_recv = true;

        while(any_recv && sock_readable(sock))
        {
            sockaddr_storage to_server;

            auto data = udp_receive_from(sock, &to_server);

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

                if(type == message::PING_GAMESERVER_RESPONSE)
                {
                    handle_ping_gameserver_response(fetch);

                    ///ok we've got a ping back
                    set_gameserver_ping(current_server_pings[i].address, current_server_pings[i].port, current_server_pings[i].clk.getElapsedTime().asMicroseconds(), server_list);

                    terminate = true;
                }
            }
        }

        if(!terminate)
            continue;

        sock.close();

        current_server_pings.erase(current_server_pings.begin() + i);
        i--;
    }
}

std::vector<game_server> merge_gameservers(const std::vector<game_server>& old_servers, const std::vector<game_server>& new_servers)
{
    std::vector<game_server> ret = new_servers;

    for(game_server& new_server : ret)
    {
        for(const game_server& old_server : old_servers)
        {
            ///this has probably changed, and is also likely not useful information
            ///but its here anyway for debugging potentially
            std::string new_my_port = new_server.my_port_to_them;

            if(new_server.address == old_server.address && new_server.their_host_port == old_server.their_host_port)
            {
                new_server = old_server;
            }

            new_server.my_port_to_them = new_my_port;
        }
    }

    return ret;
}

///so basically the whole client -> master server stuff is an infinite pile of total bullshit
///setting a server to join shouldn't be a tick operation, it should be a one off so we can do it through the UI
///rewrite this
void server_networking::tick(object_context* ctx, object_context* tctx, gameplay_state* st, physics* phys)
{
    this_frame_stats = network_statistics();

    tick_gameserver_ping_responses();

    ///tries to join
    join_master();

    bool any_recv_from_master = true;

    while(any_recv_from_master && sock_readable(to_master_udp))
    {
        sockaddr_storage to_master_store;

        //auto data = tcp_recv(to_master);

        auto data = udp_receive_from(to_master_udp, &to_master_store);

        any_recv_from_master = data.size() > 0;

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

                //if(servs.size() > 0)
                {
                    //server_list = servs;
                    server_list = merge_gameservers(server_list, servs);

                    has_serverlist = true;

                    print_serverlist();

                    ///we're done here
                    //to_master.close();
                }
                /*else
                {
                    lg::log("Network error or 0 gameservers available");

                    pinged = false; ///invalid response, ping again
                }*/
            }
        }
    }

    bool any_recv = true;

    while(any_recv && sock_readable(connected_server.to_game))
    {
        auto data = udp_receive_from(connected_server.to_game, &connected_server.to_game_store);
        is_init = true;

        any_recv = data.size() > 0;

        byte_fetch fetch;
        fetch.ptr.swap(data);

        this_frame_stats.bytes_in += data.size();

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
                int32_t player_id = fetch.get<net_type::player_t>();

                int32_t component_id = fetch.get<net_type::component_t>();

                int32_t len = fetch.get<net_type::len_t>();

                void* payload = fetch.get(len);

                int found_end = fetch.get<int32_t>();

                if(canary_end != found_end)
                {
                    lg::log("bad forward canary");

                    continue;
                }

                if(connected_server.discovered_fighters[player_id].id == -1 && connected_server.have_id)
                {
                    connected_server.discovered_fighters[player_id] = make_networked_player(player_id, ctx, tctx, st, phys, graphics_settings);

                    for(auto& i : connected_server.discovered_fighters)
                    {
                        if(i.first == player_id)
                            continue;
                    }

                    lg::log("made a new networked player ", player_id);

                    continue;
                }

                ///I don't have my id yet, which means that everything might be fucked
                ///I actually am not 100% sure if this is a problem

                if(!connected_server.have_id)
                {
                    lg::log("no id, skipping");
                    connected_server.discovered_fighters.clear();
                    continue;
                }

                network_player& play = connected_server.discovered_fighters[player_id];

                std::map<int, ptr_info> arg_map = build_fighter_network_stack(&play, &connected_server);

                if(component_id < 0 || component_id >= arg_map.size())
                {
                    lg::log("err in forwarding");
                    continue; ///?
                }

                ptr_info comp = arg_map[component_id];

                if(len != comp.size || comp.size == 0 || comp.ptr == nullptr)
                {
                    lg::log("err in argument");
                    continue;
                }

                memmove(comp.ptr, payload, comp.size);

                ///fucking thank christ, its not a networking issue
                //if(is_damage_info(play.fight, comp.ptr))
                //    lg::log("damage");

                play.disconnect_timer.restart();

                if(connected_server.packet_callback_perplayer.find(player_id) != connected_server.packet_callback_perplayer.end())
                {
                    if(connected_server.packet_callback_perplayer[player_id].find(component_id) != connected_server.packet_callback_perplayer[player_id].end())
                    {
                        connected_server.packet_callback_perplayer[player_id][component_id](comp.ptr, comp.size);
                    }
                }
            }

            if(type == message::CLIENTJOINACK)
            {
                int32_t recv_id = fetch.get<int32_t>();

                int32_t canary_found = fetch.get<int32_t>();

                if(canary_found == canary_end)
                {
                    connected_server.joined_server = true;
                    connected_server.have_id = true;
                    connected_server.my_id = recv_id;

                    lg::log("Got joinack, I have id: ", connected_server.my_id);
                }
            }

            if(type == message::TEAMASSIGNMENT)
            {
                int32_t client_id = fetch.get<int32_t>();
                int32_t team = fetch.get<int32_t>();

                int32_t canary_found = fetch.get<int32_t>();

                //printf("teamassign id %i team %i\n", client_id, team);

                if(canary_found == canary_end)
                {
                    if(connected_server.have_id &&
                            connected_server.discovered_fighters.find(client_id) != connected_server.discovered_fighters.end() &&
                            connected_server.discovered_fighters[client_id].fight != nullptr)
                    {
                        connected_server.discovered_fighters[client_id].fight->set_team(team);
                    }
                    else
                    {
                        lg::log("teamassignskip");
                    }
                }
                else
                {
                    lg::log("Team assign canary err");
                }

                //printf("postteam\n");
            }

            if(type == message::GAMEMODEUPDATE)
            {
                just_new_round = false;

                bool prev_game = connected_server.game_info.game_over();

                connected_server.game_info.process_gamemode_update(fetch);

                ///just swapped from not game over to it is game over!
                if(prev_game == true && !connected_server.game_info.game_over())
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
                    if(connected_server.have_id && connected_server.discovered_fighters[connected_server.my_id].fight != nullptr)
                        connected_server.discovered_fighters[connected_server.my_id].fight->respawn(new_pos);
                    else
                        lg::log("respawn skip");
                }
                else
                {
                    lg::log("canary error in respawnresponse");
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
                    connected_server.respawn_inf = {time_elapsed, respawn_time};
                }
                else
                {
                    lg::log("canary err in respawninfo");
                }
            }

            if(type == message::FORWARDING_RELIABLE)
            {
                reliable_manager.insert_forwarding_from_forwarding_reliable_into_stream(fetch);
            }

            if(type == message::FORWARDING_RELIABLE_ACK)
            {
                reliable_manager.process_forwarding_reliable_ack(fetch);
            }

            ///we need a generic message transport system so i dont have to update the gameservers
            if(type == message::PING)
            {
                handle_ping(fetch);
            }

            if(type == message::PING_DATA)
            {
                handle_ping_data(fetch);
            }

            ///should never happen
            if(type == message::PING_RESPONSE)
            {
                /*handle_ping_response(fetch);*/

                lg::log("Err, this is totally invalid, ping response");
            }

            if(type == message::PING_GAMESERVER_RESPONSE)
            {
                ///but ignore the ping we set, as we already have this
                handle_ping_gameserver_response(fetch);
            }
        }
    }

    if(is_init && connected_server.to_game.valid())// && (time_since_last_send.getElapsedTime().asMicroseconds() / 1000.f) > time_between_sends_ms)
    {
        if(connected_server.have_id && connected_server.discovered_fighters[connected_server.my_id].fight != nullptr)
        {
            fighter* nfight = connected_server.discovered_fighters[connected_server.my_id].fight;
            network_fighter* net_nfight = connected_server.discovered_fighters[connected_server.my_id].net_fighter;

            network_fighter network_backup = *net_nfight;

            ///remove damage I've taken from player who i blocked in a clientside parry
            nfight->process_delayed_deltas();
            nfight->eliminate_clientside_parry_invulnerability_damage();

            ///so, everyone receives the hp_delta of the client, but only the hoest is updating this piece of information here
            for(int i=0; i<nfight->parts.size(); i++)
            {
                part& p = nfight->parts[i];
                network_part_info& net_p = net_nfight->network_parts[i];

                ///this structure is NOT polluted *EVER* with local client data
                ///local client data is stored in a separate structure, and only used above
                ///this copy is restored afterwards
                if(net_p.requested_damage_info.networked_val.hp_delta != 0)
                {
                    delayed_delta delt;
                    ///hmm. Update internal is never called on the networking model stuff
                    delt.delayed_info = net_p.requested_damage_info.networked_val;

                    int32_t id_who_hit_me = net_p.requested_damage_info.networked_val.id_hit_by;

                    if(id_who_hit_me >= 0)
                    {
                        network_player& play = connected_server.discovered_fighters[id_who_hit_me];

                        if(play.id >= 0)
                        {
                            float my_ping = nfight->net.ping;
                            float their_ping = play.fight->net.ping;

                            #ifndef DELAY_SIMULATE
                            float full_rtt_time = my_ping + their_ping;
                            #else
                            float full_rtt_time = my_ping + their_ping + delay_ms*2;
                            #endif

                            delt.delay_time_ms = full_rtt_time;

                            lg::log("delaying by ", delt.delay_time_ms, "ms");
                        }
                        else
                        {
                            lg::log("Error, invalid fighter beginning delayed damage delta ", play.id);

                            connected_server.discovered_fighters.erase(id_who_hit_me);
                        }
                    }

                    p.net.delayed_delt.push_back(delt);

                    lg::log("Logging the start of a delayed delta from ", net_p.requested_damage_info.networked_val.id_hit_by);

                    ///if two packets arrive at once, this will not work correctly
                    nfight->last_hp_delta = net_p.requested_damage_info.networked_val.hp_delta;
                    nfight->last_part_id = i;

                    ///???
                    net_p.requested_damage_info.networked_val.hp_delta = 0.f;

                    nfight->player_id_i_was_last_hit_by = net_p.requested_damage_info.networked_val.id_hit_by;

                    //printf("You've been hit by, you've been struck by, player with id %i\n", my_fighter->player_id_i_was_last_hit_by);
                }
            }

            ///wtf? We're overwriting the whole networking model for this fighter!!
            *connected_server.discovered_fighters[connected_server.my_id].net_fighter = connected_server.discovered_fighters[connected_server.my_id].fight->construct_network_fighter();

            std::map<int, ptr_info> host_stack = build_host_network_stack(&connected_server.discovered_fighters[connected_server.my_id], &connected_server);

            ///update remote fighters about me
            for(auto& i : host_stack)
            {
                ptr_info inf = i.second;

                int32_t id = i.first;

                byte_vector vec;
                vec.push_back(canary_start);
                vec.push_back(message::FORWARDING);
                vec.push_back<net_type::player_t>(connected_server.my_id);
                vec.push_back<net_type::component_t>(id);

                vec.push_back<net_type::len_t>(inf.size);
                vec.push_back((uint8_t*)inf.ptr, inf.size);

                vec.push_back(canary_end);

                //udp_send_to(to_game, vec.ptr, (const sockaddr*)&to_game_store);
                network_update_wrapper_clump(this, vec);
            }

            ///uuh. Looking increasingly like we should just include the home fighter in this one, eh?
            ///all networking for foreign fighters and some of my own (its the same for both)
            ///except home fighters network more things per-frame
            for(auto& net_fighter : connected_server.discovered_fighters)
            {
                fighter* fight = net_fighter.second.fight;
                int fight_id = net_fighter.first;
                network_fighter* net_fight = net_fighter.second.net_fighter;

                ///? should be impossibru
                if(!fight)
                {
                    lg::log("Null fighter, should be impossible");
                    continue;
                }

                network_fighter backup = *net_fight;

                fight->modify_existing_network_fighter_with_local(*net_fight);

                if(net_fight->network_fighter_inf.recoil_requested.should_send())
                {
                    network_update_element<int32_t>(this, net_fight->network_fighter_inf.recoil_requested.get_networking_ptr(), &net_fighter.second);
                }

                if(net_fight->network_fighter_inf.recoil_forced.should_send())
                {
                    network_update_element<int32_t>(this, net_fight->network_fighter_inf.recoil_forced.get_networking_ptr(), &net_fighter.second);
                }

                /*if(fight->net.recoil_dirty)
                {
                    ///make me reliable too! yay!
                    ///make reliable?
                    network_update_element<int32_t>(this, &fight->net.recoil, &net_fighter.second);
                    network_update_element<int32_t>(this, &fight->net.force_recoil, &net_fighter.second);

                    fight->net.recoil_dirty = false;
                }*/

                ///ok so this still doesnt work for quite a few reasons
                ///mainly we send the delta and then 0 it, which makes parts respawn
                ///ok so under the delta scheme, tha player never actually dies
                ///which means that under the current method
                ///they'll just be teleported across the map
                ///parts don't reinitialise properly
                ///I believe this is fixed now
                if(connected_server.my_id != net_fighter.first)
                {
                    ///maybe if fight->local?
                    if(fight->local.send_clang_audio)
                    {
                        fight->net.play_clang_audio = 1;

                        network_update_element_reliable<int32_t>(this, &fight->net.play_clang_audio, &net_fighter.second);

                        fight->net.play_clang_audio = 0;
                        fight->local.send_clang_audio = 0;
                    }

                    //for(part& i : fight->parts)
                    for(int kk=0; kk < fight->parts.size(); kk++)
                    {
                        part& local_part = fight->parts[kk];

                        /*if(i.net.hp_dirty)
                        {
                            network_update_element_reliable<damage_information>(this, &i.net.damage_info, &net_fighter.second);

                            i.net.damage_info.hp_delta = 0.f;

                            i.net.hp_dirty = false;
                        }*/

                        network_part_info& net_part = net_fight->network_parts[kk];

                        ///so the local hp delta should be reset in construct from network
                        if(net_part.requested_damage_info.should_send())
                        {
                            network_update_element_reliable<damage_info>(this, net_part.requested_damage_info.get_networking_ptr(), &net_fighter.second);

                            //printf("Sending damage info %f %i\n", net_part.requested_damage_info.networked_val.hp_delta, net_part.requested_damage_info.networked_val.id_hit_by);
                        }

                        if(local_part.local.send_hit_audio)
                        {
                            local_part.net.play_hit_audio = 1;

                            network_update_element_reliable<int32_t>(this, &local_part.net.play_hit_audio, &net_fighter.second);

                            local_part.net.play_hit_audio = 0;
                            local_part.local.send_hit_audio = 0;
                        }
                    }
                }

                if(fight->net.reported_dead)
                {
                    int32_t player_id = connected_server.get_id_from_fighter(fight);
                    int32_t player_who_killed_me_id = fight->player_id_i_was_last_hit_by;
                    ///im pretty sure this is valid if fight->net.reported_dead is true
                    ///the sequence is - fighter hits player, broadcasts hp_delta and their id for the hit into
                    ///fight recognises his own death on every person's game and sends reported dead (checked_death) to server (this function!)
                    ///that means that the last hit id must be valid at this point, unless there is a logic error

                    ///this should be impossible
                    if(player_id == -1)
                    {
                        lg::log("Fighter reported dead, but does not exist on networking");
                        continue;
                    }

                    lg::log("fighter killed by ", player_who_killed_me_id);

                    byte_vector vec;
                    vec.push_back<int32_t>(canary_start);
                    vec.push_back<int32_t>(message::REPORT);
                    vec.push_back<int32_t>(report::DEATH);
                    vec.push_back<int32_t>(player_id);
                    vec.push_back<int32_t>(sizeof(player_who_killed_me_id)); ///4 bytes of extra data
                    vec.push_back<int32_t>(player_who_killed_me_id); ///extra data
                    vec.push_back<int32_t>(canary_end);

                    udp_send(connected_server.to_game, vec.ptr);

                    fight->net.reported_dead = 0;
                }

                *net_fight = backup;
            }

            fighter* my_fighter = connected_server.discovered_fighters[connected_server.my_id].fight;
            network_fighter* net_fighter = connected_server.discovered_fighters[connected_server.my_id].net_fighter;

            ///my name is not my network name
            ///update my network name and pipe to other clients
            if(my_fighter->name_resend_timer.getElapsedTime().asMilliseconds() > my_fighter->name_resend_time)
            {
                network_update_element<vec<MAX_NAME_LENGTH + 1, int8_t>>(this, &net_fighter->network_fighter_inf.name, &connected_server.discovered_fighters[connected_server.my_id]);

                my_fighter->name_resend_timer.restart();
            }

            ///spam server with packets until it respawns us
            if(my_fighter->dead())
            {
                byte_vector vec;
                vec.push_back<int32_t>(canary_start);
                vec.push_back<int32_t>(message::RESPAWNREQUEST);
                vec.push_back<int32_t>(canary_end);

                udp_send(connected_server.to_game, vec.ptr);
            }

            time_since_last_send.restart();

            *net_nfight = network_backup;
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

    for(auto& i : connected_server.discovered_fighters)
    {
        if(i.first == connected_server.my_id)
        {
            if(i.second.id >= 0 && i.second.fight)
                i.second.fight->save_network_representation(*i.second.net_fighter);

            continue;
        }

        if(i.second.id < 0)
        {
            lg::log("super bad error, invalid fighter");
            continue;
        }

        ///
        i.second.fight->construct_from_network_fighter(*i.second.net_fighter);

        ///for some reason, its respawning the other player a 2/3 parts dead
        ///then 3/3 parts die, and then goes to 0/3 parts
        ///makes no sense whatsoever
        ///perhaps because of the network update mechanism

        ///ok, so its because i send the death thing
        ///they send me the hp they have currently
        ///this causes me to respawn the fighter
        ///then that causes me to overwrite their hp with the spawned fighter's hp
        ///ffs
        i.second.fight->manual_check_part_alive();

        ///we need a die if appropriate too
        i.second.fight->respawn_if_appropriate();
        i.second.fight->checked_death();

        i.second.fight->overwrite_parts_from_model();
        i.second.fight->manual_check_part_death();

        //i.second.fight->my_cape.tick(i.second.fight);

        i.second.fight->shared_tick(this);

        i.second.fight->tick_cape();

        i.second.fight->do_foot_sounds();

        i.second.fight->update_texture_by_part_hp();

        ///this might cause a small delay as net sound will get sent NEXT tick
        i.second.fight->check_and_play_sounds();

        i.second.fight->position_cosmetics();

        i.second.fight->update_last_hit_id();

        ///we're setting my fighters network recoil stuff, but it isnt being applied for some reason
        ///probably due to the order of applying network models
        ///so at this point, this fighter has had its net_fighter_copy thing updated with recoil request
        ///if clientside parry
#define ENABLE_CLIENTSIDE_PARRY
#ifdef ENABLE_CLIENTSIDE_PARRY
        if(connected_server.have_id && connected_server.discovered_fighters[connected_server.my_id].fight)
            i.second.fight->check_clientside_parry(connected_server.discovered_fighters[connected_server.my_id].fight);
#endif

        i.second.fight->network_update_render_positions();

        ///uuh. there uuh. Ok
        ///Why is this here? Seriously?
        ///Subobjects for robustness?
        i.second.fight->weapon.obj()->set_pos(i.second.fight->weapon.obj()->pos);
        i.second.fight->weapon.obj()->set_rot(i.second.fight->weapon.obj()->rot);

        //i.second.fight->weapon.set_pos(xyz_to_vec(i.second.fight->weapon.obj()->pos));
        //i.second.fight->weapon.set_rot(xyz_to_vec(i.second.fight->weapon.obj()->rot));


        ///death is dynamically calculated from part health
        if(!i.second.fight->dead())
        {
            //i.second.fight->update_name_info(true);

            i.second.fight->update_lights();
        }

        if(i.second.disconnect_timer.getElapsedTime().asMicroseconds() / 1000.f >= i.second.disconnect_time_ms
                && !i.second.fight->dead())
        {
            i.second.fight->die();
            i.second.cleanup = true;

            lg::log("Disconnected player ", i.second.fight->network_id);
        }
    }

    for(auto it = connected_server.discovered_fighters.begin(); it != connected_server.discovered_fighters.end(); it++)
    {
        if(it->second.cleanup)
        {
            it = connected_server.discovered_fighters.erase(it);
        }
    }

    connected_server.game_info.tick();
    reliable_manager.tick(connected_server.to_game);
    packet_clump.tick();
}

void server_networking::print_serverlist()
{
    lg::log("START GAMESERVER LIST:");

    for(int i=0; i<(int)server_list.size(); i++)
    {
        lg::log("SN: ", i, " ", server_list[i].address.c_str(), ":", server_list[i].their_host_port.c_str());
    }
}

///so, its possible that the spamming of no id is causing problems
///nope, thatll just leak memory, which is uuh. Fine? Probably causing huge memory leaks?
network_player server_networking::make_networked_player(int32_t id, object_context* ctx, object_context* tctx, gameplay_state* current_state, physics* phys, int quality)
{
    fighter* net_fighter = new fighter(*ctx, *ctx->fetch());

    net_fighter->set_team(0);
    net_fighter->set_pos({0, 0, 0});
    net_fighter->set_rot({0, 0, 0});
    net_fighter->set_quality(quality); ///???
    net_fighter->set_gameplay_state(current_state);

    net_fighter->set_network_id(id);

    ctx->load_active();

    net_fighter->set_physics(phys);
    net_fighter->update_render_positions();

    ctx->build_request();

    net_fighter->update_lights();

    network_player play;
    play.fight = net_fighter;
    play.id = id;
    play.net_fighter = new network_fighter;

    //ctx->flip();

    play.fight->set_secondary_context(tctx);

    play.fight->set_name("Loading");

    return play;
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
        lg::log("err in proces gamemode updates bad canary");
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
    return current_session_state.game_over(current_session_boundaries);
}

///needs to be gamemode specific really
std::string gamemode_info::get_game_over_string()
{
    return current_session_state.get_game_over_string(current_session_boundaries);
}

///gamemode?
///centre align?
std::string gamemode_info::get_display_string()
{
    return current_session_state.get_current_game_state_string(current_session_boundaries);
}

std::vector<fighter*> server_networking::get_fighters()
{
    std::vector<fighter*> ret;

    for(auto& i : connected_server.discovered_fighters)
    {
        if(i.second.fight)
        {
            ret.push_back(i.second.fight);
        }
    }

    return ret;
}

void server_networking::set_graphics(int graphics)
{
    graphics_settings = graphics;
}

network_statistics server_networking::get_frame_stats()
{
    return this_frame_stats;
}
