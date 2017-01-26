#include "ui_manager.hpp"
#include "../imgui/imgui-SFML.h"
#include "../openclrenderer/settings_loader.hpp"
#include "fighter.hpp"
#include "server_networking.hpp"
#include "imgui_extension.hpp"

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

    if(saved_settings_w != sett->width || saved_settings_h != sett->height)
    {
        saved_settings_w = sett->width;
        saved_settings_h = sett->height;

        res_x.val = saved_settings_w;
        res_y.val = saved_settings_h;
    }

    //printf("RES X Y %i %i\n", res_x.val, res_y.val);

    mouse_sensitivity.set_bound(0.001f, 100.f);
    vals.mouse_sens = mouse_sensitivity.instantiate_and_get("Mouse sens").ret;

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

    vals.frames_of_input_lag = frames_of_input_lag.instantiate_and_get("Frames of input lag").ret;

    vals.horizontal_fov_degrees = horizontal_fov_degrees.instantiate_and_get("FoV angle (horizontal)").ret;

    if(ImGui::Button("Update Resolution and/or FoV"))
    {
        sett->width = vals.width;
        sett->height = vals.height;
        sett->horizontal_fov_degrees = vals.horizontal_fov_degrees;

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

void ui_manager::tick_health_display(fighter* my_fight)
{
    /*PlotHistogram(const char* label, const float* values, int values_count,
                  int values_offset = 0, const char* overlay_text = NULL,
                  float scale_min = FLT_MAX, float scale_max = FLT_MAX,
                  ImVec2 graph_size = ImVec2(0,0), int stride = sizeof(float));*/

    using namespace bodypart;

    std::vector<float> vals;
    std::vector<std::string> labels;
    std::string rtext;

    for(int i=HEAD; i <= RFOOT; i++)
    {
        if(i == LHAND || i == RHAND)
            continue;

        vals.push_back(my_fight->parts[i].hp);
        labels.push_back(short_names[i]);

        rtext = rtext + short_names[i];
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

void ui_manager::tick_render()
{
    if(!any_render)
        return;

    ImGui::Render();

    any_render = false;
}
