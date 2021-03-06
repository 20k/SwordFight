#include "ui_manager.hpp"
#include "../imgui/imgui-SFML.h"
#include "../openclrenderer/settings_loader.hpp"
#include "fighter.hpp"
#include "server_networking.hpp"
#include "imgui_extension.hpp"
#include "server_networking.hpp"
#include "../sword_server/game_mode_shared.hpp"

int window_element_ids::label_gid;

ui_manager::ui_manager()
{
    ImGuiIO io = ImGui::GetIO();
    io.MouseDrawCursor = false;

    style = ImGui::GetStyle();
}

void ui_manager::init(settings& s)
{
    sett = &s;

    mouse_sensitivity.set_bound(0.01f, 5.f);
}

void ui_manager::tick(float ftime_ms)
{
    sf::Time t = sf::microseconds(ftime_ms * 1000.f);

    ImGui::SFML::Update(t);
}

void ui_manager::tick_settings(float ftime_ms)
{
    bool config_dirty = false;

    ImGui::Begin("Config (Autosaves). Click and drag, or double click"); // begin window

    mouse_sensitivity.set_slide_speed(0.01f);
    motion_blur_strength.set_slide_speed(0.01f);
    motion_blur_camera_contribution.set_slide_speed(0.01f);

    horizontal_fov_degrees.set_slide_speed(0.1f);


    mouse_sensitivity.set_default(sett->mouse_sens);

    res_x.set_bound(0, 4096);
    res_y.set_bound(0, 4096);

    res_x.set_default(sett->width);
    res_y.set_default(sett->height);

    motion_blur_strength.set_default(sett->motion_blur_strength);
    motion_blur_camera_contribution.set_default(sett->motion_blur_camera_contribution);

    player_name.set_default(sett->name);

    use_post_aa.set_default(sett->use_post_aa);
    use_raw_input.set_default(sett->use_raw_input);

    frames_of_input_lag.set_default(sett->frames_of_input_lag);
    frames_of_input_lag.set_getter_integral_bound(0, 5);

    horizontal_fov_degrees.set_default(sett->horizontal_fov_degrees);
    horizontal_fov_degrees.set_bound(1, 170);

    use_frametime_management.set_default(sett->use_frametime_management);

    is_fullscreen.set_default(sett->is_fullscreen);

    if(saved_settings_w != sett->width || saved_settings_h != sett->height)
    {
        saved_settings_w = sett->width;
        saved_settings_h = sett->height;

        res_x.val = saved_settings_w;
        res_y.val = saved_settings_h;
    }

    if(saved_settings_is_fullscreen != sett->is_fullscreen)
    {
        saved_settings_is_fullscreen = sett->is_fullscreen;

        is_fullscreen.val = sett->is_fullscreen;
    }

    //printf("RES X Y %i %i\n", res_x.val, res_y.val);

    mouse_sensitivity.set_bound(0.001f, 100.f);
    vals.mouse_sens = mouse_sensitivity.instantiate_and_get("Mouse sens").ret;

    if(ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Source mouse sens");
    }

    vals.width = res_x.instantiate_and_get("Res x").ret;
    vals.height = res_y.instantiate_and_get("Res y").ret;

    std::string new_name = player_name.instantiate_and_get("Player Name").ret;

    while(new_name.size() >= MAX_NAME_LENGTH - 1)
    {
        new_name.pop_back();
    }

    player_name.assign(new_name);

    if(sett->name != new_name)
    {
        vals.player_name = new_name;

        sett->name = new_name;

        config_dirty = true;
    }

    motion_blur_strength.set_bound(0.f, 2.f);
    motion_blur_camera_contribution.set_bound(0.f, 1.f);

    vals.motion_blur_strength = motion_blur_strength.instantiate_and_get("Motion blur strength").ret;
    vals.motion_blur_camera_contribution = motion_blur_camera_contribution.instantiate_and_get("Motion blur camera contribution").ret;

    if(sett->motion_blur_strength != vals.motion_blur_strength)
    {
        sett->motion_blur_strength = vals.motion_blur_strength;

        config_dirty = true;
    }

    if(sett->motion_blur_camera_contribution != vals.motion_blur_camera_contribution)
    {
        sett->motion_blur_camera_contribution = vals.motion_blur_camera_contribution;

        config_dirty = true;
    }

    vals.use_post_aa = use_post_aa.instantiate_and_get("Use post AA").ret;
    vals.use_raw_input = use_raw_input.instantiate_and_get("Use raw input").ret;
    vals.use_frametime_management = use_frametime_management.instantiate_and_get("Use frametime management").ret;

    if(ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Adds small delays (up to 0.5ms) to provide a slightly more consistent frame pacing");
    }

    vals.is_fullscreen = is_fullscreen.instantiate_and_get("Fullscreen").ret;

    vals.frames_of_input_lag = frames_of_input_lag.instantiate_and_get("Frames of input lag").ret;

    vals.horizontal_fov_degrees = horizontal_fov_degrees.instantiate_and_get("FoV angle (horizontal)").ret;

    if(ImGui::Button("Update Resolution, Fullscreen, and/or FoV"))
    {
        sett->width = vals.width;
        sett->height = vals.height;
        sett->horizontal_fov_degrees = vals.horizontal_fov_degrees;
        sett->is_fullscreen = vals.is_fullscreen;

        config_dirty = true;
    }

    if(ImGui::Button("Toggle Frametime Window"))
    {
        internal_ftime_show_toggle = !internal_ftime_show_toggle;
    }

    if(ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Or press F1!");
    }

    if(ImGui::Button("Toggle Net Stats Window"))
    {
        internal_net_stats_show_toggle = !internal_net_stats_show_toggle;
    }

    if(ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Or press F2!");
    }

    ImGui::End(); // end window

    if(sett->mouse_sens != vals.mouse_sens)
    {
        sett->mouse_sens = vals.mouse_sens;

        config_dirty = true;
    }

    if(sett->use_post_aa != vals.use_post_aa)
    {
        sett->use_post_aa = vals.use_post_aa;

        config_dirty = true;
    }

    if(sett->use_raw_input != vals.use_raw_input)
    {
        sett->use_raw_input = vals.use_raw_input;

        config_dirty = true;
    }

    if(sett->frames_of_input_lag != vals.frames_of_input_lag)
    {
        sett->frames_of_input_lag = vals.frames_of_input_lag;

        config_dirty = true;
    }

    ///we should really use a string so we can iterate
    if(sett->use_frametime_management != vals.use_frametime_management)
    {
        sett->use_frametime_management = vals.use_frametime_management;

        config_dirty = true;
    }

    if(config_dirty)
    {
        sett->save("./res/settings.txt");
    }

    any_render = true;
}

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out << std::setprecision(n) << a_value;
    return out.str();
}

