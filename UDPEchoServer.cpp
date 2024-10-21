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
#include <vector>
#include <random>
#include <algorithm>
#include <thread>
#include <mutex>

using namespace std;

#define ECHOMAX 255     // Longest string to echo

struct Card {
    char suit;
    int rank;
    bool faceUp;
};

struct State {
    vector<Card> deck;
    vector<Card> discard;           /* discard pile */
    vector<<vector<Card>> hands;    /* 2d array of player's hands */
    vector<int> scores;             /* each players score */

    int currTurn;                   /* whos turn is it */
    int holesPlayed;                /* # holes played */
};

/* not neccesary i think, storing this in State */
struct Hand {
    vector<Card> hand; /* pair with card, bool face up?*/
    vector<bool> faceUp;
}

/* create deck with correct # of cards, suits, ranks etc */
void createDeck(vector<Card>& deck)
{
    string suits = "SHDC";      /* spades, hearts, diamonds, clubs */
    /* for each char in suits (4), go through each rank (1-king) and add a card of that suit and rank into the deck
    */
    for (char suit : suits)
    {
        for (int rank = 1; rank <= 13; ++rank)
        {
            // Card curr = {rank, suit, false};
            deck.push_back*({rank, suit, false});
        }
    }
}

/* Shuffle the deck of cards. Form a random permutation of the first 52 integers, and arrange the deck of cards
according to it. 
*/
void my_shuffle(vector<Card>& deck)
{
    /* see notes for documentation details */
    random_device rd;
    mt19937 g(rd());                        /* this is somethign crazy */
    shuffle(deck.begin(), deck.end(), g);   /* predefined function */ 
}
/* Using the information in the tuples (e.g., Table 1), cycle
through the players 6 times to deal 6 cards from the deck; always follow the order returned in the start game
command when cycling through players.
*/
void deal(State& gameState, int numPlayers)
{
    gameState.hands.resize(numPlayers);         /* resize hands vector to numPlayers */
    /* each player will get six cards */
    for (int i = 0; i < 6; ++i)
    {
        /* deal to dealer (not included in numPlayers) */
        gameState.hands[0].push_back(gameState.deck.back()); 
        gameState.deck.pop_back();                           

        /* go through each player */
        for (int p = 0; p < numPlayers; ++p)
        {   
            /* */
            gameState.hands[p].push_back(gameState.deck.back()); /* take last card from shuffled deck and insert into players hand */
            gameState.deck.pop_back();                           /* remove freshly inserted card from deck */
        }
    }

    gameState.discardPile.push_back(gameState.deck.back());
    gameState.deck.pop_back();

}

//unique player to be stored in player database
struct Player {
    //store info about each player
    string playerName;                              /* playerName is an alphabetic string of L <= 15 */
    string ipv4;                                    /* stores non-unique ip address */
    //store tracker and player port
    unsigned short t_port;                          /* T-port used for communication between this player and the tracker */
    unsigned short p_port;                          /* P-port for communication between players */

    string gameState;                               /* store state of game, being open or currently being played */
};

struct Game {
    string dealerName;  // names of players
    string player2;
    string player3;
    string player4;

    int numPlayers;     // number of players in a game
    int gameID;         // unique game id
    State = state;
};

/* ---------------------------------------------------------------------------------------------------------------------------- */
/* MAP NOTES:
    - string is key, Player/Game is actual object
    - to accsess values of Player/Game object in question, create a new Player/Game object using
       Player/Game curr = Database.find(Name); 
    - this will return a pointer to that Player/Game object in the database and store it in curr
    - from here, checks or deletion/insertion can be done
        curr->second.gameState/gameID/whatever is in the player/game struct
*/
/* database for players and games */

map<string, Player> playerDatabase;
map<string, Game> gameDatabase;

/* id for game */
int nextGame = 1; 

/* helper function to count # of free players */
int countFreePlayers(string dealerName)
{
    int count = 0;
    /* iterator to go through each Player in map */
    map<string, Player>::iterator it = playerDatabase.begin();

    while (it != playerDatabase.end())
    {
        /* check if not looking at dealer and if player is free */
        if (it->second.playerName != dealerName && it->second.gameState == "free")
        {
            count++;
            cout << "free player found, count = " << count << endl;
        }
        /* increment to next item in map */
        ++it;
    }   

    return count;
}

