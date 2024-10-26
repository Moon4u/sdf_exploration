
#include <algorithm> // std::clamp

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_vulkan.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <implot.h>


#define SCREEN_WIDTH  1920.0
#define SCREEN_HEIGHT 1080.0

#define TEXTURE_WIDTH  800.0
#define TEXTURE_HEIGHT 450.0

#define PLOT_RESOLUTION_WIDTH  200
#define PLOT_RESOLUTION_HEIGHT 225

static_assert((int)(TEXTURE_WIDTH)  % PLOT_RESOLUTION_WIDTH  == 0, "TEXTURE_WIDTH must be multiple of PLOT_RESOLUTION_WIDTH");
static_assert((int)(TEXTURE_HEIGHT) % PLOT_RESOLUTION_HEIGHT == 0, "TEXTURE_HEIGHT must be multiple of PLOT_RESOLUTION_HEIGHT");

constexpr int PLOT_WIDTH  = (int)(TEXTURE_WIDTH ) / PLOT_RESOLUTION_WIDTH;
constexpr int PLOT_HEIGHT = (int)(TEXTURE_HEIGHT) / PLOT_RESOLUTION_HEIGHT;

#define RADIUS 0.5


float mix(float a, float b, float t) {
    //return a + t * (b - a);
    return a * (1 - t) + b * t;
}

float smoothstep(float edge0, float edge1, float x) {
    float temp = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    // Evaluate polynomial
    return temp * temp * (3 - 2 * temp);
}

void sdgCircle(float px, float py, float* out)
{
    float l = sqrt(px * px + py * py);
    out[0] = l - RADIUS;
    out[1] = px / l;
    out[2] = py / l;
}

void screen_space_coords(int x, int y, float *px, float *py)
{
    float cx = x;
    float cy = TEXTURE_HEIGHT - y;

    *px = (2.0f * cx - (float)(TEXTURE_WIDTH))  / (float)(TEXTURE_HEIGHT) + 0.0001f;
    *py = (2.0f * cy - (float)(TEXTURE_HEIGHT)) / (float)(TEXTURE_HEIGHT) + 0.0001f;
}

void pixel_shader(int x, int y, float* rgb)
{
    float px, py;
    screen_space_coords(x, y, &px, &py);

    float sdg[3];
    sdgCircle(px, py, sdg);

    float d  = sdg[0];
    float gx = sdg[1];
    float gy = sdg[2];

    float r, g, b;
    if (d>0.0f)
    {
        r = 0.9f;
        g = 0.6f;
        b = 0.3f;
    }
    else
    {
        r = 0.4f;
        g = 0.7f;
        b = 0.85f;
    }

    r *= 1.0f + 0.5f * gx;
    g *= 1.0f + 0.5f * gy;

    r *= 1.0f - 0.5f * expf(-16.0f * fabs(d));
    g *= 1.0f - 0.5f * expf(-16.0f * fabs(d));
    b *= 1.0f - 0.5f * expf(-16.0f * fabs(d));

    r *= 0.9f + 0.1f * cosf(150.0f * d);
    g *= 0.9f + 0.1f * cosf(150.0f * d);
    b *= 0.9f + 0.1f * cosf(150.0f * d);

    r = mix(r, 1.0f, 1.0f - smoothstep(0.0f, 0.01f, fabs(d)));
    g = mix(g, 1.0f, 1.0f - smoothstep(0.0f, 0.01f, fabs(d)));
    b = mix(b, 1.0f, 1.0f - smoothstep(0.0f, 0.01f, fabs(d)));

    rgb[0] = std::clamp(r, 0.0f, 1.0f);
    rgb[1] = std::clamp(g, 0.0f, 1.0f);
    rgb[2] = std::clamp(b, 0.0f, 1.0f);
}

