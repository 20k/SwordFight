#ifndef RELIABILITY_SHARED_HPP_INCLUDED
#define RELIABILITY_SHARED_HPP_INCLUDED

#include <vector>
#include <stdint.h>
#include <SFML/system.hpp>

struct byte_fetch;
struct byte_vector;
struct udp_sock;
struct forwarding_info;
struct sockaddr_storage;

struct reliability_manager
{
    ///data to send every tick
    //std::vector<forwarding_info> forwarding_data;
    std::vector<forwarding_info> data_receiving;
    std::vector<forwarding_info> data_sending;

    std::vector<uint32_t> packet_ids_to_ack;

    ///not a regular message, only the critical data
    ///we actually want to send
    ///this ONLY WILL WORK with forwarding data
    ///although, i can extend this easily so
    void add(const byte_vector& vec);
    void add(const byte_vector& vec, uint32_t reliable_id);

    byte_vector strip_forwarding(const byte_vector& vec);

    void tick(udp_sock& sock);
    void tick(udp_sock& sock, sockaddr_storage& store);

    static byte_vector strip_data_from_forwarding_reliable(byte_fetch& fetch, uint32_t& out_reliable_id);
    void insert_forwarding_from_forwarding_reliable_into_stream(byte_fetch& fetch);
    void process_forwarding_reliable_ack(byte_fetch& fetch);
    void register_ack_forwarding_reliable(uint32_t reliable_id);

    uint32_t gid = 0;
    sf::Clock clk;
};

#endif // RELIABILITY_SHARED_HPP_INCLUDED