string checkParams(int& numPlayers, int& numHoles, string& playerName, string& response, bool& shouldReturn)
{
    if (!(numPlayers >= 2 && numPlayers <= 4))
    {
        response = "FAILURE: number of players not in bounds\n";
        shouldReturn = true;

        return response;
    }
    if (!(numHoles >= 1 && numHoles <= 9))
    {
        response = "FAILURE: number of holes not in bounds\n";
        shouldReturn = true;

        return response;
    }
    /* check that player is in database and free
    */
    auto dealerTemp = playerDatabase.find(playerName);                                          /* this will be used later */
    if (dealerTemp == playerDatabase.end())
    {
        response = "FAILURE: player not in databse\n";
        shouldReturn = true;

        return response;
    }
    if (dealerTemp->second.gameState != "free")
    {
        response = "FAILURE: dealer is not free\n";
        shouldReturn = true;

        return response;
    }
    /* check that there are enough available players */
    if (countFreePlayers(playerName) < numPlayers)
    {
        response = "FAILURE: not enough free players\n";
        shouldReturn = true;

        return response;
    }

    return "error";
}
/* helper function to populate the freePlayers string */
vector<string> populate(int n, string dealerName)
{
    /* iterator to go through each Player in map */

    /* to randomize, perhaps create a copy of playerDatabase, use  random_device rd; mt19937 g(rd()))shuffle(temp.begin(), temp.end(), g);
    
        wont try this until after base functionality is established 
    */

    vector<string> free;
    map<string, Player>::iterator it = playerDatabase.begin();
    int count = 0;
    /* while not at end of map */
    while (count < n && it != playerDatabase.end())     /* <= n ?*/
    {
        /* check if not looking at dealer and if player is free */
        if (it->second.playerName != dealerName && it->second.gameState == "free")
        {
            cout << "free player found, adding to free players vector\n";
            free.push_back(it->second.playerName);                     /* add to vector */
            it->second.gameState = "in-game";       /* change gameState */
            count++;
        }
        /* increment to next item in map */
        ++it;
    }

    return free;
}




/* takes a message with player name, #players, #holes
    must parse this message to initiate a game with correct paramaters (func def should be similar to registerFunc, message handling as well
    - FAILURE: 
        - player is not registered
        - n is not in proper range (2-4)
        - not at least n other players in tracker (server)
        - holes is not in proper range (1-9 inclusive)
    - otherwise, tracer retursn SUCCCESS
        - assigns new game-identifier for the games
        - stores state recording that player is the dealer of the game
        - trakcer alos selects n free players at random from those registerd as players in the game <import rand?, research!>
        - returned to dealer is game-identifer && list of random players <vector pair>? 
        - list consists o fplayer name, ipv4 addresses, p-port num ( of dealer and of other players ) stored in database
        - tracker also updates the state of each player from free to in-play
        - dealer will preform additional steps to set-up the play of the game,

        */
string start(const string& message, struct sockaddr_in& clientAddr, int sock)
{
    /* parse message for tokens */
    string keyword, playerName, response = "FAILURE: error";  /* resposne to be returned later*/
    int numPlayers, numHoles;           /* i think issues will arise with numPlayers */
    unsigned short t_port, p_port;      /* t_port not neccessary i think, just copying from my registerfunc to get somethin started */
    bool shouldReturn = false;
   
    stringstream ss(message);
    ss >> keyword >> playerName >> numPlayers >> numHoles;

    /* step 1: error checking
       - check that numPlayers/Holes is within correct bounds, player is registerd, and n other players in tracker 
       - put this logic in string helperFunc(int numPlayers, int numHoles, string playerName, that returns bool shouldContinue?
       if (!(shouldcontinue)
       {
       return response_
    */
    
    /* i think i need this later */
    auto dealerTemp = playerDatabase.find(playerName);
    if (dealerTemp == playerDatabase.end())
    {
        response = "FAILURE: player not in database\n";

        return response;
    } 

    Player& dealer = dealerTemp->second;   
    
    response = checkParams(numPlayers, numHoles, playerName, response, shouldReturn);
    cout << "shouldReturn: " << shouldReturn << endl;
    if (shouldReturn)
    {
        cout << "response from start: " << response << endl;
        return response;
    }

    /* checking is done, now can start with actual game creation */

    /* step 2: start game with n players */

    /* initialize new game with inputted values */
    Game newGame;
    newGame.numPlayers = numPlayers + 1;    /* +1 to include the dealer */
    newGame.gameID = nextGame++;          /* set it to curr value of nextGameID then increment it */

    /* add dealer before dealing with other players */
    newGame.dealerName = playerName;
    playerDatabase[playerName].gameState = "in-game";

    vector<string> freePlayers;             /* will be used to store players in game */
    /* freePlayers.push_back(playerName);      /* add dealer to game and change status */

    /* populate freePlayers vector with n freePlayers */
    freePlayers = populate(numPlayers, playerName);

    /* we now should have a vector that contains the name of 4 free players including the dealer */

    /* add freePlayers to the game */
    newGame.player2 = freePlayers[0];       /* always have at least one other player */
    if (numPlayers >= 2)                             /* check if more than 2 players (not including dealer) */
    {
        newGame.player3 = freePlayers[1];   
    }
    if (numPlayers == 3)
    {
        newGame.player4 = freePlayers[2];
    }

    /* add fully initialized newGame to the gameDatabase, maybe change gameDatabase to <int, Game> */
    gameDatabase[to_string(newGame.gameID)] = newGame;


    /* heavy lifting is done, now, return to main the correct resposne code */
    response = "SUCCESS: Game " + to_string(newGame.gameID) + " started.\n";
    response += "Dealer: " + playerName + " " + dealer.ipv4 + " " to_string(dealer.p_port) + "\n";
    response += "Players: ";
    /* iterate through freePlayers to print each ones names */
            /*
            for (int i = 0; i < freePlayers.size(); ++i)
            {
                response += freePlayers[i].playerName + " ";    /* could format better 
            }
            */
    for (const auto& playerName : freePlayers)
    {
        auto currPlayer = playerDatabase.find(playerName);
        if (currPlayer != playerDatabase.end())
        {
            response += playerName + " " + currPlayer->second.ipv4 + " " + to_string(currPlayer->second.p_port) + "\n";
        }
    }
    response += "\n";

    /* initialize game */
    createDeck(newGame.state.deck);
    my_shuffle(newGame.state.deck);
    deal(newGame.state, newGame.numPlayers);
    /* initialzie game with shuffled deck, i think there are issues with num players */
    newGame.state.curTurn = 0;
    newGame.state.holesPlayed = 0;
    newGame.state.score.resize(newGame.numPlayers + 1, 0);  /* scores = 0 */

    /* possible trailing comma, not sure how to remove */
    return response;
}


