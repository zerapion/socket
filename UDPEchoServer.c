// Implements the server side of an echo client-server application program.
// The client reads ITERATIONS strings from stdin, passes the string to the
// this server, which simply sends the string back to the client.
//
// Compile on general.asu.edu as:
//   g++ -o server UDPEchoServer.c
//
// Only on general3 and general4 have the ports >= 1024 been opened for
// application programs.


#include <stdio.h>      // for printf() and fprintf()
#include <sys/socket.h> // for socket() and bind()
#include <arpa/inet.h>  // for sockaddr_in and inet_ntoa()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()
#include <string>       // for strings
#include <iostream>
#include <map>          // for database 
#include <sstream>

using namespace std;

#define ECHOMAX 255     // Longest string to echo

//unique player to be stored in player database
struct Player {
    //store info about each player
    string playerName;                              //playerName is an alphabetic string of L <= 15
    string ipv4;                                    //stores non-unique ip address
    //store tracker and player port
    unsigned short t_port;                          //T-port used for communication between this player and the tracker
    unsigned short p_port;                          //P-port for communication between players

    string gameState;                               //store state of game, being open or currently being played
};

struct Game {
    string dealerName; // names of players
    string player2;
    string player3;
    string player4;

    int numPlayers;     // number of players in a game

    int gameID;         // unique game id

};

//database for players

map<string, Player> playerDatabase;
map<string, Game> gameDatabase;

