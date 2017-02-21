#ifndef SHARED_MAPEDITOR_HPP_INCLUDED
#define SHARED_MAPEDITOR_HPP_INCLUDED

#include <vector>
#include <string>
#include <fstream>
#include <vec/vec.hpp>
#include "../openclrenderer/objects_container.hpp"
#include "../openclrenderer/object_context.hpp"

inline
void modify_texture_colour_dynamic(object_context& ctx)
{
    objects_container* found_object = nullptr;

    for(objects_container* c : ctx.containers)
    {
        if(c->file.find("naturePack_007") != std::string::npos)
        {
            found_object = c;
            break;
        }
    }

    if(found_object == nullptr)
    {
        lg::log("NONE FOUND ERIROP");
        return;
    }

    int green_tid = found_object->objs[0].tid;
    int brown_tid = found_object->objs[1].tid;

    texture* green_tex = ctx.tex_ctx.id_to_tex(green_tid);
    texture* brown_tex = ctx.tex_ctx.id_to_tex(brown_tid);

    green_tex->unload();
    green_tex->set_load_from_other_texture(brown_tex);

    ctx.build(true);
}

std::vector<objects_container*> load_level_from_file(object_context& ctx, const std::string& file);
void load_floor_from_file(objects_container* obj, const std::string& file);

inline
std::vector<objects_container*> load_level_from_file(object_context& ctx, const std::string& file)
{
    std::ifstream stream(file);

    std::vector<objects_container*> objs;

    if(stream.good())
    {
        std::string file;

        while(std::getline(stream, file))
        {
            std::string posstr;
            std::getline(stream, posstr);

            std::vector<std::string> splitted = split(posstr, ' ');

            cl_float4 pos;

            pos.x = atof(splitted[0].c_str());
            pos.y = atof(splitted[1].c_str());
            pos.z = atof(splitted[2].c_str());

            std::string scalestr;
            std::getline(stream, scalestr);

            float scale = atof(scalestr.c_str());

            std::string quatstr;
            std::getline(stream, quatstr);

            std::vector<std::string> quat_split = split(quatstr, ' ');

            quat rquat;
            rquat.q.v[0] = atof(quat_split[0].c_str());
            rquat.q.v[1] = atof(quat_split[1].c_str());
            rquat.q.v[2] = atof(quat_split[2].c_str());
            rquat.q.v[3] = atof(quat_split[3].c_str());

            int should_quantise;

            std::string shouldquant;
            std::getline(stream, shouldquant);

            should_quantise = atoi(shouldquant.c_str());

            float quant_grid;

            std::string quantgrid;
            std::getline(stream, quantgrid);

            quant_grid = atof(quantgrid.c_str());

            objects_container* c = ctx.make_new();

            c->set_file(file);
            c->set_active(true);

            c->set_pos(pos);

            ctx.load_active();

            c->set_rot_quat(rquat);
            c->set_quantise_position(should_quantise, quant_grid);

            ///found something invalid in the save, ah well
            if(!c->isloaded)
                c->set_active(false);

            c->set_dynamic_scale(scale);

            objs.push_back(c);
        }
    }

    ctx.build_request();

    stream.close();

    modify_texture_colour_dynamic(ctx);

    return objs;
}

inline
void load_floor_from_file(objects_container* obj, const std::string& file)
{
    FILE* pFile = fopen((file + "d").c_str(), "rb");

    assert(pFile != nullptr);

    fseek(pFile, 0L, SEEK_END);
    int file_len = ftell(pFile);
    rewind(pFile);

    int num_tris = file_len / sizeof(triangle);

    obj->unload();

    texture* dummy_tex = obj->parent->tex_ctx.make_new();
    dummy_tex->set_create_colour(sf::Color(255, 255, 255), 32, 32);

    object floor_obj;
    floor_obj.isloaded = true;
    floor_obj.tri_list.resize(num_tris);
    floor_obj.tri_num = floor_obj.tri_list.size();
    floor_obj.tid = dummy_tex->id;

    obj->objs.push_back(floor_obj);

    {
        //floor->objs[0].tri_list.resize(num_tris);

        fread(&obj->objs[0].tri_list[0], sizeof(triangle), num_tris, pFile);

        fclose(pFile);

        obj->parent->build_request();
    }

    quat q;
    q.load_from_euler({-M_PI/2, 0, 0});

    obj->set_rot_quat(q);
    obj->set_dynamic_scale(30.f);

    ///wait. I don't need to do this and is actively counterproductive to my mental model of how this works
    ///bloody fix this dirty hack
    /*for(triangle& t : obj->objs[0].tri_list)
    {
        t.vertices[0].set_pad(floor->objs[0].object_g_id);
    }

    ctx.build(true);*/

    obj->isloaded = true;
    obj->set_active(true);

    obj->parent->build(true);
}


#endif // SHARED_MAPEDITOR_HPP_INCLUDED
