#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <exception>
#include <string>
#include <thread>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include "date.h"
#include "ed25519.hpp"
#include "protocal.hpp"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#if defined(__WIN32__)
#include <winsock2.h>
#elif defined(__linux__)
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
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

bool live = true;
constexpr int maxRetry = 3;
std::array<unsigned char, 32> serverPublicKey;
std::array<unsigned char, 64> serverPrivateKey;
std::unordered_map<std::string, std::array<unsigned char, 32>> registeredUsers;

void loadKey()
{
    std::fstream keyPairLoader;
    keyPairLoader.open("server.key", std::ios::in);
    /* Keys exist, just load */
    if(keyPairLoader.is_open())
    {
        
    }
    else /* Creating keys */
    {

    }
}

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
void update(int socketdescriptor, char *buf, int size, std::vector<std::string> &logger, std::unordered_map<int, std::string> &phonebook, std::vector<int> &clientDescriptors)
{   
    bool updating = true;
    std::mutex locker;
    char handshakeBuffer[BUFSIZ];
    ::send(socketdescriptor, ecl::serializeCommand(ecl::command{ecl::NOTAUTHORIZED, "Please identify yourself"}).c_str(), 
            ecl::serializeCommand(ecl::command{ecl::NOTAUTHORIZED, "Please identify yourself"}).length(), 0);
    
    /* Start matching */
    int loginAttempts = 0;
    bool isAuthorized = false;
    std::string username;
    while(!isAuthorized)
    {
        if(::recvfrom(socketdescriptor, handshakeBuffer, BUFSIZ, 0, NULL, NULL) <= 0)
        {
            std::cout << "Remote closed connection." << std::endl;
            close(socketdescriptor);
            return;
        }
        ecl::command initCommand = ecl::deserializeCommand(std::string(handshakeBuffer));
        std::string attemptLoginUsername = initCommand.content;
        switch(initCommand.type) {
            case ecl::LOGIN: {
                std::string timeStamp = std::to_string(std::chrono::nanoseconds(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
                ::send(socketdescriptor, ecl::serializeCommand(ecl::command{ecl::LOGINRESPOND, timeStamp}).c_str(),
                        ecl::serializeCommand(ecl::command{ecl::LOGINRESPOND, timeStamp}).length(), 0);
                ::recvfrom(socketdescriptor, handshakeBuffer, BUFSIZ, 0, NULL, NULL);
                auto loginRespond = ecl::deserializeCommand(std::string(handshakeBuffer));
                if(loginRespond.type != ecl::LOGINRESPOND)
                {
                    ::send(socketdescriptor, ecl::serializeCommand(ecl::command{ecl::NOTAUTHORIZED, "Invalid operation."}).c_str(),
                        ecl::serializeCommand(ecl::command{ecl::NOTAUTHORIZED, "Invalid operation."}).length(), 0);
                    loginAttempts++;
                }
                else
                {
                    std::array<unsigned char, 64> signedMessage;
                    memcpy(signedMessage.data(), handshakeBuffer, 64);
                    /* Verifying */
                    if(ed25519_verify(signedMessage.data(), reinterpret_cast<const unsigned char*>(timeStamp.c_str()),
                        signedMessage.size(), registeredUsers[attemptLoginUsername].data()))
                    {
                        ::send(socketdescriptor, ecl::serializeCommand(ecl::command{ecl::SUCCESS, "User logon."}).c_str(),
                            ecl::serializeCommand(ecl::command{ecl::SUCCESS, "User logon."}).length(), 0);
                        isAuthorized = true;
                        username = attemptLoginUsername;
                        break;
                    }
                    else
                    {
                        ::send(socketdescriptor, ecl::serializeCommand(ecl::command{ecl::NOTAUTHORIZED, "Incorrect keys."}).c_str(),
                            ecl::serializeCommand(ecl::command{ecl::NOTAUTHORIZED, "Incorrect keys."}).length(), 0);
                    }
                }
            }
            break;
            case ecl::REG: {
                
            }break;
            default:
                ::send(socketdescriptor, ecl::serializeCommand(ecl::command{ecl::NOTAUTHORIZED, "Invalid operation."}).c_str(),
                        ecl::serializeCommand(ecl::command{ecl::NOTAUTHORIZED, "Invalid operation."}).length(), 0);
                loginAttempts++;
            break;
        }
        if(loginAttempts > maxRetry) {
            ::send(socketdescriptor, ecl::serializeCommand(ecl::command{ecl::NOTAUTHORIZED, "Maximun limit reached."}).c_str(),
                        ecl::serializeCommand(ecl::command{ecl::NOTAUTHORIZED, "Maximun limit reached."}).length(), 0);
            close(socketdescriptor);
            return;
        }
    }

    locker.lock();
    /* Broadcast new member */
    for(int i = 0; i < clientDescriptors.size(); i++)
    {
        ::send(clientDescriptors[i], (std::string("Welcome ") + username + std::string(" into the room.\n")).c_str(), (std::string("Welcome ") + username + std::string(" into the room.\n")).length(), 0);
    }
    clientDescriptors.push_back(socketdescriptor);
    phonebook[socketdescriptor] = std::string(username);
    locker.unlock();
    
    
    char mutexBuffer[BUFSIZ];
    while(live && updating)
    {
        if(::recv(socketdescriptor, mutexBuffer, size, 0) <= 0)
        {
            std::cout << phonebook[socketdescriptor] + " disconnected." << std::endl;
            close(socketdescriptor);
            return;  
        }
        locker.lock();
        memcpy(buf, mutexBuffer, BUFSIZ);
        /* Broadcast new message */
        for(int i = 0; i < clientDescriptors.size(); i++)
        {
            if(socketdescriptor != clientDescriptors[i])
                ::send(clientDescriptors[i], (phonebook[socketdescriptor] + std::string(": ") + std::string(buf)).c_str(), (phonebook[socketdescriptor] + std::string(": ") + std::string(buf)).length(), 0);
        }
        logger.push_back(date::format("%F %T", std::chrono::system_clock::now()) + std::string(" Remote: ") + std::string(buf));
        memset(buf, 0, BUFSIZ);
        locker.unlock();
    }
}

void newClientAccepter(int socketDescriptor, std::vector<int> &clientDescriptors, char *recvbuf, int size, std::vector<std::string> &chatHistory, std::unordered_map<int, std::string> &phonebook)
{
    char userNameBuffer[BUFSIZ];
    while(live)
    {
        int cd;
        sockaddr client;
        socklen_t client_len;
        if((cd = ::accept(socketDescriptor, &client, &client_len)) < 0)
            throw std::runtime_error("failed to accept");
        std::thread receiving(update, cd, recvbuf, BUFSIZ, std::ref(chatHistory), std::ref(phonebook), std::ref(clientDescriptors));
        receiving.detach();
    }
}

int main(int argc, char **argv)
{
    /* Creating resources */
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    char sendbuf[BUFSIZ];
    char recvbuf[BUFSIZ];
    std::vector<std::string> chatHistory;
    sockaddr_in server_sockaddr;
    server_sockaddr.sin_port = htons(12001);
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    
    /* binding port */
    if(::bind(sd, reinterpret_cast<sockaddr*>(&server_sockaddr), sizeof(server_sockaddr)) < 0)
        throw std::runtime_error("failed to bind port");
    if(::listen(sd, 10) < 0)
        throw std::runtime_error("failed to listen");

    std::vector<int> clientDescriptors;
    std::unordered_map<int, std::string> phonebook;
    std::thread accepter(newClientAccepter, sd, std::ref(clientDescriptors), recvbuf, BUFSIZ, std::ref(chatHistory), std::ref(phonebook));
    accepter.detach();

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
            {
                close(sd);
                done = true;
                return 0;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Messages");
            for(int i = 0; i < chatHistory.size(); i++)
            {
                ImGui::Text("%s", chatHistory[i].c_str());
            }
            ImGui::SetScrollY(ImGui::GetScrollMaxY());
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
    close(sd);
    return 0;
}

/* enum COMMAND_TYPE{
    TEXT,
    REG,
    LOGIN,
    RENAME,
    DELETE
};

struct COMMAND{
    COMMAND_TYPE type;
    std::string content;
};

COMMAND parser(std::string content="LOGIN ACCOUNT")
{
    return COMMAND{LOGIN, std::string("ACCOUNT")};
} */