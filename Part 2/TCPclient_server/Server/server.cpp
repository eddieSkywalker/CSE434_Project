//
//  server.c
//
//  Created by Eddie Schweitzer & Chris Ward on 9/6/16.
//  Course CSE434, Computer Networks
//  Semester: Fall 2016
//  Project Part: 1
//  Time Spent: 30+ hours
/*
 
1) The advantage to a concurrent, connection oriented system is that the server can handle multiple 
connections at once, whereas an iterative server would have to only have one connection at a time. 
This is obviously an advantage because in real life, it is very likely that many people
will want to connect to the same server at the same time.
 
The disadvantage is the zombie problem, where once someone has connected and disconnected 
they leave behind a ghost in the process kernel that contains information about the process that had occured.
This is what a zombie is, when a process has been terminated but is not allowed to completely
die because at some point the server might want information related to the death of that process.
Over time, with many connections, these zombies can slow down the system during operations by the server,
as they clog up the process table in the kernel. This is also true over any architecture, so the problem
is persistent. 

2) In general, whenever a server will be needed by many users at the same time, you do not want to use an iterative server
so a concurrent server is a good idea. Furthermore, the appropriate time to use a concurrent, connection oriented server 
process is when the server will be doing a lot of reading and writing over a long period of time, and when you need a 
connection to be reliable. For example, FTP and Telnet often send and receive data to and from servers over much time,
so in these casesa a concurrent, connection oriented server used. However, the concurrent, connection oriented model
has a very high overhead because of the nature of how forking works, creating many different connections inside of a server,
so this process should not when the average processing time of requests is small compared to the overhead caused by the threads. 
*/

//header files

//input - output declarations included in all C programs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <fstream>
#include <iostream>

//contains definitions of a number of data types used in system calls
#include <sys/types.h>

//definitions of structures needed for sockets
#include <sys/socket.h>

//in.h contains constants and structures needed for internet domain addresses
#include <netinet/in.h>

using namespace std;

//variable declarations

