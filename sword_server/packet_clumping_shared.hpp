#ifndef PACKET_CLUMPING_SHARED_HPP_INCLUDED
#define PACKET_CLUMPING_SHARED_HPP_INCLUDED

#include <net/shared.hpp>
#include <map>
#include <vector>

struct net_dest
{
    udp_sock sock;
    sockaddr_storage store;
    std::vector<char> data;
};

struct net_info
{
    udp_sock sock;
    sockaddr_storage store;
};


struct packet_clumper
{
    std::vector<net_dest> destination_to_senddata;
    std::vector<net_info> destinations;

    void add_send_data(udp_sock& sock, sockaddr_storage& store, const std::vector<char>& dat)
    {
        net_dest dest = {sock, store, dat};

        destination_to_senddata.push_back(dest);

        net_info info = {sock, store};

        for(auto& i : destinations)
        {
            if(i.store == info.store)
                return;
        }

        destinations.push_back(info);
    }

    void tick()
    {
        for(auto& i : destinations)
        {
            std::vector<char> data;

            for(auto& nd : destination_to_senddata)
            {
                if(i.store == nd.store)
                {
                    data.insert(data.end(), nd.data.begin(), nd.data.end());
                }
            }

            udp_send_to(i.sock, data, (sockaddr*)&i.store);
        }

        destination_to_senddata.clear();
        destinations.clear();
    }
};

#endif // PACKET_CLUMPING_SHARED_HPP_INCLUDED
