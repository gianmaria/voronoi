#include <cstdlib>

#include <array>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

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
    float x;
    float y;

    static u32 len(const vec2& vec)
    {
        return static_cast<u32>(vec.x * vec.x + vec.y * vec.y);
    }

    vec2& operator+=(const vec2& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
};

vec2 operator-(const vec2& lhs, const vec2& rhs)
{
    vec2 res{};
    res.x = lhs.x - rhs.x;
    res.y = lhs.y - rhs.y;
    return res;
}

vec2 operator+(const vec2& lhs, const vec2& rhs)
{
    vec2 res{};
    res.x = lhs.x + rhs.x;
    res.y = lhs.y + rhs.y;
    return res;
}

vec2 operator*(const vec2& a, float val)
{
    vec2 res{};
    res.x = a.x * val;
    res.y = a.y * val;
    return res;
}

struct Seed
{
    vec2 pos;
    vec2 vel;
    float r;
    u32 color;
};


constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr int NO_FLAGS = 0;
constexpr int THREAD_COUNT = 4;
constexpr int PITCH = WINDOW_WIDTH * 4;
constexpr int SEED_COUNT = 20;

static u32 pixels[WINDOW_HEIGHT][WINDOW_WIDTH];

constexpr u32 COLOR_BLACK = 0x000000ff;

constexpr std::array<u32, 10> palette = {
    {0x705041ff,
    0xcd5b45ff,
    0xfbd870ff,
    0x208f3fff,
    0x2590c6ff,
    0xf3c681ff,
    0x1279c8ff,
    0xe53939ff,
    0x14c5b6ff,
    0x68a247ff}
};

std::array<Seed, SEED_COUNT> seeds;
SDL_Color white = {255, 255, 255, 255};
SDL_Color black = {0, 0, 0, 255};


int rand_between(int min, int max)
{
    static std::random_device rd;  //Will be used to obtain a seed for the random number engine
    static std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<int> distrib(min, max);
    return distrib(gen);
}

void draw_circle(vec2 pos, float r, u32 color)
{
    SDL_Rect rect = {
        .x = static_cast<int>(pos.x - r),
        .y = static_cast<int>(pos.y - r),
        .w = static_cast<int>(r * 2.0f),
        .h = static_cast<int>(r * 2.0f)
    };

    if (rect.y < 0)
        rect.y = 0;
    else if (rect.y + rect.h > WINDOW_HEIGHT)
        rect.h = WINDOW_HEIGHT - rect.y;

    if (rect.x < 0)
        rect.x = 0;
    else if (rect.x + rect.w > WINDOW_WIDTH)
        rect.w = WINDOW_WIDTH - rect.x;

    u32 r2 = static_cast<u32>(r * r);

    for (int y = rect.y;
         y < rect.h + rect.y;
         ++y)
    {
        for (int x = rect.x;
             x < rect.w + rect.x;
             ++x)
        {
            vec2 point
            {
                .x = static_cast<float>(x),
                .y = static_cast<float>(y)
            };

            if (vec2::len(point - pos) < r2)
            {
                pixels[y][x] = color;
            }
        }
    }
}

void randomize_seeds()
{
    for (size_t i = 0;
         i < seeds.size();
         ++i)
    {
        auto& seed = seeds[i];
        seed.pos.x = static_cast<float>(rand_between(0, WINDOW_WIDTH));
        seed.pos.y = static_cast<float>(rand_between(0, WINDOW_HEIGHT));

        seed.vel.x = 0;
        seed.vel.y = 0;

        seed.r = 5.0f;
#if 0
        auto index = rand_between(0, palette.size() - 1);
#else
        auto index = i;
#endif
        seed.color = palette[index % palette.size()];
    }

    //seeds[0].pos.x = 70.0f;
    //seeds[0].pos.y = 100.0f;
    seeds[0].vel.x = 30.0f;
    seeds[0].vel.y = 35.0f;
    seeds[0].r = 5.0f;
}

