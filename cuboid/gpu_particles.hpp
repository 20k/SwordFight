#ifndef GPU_PARTICLES_HPP_INCLUDED
#define GPU_PARTICLES_HPP_INCLUDED

#include <vector>
#include "../openclrenderer/ocl.h"
#include "../openclrenderer/engine.hpp"
#include <vec/vec.hpp>

struct particle_info
{
    float density;
    float temp;
};

struct particle_vec
{
    std::vector<cl_float4> p1;
    std::vector<cl_float4> p2;
    std::vector<cl_uint> colours;

    compute::buffer bufs[2];
    compute::buffer g_col;

    int which;
    int nwhich;

    cl_int num;

    particle_vec(std::vector<cl_float4> v1, std::vector<cl_float4> v2, std::vector<cl_uint> c1)
    {
        p1 = v1;
        p2 = v2;
        colours = c1;

        bufs[0] = engine::make_read_write(sizeof(cl_float4)*p1.size(), p1.data());
        //bufs[1] = engine::make_read_write(sizeof(cl_float4)*p2.size(), p2.data());

        g_col =  engine::make_read_write(sizeof(cl_uint)*colours.size(), colours.data());

        which = 0;
        nwhich = 1;

        num = v1.size();
    }

    void flip()
    {
        nwhich = which;

        which = (which + 1) % 2;
    }
};

struct particle_intermediate
{
    std::vector<cl_float4> p1;
    std::vector<cl_float4> p2;
    std::vector<cl_uint> colours;

    particle_intermediate append(const particle_intermediate& n)
    {
        p1.insert(p1.end(), n.p1.begin(), n.p1.end());
        //p2.insert(p2.end(), n.p2.begin(), n.p2.end());
        colours.insert(colours.end(), n.colours.begin(), n.colours.end());

        return *this;
    }

    particle_vec build()
    {
        return particle_vec(p1, p2, colours);
    }
};

particle_intermediate make_particle_jet(int num, vec3f start, vec3f dir, float len, float angle, vec3f col, float speedmod);

void process_pvec(particle_vec& v1, float frac, float old_frac, compute::buffer& screen_buf, float frac_remaining);


#endif // GPU_PARTICLES_HPP_INCLUDED