// function to handle register requests, 
// const string message contains "register <player> <IPv4> <t-port> <p-port>" (& to save memory since string isnt being manipulated)
// struct sockaddr_in contains address and port number (& saves memory)
string registerFunc(const string &message, struct sockaddr_in& clientAddr, int sock)
{
    // tokens
    string keyword, playerName, ipv4;
    unsigned short t_port, p_port;
    string response;

    // seperate message into seperate tokens, original message is seperated by spaces so ss works nicely
    stringstream ss(message);
    ss >> keyword >> playerName >> ipv4 >> t_port >> p_port;

    // players must be unique, key is player name
    if (playerDatabase.count(message) > 0)
    {
        // player is found, return FAILURE
        cout << "server: Player name " << playerName << "already found in database." << endl;
        sendto(sock, "FAILURE: Given player name already exists.", 41, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
    }
    else
    {
        // player is not found, add and return SUCCESS
        playerDatabase[playerName] = {playerName, ipv4, t_port, p_port, "free"};        // tracker stores a tuple associated with teh player in a database.
                                                                                        // Also sets the state of the player to FREE indicating availability to play a game
        cout << "server: Registered player with name: " << playerName << endl;         // print success message from server

        //sendto(sock, "SUCCESS: Player registered.", 28, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));   // send correct infoto sock

        // FROM TEMPLATE MAIN FUNCTION:
        // // Send received datagram back to the client
        // if (sendto(sock, echoBuffer, strlen(echoBuffer), 0, (struct sockaddr*)&echoClntAddr, sizeof(echoClntAddr)) != strlen(echoBuffer))
    }

    response = "SUCCESS: Player " + playerName + " registered.\n";
    return response;
}

/*  redundant helper function, i phased this out
        returns if player is involved in an ongoing game, 1 == free , 0 == not in database or not free
        i think checking if in database is redundant
*/
bool getPlayerState(const string& playerName)
{
    auto temp = playerDatabase.find(playerName);

    // if name is not found in system or if state != free, return 0
    if (playerDatabase.find(playerName) == playerDatabase.end() || playerDatabase.find(playerName)->second.gameState != "free")
    {
        return 0;
    }
    else
    {
        //player in database and state == free, return 1
        return 1;
    }

    return 0;
}

/* query players currently registed with tracker, returns a list of players and their attributes */
string queryPlayersFunc(struct sockaddr_in& clientAddr, int sock)
{
    cout << "start querying" << endl << endl;

    stringstream response;                  //type stringstream to make adding to it easier
    //check if there exists players
    if (playerDatabase.empty())
    {
        response << "0\n";                  // no players found, set response == 0
    }
    else
    {
        // players found
        // return code == # of registered players, i.e. players in database
        response << playerDatabase.size() << endl;
        // navigate through each player, add their attributes to the response
        map<string, Player>::iterator it = playerDatabase.begin();

        while (it != playerDatabase.end())
        {
            Player& temp = it->second;      // second == touple with player attributes
            // add attributes to response
            response << temp.playerName << " " << temp.ipv4 << " " << temp.t_port << " " << temp.p_port << endl;
            //iterate to next entry
            ++it;
        }
    }

    cout << response.str() << endl;
    // send response back through sock
    // CONVERT SS BACK TO STRING!
    sendto(sock, response.str().c_str(), response.str().size()/*size of message*/, 0/*no special flags*/, (struct sockaddr*)&clientAddr/*given client address*/, sizeof(clientAddr));

    return response.str();
}

string queryGamesFunc(struct sockaddr_in& clientAddr, int sock)
{
    stringstream response;

    if (gameDatabase.empty())
    {
        response << "0\n";          // no games ongoing, return 0 and empty list
    }

    //return
    sendto(sock, response.str().c_str(), response.str().size()/*size of message*/, 0/*no special flags*/, (struct sockaddr*)&clientAddr/*given client address*/, sizeof(clientAddr));

    return response.str();
}

/*  
    - Removes state info about player at tracker and exit the application
    - Command returns success if and only if the given player is not involved in ongoing games
    - In the case they are, the tuple associated with the peer is deleted from database and the peer can safely exit golf game
      peer is involved in a game, command returns FAILURE
*/
string deregisterFunc(const string& message, struct sockaddr_in& clientAddr, int sock)
{
    /* create response and attributes to be parsed from message */
    string response;
    string keyword, playerName;

    /* convert to ss to be able to parse(seperated by spaces) */
    stringstream ss(message);
    ss >> keyword >> playerName;

    //check if player exists
    if (playerDatabase.find(playerName) == playerDatabase.end())
    {
        response = "FAILURE: Given Player not found in database.\n";
    }
    else
    {
        /* player exists
            now, create a player object to perform checks on and deregister
        */
        auto currPlayer = playerDatabase.find(playerName);                                                      /* check database map for existence of player w/ given player name */
        
        if (currPlayer->second.gameState != "free")
        {
            // player in database but not free, return 0
            response = "FAILURE: Given Player is participating in an ongoing game and cannot be removed.\n";
        }
        else
        {
            // player is free, remove from database and exit
            playerDatabase.erase(currPlayer);                                                                   /* bug: was just removing playername, not actual player, silly mistake */
            
            // add correct code to response, to be 'returend'
            response = "SUCCESS: Player " + playerName + " is deregistered.\n";
            printf("server: Deregistered %s.\n", playerName.c_str());
        }
    }
    //send built response str through sock
    //sendto(sock, response.str().c_str(), response.str().size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));

    return response;
}




// External error handling function
void DieWithError(const char* errorMessage) 
{
    perror(errorMessage);
    exit(1);
}


int main(int argc, char* argv[])
{
    int sock;                        // Socket
    struct sockaddr_in echoServAddr; // Local address of server
    struct sockaddr_in echoClntAddr; // Client address
    unsigned int cliAddrLen;         // Length of incoming message
    char echoBuffer[ECHOMAX];      // Buffer for echo string
    unsigned short echoServPort;     // Server port
    int recvMsgSize;                 // Size of received message

    if (argc != 2)         // Test for correct number of parameters
    {
        fprintf(stderr, "Usage:  %s <UDP SERVER PORT>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  // First arg: local port

    // Create socket for sending/receiving datagrams
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("server: socket() failed");

    // Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr)); // Zero out structure
    echoServAddr.sin_family = AF_INET;                  // Internet address family
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
    echoServAddr.sin_port = htons(echoServPort);      // Local port

    // Bind to the local address
    if (bind(sock, (struct sockaddr*)&echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("server: bind() failed");

    printf("server: Port server is listening to is: %d\n", echoServPort);

    for (;;) // Run forever
    {
        cliAddrLen = sizeof(echoClntAddr);

        // Block until receive message from a client
        if ((recvMsgSize = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr*)&echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("server: recvfrom() failed");

        echoBuffer[recvMsgSize] = '\0';
        //string to be returned to client
        string response = "";
        //parse string for instruction keyword, call corresponding function
        string message(echoBuffer);
        if (message.find("register") == 0)
        {
            response = registerFunc(message, echoClntAddr, sock);
        }
        else if (message == "query players")
        {
            cout << "querying players" << endl;
            response = queryPlayersFunc(echoClntAddr, sock);
        }
        else if (message == "query games")
        {
           response = queryGamesFunc(echoClntAddr, sock);
        }
        else if (message.find("de-register") == 0)
        {
            response = deregisterFunc(message, echoClntAddr, sock);
        }
        else
        {
            printf("server: invalid command\n");
        }

        printf("server: received string ``%s'' from client on IP address %s\n", echoBuffer, inet_ntoa(echoClntAddr.sin_addr));

        /*
        // Send received datagram back to the client
        if (sendto(sock, echoBuffer, strlen(echoBuffer), 0, (struct sockaddr*)&echoClntAddr, sizeof(echoClntAddr)) != strlen(echoBuffer))
          DieWithError("server: sendto() sent a different number of bytes than expected");
          */
        if (!response.empty())
        {
            sendto(sock, response.c_str(), response.size(), 0, (struct sockaddr*)&echoClntAddr, sizeof(echoClntAddr));
        }
    }

    close(sock);

    return 0;
    // NOT REACHED */
}
