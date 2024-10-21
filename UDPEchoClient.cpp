// Implements the client side of an echo client-server application program.
// The client reads ITERATIONS strings from stdin, passes the string to the
// server, which simply echoes it back to the client.
//
// Compile on general.asu.edu as:
//   g++ -o client UDPEchoClient.c
//
// Only on general3 and general4 have the ports >= 1024 been opened for
// application programs.
#include <stdio.h>      // for printf() and fprintf()
#include <sys/socket.h> // for socket(), connect(), sendto(), and recvfrom()
#include <arpa/inet.h>  // for sockaddr_in and inet_addr()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()
#include <string>       // for strings
#include <iostream>
#include <map>          // for database 
#include <sstream>

using namespace std;

#define ECHOMAX 255     // Longest string to echo
#define ITERATIONS	5   // Number of iterations the client executes

// functions to send message to server




void DieWithError( const char *errorMessage ) // External error handling function
{
    perror( errorMessage );
    exit(1);
}

int main( int argc, char *argv[] )
{
    size_t nread;
    int sock;                        // Socket descriptor
    struct sockaddr_in echoServAddr; // Echo server address
    struct sockaddr_in fromAddr;     // Source address of echo
    unsigned short echoServPort;     // Echo server port
    unsigned int fromSize;           // In-out of address size for recvfrom()
    char *servIP;                    // IP address of server
    //char *echoString = NULL;         // String to send to echo server
    string echoString;
    size_t echoStringLen = ECHOMAX;               // Length of string to echo
    int respStringLen;               // Length of received response

    //echoString = (char *) malloc( ECHOMAX );

    if (argc < 3)    // Test for correct number of arguments
    {
        fprintf( stderr, "Usage: %s <Server IP address> <Echo Port>\n", argv[0] );
        exit( 1 );
    }

    servIP = argv[ 1 ];  // First arg: server IP address (dotted decimal)
    echoServPort = atoi( argv[2] );  // Second arg: Use given port

    printf( "client: Arguments passed: server IP %s, port %d\n", servIP, echoServPort );

    // Create a datagram/UDP socket
    if( ( sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
        DieWithError( "client: socket() failed" );

    // Construct the server address structure
    memset( &echoServAddr, 0, sizeof( echoServAddr ) ); // Zero out structure
    echoServAddr.sin_family = AF_INET;                  // Use internet addr family
    echoServAddr.sin_addr.s_addr = inet_addr( servIP ); // Set server's IP address
    echoServAddr.sin_port = htons( echoServPort );      // Set server's port

	// Pass string back and forth between server ITERATIONS times

	//printf( "client: Enter commands\n", ITERATIONS );
    //cout << "client: Enter commands" << endl;
    /*
    for( int i = 0; i < ITERATIONS; i++ )
    {
        printf( "\nEnter string to echo: \n" );
        if( ( nread = getline( &echoString, &echoStringLen, stdin ) ) != -1 )
        {
            echoString[ (int) strlen( echoString) - 1 ] = '\0'; // Overwrite newline
            printf( "\nclient: reads string ``%s''\n", echoString );
        }
        else
            DieWithError( "client: error reading string to echo\n" );

        // Send the string to the server
        if( sendto( sock, echoString, strlen( echoString ), 0, (struct sockaddr *) &echoServAddr, sizeof( echoServAddr ) ) != strlen(echoString) )
       		DieWithError( "client: sendto() sent a different number of bytes than expected" );

        // Receive a response
        fromSize = sizeof( fromAddr );

        if( ( respStringLen = recvfrom( sock, echoString, ECHOMAX, 0, (struct sockaddr *) &fromAddr, &fromSize ) ) > ECHOMAX )
            DieWithError( "client: recvfrom() failed" );

        echoString[ respStringLen ] = '\0';

        if( echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr )
            DieWithError( "client: Error: received a packet from unknown source.\n" );

 		printf( "client: received string ``%s'' from server on IP address %s\n", echoString, inet_ntoa( fromAddr.sin_addr ) );
    }

    */

    //infinitely recieve commands
    while (true)
    {
        cout << endl << "Enter Command: " << endl;
        //get command
        //cout << "getting command" << endl;
        getline(cin, echoString);
        cout << "Sending: " << echoString << endl;
        //cout << "Sending: " << echoString << " (length: " << echoString.length() << ")" << endl;


        //if user exits, break out
        if (echoString == "exit") {
            break;
        }

        //send to server, check if it worked
        if (sendto(sock, echoString.c_str(), echoString.length(), 0, (struct sockaddr*)&echoServAddr, sizeof(echoServAddr)) != echoString.length())
        {
            DieWithError("client: sendto() sent a different number of bytes than expected");
        }

        //recieve response from server
        socklen_t fromSize = sizeof(echoServAddr);
        //issues with size of char its recieinvg? https://pubs.opengroup.org/onlinepubs/007904975/functions/recvfrom.html
        // attemping to fix with buffer[echomax]
        char buffer[ECHOMAX];
        int respStringLen = recvfrom(sock, buffer, ECHOMAX, 0, (struct sockaddr*)&echoServAddr, &fromSize);

        echoString.assign(buffer, respStringLen);

        //check if response received
        if (respStringLen < 0)
        {
            DieWithError("client: recvfrom() failed");

        }

        echoString[respStringLen] = '\0'; // Null-terminate received string
        cout << "client: response recieved: " << echoString << endl;
        //loop
    }
    
    close( sock );
    exit( 0 );
}
