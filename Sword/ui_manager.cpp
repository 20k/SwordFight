#include "ui_manager.hpp"
#include "../imgui/imgui-SFML.h"
#include "../openclrenderer/settings_loader.hpp"
#include "fighter.hpp"

int window_element_ids::label_gid;

ui_manager::ui_manager()
{
    ImGuiIO io = ImGui::GetIO();
    io.MouseDrawCursor = false;
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

    mouse_sensitivity.set_default(sett->mouse_sens);
    res_x.set_bound(0, 4096);
    res_y.set_bound(0, 4096);

    res_x.set_default(sett->width);
    res_y.set_default(sett->height);

    player_name.set_default(sett->name);

    if(saved_settings_w != sett->width || saved_settings_h != sett->height)
    {
        saved_settings_w = sett->width;
        saved_settings_h = sett->height;

        res_x.val = saved_settings_w;
        res_y.val = saved_settings_h;
    }

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

    if(ImGui::Button("Update Resolution"))
    {
        sett->width = vals.width;
        sett->height = vals.height;

        config_dirty = true;
    }

    ImGui::End(); // end window

    if(sett->mouse_sens != vals.mouse_sens)
    {
        sett->mouse_sens = vals.mouse_sens;

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

void ui_manager::tick_frametime_graph(float ftime)
{
    if(!ftime_paused)
        ftime_history.push_back(ftime);

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

    std::string top = to_string_with_precision(maxf, 2) + " max";
    std::string mid = to_string_with_precision(smooth_ftime, 2) + " current";
    std::string bot = to_string_with_precision(minf, 2) + " min";

    ImGui::BeginGroup();

    ImGui::Text(top.c_str());

    ImGui::Dummy(ImVec2(1, height/2 - text_height*2));

    ImGui::Text(mid.c_str());

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

    if(ftime_history.size() > 400)
        ftime_history.erase(ftime_history.begin());

    any_render = true;
}

void ui_manager::tick_render()
{
    if(!any_render)
        return;

    ImGui::Render();

    any_render = false;
}
