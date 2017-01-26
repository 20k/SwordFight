#ifndef UI_MANAGER_HPP_INCLUDED
#define UI_MANAGER_HPP_INCLUDED

#include "../imgui/imgui.h"
#include <string>
#include <vec/vec.hpp>

struct settings;

namespace window_element_ids
{
    extern int label_gid;
}

template<typename T>
struct window_element_getter_base
{
    static int label_gid;
    std::string internal_label = "Default " + std::to_string(window_element_ids::label_gid++);


    vec<2, T> bound;
    bool has_bound = false;

    ///ok i fucked up, we should enforce this for everything but i'm tired
    void set_getter_integral_bound(T lbound, T ubound)
    {
        bound.x() = lbound;
        bound.y() = ubound;

        has_bound = true;
    }

    struct rval
    {
        T ret;
        bool clicked;
    };
};

template<typename T>
struct window_element_getter : window_element_getter_base<T>
{

};

template<>
struct window_element_getter<std::string> : window_element_getter_base<std::string>
{
    char val[255] = {0};
    bool has_default = false;

    rval instantiate_and_get()
    {
        return instantiate_and_get(internal_label);
    }

    rval instantiate_and_get(const std::string& label)
    {
        bool ret = ImGui::InputText(label.c_str(), val, sizeof(val)-1);

        return {std::string(val), ret};
    }

    void clear()
    {
        memset(val, 0, 255);
    }

    void assign(const std::string& str)
    {
        for(int i=0; i<254 && i < str.size(); i++)
        {
            val[i] = str[i];
        }
    }

    void set_default(const std::string& def)
    {
        if(has_default)
            return;

        clear();

        for(int i=0; i<def.size() && i < 254; i++)
        {
            val[i] = def[i];
        }

        has_default = true;
    }
};

template<>
struct window_element_getter<int> : window_element_getter_base<int>
{
    int val;
    bool has_default = false;

    rval instantiate_and_get()
    {
        return instantiate_and_get(internal_label);
    }

    rval instantiate_and_get(const std::string& label)
    {
        bool ret = ImGui::InputInt(label.c_str(), &val);

        if(has_bound)
            val = clamp(val, bound.x(), bound.y());

        rval rv;
        rv.ret = (val);
        rv.clicked = ret;

        return rv;
    }

    void set_default(int _val)
    {
        if(has_default)
            return;

        val = _val;
        has_default = true;
    }
};


template<>
struct window_element_getter<float> : window_element_getter_base<float>
{
    float val;
    bool has_default = false;

    rval instantiate_and_get()
    {
        return instantiate_and_get(internal_label);
    }

    rval instantiate_and_get(const std::string& label)
    {
        bool ret = ImGui::InputFloat(label.c_str(), &val);

        if(has_bound)
            val = clamp(val, bound.x(), bound.y());

        rval rv;
        rv.ret = (val);
        rv.clicked = ret;

        return rv;
    }

    void set_default(float _val)
    {
        if(has_default)
            return;

        val = _val;
        has_default = true;
    }
};


template<typename T>
struct window_element_checkbox : window_element_getter_base<T>
{

};

template<>
struct window_element_checkbox<int> : window_element_getter_base<int>
{
    bool val;
    bool has_default = false;

    rval instantiate_and_get()
    {
        return instantiate_and_get(internal_label);
    }

    rval instantiate_and_get(const std::string& label)
    {
        bool ret = ImGui::Checkbox(label.c_str(), &val);

        rval rv;
        rv.ret = (val);
        rv.clicked = ret;

        return rv;
    }

    void set_default(float _val)
    {
        if(has_default)
            return;

        val = _val;
        has_default = true;
    }
};

template<typename T>
struct window_slider_getter_base
{
    float slide_speed = 1.f;

    void set_slide_speed(float speed)
    {
        slide_speed = speed;
    }

    vec<2, T> vbound;

    void set_bound(T _min, T _max)
    {
        vbound = {_min, _max};
    }
};

template<typename T>
struct window_slider_getter
{

};

template<>
struct window_slider_getter<float> : window_element_getter<float>, window_slider_getter_base<float>
{
    rval instantiate_and_get(const std::string& label)
    {
        bool ret = ImGui::DragFloat(label.c_str(), &val, slide_speed, vbound.v[0], vbound.v[1]);

        rval rv;
        rv.ret = (val);
        rv.clicked = ret;

        return rv;
    }
};

template<>
struct window_slider_getter<int> : window_element_getter<int>, window_slider_getter_base<int>
{
    rval instantiate_and_get(const std::string& label)
    {
        bool ret = ImGui::DragInt(label.c_str(), &val, slide_speed, vbound.v[0], vbound.v[1]);

        rval rv;
        rv.ret = (val);
        rv.clicked = ret;

        return rv;
    }
};

///load from file
struct configuration_values
{
    float mouse_sens = 1.f;
    int width = 1, height = 1;
    std::string player_name;
    float motion_blur_strength;
    float motion_blur_camera_contribution;
    int use_post_aa = 1;
    int use_raw_input = 1;
    int frames_of_input_lag = 1;
    float horizontal_fov_degrees = 120;
};

struct fighter;
struct network_statistics;

///we need a network statistics graph so we can more easily, and objectively look at network traffic
struct ui_manager
{
    ImGuiStyle style;

    configuration_values vals;

    window_slider_getter<float> mouse_sensitivity;

    window_slider_getter<int> res_x;
    window_slider_getter<int> res_y;

    window_slider_getter<float> motion_blur_strength;
    window_slider_getter<float> motion_blur_camera_contribution;

    window_element_getter<std::string> player_name;

    window_element_checkbox<int> use_post_aa;
    window_element_checkbox<int> use_raw_input;

    window_element_getter<int> frames_of_input_lag;
    window_slider_getter<float> horizontal_fov_degrees;

    int saved_settings_w = 0;
    int saved_settings_h = 0;

    settings* sett;

    ui_manager();

    void init(settings& s);


    void tick(float ftime);

    void tick_settings(float ftime_ms);

    void tick_frametime_graph(float ftime_ms, bool display);
    void tick_networking_graph(const network_statistics& net_stats);

    void tick_health_display(fighter* my_fight);

    void tick_render();

    std::vector<float> ftime_history;
    std::vector<network_statistics> net_stats_history;

    bool any_render = false;

    bool ftime_paused = false;

    bool internal_ftime_show_toggle = false;
    bool internal_net_stats_show_toggle = false;
};

#endif // UI_MANAGER_HPP_INCLUDED