float get_smoothed(const std::vector<float>& vals, int n)
{
    int start = vals.size() - n - 1;

    if(start < 0)
        start = 0;

    int c = 0;

    float accum = 0.f;

    for(int i=start; i < vals.size(); i++)
    {
        accum += vals[i];

        c++;
    }

    if(c == 0)
        return accum;

    accum /= c;

    return accum;
}

float get_stddev(const std::vector<float>& vals, int n)
{
    if(vals.size() == 0)
        return 0.f;

    float mean = get_smoothed(vals, n);

    float acc = 0;

    for(auto& i : vals)
    {
        float dev = (i - mean);

        dev *= dev;

        acc += dev;
    }

    acc /= vals.size();

    float stddev = sqrt(acc);

    return stddev;
}

float get_average_frametime_variation(const std::vector<float>& vals)
{
    if(vals.size() < 2)
        return 0.f;

    float acc = 0;

    for(int i=1; i<vals.size(); i++)
    {
        float cur = vals[i];
        float last = vals[i-1];

        float diff = fabs(cur - last);

        acc += diff;
    }

    acc /= vals.size() - 1;

    return acc;
}

void ui_manager::tick_frametime_graph(float ftime, bool display)
{
    if(!ftime_paused)
        ftime_history.push_back(ftime);

    if(ftime_history.size() == 0)
        return;

    if(display ^ internal_ftime_show_toggle)
    {
        ImGui::Begin("Frametime"); // begin window

        ///PlotLines(const char* label, const float* values, int values_count,
                     ///int values_offset = 0, const char* overlay_text = NULL,
                     ///float scale_min = FLT_MAX, float scale_max = FLT_MAX,
                     ///ImVec2 graph_size = ImVec2(0,0), int stride = sizeof(float));

        float maxf = 32.f;
        float minf = 4.f;

        float height = 50.f;

        float text_height = ImGui::GetFontSize();

        ImGui::PlotLines("", &ftime_history[0], ftime_history.size(), 0, nullptr, minf, maxf, ImVec2(400, height));

        ImGui::SameLine();

        float smooth_ftime = get_smoothed(ftime_history, 50);
        float stddev = get_stddev(ftime_history, 50);
        float average_frametime_variation = get_average_frametime_variation(ftime_history);

        std::string top = to_string_with_precision(maxf, 2) + " max";
        std::string mid = to_string_with_precision(smooth_ftime, 2) + "ms " + to_string_with_precision(stddev, 2) + "stddev " + to_string_with_precision(average_frametime_variation, 2) + " inter-frame variance";
        std::string bot = to_string_with_precision(minf, 2) + " min";

        ImGui::BeginGroup();

        ImGui::Text(top.c_str());

        ImGui::Dummy(ImVec2(1, height/2 - text_height*2));

        ImGui::Text(mid.c_str());

        ///ok, i admit. I have no idea if this is really correct
        float so_far = height/2 + text_height*4;

        float left = height - so_far;

        ImGui::Dummy(ImVec2(1, left - text_height));

        ImGui::Text(bot.c_str());

        ImGui::EndGroup();

        if(ImGui::Button("Toggle Pause"))
        {
            ftime_paused = !ftime_paused;
        }

        ImGui::End(); // end window

        any_render = true;
    }

    if(ftime_history.size() > 400)
        ftime_history.erase(ftime_history.begin());
}

