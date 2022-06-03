// TODO: Textures shouldn't disappear on window resize.
// TODO: Use deltaTime and delete SDL_Delay(16.666)?
// TODO: Add moving ghosts. Maybe get position of random pointRect (they should exist even after collecting them by the player) and use A* to go to that point? How to put random pointRect into A* algorithm?
// TODO: Add animation to player.
// TODO: Add teleport?
// TODO: Add gameover and win states.
// TODO: Add eating of ghosts.
// TODO: Add big points on the map?
// TODO: Add sounds and music?
// TODO: Remember to add source code to disk drive on project end.
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_net.h>
#include <SDL_ttf.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>
//#include <SDL_gpu.h>
//#include <SFML/Network.hpp>
//#include <SFML/Graphics.hpp>
#include <algorithm>
#include <atomic>
#include <codecvt>
#include <functional>
#include <locale>
#include <mutex>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#endif
#ifdef __ANDROID__
#include "vendor/PUGIXML/src/pugixml.hpp"
#include <android/log.h> //__android_log_print(ANDROID_LOG_VERBOSE, "AppName_", "Example number log: %d", number);
#include <jni.h>
#include "vendor/GLM/include/glm/glm.hpp"
#include "vendor/GLM/include/glm/gtc/matrix_transform.hpp"
#include "vendor/GLM/include/glm/gtc/type_ptr.hpp"
#include "vendor/IMGUI/imgui.h"
#include "vendor/IMGUI/backends/imgui_impl_sdl.h"
#include "vendor/IMGUI/backends/imgui_impl_sdlrenderer.h"
#include "vendor/IMGUI/misc/cpp/imgui_stdlib.h"
#else
#include <filesystem>
#include <pugixml.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_sdlrenderer.h>
#include <misc/cpp/imgui_stdlib.h>
using namespace std::chrono_literals;
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

// 240 x 240 (smart watch)
// 240 x 320 (QVGA)
// 360 x 640 (Galaxy S5)
// 640 x 480 (480i - Smallest PC monitor)

#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define PLAYER_SPEED 1
#define TILE_SIZE 8
SDL_Color TILE_EMPTY_COLOR{ 125, 125, 125 };
SDL_Color TILE_PLAYER_COLOR{ 0, 255, 0 };
SDL_Color TILE_WALL_COLOR{ 80, 50, 50 };

int windowWidth = 240;
int windowHeight = 320;
SDL_Point mousePos;
SDL_Point realMousePos;
bool keys[SDL_NUM_SCANCODES];
bool buttons[SDL_BUTTON_X2 + 1];
SDL_Window* window;
SDL_Renderer* renderer;

void logOutputCallback(void* userdata, int category, SDL_LogPriority priority, const char* message)
{
    std::cout << message << std::endl;
}

int random(int min, int max)
{
    return min + rand() % ((max + 1) - min);
}

int SDL_QueryTextureF(SDL_Texture* texture, Uint32* format, int* access, float* w, float* h)
{
    int wi, hi;
    int result = SDL_QueryTexture(texture, format, access, &wi, &hi);
    if (w) {
        *w = wi;
    }
    if (h) {
        *h = hi;
    }
    return result;
}

SDL_bool SDL_PointInFRect(const SDL_Point* p, const SDL_FRect* r)
{
    return ((p->x >= r->x) && (p->x < (r->x + r->w)) && (p->y >= r->y) && (p->y < (r->y + r->h))) ? SDL_TRUE : SDL_FALSE;
}

std::ostream& operator<<(std::ostream& os, SDL_FRect r)
{
    os << r.x << " " << r.y << " " << r.w << " " << r.h;
    return os;
}

std::ostream& operator<<(std::ostream& os, SDL_Rect r)
{
    SDL_FRect fR;
    fR.w = r.w;
    fR.h = r.h;
    fR.x = r.x;
    fR.y = r.y;
    os << fR;
    return os;
}

SDL_Texture* renderText(SDL_Texture* previousTexture, TTF_Font* font, SDL_Renderer* renderer, const std::string& text, SDL_Color color)
{
    if (previousTexture) {
        SDL_DestroyTexture(previousTexture);
    }
    SDL_Surface* surface;
    if (text.empty()) {
        surface = TTF_RenderUTF8_Blended(font, " ", color);
    }
    else {
        surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    }
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
    }
    else {
        return 0;
    }
}

struct Text {
    std::string text;
    SDL_Surface* surface = 0;
    SDL_Texture* t = 0;
    SDL_FRect dstR{};
    bool autoAdjustW = false;
    bool autoAdjustH = false;
    float wMultiplier = 1;
    float hMultiplier = 1;

    ~Text()
    {
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
    }

    Text()
    {
    }

