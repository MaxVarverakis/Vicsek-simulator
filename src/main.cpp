#include <iostream>
#include <math.h>
#include <random>
#include <thread>
#include <iomanip>
#include <chrono>

// OpenGL helpers
#include "VertexBuffer/VertexBuffer.hpp"
#include "IndexBuffer/IndexBuffer.hpp"
#include "VertexArray/VertexArray.hpp"
#include "Shader/Shader.hpp"
#include "VertexBufferLayout/VertexBufferLayout.hpp"
#include "Triangle/Triangle.hpp"

// ImGui stuff here
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

// project-specific includes go here
#include "Swarm/Swarm.hpp"

SDL_Window* window;
SDL_GLContext gl_context;

const uint8_t num_threads { static_cast<uint8_t>(std::thread::hardware_concurrency()) };

const bool renderGraphics { true };

bool is_running;
bool paused { true };
bool step { false };

const float world_scale { 1.0f };

const float cellSize { 50.0f / world_scale }; // side length of square cell
const float targetWidth { 1440.0f / world_scale };
const float targetHeight { 846.0f / world_scale };


// project-specific settings
// const float shape_scale { 1 / 64.0f };
const float shape_scale { 5.0f / world_scale };
const unsigned int numObjs { 5000 };
const unsigned int neighborCount { 5 };
const float DT { 0.025f };
float v_magnitude { 200.0f };
float noiseScale { 0.2f };
unsigned int terminationCount = 300;
bool color { false };
bool ui_collapsed { true };

static const std::vector<glm::vec4> defaultColors(numObjs, glm::vec4(1.0f));

// initialize random
std::random_device rd;


void set_sdl_gl_attributes()
{
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // nice to have here (slows things down!)
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1); // Enable multisampling
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // 4x MSAA (Sweet spot for quality/performance)
}

void evolveTime(SDL_Event& event)
{
    if (event.type == SDL_KEYDOWN)
    {
        switch (event.key.keysym.sym)
        {
            case SDLK_ESCAPE:
                is_running = false;
                break;
            case SDLK_SPACE:
                paused = !paused;
                break;
            case SDLK_RIGHT:
                step = true;
                break;
            case SDLK_c:
                color = !color;
                break;
            case SDLK_m:
                ui_collapsed = ! ui_collapsed;
                break;
            default:
                break;
        }
    }
}

void printKey()
{
    std::cout << '\n' << "###################################" << '\n';
    std::cout << "KEY" << '\n';
    std::cout << "Spacebar" << '\t' << "Play/pause" << '\n';
    std::cout << "Right arrow" << '\t' << "Time forward" << '\n';
    // std::cout << "Left arrow" << '\t' << "Time reverse" << '\n';
    // std::cout << "Enter/Return" << '\t' << "Reset particles" << '\n';
    std::cout << "###################################" << '\n' << '\n';
}