//sockfd and newsockfd are file descriptors. These two variables store the values returned by the socket system call and the accept system call.
//portno stores the port number on which the server accepts connections.
int sockfd, newsockfd, portno, pid;
int clientNumberArray [10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
bool duplicateUser = false;
bool initUserCheck = false;
int n;
int totalClients = 0;
bool readRequest = false;
bool userIsFinished = false;
bool writeRequest = false;

//server reads characters from the socket connection into this buffer.
char buffer[256];

//This function is called when a system call fails. It displays a message about the error on stderr and then aborts the program.
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void processSocket(int newsockfd)
{
    //After a client has successfully connected init buffer using bzero()
    bzero(buffer,256);

    //read call uses new file descriptor, the one returned by accept()
    /*this call reads r/w the command from user */
    n = read(newsockfd,buffer,255);
    if (n < 0) error("ERROR reading from socket");
    
    if (n == 0) //if client exits program
    {
        totalClients--;
        printf("Client Connection Closed.\n");
        exit(0);
    }
    else{
        cout << buffer << endl;
    }
    
    string filename = strtok(buffer, ","); //split arguments from client (read/write)
    string mode = strtok(NULL, "");
	char * sToPass = new char[(filename.length() + 1)];
	strcpy(sToPass, filename.c_str());
    
    string str2 = " r";
    if(mode.find(str2) != string::npos)
    {
        readRequest = true;
    }
    
    string str3 = " w";
    if(mode.find(str3) != string::npos)
    {
        writeRequest = true;
    }
    
    //requesting to read a file
    if(readRequest == true)
    {
        printf("Client requesting read.\n");
        
        //setup parameters for opening file
        ifstream fileReader (sToPass); //Stream class to read from file
        string line;
//        fileReader(filename);
        
        if(fileReader.is_open() == true) //check that file successfully opened
        {
            //sends line by line to client until finished
   	 while(getline(fileReader, line) != 0)
            {
                write(newsockfd, &line, sizeof(line));
                if (n < 0) error("ERROR writing to socket");
            }
            
            printf("Client requesting read finished. Closing request..\n");
            close(sockfd);
            userIsFinished = true;
            fileReader.close();
        }
        else //if user is requesting to read a file that does not exist
        {
            n = write(newsockfd,"File not found.\n", strlen("File not found.\n"));
            if (n < 0) error("ERROR writing to socket");
            
            printf("Client requesting read for file not available. Closing request..\n");
            close(sockfd);
            userIsFinished = true;
            fileReader.close();
        }

    }
    //if client requesting to write(transfer) file to server
    else if(writeRequest == true)
    {
        cout << "Client requesting write.\n" << endl;
        
        //setup parameters for writing to file
        ofstream fileWriter; //Stream class to write on files
        string line;
        fileWriter.open(sToPass);
        
        //Server must WRITE one more time before writing data!
        write(newsockfd,"Beginning writing.\n", strlen("Beginning writing.\n"));
        
        while((n = read(newsockfd,buffer, strlen(buffer))) != 0) //read then write line by line to server stored file
        {
            fileWriter << buffer;
        }
        
        printf("Client done writing to server. Closing request..\n");
        close(sockfd);
        userIsFinished = true;
        fileWriter.close();
        
    }
    else
    {
        printf("ELSE!");
        n = write(newsockfd,"File not founddd.\n", strlen("File not founddd.\n"));
    }
}

int main(int argc, char *argv[])
{
    //clilen stores the size of the address of the client. This is required for the accept system call.
    socklen_t clilen;
    
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
    
//Step 1: create socket
    //it take three arguments - address domain, type of socket, protocol (zero allows the OS to choose thye appropriate protocols based on type of socket)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    
    //set server address buffer with zeros using bzero or memset
    //two arguments - pointer to buffer and sizeof buffer
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    //atoi() function can be used to convert port number from a string of digits to an integer, if your input is in the form of a string.
    // verify port #
    if(argc == 2)
    {
        // convert to int
        portno = atoi(argv[1]);
    }
    else
        error("ERROR starting server");
    
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
        if (newsockfd < 0) error("ERROR on accept");
        
        /* Check to see if Client Connecting is Unique. */
        int submittedClientID;
        
        n = read(newsockfd,&submittedClientID,sizeof(int));
        if (n < 0) error("ERROR reading from socket");
        int i;
        
        // cycle array to search for duplicate
        for(i = 0; i < 10; i++)
        {
            if(clientNumberArray[i] == submittedClientID)
            {
                duplicateUser = true;
            }
        }
            
        if(duplicateUser == false && totalClients <= 5)
        {
            int i = 0;
            while(clientNumberArray[i] != -1 && i < 5)
            {
                i++;
            }
            
            //add user to stored client array
            totalClients++;
            clientNumberArray[i] = submittedClientID;
            printf("Client: %d %d Connected.\n", submittedClientID, totalClients);
            
            n = write(newsockfd,"Connection established.\n", strlen("Connection established.\n"));
            if (n < 0) error("ERROR writing to socket in dupe user True");
            
            // create child process
            pid = fork();
            
            if (pid < 0)
                error("ERROR on fork");
            else if (pid == 0)
            {
                /* This is the client process */
                close(sockfd);
                while(1)
                processSocket(newsockfd);
                exit(0);
            }
            else
                close(newsockfd);
        }
        else if (duplicateUser == true)
        {
            // client already registered, deny request to server.
            write(newsockfd, "Duplicate user.", strlen("duplicate user."));
            cout << (totalClients);
            if (n < 0) error("ERROR writing to socket Dupe user False");
            close(newsockfd);
        }
        /* End Check */
        
    } /* end while loop */
    
    //close connections using file descriptors
    close(newsockfd);
    close(sockfd);
    
    return 0;
}