int main()
{
    float PLOT_LINE_LEN = sqrt(PLOT_WIDTH * PLOT_WIDTH + PLOT_HEIGHT * PLOT_HEIGHT);
    struct PlotData
    {
        float ox;
        float oy;
        float dx;
        float dy;
        float dist;
    };



    PlotData *plt_data = new PlotData[PLOT_RESOLUTION_HEIGHT * PLOT_RESOLUTION_WIDTH];

    float max_dist = std::numeric_limits<float>::min();
    float min_dist = std::numeric_limits<float>::max();

    for (int j = 0; j < PLOT_RESOLUTION_HEIGHT; ++j)
    {
        for(int i = 0; i < PLOT_RESOLUTION_WIDTH; ++i)
        {
            int index = i + j * PLOT_RESOLUTION_WIDTH;
            int x = i * PLOT_WIDTH;
            int y = j * PLOT_HEIGHT;

            float px, py;
            float out[3];
            screen_space_coords(x, y, &px, &py);
            sdgCircle(px, py, out);
            plt_data[index].ox   = x;
            plt_data[index].oy   = TEXTURE_HEIGHT - y;
            plt_data[index].dx   = out[1];
            plt_data[index].dy   = out[2];
            plt_data[index].dist = out[0];

            max_dist = fmax(out[0], max_dist);
            min_dist = fmin(out[0], min_dist);
        }
    }

    //float pow_min = min_dist;
    //float pow_max = max_dist;
    //for (int j = 0; j < PLOT_RESOLUTION_HEIGHT; ++j)
    //{
    //    for(int i = 0; i < PLOT_RESOLUTION_WIDTH; ++i)
    //    {
    //        int   index    = i + j * PLOT_RESOLUTION_WIDTH;
    //        float pow_dist = plt_data[index].dist;

    //        plt_data[index].dist = (pow_dist - pow_min) / (pow_max - pow_min);
    //    }
    //}


    SDL_InitFlags sdl_init_flags = 0;
    sdl_init_flags |= SDL_INIT_AUDIO;
    sdl_init_flags |= SDL_INIT_VIDEO;
    sdl_init_flags |= SDL_INIT_JOYSTICK;
    sdl_init_flags |= SDL_INIT_HAPTIC;
    sdl_init_flags |= SDL_INIT_GAMEPAD;
    sdl_init_flags |= SDL_INIT_EVENTS;
    sdl_init_flags |= SDL_INIT_SENSOR;
    sdl_init_flags |= SDL_INIT_CAMERA;

    int renderDrivers = SDL_GetNumRenderDrivers();
    for (int i = 0; i < renderDrivers; ++i)
    {
        printf("%d: %s\n", i, SDL_GetRenderDriver(i));
    }

    if (!SDL_Init(sdl_init_flags))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Could not initialize SDL", nullptr);
        return 1;
    }

    SDL_Window*   window   = SDL_CreateWindow("Test", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    if (window == nullptr)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Could not create a window", nullptr);
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);





    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);




    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);



    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, TEXTURE_WIDTH, TEXTURE_HEIGHT);
    SDL_SetRenderTarget(renderer, texture);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    float rgb[3];
    for (int x = 0; x < TEXTURE_WIDTH; ++x)
    {
        for (int y = 0; y < TEXTURE_HEIGHT; ++y)
        {
            pixel_shader(x, y, rgb);

            SDL_SetRenderDrawColor(renderer, rgb[0] * 255, rgb[1] * 255, rgb[2] * 255, 255);
            SDL_RenderPoint(renderer, x, y);
        }
    }
    SDL_SetRenderTarget(renderer, NULL);


    float  px, py;
    float  out[3];
    float  xy[2];
    bool   has_pressed = false;
    bool   has_debug_info = false;

    int debug_pixels[2]{};

    PlotData debug_plot;
    PlotData extra_debug_plot[5];

    while (true)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    return 0;

                case SDL_EVENT_MOUSE_MOTION:
                {
                    if (event.motion.state & 0x1)
                    {
                        has_pressed = true;
                        xy[0] = (event.motion.x / SCREEN_WIDTH ) * TEXTURE_WIDTH;
                        xy[1] = (event.motion.y / SCREEN_HEIGHT) * TEXTURE_HEIGHT;

                        screen_space_coords(xy[0], xy[1], &px, &py);
                        sdgCircle(px, py, out);

                        pixel_shader(xy[0], xy[1], rgb);
                    }
                    break;
                }

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                {
                    has_pressed = true;
                    xy[0] = (event.motion.x / SCREEN_WIDTH ) * TEXTURE_WIDTH;
                    xy[1] = (event.motion.y / SCREEN_HEIGHT) * TEXTURE_HEIGHT;

                    screen_space_coords(xy[0], xy[1], &px, &py);
                    sdgCircle(px, py, out);

                    pixel_shader(xy[0], xy[1], rgb);

                    break;
                }
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);

        if (has_pressed)
        {
            ImGui_ImplSDLRenderer3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();


            ImGui::Begin("Hello, world!");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::Text("Mouse position (%f, %f)", xy[0], xy[1]);
            ImGui::Text("px %f, py %f", px, py);
            ImGui::Text("dist %f, dir/x %f, dir/y %f", out[0], out[1], out[2]);
            ImGui::ColorEdit3("Pixel on the screen", rgb, ImGuiColorEditFlags_NoPicker);
            ImGui::InputInt2("Debug Coordinates", debug_pixels);
            if (ImGui::Button("Recompute (use breakpoint)"))
            {
                constexpr float small = 0.00001;
                pixel_shader(debug_pixels[0], debug_pixels[1], rgb);

                float px, py;
                float out[3];

                screen_space_coords(debug_pixels[0], debug_pixels[1], &px, &py);
                sdgCircle(px, py, out);
                debug_plot.ox   = debug_pixels[0];
                debug_plot.oy   = debug_pixels[1];
                debug_plot.dx   = out[1];
                debug_plot.dy   = out[2];
                debug_plot.dist = out[0];

                sdgCircle(px + small, py, out);
                extra_debug_plot[0].dx = out[1] - debug_plot.dx;
                extra_debug_plot[0].dy = out[2] - debug_plot.dy;

                sdgCircle(px, py + small, out);
                extra_debug_plot[1].dx = out[1] - debug_plot.dx;
                extra_debug_plot[1].dy = out[2] - debug_plot.dy;

                sdgCircle(px + small, py + small, out);
                extra_debug_plot[2].dx = out[1] - debug_plot.dx;
                extra_debug_plot[2].dy = out[2] - debug_plot.dy;

                extra_debug_plot[3].dx = out[1] - extra_debug_plot[0].dx;
                extra_debug_plot[3].dy = out[2] - extra_debug_plot[0].dy;

                extra_debug_plot[4].dx = out[1] - extra_debug_plot[1].dx;
                extra_debug_plot[4].dy = out[2] - extra_debug_plot[1].dy;

                has_debug_info  = true;
            }
            if (has_debug_info)
            {
                ImGui::Text("Pos (%f, %f) dir (%f, %f) dist %f", debug_plot.ox, debug_plot.oy, debug_plot.dx, debug_plot.dy, debug_plot.dist);
                for(int i = 0; i < 5; ++i)
                {
                    ImGui::Text("?%d, (%f %f)", i, extra_debug_plot[i].dx, extra_debug_plot[i].dy);
                }
            }



            ImGui::Begin("Plot");
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImPlot::SetNextAxisLimits(ImAxis_X1, 0, TEXTURE_WIDTH );
            ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, TEXTURE_HEIGHT);
            if (ImPlot::BeginPlot("Derivatives", ImVec2(TEXTURE_WIDTH * 2, TEXTURE_HEIGHT * 2)))
            {
                for (int j = 0; j < PLOT_RESOLUTION_HEIGHT; ++j)
                {
                    for(int i = 0; i < PLOT_RESOLUTION_WIDTH; ++i)
                    {
                        int index = i + j * PLOT_RESOLUTION_WIDTH;
                        PlotData& plt = plt_data[index];

                        float dirx = plt.ox + (plt.dx * plt.dist * PLOT_LINE_LEN);
                        float diry = plt.oy + (plt.dy * plt.dist * PLOT_LINE_LEN);

                        ImVec2 p1 = ImPlot::PlotToPixels(ImPlotPoint(plt.ox, plt.oy));
                        ImVec2 p2 = ImPlot::PlotToPixels(ImPlotPoint(dirx, diry));

                        draw_list->AddLine(p1, p2, IM_COL32(255, 0, 0, 255), 2.0f);

                        ImVec2 dir = ImVec2(p2.x - p1.x, p2.y - p1.y);
                        ImVec2 normal = ImVec2(-dir.y * 0.1f, dir.x * 0.1f);
                        ImVec2 arrowBase = ImVec2(p2.x - dir.x * 0.2f, p2.y - dir.y * 0.2f);
                        draw_list->AddLine(p2, ImVec2(arrowBase.x + normal.x, arrowBase.y + normal.y), IM_COL32(255, 0, 0, 255), 2.0f);
                        draw_list->AddLine(p2, ImVec2(arrowBase.x - normal.x, arrowBase.y - normal.y), IM_COL32(255, 0, 0, 255), 2.0f);

                    }
                }
                ImPlot::EndPlot();
            }
            ImGui::End();


            ImGui::End();
            ImGui::EndFrame();

            ImGui::Render();
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
