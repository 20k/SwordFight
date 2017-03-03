#ifndef NETWORK_UPDATE_WRAPPERS_HPP_INCLUDED
#define NETWORK_UPDATE_WRAPPERS_HPP_INCLUDED

template<typename T>
inline
int get_position_of(std::map<int, ptr_info>& stk, T* elem)
{
    for(int i=0; i<stk.size(); i++)
    {
        if(stk[i].ptr == elem)
            return i;
    }

    return -1;
}

///any network discovered fighter

struct delay_information
{
    sf::Clock time;
    byte_vector vec;
    int which = 0; ///regular or reliable
    server_networking* net = nullptr;
};

//#define DELAY_SIMULATE

extern std::vector<delay_information> delay_vectors;
extern float delay_ms;

inline
void
network_update_wrapper(server_networking* net, const byte_vector& vec)
{
    #ifndef DELAY_SIMULATE
    udp_send_to(net->connected_server.to_game, vec.ptr, (const sockaddr*)&net->connected_server.to_game_store);
    #else
    delay_information info;
    info.vec = vec;
    info.net = net;

    delay_vectors.push_back(info);
    #endif

    net->this_frame_stats.bytes_out += vec.ptr.size();
}

inline
void
network_update_wrapper_clump(server_networking* net, const byte_vector& vec)
{
    ///if we simulate delays, we won't clump. But this isn't the end of the world
    #ifndef DELAY_SIMULATE
    //udp_send_to(net->to_game, vec.ptr, (const sockaddr*)&net->to_game_store);
    net->packet_clump.add_send_data(net->connected_server.to_game, net->connected_server.to_game_store, vec.ptr);
    #else
    delay_information info;
    info.vec = vec;
    info.net = net;

    delay_vectors.push_back(info);
    #endif

    net->this_frame_stats.bytes_out += vec.ptr.size();
}

///why don't we clump this..?
template<typename T>
inline
void
network_update_element(server_networking* net, T* element, network_player* net_fight, int S = sizeof(T))
{
    fighter* fight = net_fight->fight;

    auto memory_map = build_fighter_network_stack(net_fight, &net->connected_server);

    int32_t pos = get_position_of(memory_map, element);

    if(pos == -1)
    {
        lg::log("Error in network update element -1");
        return;
    }

    int32_t network_id = net->connected_server.get_id_from_fighter(fight);

    if(network_id == -1)
    {
        lg::log("Error in network update netid -1");
        return;
    }

    if(!net->is_init || !net->connected_server.to_game.valid())
    {
        lg::log("Some kind of... network error, spewing errors back into the observable universe!");
        return;
    }

    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::FORWARDING);
    vec.push_back<net_type::player_t>(network_id);
    vec.push_back<net_type::component_t>(pos);

    //int32_t S = sizeof(T);

    vec.push_back<net_type::len_t>(S);
    vec.push_back((uint8_t*)element, S);
    vec.push_back(canary_end);

    network_update_wrapper(net, vec);
}

template<typename T>
inline
void
network_update_element_reliable(server_networking* net, T* element, network_player* net_fight)
{
    auto memory_map = build_fighter_network_stack(net_fight, &net->connected_server);

    int32_t pos = get_position_of(memory_map, element);

    if(pos == -1)
    {
        lg::log("Error in reliable network update element -1");
        return;
    }

    int32_t network_id = net->connected_server.get_id_from_fighter(net_fight->fight);

    if(network_id == -1)
    {
        lg::log("Error in reliable network update netid -1");
        return;
    }

    if(!net->is_init || !net->connected_server.to_game.valid())
    {
        lg::log("Some kind of... reliable network error, spewing errors back into the observable universe!");
        return;
    }

    byte_vector vec;
    vec.push_back<int32_t>(network_id);
    vec.push_back<int32_t>(pos);

    int32_t S = sizeof(T);

    vec.push_back<int32_t>(S);
    vec.push_back((uint8_t*)element, S);

    #ifndef DELAY_SIMULATE
    net->reliable_manager.add(vec);
    #else
    delay_information info;
    info.vec = vec;
    info.which = 1;
    info.net = net;

    delay_vectors.push_back(info);
    #endif
}

inline
void delay_tick()
{
    for(int i=0; i<delay_vectors.size(); i++)
    {
        delay_information& info = delay_vectors[i];

        if(info.time.getElapsedTime().asMicroseconds() / 1000.f > delay_ms)
        {
            if(info.which == 0)
                udp_send_to(info.net->connected_server.to_game, info.vec.ptr, (const sockaddr*)&info.net->connected_server.to_game_store);
            else
                info.net->reliable_manager.add(info.vec);

            //printf("dinfo %f\n", info.time.getElapsedTime().asMicroseconds() / 1000.f);

            delay_vectors.erase(delay_vectors.begin() + i);
            i--;
        }
    }
}


#endif // NETWORK_UPDATE_WRAPPERS_HPP_INCLUDED
