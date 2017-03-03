#include "reliability_shared.hpp"
#include <net/shared.hpp>
#include "master_server/network_messages.hpp"
#include "../openclrenderer/logging.hpp"

///wraps data in forward reliable, and gives it an id
void reliability_manager::add(const byte_vector& vec)
{
    std::vector<char> data = vec.ptr;

    uint32_t id = gid++;

    byte_vector ret;
    ret.push_back(canary_start);
    ret.push_back(message::FORWARDING_RELIABLE);
    ret.push_back(id);
    ret.push_string(data, data.size());
    ret.push_back(canary_end);

    forwarding_info inf;
    inf.data = ret;
    inf.id = id;

    data_sending.push_back(inf);
}

///server and client keep different gid counts, which means that the
///reliability ids will conflict
///we can fix this by fixing the retarded tick function
////SISIGHSIGH
void reliability_manager::add(const byte_vector& vec, uint32_t reliable_id)
{
    for(auto& i : data_sending)
    {
        ///already going to send this data
        if(i.id == reliable_id)
            return;
    }

    std::vector<char> data = vec.ptr;

    uint32_t id = reliable_id;

    byte_vector ret;
    ret.push_back(canary_start);
    ret.push_back(message::FORWARDING_RELIABLE);
    ret.push_back(id);
    ret.push_string(data, data.size());
    ret.push_back(canary_end);

    forwarding_info inf;
    inf.data = ret;
    inf.id = id;

    data_sending.push_back(inf);
}

///stripper with class
/*byte_vector reliability_manager::strip_forwarding(const byte_vector& vec)
{
    std::vector<char> data = vec.ptr;

    int front_data_to_strip = sizeof(canary_start) + sizeof(message::FORWARDING);
    int rear_data_to_strip = sizeof(canary_end);

    for(int i=0; i<front_data_to_strip; i++)
    {
        data.erase(data.begin());
    }

    for(int i=0; i<rear_data_to_strip; i++)
    {
        data.pop_back();
    }

    byte_vector ret;
    ret.push_string(data, data.size());

    return ret;
}*/

void reliability_manager::tick(udp_sock& sock)
{
    auto store = sock.get_peer_sockaddr();

    tick(sock, store);
}

void reliability_manager::tick(udp_sock& sock, sockaddr_storage& store)
{
    if(!sock.valid())
    {
        data_receiving.clear();
        data_sending.clear();
        packet_ids_to_ack.clear();

        return;
    }

    float time_elapsed = clk.getElapsedTime().asMicroseconds() / 1000.f;
    clk.restart();

    const float timeout_time = 2000.f; ///ms

    ///for all of our forwarding data, send it to the server
    for(int i=0; i<data_sending.size(); i++)
    {
        forwarding_info& inf = data_sending[i];

        ///if we're meant to be sending the data, send it
        ///otherwise we simply received the data and we're piping acks
        if(!inf.skip_send)
            udp_send_to(sock, inf.data.ptr, (const sockaddr*)&store);

        inf.time_elapsed += time_elapsed;

        if(inf.time_elapsed > timeout_time)
        {
            data_sending.erase(data_sending.begin() + i);
            i--;
        }
    }

    for(int i=0; i<data_receiving.size(); i++)
    {
        forwarding_info& inf = data_receiving[i];

        inf.time_elapsed += time_elapsed;

        if(inf.time_elapsed > timeout_time)
        {
            data_receiving.erase(data_receiving.begin() + i);
            i--;
        }
    }

    ///for all of the data we've received from the server
    ///that needs to be acked
    ///we could probably remove this entirely, and just use data_receiving
    for(int i=0; i<packet_ids_to_ack.size(); i++)
    {
        ///find the original forwarding data as that contains
        ///the ack status. This is a pretty shitty way of doing this
        //for(auto& inf : forwarding_data)
        {
            ///if we've found the original data and we haven't acked
            ///send an ack
            ///invalid
            //if(inf.id == packet_ids_to_ack[i] && !inf.sent_ack)
            {
                byte_vector vec;
                vec.push_back(canary_start);
                vec.push_back(message::FORWARDING_RELIABLE_ACK);
                vec.push_back<uint32_t>(packet_ids_to_ack[i]);
                vec.push_back(canary_end);

                udp_send_to(sock, vec.data(), (const sockaddr*)&store);

                /*inf.sent_ack = true;

                break;*/
            }
        }
    }

    ///we've acked all packets that need acking
    packet_ids_to_ack.clear();
}


