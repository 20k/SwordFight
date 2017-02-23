#include <iostream>
#include <SFML/Graphics.hpp>
//#include "../openclrenderer/smoke.hpp"
//#include "../sword/vec.hpp"
#include <vec/vec.hpp>

using namespace std;


#define IX(x, y) (y)*width + (x)


void plop(int x, int y, vec3f* r, int width)
{
    uint32_t pos = y*width + x;

    r[pos] = r[pos] + (vec3f){0, 0, 1};

    for(int j=-1; j<2; j++)
    {
        for(int i=-1; i<2; i++)
        {
            //if(x + i < 0 || x + i >= width || y + j < 0 || y + j >= width)
            //    continue;

            int xv = (x + i + width) % width;
            int yv = (y + j + width) % width;

            r[IX(xv, yv)] = r[IX(xv, yv)] + (vec3f){i, j, 1};
        }
    }
}

struct setting
{
    int width, height, num;
};

namespace setting_list
{
    enum setting_list
    {
        FLOOR,
        BODYPART,
        COUNT
    };
}

typedef setting_list::setting_list setting_t;

std::map<setting_t, setting> options =
{
    {setting_list::FLOOR, {2048, 2048, 10000000}},
    {setting_list::BODYPART, {1024, 1024, 10000000/4}}
};

int main()
{
    setting_t type = setting_list::FLOOR;

    setting s = options[type];

    int width = s.width;
    int height = s.height;
    int depth = 3;

    /*float* tw1, *tw2, *tw3;

    ///needs to be nw, nh, nd
    tw1 = new float[width*height*depth];
    tw2 = new float[width*height*depth];
    tw3 = new float[width*height*depth];*/

    //cl_float3* out;

    ///needs to be nw, nh, nd
    //out = new cl_float3[width*height*depth];


    vec3f* r = new vec3f[width*height];

    /*for(unsigned int i = 0; i<width*height*depth; i++)
    {
        //tw1[i] = randf_s();
        //tw2[i] = randf_s();
        //tw3[i] = randf_s();

    }*/

    for(uint32_t i=0; i<width*height; i++)
        r[i] = {0,0,1};


    /*for(int z=0; z<depth; z++)
        for(int y=0; y<height; y++)
            for(int x=0; x<width; x++)
    {
        int imin = -0;
        int imax = 1;

        uint32_t pos = z*width*height + y*width + x;

        //cl_float3 val = y_of(x, y, z, width, height, depth, tw1, tw2, tw3, imin, imax);

        //out[z*width*height + y*width + x] = val;

        out[pos] = {tw1[pos], tw2[pos], tw3[pos]};
    }*/

    int num = s.num;

    for(int i=0; i<num; i++)
    {
        plop(randf_s(0, width-1), randf_s(0, height-1), r, width);
    }

    sf::Image img;
    img.create(width, height);

    for(int y=0; y<height-0; y ++)
    {
        for(int x=0; x<width-0; x ++)
        {
            uint32_t pos = y*width + x;

            /*vec3f accum = {0};

            for(int j=-1; j<2; j++)
            {
                for(int i=-1; i<2; i++)
                {
                    uint32_t p2 = z*width*height + (y+j)*width + x + i;

                    accum = accum + r[p2];
                }
            }*/

            vec3f accum = r[pos];

            accum = accum.norm();

            accum = accum + 1.f;

            accum = accum / 2.f;

            //printf("%f %f %f\n", EXPAND_3(accum));

            accum = clamp(accum, 0.f, 1.f);

            accum = accum * 255.f;

            img.setPixel(x, y, {accum.v[0], accum.v[1], accum.v[2]});
        }
    }


    /*for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            uint32_t pos = 1*width*height + y*width + x;

            auto val = xyz_to_vec(out[pos]);

            //val = (val + 1.f) / 2.f;

            if(val.v[1] < 0)
                val.v[1] = -val.v[1];

            val = clamp(val, 0.f, 1.f);

            val = val * 255.f;

            sf::Color col(val.v[0], val.v[1], val.v[2]);

            img.setPixel(x, y, col);
        }
    }*/

    img.saveToFile("norm_body_correct.png");

    return 0;
}