    Text(const Text& rightText)
    {
        text = rightText.text;
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
        if (rightText.surface) {
            surface = SDL_ConvertSurface(rightText.surface, rightText.surface->format, SDL_SWSURFACE);
        }
        if (rightText.t) {
            t = SDL_CreateTextureFromSurface(renderer, surface);
        }
        dstR = rightText.dstR;
        autoAdjustW = rightText.autoAdjustW;
        autoAdjustH = rightText.autoAdjustH;
        wMultiplier = rightText.wMultiplier;
        hMultiplier = rightText.hMultiplier;
    }

    Text& operator=(const Text& rightText)
    {
        text = rightText.text;
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
        if (rightText.surface) {
            surface = SDL_ConvertSurface(rightText.surface, rightText.surface->format, SDL_SWSURFACE);
        }
        if (rightText.t) {
            t = SDL_CreateTextureFromSurface(renderer, surface);
        }
        dstR = rightText.dstR;
        autoAdjustW = rightText.autoAdjustW;
        autoAdjustH = rightText.autoAdjustH;
        wMultiplier = rightText.wMultiplier;
        hMultiplier = rightText.hMultiplier;
        return *this;
    }

    void setText(SDL_Renderer* renderer, TTF_Font* font, std::string text, SDL_Color c = { 255, 255, 255 })
    {
        this->text = text;
#if 1 // NOTE: renderText
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
        if (text.empty()) {
            surface = TTF_RenderUTF8_Blended(font, " ", c);
        }
        else {
            surface = TTF_RenderUTF8_Blended(font, text.c_str(), c);
        }
        if (surface) {
            t = SDL_CreateTextureFromSurface(renderer, surface);
        }
#endif
        if (autoAdjustW) {
            SDL_QueryTextureF(t, 0, 0, &dstR.w, 0);
        }
        if (autoAdjustH) {
            SDL_QueryTextureF(t, 0, 0, 0, &dstR.h);
        }
        dstR.w *= wMultiplier;
        dstR.h *= hMultiplier;
    }

    void setText(SDL_Renderer* renderer, TTF_Font* font, int value, SDL_Color c = { 255, 255, 255 })
    {
        setText(renderer, font, std::to_string(value), c);
    }

    void draw(SDL_Renderer* renderer)
    {
        if (t) {
            SDL_RenderCopyF(renderer, t, 0, &dstR);
        }
    }
};

int SDL_RenderDrawCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
    int offsetx, offsety, d;
    int status;

    offsetx = 0;
    offsety = radius;
    d = radius - 1;
    status = 0;

    while (offsety >= offsetx) {
        status += SDL_RenderDrawPoint(renderer, x + offsetx, y + offsety);
        status += SDL_RenderDrawPoint(renderer, x + offsety, y + offsetx);
        status += SDL_RenderDrawPoint(renderer, x - offsetx, y + offsety);
        status += SDL_RenderDrawPoint(renderer, x - offsety, y + offsetx);
        status += SDL_RenderDrawPoint(renderer, x + offsetx, y - offsety);
        status += SDL_RenderDrawPoint(renderer, x + offsety, y - offsetx);
        status += SDL_RenderDrawPoint(renderer, x - offsetx, y - offsety);
        status += SDL_RenderDrawPoint(renderer, x - offsety, y - offsetx);

        if (status < 0) {
            status = -1;
            break;
        }

        if (d >= 2 * offsetx) {
            d -= 2 * offsetx + 1;
            offsetx += 1;
        }
        else if (d < 2 * (radius - offsety)) {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }

    return status;
}

int SDL_RenderFillCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
    int offsetx, offsety, d;
    int status;

    offsetx = 0;
    offsety = radius;
    d = radius - 1;
    status = 0;

    while (offsety >= offsetx) {

        status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx,
            x + offsety, y + offsetx);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety,
            x + offsetx, y + offsety);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety,
            x + offsetx, y - offsety);
        status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx,
            x + offsety, y - offsetx);

        if (status < 0) {
            status = -1;
            break;
        }

        if (d >= 2 * offsetx) {
            d -= 2 * offsetx + 1;
            offsetx += 1;
        }
        else if (d < 2 * (radius - offsety)) {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }

    return status;
}

struct Clock {
    Uint64 start = SDL_GetPerformanceCounter();

    float getElapsedTime()
    {
        Uint64 stop = SDL_GetPerformanceCounter();
        float secondsElapsed = (stop - start) / (float)SDL_GetPerformanceFrequency();
        return secondsElapsed * 1000;
    }

    float restart()
    {
        Uint64 stop = SDL_GetPerformanceCounter();
        float secondsElapsed = (stop - start) / (float)SDL_GetPerformanceFrequency();
        start = SDL_GetPerformanceCounter();
        return secondsElapsed * 1000;
    }
};

#ifndef __ANDROID__
SDL_bool SDL_FRectEmpty(const SDL_FRect* r)
{
    return ((!r) || (r->w <= 0) || (r->h <= 0)) ? SDL_TRUE : SDL_FALSE;
}
#endif

