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
    bool config_dirty = false;

    sf::Time t = sf::microseconds(ftime_ms * 1000.f);

    ImGui::SFML::Update(t);

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

    ImGui::Render();

    if(sett->mouse_sens != vals.mouse_sens)
    {
        sett->mouse_sens = vals.mouse_sens;

        config_dirty = true;
    }


    if(config_dirty)
    {
        sett->save("./res/settings.txt");
    }
}