/* end game if dealer and ID exist and match, free all players in game */
string endGame(const string& message, struct sockaddr_in& clientAddr, int sock)
{
    /* same as before, parse message for neccessary tokens */
    string keyword, gameID, playerName, response;

    stringstream ss(message);
    ss >> keyword >> gameID >> playerName;

    auto gameCheckTemp = gameDatabase.find(gameID);
    /* check if gameID in database */
    if (gameCheckTemp == gameDatabase.end())
    {
        response = "FAILURE: Game id not in database.\n";
        
        return response;
    }
    Game& gameCheck = gameCheckTemp->second;
    /* gameID is in database, check if dealer name is correct */
    if (gameCheck.dealerName != playerName)
    {
        response = "FAILURE: Specified player name does not match name of specified Game's dealer.\n";

        return response;
    }

    /* game is in database with correct dealer, move on to deleting from database and setting players to "free" from "in-play" */
    /* there is at least two players, so we have to check if players 3 and 4 exist before changing their attributes */
    
    playerDatabase[gameCheck.dealerName].gameState = "free";    /* garunteed to be a dealer and one other, change their state */
    playerDatabase[gameCheck.player2].gameState = "free";
    if (gameCheck.numPlayers == 2)                              /* check if players 3 and 4 exist before changing state */
    {
        playerDatabase[gameCheck.player3].gameState = "free";
    }
    if (gameCheck.numPlayers == 3)
    {
        playerDatabase[gameCheck.player4].gameState = "free";
    }

    /* players freed, delete from database */
    gameDatabase.erase(gameID);
    response = "SUCCESS: Game with ID " + gameID + " has been ended.\n";        /* could attribute to show who won? */
    
    return response;
}


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
    if (playerDatabase.count(playerName) > 0)
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
    //sendto(sock, response.str().c_str(), response.str().size()/*size of message*/, 0/*no special flags*/, (struct sockaddr*)&clientAddr/*given client address*/, sizeof(clientAddr));

    return response.str();
}

/* returns number of ongoing games and a list with info of each game ( gameid, name of dealer, naems of other players) */
string queryGamesFunc(struct sockaddr_in& clientAddr, int sock)
{
    stringstream response;

    if (gameDatabase.empty())
    {
        response << "0\n";          /* no games ongoing, return 0 and empty list */
        
        return response.str();
    }

    response << "Number of ongoing games: " << gameDatabase.size() << "\nGame Information: \n";
   
    /* iterate through each game in database*/
    map<string, Game>::iterator it = gameDatabase.begin();
    while (it != gameDatabase.end())
    {
        /* format response with curr games info*/
        response << "Game ID: " << it->second.gameID << "\n"
            << "Dealer: " << it->second.dealerName << "\n"
            << "Players: " << it->second.dealerName << ", " << it->second.player2;
        if (it->second.numPlayers == 2)                              /* check if players 3 and 4 exist before adding to response */
        {
            response << ", " << it->second.player3;
        }
        if (it->second.numPlayers == 3)
        {
            response << ", " << it->second.player4;
        }
        response << "\n";
        
        /* increment to next item in map */
        ++it;
    }

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

string handleMove(const string& message, struct sockaddr_in& clientAddr, int sock)
{
    string keyword, gameID, playerName, action;
    string ss(message);
    ss >> keyword >> gameID >> playerName >> action;

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
        else if (message.find("start game") == 0)
        {
            response = start(message, echoClntAddr, sock);
        }
        else if (message.find("end") == 0)
        {
            response = endGame(message, echoClntAddr, sock);
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
