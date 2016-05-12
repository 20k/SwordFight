#include <SFML/Graphics.hpp>

#include "browser.hpp"

#include <net/shared.hpp>
#include "../../sword_server/master_server/network_messages.hpp"
#include "../../sword_server/master_server/server.hpp"
//#include "../"


sock_info try_tcp_connect(const std::string& address, const std::string& port)
{
    sock_info inf;

    inf = tcp_connect(address, port, 1, 0);

    return inf;
}

///do this ? (functionme you idiot)
/*if(sock_readable(to_master))
{
    auto data = tcp_recv(to_master);

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
            auto servs = network_static_whateverget_serverlist(fetch);

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
                lg::log("Network error or 0 gameservers available");

                pinged = false; ///invalid response, ping again
            }
        }
    }
}*/





int main()
{
    sf::RenderWindow win(sf::VideoMode(800, 600), "hello");

    /*ui_element root(ui::TEXT);

    root.text = "hi";

    root.set_relative_pos({10, 0});

    ui_element subnode(ui::TEXT);

    subnode.text = "bum";

    subnode.set_parent(&root);

    subnode.set_relative_pos({10, 0});


    ui_element r2(ui::TEXT);
    r2.text = "r2";

    r2.set_relative_pos({0,0});

    r2.set_parent(&subnode);

    ui_element r3(ui::TEXT);
    r3.text = "r3";

    r3.set_relative_pos({0,0});

    r3.set_parent(&root);

    root.propagate_positions({0,0});*/

    ui_element root(ui::NONE);
    root.vertical_pad = 0;

    sock_info inf = try_tcp_connect(MASTER_IP, MASTER_PORT);
    tcp_sock to_server;

    //std::map<std::string, serv> server_map;

    while(win.isOpen())
    {
        sf::Event event;

        while(win.pollEvent(event))
        {

        }

        if(!to_server.valid())
        {
            if(!inf.within_timeout())
            {
                ///resets sock info
                inf.retry();
                inf = try_tcp_connect(MASTER_IP, MASTER_PORT);

                printf("trying\n");
            }

            if(inf.valid())
            {
                to_server = tcp_sock(inf.get());

                byte_vector vec;
                vec.push_back(canary_start);
                vec.push_back(message::CLIENT);
                vec.push_back(canary_end);

                tcp_send(to_server, vec.ptr);

                printf("found master server\n");
            }

            //continue;
        }

        /*root.draw(win);
        subnode.draw(win);
        r2.draw(win);
        r3.draw(win);*/

        root.draw_tree(win);

        win.display();
        win.clear(sf::Color(0,0,0));
    }

}
