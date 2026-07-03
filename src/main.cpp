#include <iostream>
#include <math.h>
#include <random>
#include <thread>
#include <iomanip>

// OpenGL helpers
#include "VertexBuffer/VertexBuffer.hpp"
#include "IndexBuffer/IndexBuffer.hpp"
#include "VertexArray/VertexArray.hpp"
#include "Shader/Shader.hpp"
#include "VertexBufferLayout/VertexBufferLayout.hpp"
#include "Triangle/Triangle.hpp"

// project-specific includes go here
// #include "Utilities/Utilities.hpp"

SDL_Window* window;
SDL_GLContext gl_context;

const uint8_t num_threads { static_cast<uint8_t>(std::thread::hardware_concurrency()) };

const bool renderGraphics { true };

bool is_running;
bool paused { true };
bool step { false };

const float width { 768.0f };
const float height { 768.0f };
const float triangle_scale { 1 / 16.0f };

// const double DT { 0.025 };

// project-specific settings
// unsigned int res { 20 / 2 };

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

    // nice to have here
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1); // Enable multisampling
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // 4x MSAA (Sweet spot for quality/performance)
}

void evolveTime(SDL_Event& event)
{
    if (event.type == SDL_KEYDOWN)
    {
        if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
        {
            paused = paused ? false : true;
        }
        else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
        {
            step = true;
        }
        // else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT)
        // {
        //     GLOBAL_TIME < DT ? GLOBAL_TIME = 0.0 : GLOBAL_TIME -= DT; // prevent negative time
        // }
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

int main()
{
    if (renderGraphics)
    {
        if(SDL_Init(SDL_INIT_EVERYTHING)==0)
        {
            std::cout<<"SDL2 initialized successfully."<<std::endl;
            set_sdl_gl_attributes();
            
            window = SDL_CreateWindow("Window Title", 0.0f, 0.0f, static_cast<int>(width), static_cast<int>(height), SDL_WINDOW_OPENGL);
            gl_context = SDL_GL_CreateContext(window);
            SDL_GL_SetSwapInterval(1);

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

            GLCall(glEnable(GL_MULTISAMPLE));

            printKey();

            // buffer stuff and initialization happens here

            std::vector<Triangle> tris;
            unsigned int num_objs { 1 };
            tris.reserve(num_objs);
            std::vector<glm::mat4> modelMatrices;
            modelMatrices.reserve(num_objs);
            glm::mat4 instMod = glm::mat4(1.0f);
            instMod = glm::translate(instMod, glm::vec3(width/2.0f, height/2.0f, 0.0f));
            instMod = glm::rotate(instMod, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            modelMatrices.emplace_back(instMod);
            
            // create triangles
            for (unsigned int i = 0; i < num_objs; ++i)
            {
                tris.emplace_back(Triangle(triangle_scale * glm::vec2(width/2.0f, 0.0f), triangle_scale * glm::vec2(width/4.0f, height), glm::fvec4(1.0f)));
            }

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

            VertexBuffer instanceVBO(modelMatrices.data(), static_cast<unsigned int>(modelMatrices.size() * sizeof(glm::mat4)), GL_DYNAMIC_DRAW);
            tri_VAO.addInstancedBuffer(instanceVBO, 2);

            // constructor automatically binds buffer
            IndexBuffer tri_IBO(triangles.m_indices.data(), static_cast<unsigned int>(triangles.m_indices.size()));

            // set up MPV matrix
            glm::mat4 proj { glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f) };
            glm::mat4 view { glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0)) };
            glm::mat4 model { glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) };
            glm::mat4 MVP { proj * view * model };

            // tri_shader stuff happens here
            Shader tri_shader("/Users/max/OpenGL_Framework/res/shaders", "triangle_");
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

            while(is_running)
            {
                while(SDL_PollEvent(&event))
                {
                    if( event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) )
                    {
                        is_running = false;
                    }

                    evolveTime(event);
                }

                // evolve time if unpaused
                if (not paused)
                {
                    // evolve(DT)
                    // func wrapper for "write val to file"
                    // Utilities::addLine("path/to/file/name", val);
                }
                if (step)
                {
                    // evolve(DT)
                    step = false;
                }

                // updating and rendering stuff happens here
                
                // update buffers (circle position, color, alpha)
                // triangles.updateColors(values);
                // tri_VBO.updateBuffer(triangles.m_vertices.data());

                renderer.clear();

                // draw objects
                renderer.drawTriangles(tri_VAO, tri_IBO, tri_shader, static_cast<int>(num_objs));

                SDL_GL_SwapWindow(window);
            }

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
        for (int i = 0; i < 112; ++i)
        {
            std::cout << '\n' << "Iteration " << i << '\n' << '\n';

            std::vector<std::thread> workers;
            workers.reserve((num_threads));
            for (std::size_t thread_ID = 0; thread_ID < num_threads; ++thread_ID)
            {
                // uint32_t base_seed = rd();
                // workers.emplace_back(&Utilities::function_name, thread_ID, base_seed, width, height, res, DT, i);
            }

            for (auto& thread : workers)
            {
                thread.join();
            }
        }
    }
}