void update_seed_position(float dt)
{
    Seed& seed = seeds[0];
    seed.pos += seed.vel * dt;

    if (seed.pos.x + seed.r > WINDOW_WIDTH)
    {
        seed.pos.x = WINDOW_WIDTH - seed.r;
        seed.vel.x *= -1;
    }
    else if (seed.pos.x - seed.r < 0)
    {
        seed.pos.x = 0 + seed.r;
        seed.vel.x *= -1;
    }

    if (seed.pos.y + seed.r > WINDOW_HEIGHT)
    {
        seed.pos.y = WINDOW_HEIGHT - seed.r;
        seed.vel.y *= -1;
    }
    else if (seed.pos.y - seed.r < 0)
    {
        seed.pos.y = 0 + seed.r;
        seed.vel.y *= -1;
    }

}

void render_voronoi_helper(const SDL_Rect& region)
{
    // NOTE: top-left -> 0, 0
    //       y increasing towards bottom

    for (int y = region.y;
         y < region.h + region.y;
         ++y)
    {
        for (int x = region.x;
             x < region.w + region.x;
             ++x)
        {
            vec2 a{
                .x = static_cast<float>(x),
                .y = static_cast<float>(y)
            };
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
        draw_circle(seed.pos, seed.r, COLOR_BLACK);
    }
}

void render_voronoi()
{
    std::array<std::thread, THREAD_COUNT> threads;

    for (size_t i = 0;
         i < threads.size();
         ++i)
    {
        SDL_Rect region;
        region.x = 0;
        region.y = WINDOW_HEIGHT / THREAD_COUNT * i;
        region.w = WINDOW_WIDTH;
        region.h = WINDOW_HEIGHT / THREAD_COUNT;

        threads[i] = std::thread(render_voronoi_helper, region);
    }

    for (auto& thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }
}

void draw_text(SDL_Renderer* renderer,
               TTF_Font* font,
               const char* msg)
{

    // as TTF_RenderText_Solid could only be used on
    // SDL_Surface then you have to create the surface first
    SDL_Surface* surface =
        TTF_RenderText_Solid(font, msg, black);

    // now you can convert it into a texture
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect message_rect{}; //create a rect
    TTF_SizeText(font, msg,
                 &message_rect.w,
                 &message_rect.h);

    SDL_RenderCopy(renderer, texture, NULL, &message_rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}


int main()
{
    SDL_SetMainReady();

    if (TTF_Init() < 0)
    {
        cout << "[ERROR] cannot initialize TTF: " << SDL_GetError() << endl;
        std::exit(1);
    }

    TTF_Font* font = TTF_OpenFont("font/FreeSans.ttf", 20);

    if (!font)
    {
        cout << "[ERROR] cannot open font: " << SDL_GetError() << endl;
        std::exit(1);
    }


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
    randomize_seeds();

    render_voronoi();

    bool done = false;
    bool pause = false;
    float dt = 0.3f;
    char buffer[512]{};
    u32 counter = 0;
    float cached_dt = 0;
    float cached_fps = 0;

    while (!done)
    {
        auto begin = SDL_GetTicks64();

        SDL_Event e = {};
        while (SDL_PollEvent(&e))
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
                    randomize_seeds();
                }
                else if (e.key.keysym.sym == SDLK_SPACE)
                {
                    pause = !pause;
                }
            }
        }

        if (!pause)
        {
            update_seed_position(dt);

            render_voronoi();
        }

        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); // magenta
        SDL_RenderClear(renderer);

        SDL_UpdateTexture(texture, NULL, pixels, PITCH);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        if (counter++ % 10 == 0)
        {
            cached_dt = dt;
            cached_fps = 1.0f / dt;
        }

        snprintf(buffer, sizeof(buffer), "%.3f ms - %.2f FPS", cached_dt, cached_fps);
        draw_text(renderer, font, buffer);

        SDL_RenderPresent(renderer);

        SDL_Delay(10);
        auto end = SDL_GetTicks64();
        dt = static_cast<float>(end - begin) / 1000.0f; // in sec
    }

    return 0;

}