void imguiWindow(ImGuiIO& io, Swarm& swarm)
{
    ImGui::SetNextWindowCollapsed(ui_collapsed, ImGuiCond_Always);

    ImGui::Begin("Swarm Control Center");

    // keep bool synced with ui state
    ui_collapsed = ImGui::IsWindowCollapsed();

    // Simulation Play/Pause State
    if (paused) {
        if (ImGui::Button("Resume Simulation", ImVec2(150, 0))) { paused = false; }
    } else {
        if (ImGui::Button("Pause Simulation", ImVec2(150, 0))) { paused = true; }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Step Forward")) { step = true; }
    
    ImGui::SameLine();
    if (ImGui::Button("Quit")) { is_running = false; }

    ImGui::Separator();

    // Live Parameter Tuning
    // Note: We expose &noiseScale directly since it controls the swarm logic
    ImGui::Text("Physics Parameters:");
    ImGui::SliderFloat("Noise Scale (Eta)", &noiseScale, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat("Velocity Mag (|v|)", &v_magnitude, 0.0f, 1000.0f, "%.0f");
    
    // modify agent colors
    ImGui::Checkbox("Angle-based colors", &color);
    
    // swarm info
    ImGui::Text("Particle Count: %u", numObjs);

    ImGui::Separator();

    // Real-Time Performance Overlays
    ImGui::Text("Application Metrics:");
    ImGui::Text("Frame Rate: %.1f FPS", io.Framerate);

    ImGui::Separator();

    // velocity order parameter
    // Maintain a static rolling buffer of historical data (e.g., last 200 frames)
    const int HISTORY_SIZE = 200;
    static float phi_history[HISTORY_SIZE] = { 0.0f };
    static int head = 0;

    if (!paused)
    {
        phi_history[head] = swarm.order_param;
        head = (head + 1) % HISTORY_SIZE;
    }

    // Render the scrolling line graph
    // Arguments: Label, data array, array size, values offset, overlay text, min value, max value, graph size
    ImGui::PlotLines("##PhiGraph", phi_history, HISTORY_SIZE, head, "Global Order (Phi)", 0.0f, 1.0f, ImVec2(0, 100));
    
    ImGui::End();
}

int main()
{
    // Utilities::addLine("/Users/max/UCLA/Research/Codes/Vicsek/data/v_order_param", "noise", "order param");

    if (renderGraphics)
    {
        if(SDL_Init(SDL_INIT_EVERYTHING)==0)
        {
            std::cout<<"SDL2 initialized successfully."<<std::endl;
            set_sdl_gl_attributes();
            
            // initialize swarm up here so that we can use the SHG-adjusted width/height to fit the window perfectly
            // specifying the number of agents automatically generates random particles
            Swarm swarm(cellSize, targetWidth, targetHeight, shape_scale, rd(), noiseScale, neighborCount, v_magnitude, numObjs);
            const float width = swarm.x_max;
            const float height = swarm.y_max;

            window = SDL_CreateWindow("Window Title", 0.0f, 0.0f, static_cast<int>(width), static_cast<int>(height), SDL_WINDOW_OPENGL);
            gl_context = SDL_GL_CreateContext(window);
            SDL_GL_SetSwapInterval(1);

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

            // Setup Platform/Renderer backends
            ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
            ImGui_ImplOpenGL3_Init();

            if(glewInit() == GLEW_OK)
            {
                std::cout << "GLEW initialization successful" << std::endl;
            }
            else
            {
                std::cout << "GLEW initialization failed" << std::endl;
                return -1;
            }

            GLCall(glEnable(GL_BLEND));
            GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

            // GLCall(glEnable(GL_MULTISAMPLE));

            printKey();

            // buffer stuff and initialization happens here
            std::vector<Triangle> tris;
            tris.reserve(numObjs);
            
            // create triangle (only need one since instance drawing!)
            tris.emplace_back(Triangle(glm::vec2(0.0f, 2.0f), glm::vec2(1.7321f, -1.0f), glm::vec2(-1.7321f, -1.0f), glm::fvec4(1.0f)));
            // tris.emplace_back(Triangle(glm::vec2(0.0f, 0.0f), shape_scale * glm::vec2(width/2.0f, 0.0f), shape_scale * glm::vec2(width/4.0f, height), glm::fvec4(1.0f)));

            // prepare triangles for graphics
            Triangles triangles(tris);

            VertexArray tri_VAO;

            // constructor automatically binds buffer
            VertexBuffer tri_VBO(triangles.m_vertices.data(), static_cast<unsigned int>(triangles.m_vertices.size() * sizeof(float)), GL_STATIC_DRAW);

            VertexBufferLayout tri_layout;
            for (int i = 0; i < 2; ++i)
            {
                tri_layout.push<float>(Triangle::layout_descriptor[i]);
            }
            tri_VAO.addBuffer(tri_VBO, tri_layout);

            VertexBuffer instanceVBO(swarm.modelMatrices.data(), static_cast<unsigned int>(swarm.modelMatrices.size() * sizeof(glm::mat4)), GL_DYNAMIC_DRAW);
            tri_VAO.addInstancedBuffer(instanceVBO, 2, 16);

            swarm.colors.resize(numObjs, glm::vec4(1.0f));
            VertexBuffer colorInstanceVBO(swarm.colors.data(), static_cast<unsigned int>(swarm.colors.size() * sizeof(glm::vec4)), GL_DYNAMIC_DRAW);
            // add to slot 6 since the instance mat4's take up 4 slots (2, 3, 4, 5)
            tri_VAO.addInstancedBuffer(colorInstanceVBO, 6, 4);

            // constructor automatically binds buffer
            IndexBuffer tri_IBO(triangles.m_indices.data(), static_cast<unsigned int>(triangles.m_indices.size()));

            // set up MPV matrix
            glm::mat4 proj { glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f) };
            glm::mat4 view { glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0)) };
            glm::mat4 model { glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) };
            glm::mat4 MVP { proj * view * model };

            // tri_shader stuff happens here
            // Shader tri_shader("/Users/max/OpenGL_Framework/res/shaders", "triangle_");
            Shader tri_shader("/Users/max/UCLA/Research/Codes/Vicsek/shaders", "circle_cheat_");
            tri_shader.bind();
            tri_shader.setUniformMatrix4fv("u_MVP", MVP);

            tri_VBO.unbind();
            tri_VAO.unbind();
            tri_IBO.unbind();
            tri_shader.unbind();

            Renderer renderer;

            // Main loop
            SDL_Event event;
            is_running = true;

            // Uint64 lastTime = SDL_GetPerformanceCounter();
            // GLuint query;
            // glGenQueries(1, &query);

            while(is_running)
            {
                // Calculate dynamic dt at the very start of the frame
                // Uint64 currentTime = SDL_GetPerformanceCounter();
                // float dt = static_cast<float>(currentTime - lastTime) / static_cast<float>(SDL_GetPerformanceFrequency());
                // lastTime = currentTime;
                // if (dt > 0.1f) dt = 0.1f;

                while(SDL_PollEvent(&event))
                {
                    // (Where your code calls SDL_PollEvent())
                    ImGui_ImplSDL2_ProcessEvent(&event); // Forward your event to backend

                    if (event.type == SDL_QUIT)
                    {
                        is_running = false;
                    }
                    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                    {
                        is_running = false;
                    }

                    evolveTime(event);
                }

                // (After event loop)
                // Start the Dear ImGui frame
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplSDL2_NewFrame();
                ImGui::NewFrame();
                imguiWindow(io, swarm);
                
                // evolve time if unpaused
                if (not paused)
                {
                    swarm.velocity = v_magnitude;
                    swarm.noiseScale = noiseScale;
                    swarm.updateParticles(DT);
                    // noiseScale = std::max<float>(noiseScale - DT / 64.0f, 0.0f);
                    // swarm.noiseScale = noiseScale;
                    // // std::cout << noiseScale << '\n';
                    
                    // if (noiseScale <= 0.0f)
                    // {
                    //     --terminationCount;
                    // }
                    
                    // // func wrapper for "write vals to file"
                    // Utilities::addLine("/Users/max/UCLA/Research/Codes/Vicsek/data/v_order_param_limited", noiseScale, swarm.order_param);
                    
                    // if (terminationCount <= 0) { break; }
                }
                if (step)
                {
                    swarm.velocity = v_magnitude;
                    swarm.noiseScale = noiseScale;
                    swarm.updateParticles(DT);
                    step = false;
                }
                
                // always update colors even if paused
                swarm.colors_bool = color;

                // updating and rendering stuff happens here
                
                // auto r0 = std::chrono::high_resolution_clock::now();
                // update buffers (position, color, alpha)
                if (color)
                {
                    // triangles.udpateColors(swarm.colors);
                    colorInstanceVBO.updateBuffer(swarm.colors.data());
                }
                else
                {
                    colorInstanceVBO.updateBuffer(defaultColors.data());
                }

                // auto r1 = std::chrono::high_resolution_clock::now();
                instanceVBO.updateBuffer(swarm.modelMatrices.data());

                // auto r2 = std::chrono::high_resolution_clock::now();
                renderer.clear();

                // draw objects
                // auto r3 = std::chrono::high_resolution_clock::now();

                // glBeginQuery(GL_SAMPLES_PASSED, query);
                // glBeginQuery(GL_TIME_ELAPSED, query);
                renderer.drawTriangles(tri_VAO, tri_IBO, tri_shader, static_cast<int>(numObjs));
                // glEndQuery(GL_TIME_ELAPSED);
                // GLuint64 ns;
                // glGetQueryObjectui64v(query,
                //                     GL_QUERY_RESULT,
                //                     &ns);

                // std::cout
                // << ns / 1000000.0
                // << '\n';
                // glEndQuery(GL_SAMPLES_PASSED);

                // GLuint samples;
                // glGetQueryObjectuiv(query,
                //                     GL_QUERY_RESULT,
                //                     &samples);
                // std::cout << samples << std::endl;

                // auto t0 = std::chrono::high_resolution_clock::now();

                // glFinish();

                // auto t1 = std::chrono::high_resolution_clock::now();

                // std::cout
                //     << "glFinish: "
                //     << std::chrono::duration<double, std::milli>(t1 - t0).count()
                //     << '\n';

                // Dear ImGui Rendering
                // (Your code clears your framebuffer, renders your other stuff etc.)
                // auto r4 = std::chrono::high_resolution_clock::now();
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                // (Your code calls SDL_GL_SwapWindow() etc.)

                // auto r5 = std::chrono::high_resolution_clock::now();
                SDL_GL_SwapWindow(window);
                // auto r6 = std::chrono::high_resolution_clock::now();

                // if (!paused)
                // {
                //     std::cout << '\n';
                //     std::cout << "color upload" << '\t' << std::chrono::duration_cast<std::chrono::microseconds>(r1 - r0).count() / 1000.0 << " ms" << '\n';
                //     std::cout << "matrix upload" << '\t' << std::chrono::duration_cast<std::chrono::microseconds>(r2 - r1).count() / 1000.0 << " ms" << '\n';
                //     std::cout << "render clear" << '\t' << std::chrono::duration_cast<std::chrono::microseconds>(r3 - r2).count() / 1000.0 << " ms" << '\n';
                //     std::cout << "draw" << '\t' << std::chrono::duration_cast<std::chrono::microseconds>(r4 - r3).count() / 1000.0 << " ms" << '\n';
                //     std::cout << "imgui" << '\t' << std::chrono::duration_cast<std::chrono::microseconds>(r5 - r4).count() / 1000.0 << " ms" << '\n';
                //     std::cout << "swap" << '\t' << std::chrono::duration_cast<std::chrono::microseconds>(r6 - r5).count() / 1000.0 << " ms" << '\n';
                // }
            }

            // glDeleteQueries(1,&query);

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
            SDL_Quit();
        }
        else
        {
            std::cout<<"SDL2 initialization failed."<<std::endl;
            return -1;
        }

        return 0;
    }
    else
    {
        // parallel compute no graphics

        // TODO: thread pooling
        std::cout << "Thread count: " << static_cast<int>(num_threads) << '\n';

        std::vector<unsigned int> NNList{1, 2, 4, 8, 16, 24};
        std::vector<std::thread> workers;
        workers.reserve((NNList.size()));

        for (unsigned int i = 0; i < NNList.size(); ++i)
        {
            workers.emplace_back(&Utilities::parallelSims, cellSize, targetWidth, targetHeight, shape_scale, rd(), noiseScale, NNList[i], v_magnitude, numObjs, DT);
        }
        
        for (auto& thread : workers)
        {
            thread.join();
        }
    }
}
