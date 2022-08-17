#include <cstdlib>

#include <array>
#include <cstddef>
#include <iostream>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

using std::cout;
using std::endl;

using u32 = uint32_t;
using u64 = uint64_t;

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr int NO_FLAGS = 0;


struct vec2
{
    int x;
    int y;
};

struct Seed
{
    vec2 pos;
    unsigned int color;
};

vec2 operator-(const vec2& lhs, const vec2& rhs)
{
    vec2 res{};
    res.x = lhs.x - rhs.x;
    res.y = lhs.y - rhs.y;
    return res;
}

unsigned int len(const vec2& vec)
{
    return static_cast<unsigned>(vec.x * vec.x + vec.y * vec.y);
}


int rand_between(int min, int max)
{
    static std::random_device rd;  //Will be used to obtain a seed for the random number engine
    static std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<int> distrib(min, max);
    return distrib(gen);
}

void draw_circle(SDL_Surface* surface, vec2 pos, int r, u32 color)
{
    auto pixels = static_cast<unsigned int*>(surface->pixels);

    vec2 top_left{.x = pos.x - r, .y = pos.y + r};
    vec2 bottom_right{.x = pos.x + r, .y = pos.y - r};

    for (int y = top_left.y;
         y > bottom_right.y;
         --y)
    {
        for (int x = top_left.x;
             x < bottom_right.x + r;
             ++x)
        {
            vec2 point{.x = x, .y = y};

            if (len(point - pos) < static_cast<unsigned>(r * r))
            {
                int index = y * surface->w + x;
                if (index < 0 or index >(WINDOW_WIDTH * WINDOW_HEIGHT))
                    continue;

                pixels[index] = color; // RRGGBBAA
            }
        }
    }
}


SDL_Surface* render_shit()
{
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, WINDOW_WIDTH, WINDOW_HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888);

    if (!surface)
    {
        cout << "[ERROR] cannot create surface: " << SDL_GetError() << endl;
        std::exit(1);
    }

    if (SDL_LockSurface(surface) != 0)
    {
        cout << "[ERROR] cannot lock surface: " << SDL_GetError() << endl;
        std::exit(1);
    }

    std::array<unsigned int, 10> palette;
    /*
    palette[0] = 0xffce0bff;
    palette[1] = 0xe83f68ff;
    palette[2] = 0xab3e8fff;
    palette[3] = 0x0092d6ff;
    palette[4] = 0x00a989ff;

    palette[0] = 0x8385a6ff;
    palette[1] = 0xb3c9dfff;
    palette[2] = 0xc5d6e7ff;
    palette[3] = 0xa4a7d7ff;
    palette[4] = 0x92b2d3ff;
    palette[5] = 0xc90076ff;
    palette[6] = 0x7fc900ff;
    palette[7] = 0x00c9aeff;
    palette[8] = 0xc9c300ff;
    palette[9] = 0xc97300ff;
    */

    palette[0] = 0x705041ff;
    palette[1] = 0xcd5b45ff;
    palette[2] = 0xfbd870ff;
    palette[3] = 0x208f3fff;
    palette[4] = 0x2590c6ff;

    palette[5] = 0xf3c681ff;
    palette[6] = 0x1279c8ff;
    palette[7] = 0xe53939ff;
    palette[8] = 0x68a247ff;
    palette[9] = 0x14c5b6ff;

    std::array<Seed, 10> seeds;

    for (auto it = seeds.begin();
         it != seeds.end();
         ++it)
    {
        it->pos.x = rand_between(0, WINDOW_WIDTH);
        it->pos.y = rand_between(0, WINDOW_HEIGHT);
#if 0
        auto index = rand_between(0, palette.size() - 1);
#else
        auto index = seeds.end() - it - 1;
#endif
        it->color = palette[index];
    }

    auto pixels = static_cast<unsigned int*>(surface->pixels);
    for (int r = 0; r < surface->h; ++r)
    {
        for (int c = 0; c < surface->w; ++c)
        {
            vec2 a = {.x = c, .y = r};
            vec2& b = seeds.begin()->pos;

            //auto nearest = len(a - b);
            auto nearest = std::abs(a.x - b.x) + std::abs(a.y - b.y);
            Seed* nearest_seed = &seeds[0];

            for (auto it = seeds.begin() + 1;
                 it != seeds.end();
                 ++it)
            {
                //auto l = len(a - it->pos);
                auto l = std::abs(a.x - it->pos.x) + std::abs(a.y - it->pos.y);
                if (l < nearest)
                {
                    nearest = l;
                    auto index = seeds.end() - it;
                    nearest_seed = &seeds[index];
                }
            }

            int index = r * surface->w + c;
            pixels[index] = nearest_seed->color; // RRGGBBAA
        }
    }


    for (const auto& seed : seeds)
    {
        draw_circle(surface, seed.pos, 5, 0x000000ff);
    }

    SDL_UnlockSurface(surface);

    return surface;
}

int main()
{
    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        cout << "[ERROR] cannot initialize SDL: " << SDL_GetError() << endl;
        std::exit(1);
    }

    SDL_Window* window = SDL_CreateWindow("Voronoi",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT,
                                          NO_FLAGS);

    if (!window)
    {
        cout << "[ERROR] cannot create window: " << SDL_GetError() << endl;
        std::exit(1);
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer)
    {
        cout << "[ERROR] cannot create renderer: " << SDL_GetError() << endl;
        std::exit(1);
    }

    SDL_Surface* surface = render_shit();
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    if (!texture)
    {
        cout << "[ERROR] cannot create texture: " << SDL_GetError() << endl;
        std::exit(1);
    }

    bool done = false;

    while (!done)
    {
        SDL_Event e = {};
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                done = true;
            }
            else if (e.type == SDL_KEYUP)
            {
                if (e.key.keysym.sym == SDLK_q or
                    e.key.keysym.sym == SDLK_ESCAPE)
                {
                    done = true;
                }
                else if (e.key.keysym.sym == SDLK_F5)
                {
                    SDL_FreeSurface(surface);
                    SDL_DestroyTexture(texture);
                    surface = render_shit();
                    texture = SDL_CreateTextureFromSurface(renderer, surface);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Delay(20);
    }

    return 0;

}