SDL_bool SDL_IntersectFRect(const SDL_FRect* A, const SDL_FRect* B, SDL_FRect* result)
{
    int Amin, Amax, Bmin, Bmax;

    if (!A) {
        SDL_InvalidParamError("A");
        return SDL_FALSE;
    }

    if (!B) {
        SDL_InvalidParamError("B");
        return SDL_FALSE;
    }

    if (!result) {
        SDL_InvalidParamError("result");
        return SDL_FALSE;
    }

    /* Special cases for empty rects */
    if (SDL_FRectEmpty(A) || SDL_FRectEmpty(B)) {
        result->w = 0;
        result->h = 0;
        return SDL_FALSE;
    }

    /* Horizontal intersection */
    Amin = A->x;
    Amax = Amin + A->w;
    Bmin = B->x;
    Bmax = Bmin + B->w;
    if (Bmin > Amin)
        Amin = Bmin;
    result->x = Amin;
    if (Bmax < Amax)
        Amax = Bmax;
    result->w = Amax - Amin;

    /* Vertical intersection */
    Amin = A->y;
    Amax = Amin + A->h;
    Bmin = B->y;
    Bmax = Bmin + B->h;
    if (Bmin > Amin)
        Amin = Bmin;
    result->y = Amin;
    if (Bmax < Amax)
        Amax = Bmax;
    result->h = Amax - Amin;

    return (SDL_FRectEmpty(result) == SDL_TRUE) ? SDL_FALSE : SDL_TRUE;
}

SDL_bool SDL_HasIntersectionF(const SDL_FRect* A, const SDL_FRect* B)
{
    float Amin, Amax, Bmin, Bmax;

    if (!A) {
        SDL_InvalidParamError("A");
        return SDL_FALSE;
    }

    if (!B) {
        SDL_InvalidParamError("B");
        return SDL_FALSE;
    }

    /* Special cases for empty rects */
    if (SDL_FRectEmpty(A) || SDL_FRectEmpty(B)) {
        return SDL_FALSE;
    }

    /* Horizontal intersection */
    Amin = A->x;
    Amax = Amin + A->w;
    Bmin = B->x;
    Bmax = Bmin + B->w;
    if (Bmin > Amin)
        Amin = Bmin;
    if (Bmax < Amax)
        Amax = Bmax;
    if (Amax <= Amin)
        return SDL_FALSE;

    /* Vertical intersection */
    Amin = A->y;
    Amax = Amin + A->h;
    Bmin = B->y;
    Bmax = Bmin + B->h;
    if (Bmin > Amin)
        Amin = Bmin;
    if (Bmax < Amax)
        Amax = Bmax;
    if (Amax <= Amin)
        return SDL_FALSE;

    return SDL_TRUE;
}

float clamp(float n, float lower, float upper)
{
    return std::max(lower, std::min(n, upper));
}

std::string getCurrentDate()
{
    std::time_t t = std::time(0);
    std::tm* now = std::localtime(&t);
    std::stringstream ss;
    ss << (now->tm_year + 1900) << "-" << (now->tm_mon + 1) << "-" << now->tm_mday;
    return ss.str();
}

std::string readWholeFile(std::string path)
{
    std::stringstream ss;
    std::ifstream ifs(path, std::ifstream::in);
    ss << ifs.rdbuf();
    return ss.str();
}

void saveToFile(std::string path, std::string str)
{
    std::stringstream ss;
    ss << str;
    std::ofstream ofs(path);
    ofs << ss.str();
}

int eventWatch(void* userdata, SDL_Event* event)
{
    // WARNING: Be very careful of what you do in the function, as it may run in a different thread
    if (event->type == SDL_APP_TERMINATING || event->type == SDL_APP_WILLENTERBACKGROUND) {
    }
    return 0;
}

enum class Direction {
    Left,
    Right,
    Up,
    Down
};

struct Entity {
    SDL_Rect r{};
    int dx = 0;
    int dy = 0;
    Direction d = Direction::Down;
    SDL_Rect destinationPointR{};
};

struct Tile {
    SDL_Rect r{};
    SDL_Color c{};
};

struct Node {
    SDL_Rect r{};
    int g = 0;
    int h = 0;
    int parentIndexInClosedNodes = -1;

    // NOTE: Diagonal movement cost must be same as horizontal and vertical
    int diagonalDistance(SDL_Rect dst)
    {
        int movementCost = 1;
        return movementCost * std::max(std::abs(r.x - dst.x), std::abs(r.y - dst.y));
    }

    int getF()
    {
        return g + h;
    }
};

bool operator==(SDL_Color c1, SDL_Color c2)
{
    return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b && c1.a == c2.a;
}

bool operator!=(SDL_Color c1, SDL_Color c2)
{
    return c1.r != c2.r || c1.g != c2.g || c1.b != c2.b || c1.a != c2.a;
}

bool operator==(SDL_Point p1, SDL_Point p2)
{
    return p1.x == p2.x && p1.y == p2.y;
}

bool operator!=(SDL_Point p1, SDL_Point p2)
{
    return p1.x != p2.x || p1.y != p2.y;
}

