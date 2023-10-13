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
#include "json/single_include/nlohmann/json.hpp"
#include "ed25519.hpp"
#include "protocal.hpp"
#include "base64/base64.h"
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

using json = nlohmann::json;
bool live = true;
constexpr int maxRetry = 3;
constexpr int signatureSize = 64;
std::array<unsigned char, 32> serverPublicKey;
std::array<unsigned char, 64> serverPrivateKey;
std::unordered_map<std::string, std::array<unsigned char, 32>> registeredUsers;
std::unordered_map<std::string, std::string> easyPws;

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
        /* Selection Hub */
        memset(handshakeBuffer, 0, BUFSIZ);
        if(::recvfrom(socketdescriptor, handshakeBuffer, BUFSIZ, 0, NULL, NULL) <= 0)
        {
            std::cout << "Remote closed connection." << std::endl;
            close(socketdescriptor);
            return;
        }
        
        json returnMsg;
        std::string cmd(handshakeBuffer);
        json infos = json::parse(cmd);
        std::string type = infos["type"];
        if(type == "login")
        {
            if(infos["password"] == easyPws[infos["username"]])
            {
                returnMsg["type"] = "success";
                returnMsg["ack"] = "login";
                username = infos["username"];
                isAuthorized = true;
                phonebook[socketdescriptor] = username;
            }
            else
            {
                returnMsg["type"] = "failure";
                returnMsg["ack"] = "login";
                loginAttempts++;
            }
        }
        else if(type == "register")
        {
            if(easyPws.find(infos["username"]) == easyPws.end())
            {
                easyPws[infos["username"]] = infos["password"];
                returnMsg["type"] = "success";
                returnMsg["ack"] = "register";
            }
            else
            {
                returnMsg["type"] = "failure";
                returnMsg["ack"] = "register";
            }
        }

        if(loginAttempts > maxRetry) {
            json limitReached;
            limitReached["type"] = "unauthorized";
            limitReached["info"] = "maximum limit reached";
            std::string limitReachedMsg = limitReached.dump();
            ::send(socketdescriptor, limitReachedMsg.c_str(), limitReachedMsg.length(), 0);
            close(socketdescriptor);
            return;
        }
        std::string respond = returnMsg.dump();
        ::send(socketdescriptor, respond.c_str(), respond.length(), 0);
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
        std::cout << date::format("%F %T", std::chrono::system_clock::now()) + std::string(" Remote: ") + std::string(buf) << std::endl;
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
    
    bool done = false;
    while (!done)
    {
        /*for(int i = 0; i < chatHistory.size(); i++)
        {
            printf("%s\n", chatHistory[i].c_str());
        }*/
    }
    close(sd);
    return 0;
}