float getter_func(void* data, int idx)
{
    network_statistics* dat = (network_statistics*)data;

    const float v = (dat + idx)->bytes_out;

    return v;
}

void ui_manager::tick_networking_graph(const network_statistics& net_stats)
{
    net_stats_history.push_back(net_stats);

    if(net_stats_history.size() > 400)
        net_stats_history.erase(net_stats_history.begin());

    //plotgetter data(&net_stats_history[0], sizeof(int) * 2);

    //ImGuiPlotArrayGetterData data(values, stride);
    //PlotEx(ImGuiPlotType_Lines, label, &Plot_ArrayGetter, (void*)&data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);

    if(!internal_net_stats_show_toggle)
        return;

    ImGui::Begin("Net graph");

    ImGui::PlotEx_mult(ImGuiPlotType_Lines, "Graph line", &getter_func, (void*)&net_stats_history[0], net_stats_history.size(), 0, "", FLT_MAX, FLT_MAX, ImVec2(400, 50));

    ImGui::End();

    //ImGui::PlotLines("", &ftime_history[0], ftime_history.size(), 0, nullptr, minf, maxf, ImVec2(400, height));
}

void ui_manager::tick_health_display_old(fighter* my_fight)
{
    /*PlotHistogram(const char* label, const float* values, int values_count,
                  int values_offset = 0, const char* overlay_text = NULL,
                  float scale_min = FLT_MAX, float scale_max = FLT_MAX,
                  ImVec2 graph_size = ImVec2(0,0), int stride = sizeof(float));*/

    using namespace bodypart;

    std::vector<float> vals;
    std::vector<std::string> labels;
    //std::string rtext;

    for(int i=HEAD; i <= RFOOT; i++)
    {
        if(i == LHAND || i == RHAND)
            continue;

        vals.push_back(my_fight->parts[i].hp);
        labels.push_back(short_names[i]);

        //rtext = rtext + short_names[i];
    }

    ImGui::Begin("Health Display");

    float width = 600.f;
    float elem_width = width / vals.size();

    ImGui::PlotHistogram("", &vals[0], vals.size(), 0, nullptr, 0, 1, ImVec2(width, 50));

    ///SIGH
    /*for(int i=0; i<ImGui::GetColumnsCount(); i++)
    {
        printf("hello dear\n");

        ImGui::NextColumn();
    }*/

    //ImGui::Text(rtext.c_str());

    ImGui::BeginGroup();

    float accum_start = 0.f;
    float x_space = style.ItemInnerSpacing.x;
    float x_offset = elem_width/20.f;

    for(int i=0; i<labels.size(); i++)
    {
        ImVec2 my_size = ImGui::CalcTextSize(labels[i].c_str());

        float boundary = elem_width/2 - my_size.x/2;

        ImGui::Dummy(ImVec2(boundary, 0));

        ImGui::SameLine(0, 0);

        ImGui::Text(labels[i].c_str());

        ImGui::SameLine(0, 0);

        ImGui::Dummy(ImVec2(boundary, 0));

        ImGui::SameLine(0, 0);
    }

    ImGui::EndGroup();

    ImGui::End();

    any_render = true;
}