bool operator==(SDL_Rect r1, SDL_Rect r2)
{
    return r1.x == r2.x && r1.y == r2.y && r1.w == r2.w && r1.h == r2.h;
}

bool operator!=(SDL_Rect r1, SDL_Rect r2)
{
    return r1.x != r2.x || r1.y != r2.y || r1.w != r2.w || r1.h != r2.h;
}

bool operator==(Node n1, Node n2)
{
    return n1.r == n2.r;
}

bool operator!=(Node n1, Node n2)
{
    return n1.r != n2.r;
}

void run()
{
    std::srand(std::time(0));
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    SDL_LogSetOutputFunction(logOutputCallback, 0);
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    SDL_GetMouseState(&mousePos.x, &mousePos.y);
    window = SDL_CreateWindow("PacMan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* mapT = IMG_LoadTexture(renderer, "res/map.png");
    SDL_Texture* playerT = IMG_LoadTexture(renderer, "res/player.png");
    SDL_Texture* pinkGhost = IMG_LoadTexture(renderer, "res/pinkGhost.png");
    SDL_Texture* blueGhost = IMG_LoadTexture(renderer, "res/blueGhost.png");
    SDL_Texture* orangeGhost = IMG_LoadTexture(renderer, "res/orangeGhost.png");
    TTF_Font* robotoF = TTF_OpenFont("res/roboto.ttf", 72);
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_RenderSetScale(renderer, w / (float)windowWidth, h / (float)windowHeight);
    SDL_AddEventWatch(eventWatch, 0);
    bool running = true;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window);
    ImGui_ImplSDLRenderer_Init(renderer);
    bool showDemoWindow = false;
    Entity p;
    p.r.w = 14;
    p.r.h = 14;
    p.r.x = 96;
    p.r.y = 230;
    std::vector<Tile> map;
    map.push_back(Tile());
    map.back().r.x = 29;
    map.back().r.y = 28;
    map.back().r.w = 53 - map.back().r.x + 1;
    map.back().r.h = 49 - map.back().r.y + 1;
    map.back().c = TILE_WALL_COLOR;
    map.push_back(map.front());
    map.back().r.x = 68;
    map.back().r.y = 28;
    map.back().r.w = 99 - map.back().r.x + 1;
    map.back().r.h = 49 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 114;
    map.back().r.y = 8;
    map.back().r.w = 122 - map.back().r.x + 1;
    map.back().r.h = 49 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 137;
    map.back().r.y = 28;
    map.back().r.w = 168 - map.back().r.x + 1;
    map.back().r.h = 49 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 183;
    map.back().r.y = 28;
    map.back().r.w = 206 - map.back().r.x + 1;
    map.back().r.h = 49 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 10;
    map.back().r.y = 3;
    map.back().r.w = 225 - map.back().r.x + 1;
    map.back().r.h = 9 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 10;
    map.back().r.y = 9;
    map.back().r.w = 14 - map.back().r.x + 1;
    map.back().r.h = 104 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 221;
    map.back().r.y = 9;
    map.back().r.w = 225 - map.back().r.x + 1;
    map.back().r.h = 104 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 29;
    map.back().r.y = 68;
    map.back().r.w = 53 - map.back().r.x + 1;
    map.back().r.h = 79 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 68;
    map.back().r.y = 68;
    map.back().r.w = 76 - map.back().r.x + 1;
    map.back().r.h = 138 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 76;
    map.back().r.y = 97;
    map.back().r.w = 99 - map.back().r.x + 1;
    map.back().r.h = 109 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 91;
    map.back().r.y = 68;
    map.back().r.w = 145 - map.back().r.x + 1;
    map.back().r.h = 79 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 114;
    map.back().r.y = 79;
    map.back().r.w = 122 - map.back().r.x + 1;
    map.back().r.h = 109 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 183;
    map.back().r.y = 68;
    map.back().r.w = 206 - map.back().r.x + 1;
    map.back().r.h = 79 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 160;
    map.back().r.y = 68;
    map.back().r.w = 168 - map.back().r.x + 1;
    map.back().r.h = 138 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 137;
    map.back().r.y = 98;
    map.back().r.w = 160 - map.back().r.x + 1;
    map.back().r.h = 109 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 14;
    map.back().r.y = 98;
    map.back().r.w = 53 - map.back().r.x + 1;
    map.back().r.h = 104 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 49;
    map.back().r.y = 104;
    map.back().r.w = 53 - map.back().r.x + 1;
    map.back().r.h = 138 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 11;
    map.back().r.y = 133;
    map.back().r.w = 49 - map.back().r.x + 1;
    map.back().r.h = 138 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 183;
    map.back().r.y = 98;
    map.back().r.w = 221 - map.back().r.x + 1;
    map.back().r.h = 104 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 183;
    map.back().r.y = 104;
    map.back().r.w = 188 - map.back().r.x + 1;
    map.back().r.h = 138 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 188;
    map.back().r.y = 133;
    map.back().r.w = 224 - map.back().r.x + 1;
    map.back().r.h = 139 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 91;
    map.back().r.y = 128;
    map.back().r.w = 107 - map.back().r.x + 1;
    map.back().r.h = 134 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 128;
    map.back().r.y = 128;
    map.back().r.w = 144 - map.back().r.x + 1;
    map.back().r.h = 134 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 91;
    map.back().r.y = 134;
    map.back().r.w = 95 - map.back().r.x + 1;
    map.back().r.h = 168 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 95;
    map.back().r.y = 163;
    map.back().r.w = 144 - map.back().r.x + 1;
    map.back().r.h = 168 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 140;
    map.back().r.y = 134;
    map.back().r.w = 144 - map.back().r.x + 1;
    map.back().r.h = 163 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 11;
    map.back().r.y = 158;
    map.back().r.w = 53 - map.back().r.x + 1;
    map.back().r.h = 163 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 49;
    map.back().r.y = 163;
    map.back().r.w = 53 - map.back().r.x + 1;
    map.back().r.h = 198 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 10;
    map.back().r.y = 193;
    map.back().r.w = 49 - map.back().r.x + 1;
    map.back().r.h = 198 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 10;
    map.back().r.y = 198;
    map.back().r.w = 15 - map.back().r.x + 1;
    map.back().r.h = 313 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 15;
    map.back().r.y = 247;
    map.back().r.w = 30 - map.back().r.x + 1;
    map.back().r.h = 258 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 15;
    map.back().r.y = 307;
    map.back().r.w = 225 - map.back().r.x + 1;
    map.back().r.h = 313 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 220;
    map.back().r.y = 193;
    map.back().r.w = 225 - map.back().r.x + 1;
    map.back().r.h = 307 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 183;
    map.back().r.y = 193;
    map.back().r.w = 220 - map.back().r.x + 1;
    map.back().r.h = 198 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 183;
    map.back().r.y = 158;
    map.back().r.w = 187 - map.back().r.x + 1;
    map.back().r.h = 193 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 187;
    map.back().r.y = 158;
    map.back().r.w = 224 - map.back().r.x + 1;
    map.back().r.h = 163 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 68;
    map.back().r.y = 158;
    map.back().r.w = 76 - map.back().r.x + 1;
    map.back().r.h = 198 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 160;
    map.back().r.y = 158;
    map.back().r.w = 168 - map.back().r.x + 1;
    map.back().r.h = 198 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 91;
    map.back().r.y = 188;
    map.back().r.w = 145 - map.back().r.x + 1;
    map.back().r.h = 198 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 114;
    map.back().r.y = 198;
    map.back().r.w = 122 - map.back().r.x + 1;
    map.back().r.h = 228 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 30;
    map.back().r.y = 217;
    map.back().r.w = 53 - map.back().r.x + 1;
    map.back().r.h = 228 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 45;
    map.back().r.y = 228;
    map.back().r.w = 53 - map.back().r.x + 1;
    map.back().r.h = 258 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 182;
    map.back().r.y = 217;
    map.back().r.w = 205 - map.back().r.x + 1;
    map.back().r.h = 228 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 182;
    map.back().r.y = 228;
    map.back().r.w = 191 - map.back().r.x + 1;
    map.back().r.h = 258 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 137;
    map.back().r.y = 217;
    map.back().r.w = 167 - map.back().r.x + 1;
    map.back().r.h = 228 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 68;
    map.back().r.y = 217;
    map.back().r.w = 99 - map.back().r.x + 1;
    map.back().r.h = 228 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 30;
    map.back().r.y = 277;
    map.back().r.w = 99 - map.back().r.x + 1;
    map.back().r.h = 288 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 137;
    map.back().r.y = 277;
    map.back().r.w = 205 - map.back().r.x + 1;
    map.back().r.h = 288 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 68;
    map.back().r.y = 247;
    map.back().r.w = 76 - map.back().r.x + 1;
    map.back().r.h = 277 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 206;
    map.back().r.y = 247;
    map.back().r.w = 220 - map.back().r.x + 1;
    map.back().r.h = 258 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 159;
    map.back().r.y = 247;
    map.back().r.w = 167 - map.back().r.x + 1;
    map.back().r.h = 277 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 91;
    map.back().r.y = 247;
    map.back().r.w = 144 - map.back().r.x + 1;
    map.back().r.h = 258 - map.back().r.y + 1;
    map.push_back(map.front());
    map.back().r.x = 114;
    map.back().r.y = 258;
    map.back().r.w = 122 - map.back().r.x + 1;
    map.back().r.h = 288 - map.back().r.y + 1;
    int mapSizeBeforeAddingTilesThatAreNotWalls = map.size();
    for (int x = 0; x < windowWidth; ++x) {
        for (int y = 0; y < windowHeight; ++y) {
            Tile tile;
            tile.r.w = 1;
            tile.r.h = 1;
            tile.r.x = x;
            tile.r.y = y;
            bool hasIntersection = false;
            for (int i = 0; i < mapSizeBeforeAddingTilesThatAreNotWalls; ++i) {
                if (SDL_HasIntersection(&tile.r, &map[i].r)) {
                    hasIntersection = true;
                    break;
                }
            }
            if (!hasIntersection) {
                map.push_back(tile);
            }
        }
    }
    bool wantedLeftButWasNotAble = false, wantedRightButWasNotAble = false, wantedUpButWasNotAble = false, wantedDownButWasNotAble = false;
    std::vector<SDL_Rect> pointRects;
    pointRects.push_back(SDL_Rect());
    pointRects.back().w = 2;
    pointRects.back().h = 2;
    pointRects.back().x = 22;
    pointRects.back().y = 18;
    for (int i = 0; i < 11; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().x += 8;
    }
    for (int i = 0; i < 4; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().y += 10;
    }
    for (int i = 0; i < 11; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().x -= 8;
    }
    for (int i = 0; i < 3; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().y -= 10;
    }
    pointRects.push_back(pointRects[5]);
    pointRects.back().y += 10;
    for (int i = 0; i < 24; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().y += 10;
    }
    pointRects.push_back(pointRects[26]);
    pointRects.back().y += 10;
    for (int i = 0; i < 2; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().y += 10;
    }
    for (int i = 0; i < 4; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().x += 8;
    }
    pointRects.push_back(pointRects[18]);
    pointRects.back().y += 10;
    for (int i = 0; i < 2; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().y += 10;
    }
    for (int i = 0; i < 3; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().x += 8;
    }
    pointRects.push_back(pointRects[54]);
    pointRects.back().x -= 8;
    for (int i = 0; i < 4; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().x -= 8;
    }
    pointRects.push_back(pointRects[70]);
    pointRects.back().y -= 10;
    for (int i = 0; i < 2; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().y -= 10;
    }
    for (int i = 0; i < 2; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().x -= 8;
    }
    for (int i = 0; i < 3; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().y -= 10;
    }
    for (int i = 0; i < 10; ++i) {
        if (i == 4) {
            pointRects.push_back(pointRects.back());
            pointRects.back().x += 16;
        }
        else {
            pointRects.push_back(pointRects.back());
            pointRects.back().x += 8;
        }
    }
    for (int i = 0; i < 3; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().y += 10;
    }
    for (int i = 0; i < 5; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().x -= 8;
    }
    pointRects.push_back(pointRects[pointRects.size() - 3]);
    pointRects.back().y += 10;
    for (int i = 0; i < 2; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().y += 10;
    }
    for (int i = 0; i < 3; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().x += 8;
    }
    for (int i = 0; i < 3; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().y += 10;
    }
    for (int i = 0; i < 11; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().x -= 8;
    }
    for (int i = 0; i < 2; ++i) {
        pointRects.push_back(pointRects.back());
        pointRects.back().y -= 10;
    }
    int currentPointRectsSize = pointRects.size();
    for (int i = 0; i < currentPointRectsSize; ++i) {
        pointRects.push_back(pointRects[i]);
        pointRects.back().x = windowWidth - pointRects.back().x;
    }
    std::vector<SDL_Rect> pointRectsForGhostsMovement = pointRects;
    std::vector<Entity> ghosts;
    ghosts.push_back(p);
    ghosts.back().r.x = 127;
    ghosts.back().r.y = 139;
    ghosts.push_back(ghosts.back());
    ghosts.back().r.x -= ghosts.back().r.w;
    ghosts.push_back(ghosts.back());
    ghosts.back().r.x -= ghosts.back().r.w;
    for (int i = 0; i < ghosts.size(); ++i) {
        ghosts[i].destinationPointR = pointRectsForGhostsMovement[random(0, pointRectsForGhostsMovement.size() - 2)];
    }
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                running = false;
                // NOTE: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                SDL_RenderSetScale(renderer, event.window.data1 / (float)windowWidth, event.window.data2 / (float)windowHeight);
            }
            if (event.type == SDL_KEYDOWN) {
                keys[event.key.keysym.scancode] = true;
            }
            if (event.type == SDL_KEYUP) {
                keys[event.key.keysym.scancode] = false;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                buttons[event.button.button] = true;
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                buttons[event.button.button] = false;
            }
            if (event.type == SDL_MOUSEMOTION) {
                float scaleX, scaleY;
                SDL_RenderGetScale(renderer, &scaleX, &scaleY);
                mousePos.x = event.motion.x / scaleX;
                mousePos.y = event.motion.y / scaleY;
                realMousePos.x = event.motion.x;
                realMousePos.y = event.motion.y;
            }
        }