byte_vector reliability_manager::strip_data_from_forwarding_reliable(byte_fetch& arg, uint32_t& out_reliability_id)
{
    byte_fetch fetch = arg;

    uint32_t reliable_id = fetch.get<uint32_t>();
    int32_t player_id = fetch.get<int32_t>(); ///this is per the forwarding_reliable spec, not the forwarding spec
    int32_t component_id = fetch.get<int32_t>();

    int32_t len = fetch.get<int32_t>();

    std::vector<char> dat = fetch.get_buf(len);

    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
    {
        lg::log("canary error in convert_forwarding");
        out_reliability_id = -1;
        return byte_vector();
    }

    arg = fetch;

    ///construct forwarding message, and insert it into the byte_fetch stream
    byte_vector vec;
    //vec.push_back(canary_start);
    //vec.push_back(message::FORWARDING);
    vec.push_back<int32_t>(player_id);
    vec.push_back<int32_t>(component_id);
    vec.push_back<int32_t>(len);
    vec.push_string(dat, dat.size());
    //vec.push_back(canary_end);

    out_reliability_id = reliable_id;

    return vec;
}

///received data from the server, need to register an ack
///we need a function that does exactly this but without pushing the data
///into the stream. Ie for server stuff
///we've received data, so we need to add the data to the received data
///and also set up acks to go back
void reliability_manager::insert_forwarding_from_forwarding_reliable_into_stream(byte_fetch& arg)
{
    ///ID
    ///DATA
    ///CANARY

    byte_fetch fetch = arg;

    uint32_t reliable_id = fetch.get<uint32_t>();
    int32_t player_id = fetch.get<int32_t>();
    int32_t component_id = fetch.get<int32_t>();

    int32_t len = fetch.get<int32_t>();

    //std::vector<char> dat = fetch.get_buf(len);

    std::vector<char> dat;

    //printf("got len %i ", len);

    for(int i=0; i<len; i++)
    {
        dat.push_back(fetch.get<uint8_t>());
    }

    int32_t found_end = fetch.get<int32_t>();

    if(found_end != canary_end)
    {
        lg::log("canary error in convert_forwarding");
        return;
    }

    arg = fetch;

    ///check we've not received the message before
    for(auto& i : data_receiving)
    {
        ///we've received this message before!
        ///server didn't get ack
        if(i.id == reliable_id)
        {
            //i.sent_ack = false;
            i.time_elapsed = 0.f;

            packet_ids_to_ack.push_back(i.id);

            //printf("discarding a reliable ");

            return;
        }
    }

    ///construct forwarding message, and insert it into the byte_fetch stream
    byte_vector vec;
    vec.push_back(canary_start);
    vec.push_back(message::FORWARDING);
    vec.push_back<net_type::player_t>(player_id);
    vec.push_back<net_type::component_t>(component_id);
    vec.push_back<net_type::len_t>(len);
    vec.push_string(dat, dat.size()); ///fixme
    vec.push_back(canary_end);

    fetch.push_back(vec.ptr);

    ///we don't want to send this data, only send acks for it
    forwarding_info inf;
    inf.id = reliable_id;
    inf.skip_send = true;

    packet_ids_to_ack.push_back(reliable_id);
    data_receiving.push_back(inf);

    arg = fetch;

    //printf("received a reliable, inserting into stream\n");
}

///this also needs to have a check to see if we're already
///going to ack the packet?
///explicitly register something to be acked
///so therefore we received the data
///add pretend data so that we'll pipe acks back
void reliability_manager::register_ack_forwarding_reliable(uint32_t reliable_id)
{
    forwarding_info inf;
    inf.id = reliable_id;
    inf.skip_send = true;

    packet_ids_to_ack.push_back(reliable_id);
    //forwarding_data.push_back(inf);
    data_receiving.push_back(inf); ///so we're pretending that we
}

///received an ack from data we sent to the server
void reliability_manager::process_forwarding_reliable_ack(byte_fetch& arg)
{
    byte_fetch fetch = arg;

    uint32_t reliable_id = fetch.get<uint32_t>();

    int32_t found_canary = fetch.get<int32_t>();

    if(found_canary != canary_end)
    {
        lg::log("canary error in process forwarding reliable ack");
        return;
    }

    //printf("got ack\n");

    arg = fetch;

    ///we've received an ack, now stop forwarding the data!
    for(int i=0; i<data_sending.size(); i++)
    {
        if(data_sending[i].id == reliable_id)
        {
            //printf("ack stopped a forwarding\n");

            data_sending.erase(data_sending.begin() + i);
            return;
        }
    }

    //printf("extra ack\n");
}
