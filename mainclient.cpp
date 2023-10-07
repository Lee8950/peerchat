#include <iostream>
#include <vector>
#include <deque>
#include <exception>
#include <string>
#include <thread>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include "date.h"
#if defined(__WIN32__)
#include <winsock2.h>
#elif defined(__linux__)
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/time.h>
#endif
#include <GL/gl.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"
#include "SDL2/SDL_opengles2.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

std::string CurrentDate()
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char buf[16] = { 0 };
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&now));

    return std::string(buf);
}

/**
 * @note This file is NOT the centralized server's code.
*/
void update(int socketdescriptor, char *buf, int size, std::vector<std::string> &logger)
{
    while(1)
    {
        ::recv(socketdescriptor, buf, size, 0);
        logger.push_back(date::format("%F %T", std::chrono::system_clock::now()) + std::string(" Remote: ") + std::string(buf));
        memset(buf, 0, BUFSIZ);
    }
}

int main(int argc, char **argv)
{
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    char sendbuf[BUFSIZ];
    char recvbuf[BUFSIZ];
    std::vector<std::string> chatHistory;
    sockaddr_in server_sockaddr;
    server_sockaddr.sin_port = htons(12001);
    server_sockaddr.sin_family = AF_INET;
    if(argc < 2)
        server_sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    else
        server_sockaddr.sin_addr.s_addr = inet_addr(argv[1]);
    if(::connect(sd, reinterpret_cast<sockaddr*>(&server_sockaddr), sizeof(server_sockaddr)) < 0)
        throw std::runtime_error("failed to connect");
    std::thread receiving(update, sd, recvbuf, BUFSIZ, std::ref(chatHistory));
    receiving.detach();

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("ecl_Chat", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    // Main loop
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        //ImGui::ShowDemoWindow();
        {
            ImGui::Begin("Messages");
            ImGui::InputText("Chat", sendbuf, BUFSIZ);
            ImGui::SameLine();
            if(ImGui::Button("Send"))
            {
                ::send(sd, sendbuf, BUFSIZ, 0);
                chatHistory.push_back(date::format("%F %T", std::chrono::system_clock::now()) + std::string(" You: ") + std::string(sendbuf));
                memset(sendbuf, 0, BUFSIZ);
            }
            for(int i = 0; i < chatHistory.size(); i++)
            {
                ImGui::Text("%s", chatHistory[i].c_str());
            }
            ImGui::End();
        }
        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
    
    return 0;
}