#if 1 // PLAYER_MOTION
        p.dx = 0;
        p.dy = 0;
        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
            p.dx = -1;
            p.d = Direction::Left;
        }
        if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
            p.dx = 1;
            p.d = Direction::Right;
        }
        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
            p.dy = -1;
            p.d = Direction::Up;
        }
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
            p.dy = 1;
            p.d = Direction::Down;
        }
        SDL_Rect currpr = p.r;
        p.r.x += p.dx * PLAYER_SPEED;
        p.r.y += p.dy * PLAYER_SPEED;
        p.r.x = std::clamp(p.r.x, 0, windowWidth - p.r.w);
        p.r.y = std::clamp(p.r.y, 0, windowHeight - p.r.h);
        bool corrected = false;
        for (Tile& t : map) {
            SDL_Rect xr = currpr;
            xr.x += p.dx * PLAYER_SPEED;
            if (t.c == TILE_WALL_COLOR && SDL_HasIntersection(&xr, &t.r)) {
                if (p.dx == -1) {
                    p.r.x = t.r.x + t.r.w;
                    corrected = true;
                }
                if (p.dx == 1) {
                    p.r.x = t.r.x - p.r.w;
                    corrected = true;
                }
            }
            SDL_Rect yr = currpr;
            yr.y += p.dy * PLAYER_SPEED;
            if (t.c == TILE_WALL_COLOR && SDL_HasIntersection(&yr, &t.r)) {
                if (p.dy == -1) {
                    p.r.y = t.r.y + t.r.h;
                    corrected = true;
                }
                if (p.dy == 1) {
                    p.r.y = t.r.y - p.r.h;
                    corrected = true;
                }
            }
        }
        for (Tile& t : map) {
            SDL_Rect xyr = currpr;
            xyr.x += p.dx * PLAYER_SPEED;
            xyr.y += p.dy * PLAYER_SPEED;
            if (t.c == TILE_WALL_COLOR && SDL_HasIntersection(&xyr, &t.r) && !corrected) {
                if (wantedLeftButWasNotAble && p.dx == -1) {
                    if (p.dy == -1) {
                        p.r.y = t.r.y + t.r.h;
                    }
                    if (p.dy == 1) {
                        p.r.y = t.r.y - p.r.h;
                    }
                }
                else if (wantedRightButWasNotAble && p.dx == 1) {
                    if (p.dy == -1) {
                        p.r.y = t.r.y + t.r.h;
                    }
                    if (p.dy == 1) {
                        p.r.y = t.r.y - p.r.h;
                    }
                }
                else if (wantedUpButWasNotAble && p.dy == -1) {
                    if (p.dx == -1) {
                        p.r.x = t.r.x + t.r.w;
                    }
                    if (p.dx == 1) {
                        p.r.x = t.r.x - p.r.w;
                    }
                }
                else if (wantedDownButWasNotAble && p.dy == 1) {
                    if (p.dx == -1) {
                        p.r.x = t.r.x + t.r.w;
                    }
                    if (p.dx == 1) {
                        p.r.x = t.r.x - p.r.w;
                    }
                }
                else {
                    if (p.dx == 1) {
                        if (p.dy == -1) {
                            p.r.y = t.r.y + t.r.h;
                        }
                        if (p.dy == 1) {
                            p.r.y = t.r.y - p.r.h;
                        }
                    }
                    else if (p.dy == 1) {
                        if (p.dx == -1) {
                            p.r.x = t.r.x + t.r.w;
                        }
                        if (p.dx == 1) {
                            p.r.x = t.r.x - p.r.w;
                        }
                    }
                    else if (p.dx == -1) {
                        if (p.dy == -1) {
                            p.r.y = t.r.y + t.r.h;
                        }
                        if (p.dy == 1) {
                            p.r.y = t.r.y - p.r.h;
                        }
                    }
                    else if (p.dy == -1) {
                        if (p.dx == -1) {
                            p.r.x = t.r.x + t.r.w;
                        }
                        if (p.dx == 1) {
                            p.r.x = t.r.x - p.r.w;
                        }
                    }
                }
            }
        }
        wantedLeftButWasNotAble = wantedRightButWasNotAble = wantedUpButWasNotAble = wantedDownButWasNotAble = false;
        for (Tile& t : map) {
            SDL_Rect xr = currpr;
            xr.x += p.dx * PLAYER_SPEED;
            if (t.c == TILE_WALL_COLOR && SDL_HasIntersection(&xr, &t.r)) {
                if (p.dx == -1) {
                    wantedLeftButWasNotAble = true;
                }
                if (p.dx == 1) {
                    wantedRightButWasNotAble = true;
                }
            }
            SDL_Rect yr = currpr;
            yr.y += p.dy * PLAYER_SPEED;
            if (t.c == TILE_WALL_COLOR && SDL_HasIntersection(&yr, &t.r)) {
                if (p.dy == -1) {
                    wantedUpButWasNotAble = true;
                }
                if (p.dy == 1) {
                    wantedDownButWasNotAble = true;
                }
            }
        }
