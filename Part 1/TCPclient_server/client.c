//
//  client.c
//
//  Created by Eddie Schweitzer on 9/6/16.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//contains definitions of a number of data types used in system calls
#include <sys/types.h>

//definitions of structures needed for sockets
#include <sys/socket.h>

//in.h contains constants and structures needed for internet domain addresses
#include <netinet/in.h>

//netdb.h defines the structure hostent,
#include <netdb.h>

//This function is called when a system call fails. It displays a message about the error on stderr and then aborts the program.
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    //variable declarations
    
    //sockfd is a file descriptor
    //portno stores the port number on which the server accepts connections.
    int sockfd, portno, n;
    int clientNumber;
    int i = 0;
    
    //serv_addr will contain the address of the server
    struct sockaddr_in serv_addr;
    
    //variable server is a pointer to a structure of type hostent
    /*
     struct  hostent
     {
     char    *h_name;        // official name of host
     char    **h_aliases;    // alias list
     int     h_addrtype;     // host address type
     int     h_length;       // length of address
     char    **h_addr_list;  // list of addresses from name server
     #define h_addr  h_addr_list[0]  // address, for backward compatiblity
     };
     */
    
    struct hostent *server;
    
    char buffer[256];
    
    //atoi() function can be used to convert port number from a string of digits to an integer, if your input is in the form of a string.
    //    portno = 8888;
    
    // check to verify port number was entered
    if(argc == 4)
    {
        clientNumber = atoi(argv[2]);
        portno = atoi(argv[3]);
    }
    else
        error("Error in Command Line Input.\n");
    
    // Step 1: create socket
    //it take three arguments - address domain, type of socket, protocol (zero allows the OS to choose thye appropriate protocols based on type of socket)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    
    //funcion gethostbyname() takes name as an argument and returns a pointer to a hostent containing information about that host
    //name is taken as an argument from the user
    server = gethostbyname(argv[1]);
    
    //If hostent structure is NULL (after the above assignment), the system could not locate a host with this name
    if (server == NULL)
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    //set server address buffer with zeros using bzero or memset
    //two arguments - pointer to buffer and sizeof buffer
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    //following code sets the fields in serv_addr
    //contains a code for the address family
    serv_addr.sin_family = AF_INET;
    
    //copies length bytes from s1 to s2
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    
    //contain the port number
    serv_addr.sin_port = htons(portno);
    
    // Step 2: Connect socket to address of server / (Step 4 on Server)
    //connect function is called by the client to establish a connection to the server. It takes three arguments, the socket file descriptor, the address of the host to which it wants to connect (including the port number), and the size of this address
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    
    //After a connection and the client has successfully connected to the server
    //initialize the buffer using the bzero() function
    bzero(buffer,256);
    
/* Step 2.5: Attempt to establish connection with client ID */
    
    /* Edit Send Client Number to Server */
//    int *intBuffer = &clientNumber;
//    
//    n = write(sockfd, intBuffer, sizeof(int));
    buffer = clientNumber;
    n = write(sockfd,buffer,strlen(buffer));

    if (n < 0)
        error("ERROR writing to socket");
    /* Edit End */
    
    n = read(sockfd, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("%s\n",buffer);
    
/* End Step 2.5 */
    
    printf("Please enter the message: ");
    
    //set the buffer to the message entered on console at client end for a maximum of 255 characters
    fgets(buffer,255,stdin);
    
    // Step 3: Send and Receive data using Read/Write system calls.
    //write from the buffer into the socket
    n = write(sockfd,buffer,strlen(buffer));
    
    //check if the write function was successful or not
    if (n < 0)
        error("ERROR writing to socket");
    
    //both the server can read and write after a connection has been established.
    //everything written by the client will be read by the server, and everything written by the server will be read by the client.
    bzero(buffer,256);
    
    n = read(sockfd,buffer,255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("%s\n",buffer);
    
    
    //close connections using file descriptors
    close(sockfd);
    return 0;
}
