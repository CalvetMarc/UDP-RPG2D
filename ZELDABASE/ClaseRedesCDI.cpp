#include <iostream>
#include <cstdlib>
#include <ctime>
#include "Server.h"
#include "Client.h"
#include <cfloat>


int main()
{  
    std::cout << "Enter S for server mode or C for client mode" << std::endl;
    std::string mode;
    do { std::cin >> mode; } while (mode.size() > 1 || (mode != "S" && mode != "s" && mode != "C" && mode != "c"));

    if (mode == "S" || mode == "s") {
        Server* server = new Server();
        server->InitServer();

    }
    else {
        Client* client = new Client();
        client->ConnectToServer();
    }
    std::cout << "Exit" << std::endl;
    return 0;
}

