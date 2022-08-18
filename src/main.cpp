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
    vec2 vel;
    int r;
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

void draw_circle(SDL_Renderer* renderer, vec2 pos, int r, u32 color)
{
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
                uint8_t* col = reinterpret_cast<uint8_t*>(&color);
                SDL_SetRenderDrawColor(renderer, col[3], col[2], col[1], col[0]);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
}

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

void render_voronoi(SDL_Renderer* renderer)
{
    std::array<u32, 10> palette;
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

        it->vel.x = 0;
        it->vel.y = 0;

        it->r = 5;
#if 0
        auto index = rand_between(0, palette.size() - 1);
#else
        auto index = seeds.end() - it - 1;
#endif
        it->color = palette[index];
    }

    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    SDL_RenderClear(renderer);

    {
        Timer t("render voronoi");

        for (int r = 0; r < WINDOW_HEIGHT / 1; ++r)
        {
            for (int c = 0; c < WINDOW_WIDTH / 1; ++c)
            {
                vec2 a = {.x = c, .y = r};
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

                uint8_t* col = reinterpret_cast<uint8_t*>(&nearest_seed->color);
                SDL_SetRenderDrawColor(renderer, col[3], col[2], col[1], col[0]);
                SDL_RenderDrawPoint(renderer, c, r);

            }
        }

        for (const auto& seed : seeds)
        {
            draw_circle(renderer, seed.pos, seed.r, 0x000000ff);
        }
    }

    SDL_RenderPresent(renderer);
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

    render_voronoi(renderer);

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
                    render_voronoi(renderer);
                }
            }
        }

        //SDL_Delay(10);
    }

    return 0;

}
