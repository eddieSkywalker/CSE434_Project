//
//  server.c
//
//
//  Created by Eddie Schweitzer on 9/6/16.

//header files

//input - output declarations included in all C programs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

//contains definitions of a number of data types used in system calls
#include <sys/types.h>

//definitions of structures needed for sockets
#include <sys/socket.h>

//in.h contains constants and structures needed for internet domain addresses
#include <netinet/in.h>

//This function is called when a system call fails. It displays a message about the error on stderr and then aborts the program.
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void processSocket(int sock)
{
    int n;
    char buffer[256];
    bzero(buffer, 256);
    n = read(sock, buffer, 255);
    
    if (n < 0)
        error("ERROR reading from socket.");
    
    printf("Here is the message %s\n", buffer);
    n = write(sock, "I got your message.",18);
    
    if (n < 0)
        error("ERROR writing to socket");
}

int main(int argc, char *argv[])
{
//variable declarations

    //sockfd and newsockfd are file descriptors. These two variables store the values returned by the socket system call and the accept system call.
    //portno stores the port number on which the server accepts connections.
    int sockfd, newsockfd, portno, pid;
    int clientNumberArray [5];
    bool set = false;

    //clilen stores the size of the address of the client. This is required for the accept system call.
    socklen_t clilen;

    //server reads characters from the socket connection into this buffer.
    char buffer[256];

    //sockaddr_in is a structure containing an internet address
    /*
    struct sockaddr_in
    {
        short   sin_family; //must be AF_INET
        u_short sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8]; // Not used, must be zero
    };
    */
    //in_addr structure, contains only one field, a unsigned long called s_addr.

    //serv_addr will contain the address of the server, and cli_addr will contain the address of the client which connects to the server.
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    //Step 1: create socket
    
    //it take three arguments - address domain, type of socket, protocol (zero allows the OS to choose thye appropriate protocols based on type of socket)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    //set server address buffer with zeros using bzero or memset
    //two arguments - pointer to buffer and sizeof buffer
    bzero((char *) &serv_addr, sizeof(serv_addr));

    //atoi() function can be used to convert port number from a string of digits to an integer, if your input is in the form of a string.
//    portno = 8888;
    
    // verify port #
    if(argc == 2)
    {
        // convert to int
        portno = atoi(argv[1]);
    }
    else
        error("Error Message");

    //contains a code for the address family
    serv_addr.sin_family = AF_INET;

    //contains the IP address of the host
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    //contain the port number
    serv_addr.sin_port = htons(portno);

    // Step 2: Bind socket
    
    //bind() system call binds a socket to an address, in this case the address of the current host and port number on which the server will run.
    //three arguments, the socket file descriptor, the address to which is bound, and the size of the address to which it is bound.
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    
    // Step 3: Listen for connections

    //listen system call allows the process to listen on the socket for connections.
    //The first argument is the socket file descriptor, and second is number of connections that can be waiting while the process is handling a particular connection.
    listen(sockfd,5);

    clilen = sizeof(cli_addr);
    
    // Step 4: Accept Multiple Unique Client Numbers (Active Listening)

    //accept() system call causes the process to block until a client connects to the server.
    while(1)
    {
        /* Accept actual connection from the client */
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        
        /* Check to see if Client Connected is Unique. */
        
//        // if a command line is valid.
//        if(argc == 4)
//        {
//            // checks to make sure number entered is not input already
//            for(int i = 0; i < 5; i++)
//            {
//                if(clientNumberArray[i] == submittedClientID)
//                {
//                    set = true;
//                }
//            }
//            
//            // if no duplicate client # was found, add client to array
//            if(set == false)
//            {
//                clientNumberArray[0] = submittedClientID;
//            }
//            else if (set == true)
//            {
//                // client already registered, deny request to server.
//                
//            }
//        }
        /* End Check */
        
        if (newsockfd < 0)
            error("ERROR on accept");
        
        // create child process
        pid = fork();
        
        if (pid < 0)
        {
            error("ERROR on fork.");
        }
        else if (pid == 0)
        {
            /* This is the client process */
            close(sockfd);
//            processSocket(newsockfd);
            
            //After a connection, and a client has successfully connected to the server
            //initialize the buffer using the bzero() function
            bzero(buffer,256);
            
            //reads from the socket into a buffer for a maximum of 255 characters
            //read call uses new file descriptor, the one returned by accept()
            n = read(newsockfd,buffer,255);
            if (n < 0) error("ERROR reading from socket");
            
//            int submittedClientID = buffer;
            
            //both the server/client can read and write after a connection has been established.
            //everything written by the client will be read by the server, and everything written by the server will be read by the client.
            printf("Here is the message: %s\n",buffer);
            
            n = write(newsockfd,"Server: I got your message.",18);
            if (n < 0) error("ERROR writing to socket");
            
            exit(0);
        }
        else
            close(newsockfd);
        
    } /* end while loop */

    //close connections using file descriptors
    close(newsockfd);
    close(sockfd);

    return 0;
}
