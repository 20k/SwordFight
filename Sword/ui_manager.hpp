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

    rval instantiate_and_get()
    {
        return instantiate_and_get(internal_label);
    }

    rval instantiate_and_get(const std::string& label)
    {
        bool ret = ImGui::InputText(label.c_str(), val, sizeof(val)-1);

        return {std::string(val), ret};
    }

    void set_default(const std::string& def)
    {
        memset(val, 0, 255);

        for(int i=0; i<def.size() && i < 254; i++)
        {
            val[i] = def[i];
        }
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
        bool ret = ImGui::DragFloat(label.c_str(), &val, 0.01f, vbound.v[0], vbound.v[1]);

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
        bool ret = ImGui::DragInt(label.c_str(), &val, 1, vbound.v[0], vbound.v[1]);

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
};

struct ui_manager
{
    configuration_values vals;

    window_slider_getter<float> mouse_sensitivity;

    window_slider_getter<int> res_x;
    window_slider_getter<int> res_y;

    int saved_settings_w = 0;
    int saved_settings_h = 0;

    settings* sett;

    ui_manager();

    void init(settings& s);

    void tick(float ftime_ms);
};

#endif // UI_MANAGER_HPP_INCLUDED
