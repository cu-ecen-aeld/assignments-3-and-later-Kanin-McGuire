#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

// Define the path to the data file
#define DATA_FILE "/var/tmp/aesdsocketdata"

// Declare global variables for the socket file descriptor and file pointer
int serverSocket;
FILE *filePointer;

// Signal handler function to catch SIGINT and SIGTERM signals
void signalHandler ( int sig )
{
    // Check if the signal is SIGINT or SIGTERM
    if ( sig == SIGINT || sig == SIGTERM )
    {
        // Log a message indicating the signal caught
        syslog( LOG_INFO, "Caught signal, exiting" );

        // Attempt to close the socket file descriptor
        if ( serverSocket != -1 )
        {
            close( serverSocket );
        }

        // Close the syslog connection
        closelog();

        // Exit the program
        exit( 0 );
    }
}

// Function to handle client connections
void handleClient ( int clientSocket, struct sockaddr_in clientAddr )
{
    // Declare variables to store client IP address
    char ipAddress[INET_ADDRSTRLEN];

    // Convert the client IP address to a string format
    inet_ntop( AF_INET, &clientAddr.sin_addr, ipAddress, sizeof( ipAddress ) );

    // Log a message indicating the accepted connection
    syslog( LOG_INFO, "Accepted connection from %s", ipAddress );

    // Open the data file in append mode
    filePointer = fopen( DATA_FILE, "a" );
    if ( filePointer == NULL )
    {
        // Log an error message if the file cannot be opened
        syslog( LOG_ERR, "Failed to open file %s: %s", DATA_FILE, strerror( errno ) );
        closelog();
        exit( -1 );
    }

    // Declare buffer and variables for receiving data from the client
    char buffer[1024];
    ssize_t bytesReceived;

    // Receive data from the client
    while ( ( bytesReceived = recv( clientSocket, buffer, sizeof( buffer ), 0 ) ) > 0 )
    {
        // Write the received data to the file
        fwrite( buffer, 1, bytesReceived, filePointer );

        // Check for newline character to indicate end of packet
        if ( buffer[bytesReceived - 1] == '\n' ) {
            break;
        }
    }

    // Close the file after writing
    fclose( filePointer );

    // Open the file in read mode to send the content back to the client
    filePointer = fopen( DATA_FILE, "r" );
    if ( filePointer == NULL )
    {
        // Log an error message if the file cannot be opened
        syslog( LOG_ERR, "Failed to open file %s: %s", DATA_FILE, strerror( errno ) );
        closelog();
        exit( -1 );
    }

    // Sending the full content of the file to the client
    while ( ( bytesReceived = fread( buffer, 1, sizeof( buffer ), filePointer ) ) > 0 )
    {
        send( clientSocket, buffer, bytesReceived, 0 );
    }

    // Close the file
    fclose( filePointer );

    // Close the client socket
    close( clientSocket );

    // Log a message indicating the closed connection
    syslog( LOG_INFO, "Closed connection from %s", ipAddress );
}

// Main function
int main ( int argc, char *argv[] )
{
    // Open the syslog with specified options
    openlog( "aesdsocket", LOG_PID | LOG_CONS, LOG_USER );

    // Remove the data file if it exists
    if ( remove( DATA_FILE ) == -1 && errno != ENOENT )
    {
        // Log an error message if the file cannot be removed
        syslog( LOG_ERR, "Failed to remove file %s: %s", DATA_FILE, strerror( errno ) );
        closelog();
        exit( -1 );
    }

    // Variable to determine if the program runs in daemon mode
    bool isDaemonMode = false;

    // Check if the program is running in daemon mode
    if ( argc > 1 && strcmp( argv[1], "-d" ) == 0 )
    {
        isDaemonMode = true;
    }

    // Register signal handlers for SIGINT and SIGTERM
    if ( signal( SIGINT, signalHandler ) == SIG_ERR || signal( SIGTERM, signalHandler ) == SIG_ERR )
    {
        // Log an error message if signal handlers cannot be registered
        syslog( LOG_ERR, "Failed to register signal handler: %s", strerror( errno ) );
        closelog();
        exit( -1 );
    }

    // Declare variables for address info
    struct addrinfo hints, *serviceAddr, *p;

    // Initialize hints structure
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Get address info
    int status;
    if ( ( status = getaddrinfo( NULL, "9000", &hints, &serviceAddr ) ) != 0 )
    {
        // Log an error message if address info cannot be obtained
        syslog( LOG_ERR, "Failed to get address info: %s", strerror( errno ) );
        closelog();
        exit( -1 );
    }

    // Flag to check if binding is successful
    bool isBindingSuccessful = false;

    // Iterate through the address info and bind to the first available address
    for ( p = serviceAddr; p != NULL; p = p->ai_next )
    {
        // Create socket
        serverSocket = socket( p->ai_family, p->ai_socktype, p->ai_protocol );
        if ( serverSocket == -1 )
        {
            // Log an error message if socket creation fails
            syslog( LOG_ERR, "Failed to create socket: %s", strerror( errno ) );
            continue;
        }

        // Set socket options
        if ( setsockopt( serverSocket, SOL_SOCKET, SO_REUSEADDR, &( int ){1}, sizeof( int ) ) == -1 )
        {
            // Log an error message if setting socket options fails
            syslog( LOG_ERR, "Failed to set socket options: %s", strerror( errno ) );
            close( serverSocket );
            continue;
        }

        // Bind to the address
        if ( bind( serverSocket, p->ai_addr, p->ai_addrlen ) == -1 )
        {
            // Log an error message if binding fails
            syslog( LOG_ERR, "Failed to bind: %s", strerror( errno ) );
            close( serverSocket );
            continue;
        }

        // Set the flag to true if binding is successful
        isBindingSuccessful = true;
        break;
    }

    // Free the address info
    freeaddrinfo( serviceAddr );

    // Check if binding was successful
    if ( !isBindingSuccessful )
    {
        // Log an error message if binding fails
        syslog( LOG_ERR, "Failed to bind to any address" );
        close( serverSocket );
        closelog();
        exit( -1 );
    }

    // If running in daemon mode, fork the process
    if ( isDaemonMode )
    {
        pid_t pid = fork();
        if ( pid == -1 )
        {
            // Log an error message if forking fails
            close( serverSocket );
            closelog();
            exit( -1 );
        }
        else if ( pid != 0 )
        {
            // If parent process, exit successfully
            close( serverSocket );
            closelog();
            exit( 0 );
        }
    }

    // Start listening for incoming connections
    if ( listen( serverSocket, 10 ) == -1 )
    {
        // Log an error message if listening fails
        syslog( LOG_ERR, "Failed to listen: %s", strerror( errno ) );
        closelog();
        exit( -1 );
    }

    // Accept and handle incoming client connections
    while ( 1 )
    {
        struct sockaddr_in clientAddr;
        socklen_t addrSize = sizeof( clientAddr );
        int clientSocket = accept( serverSocket, ( struct sockaddr * ) &clientAddr, &addrSize );
        if ( clientSocket == -1 )
        {
            // Log an error message if accepting connection fails
            syslog( LOG_ERR, "Failed to accept: %s", strerror( errno ) );
            continue;
        }

        // Handle the client connection
        handleClient( clientSocket, clientAddr );
    }

    return 0;
}

