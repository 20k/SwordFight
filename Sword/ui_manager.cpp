#include "ui_manager.hpp"
#include "../imgui/imgui-SFML.h"
#include "../openclrenderer/settings_loader.hpp"
#include "fighter.hpp"

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

    if(saved_settings_w != sett->width || saved_settings_h != sett->height)
    {
        saved_settings_w = sett->width;
        saved_settings_h = sett->height;

        res_x.val = saved_settings_w;
        res_y.val = saved_settings_h;
    }

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

    if(ImGui::Button("Update Resolution"))
    {
        sett->width = vals.width;
        sett->height = vals.height;

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

void ui_manager::tick_frametime_graph(float ftime, bool display)
{
    if(!ftime_paused)
        ftime_history.push_back(ftime);

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

        std::string top = to_string_with_precision(maxf, 2) + " max";
        std::string mid = to_string_with_precision(smooth_ftime, 2) + " current";
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