void ui_manager::tick_health_display(fighter* my_fight)
{
    using namespace bodypart;

    std::vector<float> vals;
    std::vector<std::string> labels;
    int max_len_label = 0;

    for(int i=HEAD; i <= RFOOT; i++)
    {
        if(bodypart::is_invincible((bodypart_t)i))
            continue;

        float hp_val = my_fight->parts[i].hp;

        hp_val = clamp(hp_val, 0.f, 1.f);

        vals.push_back(hp_val);
        labels.push_back(short_names[i]);

        max_len_label = std::max(max_len_label, (int)short_names[i].size());

        //rtext = rtext + short_names[i];
    }

    ImGui::Begin("Health Display");

    for(int i=0; i < labels.size(); i++)
    {
        std::string label = labels[i];

        for(int kk=0; kk < max_len_label - labels[i].size(); kk++)
        {
            label = label + " ";
        }

        float hp_frac = vals[i];
        float hp_interpolate_frac = pow(hp_frac, 1.5f);

        vec4f start_col = {0.9, 0.9, 0.9, 1.f};
        vec4f end_col = {0.8f, 0.1f, 0.1f, 1.f};

        vec4f ccol = start_col * hp_interpolate_frac + end_col * (1.f - hp_interpolate_frac);

        ImGui::Text(label.c_str());

        ImGui::SameLine();

        int cx = ImGui::GetCursorPosX();
        int cy = ImGui::GetCursorPosY();

        std::string idstr = "##" + std::to_string(i);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6, 0.6, 0.6, 1));

        ImGui::Button((idstr + "BACK").c_str(), ImVec2(102, 14));

        ImGui::PopStyleColor();

        ImGui::SetCursorPosX(cx+1);
        ImGui::SetCursorPosY(cy+1);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5, 0.5, 0.5, 1));

        ImGui::Button(idstr.c_str(), ImVec2(100, 12));

        ImGui::PopStyleColor();

        ImGui::SetCursorPosX(cx+1);
        ImGui::SetCursorPosY(cy+1);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ccol.x(), ccol.y(), ccol.z(), ccol.w()));

        ImGui::Button(idstr.c_str(), ImVec2(100 * hp_frac, 12));

        ImGui::PopStyleColor();
    }

    ImGui::End();

    any_render = true;
}

std::vector<int> merge_pad_lengths(const std::vector<int>& one, const std::vector<int>& two)
{
    if(one.size() < two.size())
        return two;

    std::vector<int> ret = one;

    for(int i=0; i<one.size(); i++)
    {
        ret[i] = std::max(one[i], two[i]);
    }

    return ret;
}

struct scoreboard_info
{
    std::string name;

    int kills = 0;
    int deaths = 0;

    float ping = 0;

    std::vector<int> get_pad_lengths() const
    {
        return {name.length(), std::to_string(kills).length(), std::to_string(deaths).length(), std::to_string((int)ping).length()};
    }

    std::string pad(const std::string& str, int len) const
    {
        std::string ret = str;

        for(int i=str.length(); i < len; i++)
        {
            ret = ret + " ";
        }

        for(int i=0; i<ret.size(); i++)
        {
            if(ret[i] == '\\')
            {
                ret.insert(ret.begin() + i, '\\');
                i++;
                continue;
            }
        }

        int max_size = 20;

        if(ret.size() > max_size)
            ret.resize(max_size);

        return ret;
    }

    template<typename T>
    std::string pad(T val, int len) const
    {
        std::string ret = std::to_string((int)val);

        return pad(ret, len);
    }

    std::string get_fixed_padded_length_string(const std::vector<int>& pad_lengths) const
    {
        std::string ret;

        ret = pad(name, pad_lengths[0]) + " | " +
        pad(kills, pad_lengths[1]) + " | " +
        pad(deaths, pad_lengths[2]) + " | " +
        pad(ping, pad_lengths[3]);

        return ret;
    }
};