#endif
#if 1 // GHOSTS_MOTION
      // NOTE: Ghosts motion can work like they go to pointRectsForGhostsMovement because the player needs to collect points.
      // TODO: Fix it so that it won't go through walls.
      // TODO: Use ghosts.size() below instead of 1.
        for (int i = 0; i < 0; ++i) {
            Node startNode;
            startNode.r = ghosts[i].r;
            startNode.h = startNode.diagonalDistance(ghosts[i].destinationPointR);
            std::vector<Node> openNodes;
            std::vector<Node> closedNodes;
            openNodes.push_back(startNode);
            while (!openNodes.empty()) {
                int lowestFScoreIndex = openNodes.size() - 1;
                for (int i = 0; i < openNodes.size() - 1; ++i) {
                    if (openNodes[i].getF() < openNodes[lowestFScoreIndex].getF()) {
                        lowestFScoreIndex = i;
                    }
                }
                Node curr = openNodes[lowestFScoreIndex];
                openNodes.erase(openNodes.begin() + lowestFScoreIndex);
                closedNodes.push_back(curr);
                if (SDL_HasIntersection(&curr.r, &ghosts[i].destinationPointR)) {
                    // NOTE: Path has been found.
                    Node* dstNode = &curr;
                    while (dstNode->parentIndexInClosedNodes != -1 && closedNodes[dstNode->parentIndexInClosedNodes].parentIndexInClosedNodes != -1) {
                        dstNode = &closedNodes[dstNode->parentIndexInClosedNodes];
                    }
                    ghosts[i].r = dstNode->r;
                    break;
                }
                std::vector<Node> adj(8);
                for (Node& n : adj) {
                    n.r = closedNodes.back().r;
                }
                adj[0].r.x += -TILE_SIZE;
                adj[1].r.x += TILE_SIZE;
                adj[2].r.y += -TILE_SIZE;
                adj[3].r.y += TILE_SIZE;
                adj[4].r.x += -TILE_SIZE;
                adj[4].r.y += -TILE_SIZE;
                adj[5].r.x += TILE_SIZE;
                adj[5].r.y += -TILE_SIZE;
                adj[6].r.x += -TILE_SIZE;
                adj[6].r.y += TILE_SIZE;
                adj[7].r.x += TILE_SIZE;
                adj[7].r.y += TILE_SIZE;
                for (Node& n : adj) {
                    bool ok = false;
                    for (Tile& t : map) {
                        if (SDL_HasIntersection(&n.r, &t.r) && t.c != TILE_WALL_COLOR && std::find(closedNodes.begin(), closedNodes.end(), n) == closedNodes.end()) {
                            ok = true;
                        }
                    }
                    if (ok) {
                        // TODO: Should I calc n.g somewhere before?
                        int newMovementCostToN = curr.g + curr.diagonalDistance(n.r);
                        if (newMovementCostToN < n.g || std::find(openNodes.begin(), openNodes.end(), n) == openNodes.end()) {
                            n.g = newMovementCostToN;
                            n.h = n.diagonalDistance(ghosts[i].destinationPointR);
                            for (int i = 0; i < closedNodes.size(); ++i) {
                                if (closedNodes[i] == curr) {
                                    n.parentIndexInClosedNodes = i;
                                    break;
                                }
                            }
                            if (std::find(openNodes.begin(), openNodes.end(), n) == openNodes.end()) {
                                openNodes.push_back(n);
                            }
                        }
                    }
                }
            }
        }
