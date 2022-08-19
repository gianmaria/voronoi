#include <cstdlib>

#include <array>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

using std::cout;
using std::endl;
using std::string;

using u32 = uint32_t;
using u64 = uint64_t;


struct Timer
{
    Timer(const string& name) :name(name)
    {
        begin = SDL_GetTicks64();
    }
    ~Timer()
    {
        Uint64 end = SDL_GetTicks64();
        Uint64 elapsed_ms = end - begin;
        cout << name << " took: " << elapsed_ms << "ms" << endl;
    }

    Uint64 begin;
    string name;
};

struct vec2
{
    int x;
    int y;

    static u32 len(const vec2& vec)
    {
        return static_cast<u32>(vec.x * vec.x + vec.y * vec.y);
    }
};

vec2 operator-(const vec2& lhs, const vec2& rhs)
{
    vec2 res{};
    res.x = lhs.x - rhs.x;
    res.y = lhs.y - rhs.y;
    return res;
}

struct Seed
{
    vec2 pos;
    vec2 vel;
    int r;
    unsigned int color;
};


constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr int NO_FLAGS = 0;

static u32 pixels[WINDOW_HEIGHT][WINDOW_WIDTH];

constexpr std::array<u32, 10> palette = {
    0x705041ff,
    0xcd5b45ff,
    0xfbd870ff,
    0x208f3fff,
    0x2590c6ff,
    0xf3c681ff,
    0x1279c8ff,
    0xe53939ff,
    0x14c5b6ff,
    0x68a247ff
};

std::array<Seed, 10> seeds;


int rand_between(int min, int max)
{
    static std::random_device rd;  //Will be used to obtain a seed for the random number engine
    static std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<int> distrib(min, max);
    return distrib(gen);
}

void draw_circle(vec2 pos, int r, u32 color)
{
    vec2 top_left{.x = pos.x - r, .y = pos.y + r};
    vec2 bottom_right{.x = pos.x + r, .y = pos.y - r};

    u32 r2 = static_cast<u32>(r * r);

    for (int y = top_left.y;
         y > bottom_right.y;
         --y)
    {
        for (int x = top_left.x;
             x < bottom_right.x + r;
             ++x)
        {
            vec2 point{.x = x, .y = y};

            if (vec2::len(point - pos) < r2)
            {
                pixels[y][x] = color;
            }
        }
    }
}

void init_seeds()
{
    for (size_t i = 0;
         i < seeds.size();
         ++i)
    {
        auto& seed = seeds[i];
        seed.pos.x = rand_between(0, WINDOW_WIDTH);
        seed.pos.y = rand_between(0, WINDOW_HEIGHT);

        seed.vel.x = 0;
        seed.vel.y = 0;

        seed.r = 5;
#if 0
        auto index = rand_between(0, palette.size() - 1);
#else
        auto index = i;
#endif
        seed.color = palette[index];
    }
}

void render_voronoi_helper(const SDL_Rect& region)
{
    // top-left -> 0, 0
    // y increasing towards bottom

    for (int y = region.y; 
         y < region.h + region.y; 
         ++y)
    {
        for (int x = region.x; 
             x < region.w + region.x; 
             ++x)
        {
            vec2 a = {.x = x, .y = y};
            vec2* b = &seeds[0].pos;

            //auto nearest = len(a - b);
            auto shortest_len = std::abs(a.x - b->x) + std::abs(a.y - b->y);
            Seed* nearest_seed = &seeds[0];

            for (size_t i = 1;
                 i < seeds.size();
                 ++i)
            {
                b = &seeds[i].pos;
                //auto l = len(a - it->pos);
                auto current_len = std::abs(a.x - b->x) + std::abs(a.y - b->y);
                //auto l = (std::powl(a.x - b->x, 2)) + std::powl((a.y - b->y), 2);
                if (current_len < shortest_len)
                {
                    shortest_len = current_len;
                    nearest_seed = &seeds[i];
                }
            }

            pixels[y][x] = nearest_seed->color;
        }
    }

    for (const auto& seed : seeds)
    {
        draw_circle(seed.pos, seed.r, 0x000000ff);
    }

}

void render_voronoi(SDL_Renderer* renderer, SDL_Texture* texture)
{
    init_seeds();

    {
        Timer t("render voronoi");
        render_voronoi_helper({.x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT / 4});
    }

    {
        Timer t("drawing texture business");

        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); // magenta
        SDL_RenderClear(renderer);

        SDL_UpdateTexture(texture, NULL, pixels, WINDOW_WIDTH * 4);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        SDL_RenderPresent(renderer);

    }
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

    SDL_Texture* texture = SDL_CreateTexture(renderer,
                                                    SDL_PIXELFORMAT_RGBA8888,
                                                    SDL_TEXTUREACCESS_STREAMING,
                                                    WINDOW_WIDTH, WINDOW_HEIGHT);

    render_voronoi(renderer, texture);

    bool done = false;
    while (!done)
    {
        SDL_Event e = {};
        while (SDL_WaitEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                done = true;
                break;
            }
            else if (e.type == SDL_KEYUP)
            {
                if (e.key.keysym.sym == SDLK_q or
                    e.key.keysym.sym == SDLK_ESCAPE)
                {
                    done = true;
                    break;
                }
                else if (e.key.keysym.sym == SDLK_F5)
                {
                    render_voronoi(renderer, texture);
                }
            }
        }

        //SDL_Delay(10);
    }

    return 0;

}