void ui_manager::tick_scoreboard_ui(server_networking& networking)
{
    ImGui::Begin("Scoreboard");

    std::map<int, std::vector<scoreboard_info>> team_to_info;
    std::map<int, std::vector<int>> pad_lengths;

    std::vector<std::string> display_headers = {"Name", "Kills", "Deaths", "Ping"};
    std::vector<int> display_pads;

    for(auto& i : display_headers)
    {
        display_pads.push_back(i.length());
    }

    for(auto& i : networking.connected_server.discovered_fighters)
    {
        int32_t id = i.first;

        network_player& net_player = i.second;

        game_mode_handler_shared& mode_handler_shared = networking.connected_server.game_info.shared_game_state;

        if(!mode_handler_shared.has_player_entry(id))
            continue;

        player_info_shared& info_shared = mode_handler_shared.player_info[id];

        scoreboard_info sinfo;

        sinfo.name = net_player.fight->local_name;
        sinfo.kills = info_shared.kills;
        sinfo.deaths = info_shared.deaths;
        sinfo.ping = net_player.fight->net.ping;

        team_to_info[net_player.fight->team].push_back(sinfo);

        std::vector<int> pad_lens = sinfo.get_pad_lengths();

        pad_lengths[net_player.fight->team] = merge_pad_lengths(pad_lengths[net_player.fight->team], pad_lens);
        pad_lengths[net_player.fight->team] = merge_pad_lengths(pad_lengths[net_player.fight->team], display_pads);
    }

    //if(team_to_info.size() > 0)
    //    ImGui::Columns(team_to_info.size());

    std::string team_separation_space = "        ";

    for(auto& i : team_to_info)
    {
        int team = i.first;

        const std::vector<scoreboard_info>& info = i.second;

        std::vector<std::string> cdisplay_headers = display_headers;

        for(int i=0; i<cdisplay_headers.size(); i++)
        {
            cdisplay_headers[i] = scoreboard_info().pad(cdisplay_headers[i], pad_lengths[team][i]);
        }

        std::string display_string;

        for(auto& i : cdisplay_headers)
        {
            display_string = display_string + i + " | ";
        }

        display_string.resize(display_string.size() - strlen(" | "));

        display_string = display_string + team_separation_space;

        std::string total_disp;

        for(const scoreboard_info& inf : info)
        {
            std::string disp = inf.get_fixed_padded_length_string(pad_lengths[team]) + team_separation_space + "\n";

            total_disp = total_disp + disp;
        }

        total_disp = display_string + "\n" + total_disp;

        ImGui::Text(total_disp.c_str());

        //ImGui::NextColumn();

        ImGui::SameLine();
    }

    ImGui::End();
}

void ui_manager::tick_render()
{
    if(!any_render)
        return;

    ImGui::Render();

    any_render = false;
}

///friends, search, history
void server_browser::tick(float ftime_ms, server_networking& networking)
{
    just_disconnected = false;

    std::vector<game_server>& servers = networking.server_list;

    std::vector<int> max_sizes = {-1, -1};

    for(const game_server& server : servers)
    {
        std::string name = server.address + ":" + server.their_host_port;

        std::string player_str = std::to_string(server.current_players) + "/" + std::to_string(server.max_players);

        max_sizes[0] = std::max(max_sizes[0], (int)name.length());
        max_sizes[1] = std::max(max_sizes[1], (int)player_str.length());
    }

    ImGui::Begin("Server Browser");

    for(const game_server& server : servers)
    {
        std::string name = server.address + ":" + server.their_host_port;

        std::string player_str = std::to_string(server.current_players) + "/" + std::to_string(server.max_players);

        std::string ping_str = std::to_string((int)server.ping) + "ms";

        for(int i=name.length(); i < max_sizes[0]; i++)
        {
            name = name + " ";
        }

        for(int i = player_str.length(); i < max_sizes[1]; i++)
        {
            player_str = player_str + " ";
        }

        bool any_clicked = false;

        any_clicked |= ImGui::Button(name.c_str());

        ImGui::SameLine();

        any_clicked |= ImGui::Button(player_str.c_str());

        ImGui::SameLine();

        any_clicked |= ImGui::Button(ping_str.c_str());

        ImGui::SameLine();

        any_clicked |= ImGui::Button("Connect");

        if(any_clicked)
        {
            if(networking.connected_server.joined_server)
            {
                just_disconnected = true;

                networking.connected_server.disconnect();
            }

            networking.set_game_to_join(server.address, server.their_host_port);
        }
    }

    if(ImGui::Button("Refresh"))
    {
        networking.ping_master();

        for(game_server& server : servers)
        {
            networking.ping_gameserver(server.address, server.their_host_port);

            server.pinged = true;
        }
    }

    if(networking.connected_server.joined_server)
    {
        if(ImGui::Button("Disconnect"))
        {
            if(networking.connected_server.joined_server)
            {
                just_disconnected = true;

                networking.connected_server.disconnect();
            }
        }

        std::string in_server_str = "Currently Connected to " + networking.connected_server.to_game.get_peer_ip() + ":" + networking.connected_server.to_game.get_peer_port();

        ImGui::Button(in_server_str.c_str());
    }

    for(game_server& server : servers)
    {
        if(server.pinged)
            continue;

        networking.ping_gameserver(server.address, server.their_host_port);

        server.pinged = true;
    }

    ImGui::End();
}

bool server_browser::has_disconnected()
{
    return just_disconnected;
}
