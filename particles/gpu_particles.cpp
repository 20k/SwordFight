#include "gpu_particles.hpp"


particle_intermediate make_particle_jet(int num, vec3f start, vec3f dir, float len, float angle, vec3f col, float speedmod)
{
    std::vector<cl_float4> p1;
    std::vector<cl_float4> p2;
    std::vector<cl_uint> colours;

    for(uint32_t i = 0; i<num; i++)
    {
        float len_pos = randf_s(0, len);

        float len_frac = len_pos / len;

        vec3f euler = dir.get_euler();

        euler = euler + (vec3f){0, randf_s(-angle, angle), randf_s(-angle, angle)};

        vec3f rot_pos = (vec3f){0.f, len_pos, 0.f}.rot({0.f, 0.f, 0.f}, euler);

        vec3f final_pos = start + rot_pos;

        final_pos = final_pos + randf<3, float>(-len/40.f, len/40.f);


        float mod = speedmod;

        p1.push_back({final_pos.v[0], final_pos.v[1], final_pos.v[2]});

        vec3f pos_2 = final_pos * mod;

        p2.push_back({pos_2.v[0], pos_2.v[1], pos_2.v[2]});

        col = clamp(col, 0.f, 1.f);

        colours.push_back(rgba_to_uint(col));
    }

    return particle_intermediate{p1, p2, colours};
}

void process_pvec(particle_vec& v1, float friction, compute::buffer& screen_buf, float frac_remaining)
{
    cl_float brightness = frac_remaining * 3.f + (1.f - frac_remaining) * 0.0f;

    int which = v1.which;
    int nwhich = v1.nwhich;

    cl_int num = v1.num;

    arg_list p_args;
    p_args.push_back(&num);
    p_args.push_back(&v1.bufs[which]);
    p_args.push_back(&v1.bufs[nwhich]);
    p_args.push_back(&friction);

    run_kernel_with_string("particle_explode", {num}, {128}, 1, p_args);

    arg_list r_args;
    r_args.push_back(&num);
    r_args.push_back(&v1.bufs[nwhich]);
    r_args.push_back(&v1.bufs[which]);
    r_args.push_back(&v1.g_col);
    r_args.push_back(&brightness);
    r_args.push_back(&engine::c_pos);
    r_args.push_back(&engine::c_rot);
    r_args.push_back(&engine::old_pos);
    r_args.push_back(&engine::old_rot);
    r_args.push_back(&screen_buf);

    ///render a 2d gaussian for particle effects
    run_kernel_with_string("render_gaussian_points", {num}, {128}, 1, r_args);

    v1.flip();
}