#endif
        for (int i = 0; i < ghosts.size(); ++i) {
            if (SDL_HasIntersection(&ghosts[i].r, &ghosts[i].destinationPointR)) {
                ghosts[i].destinationPointR = pointRectsForGhostsMovement[random(0, pointRectsForGhostsMovement.size() - 2)];
            }
        }
        for (int i = 0; i < pointRects.size(); ++i) {
            if (SDL_HasIntersection(&p.r, &pointRects[i])) {
                pointRects.erase(pointRects.begin() + i--);
            }
        }
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        io.MousePos.x = mousePos.x;
        io.MousePos.y = mousePos.y;
        ImGui::NewFrame();
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderCopyF(renderer, mapT, 0, 0);
        for (int i = 0; i < pointRects.size(); ++i) {
            if (pointRects[i].x == 62 && pointRects[i].y == 28 || pointRects[i].x == 38 && pointRects[i].y == 268 || pointRects[i].x == 194 && pointRects[i].y == 88 || pointRects[i].x == 186 && pointRects[i].y == 208) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
                SDL_RenderFillCircle(renderer, pointRects[i].x + pointRects[i].w / 2, pointRects[i].y + pointRects[i].h / 2, 3);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
                SDL_RenderFillRect(renderer, &pointRects[i]);
            }
        }
        SDL_RenderCopy(renderer, playerT, 0, &p.r);
        for (int i = 0; i < ghosts.size(); ++i) {
            if (i == 0) {
                SDL_RenderCopy(renderer, orangeGhost, 0, &ghosts[i].r);
            }
            else if (i == 1) {
                SDL_RenderCopy(renderer, blueGhost, 0, &ghosts[i].r);
            }
            else if (i == 2) {
                SDL_RenderCopy(renderer, pinkGhost, 0, &ghosts[i].r);
            }
        }
        SDL_RenderPresent(renderer);
    }
    // NOTE: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
}

int main(int argc, char* argv[])
{
    run();
    return 0;
}