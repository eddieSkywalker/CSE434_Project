//
//  server.c
//
//  Created by Eddie Schweitzer & Chris Ward on 9/27/16.
//  Course CSE434, Computer Networks
//  Semester: Fall 2016
//  Project Part: 2
//  Time Spent: 50+ hours

//input - output declarations included in all C programs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fstream>
#include <iostream>
#include <sys/types.h>//contains definitions of a number of data types used in system calls
#include <sys/socket.h>//definitions of structures needed for sockets
#include <netinet/in.h>//in.h contains constants and structures needed for internet domain addresses
#include <stack>
#include <sys/mman.h>
#include <array>

using namespace std;

//sockfd and newsockfd are file descriptors. These two variables store the values returned by the socket system call and the accept system call.
//portno stores the port number on which the server accepts connections.
int sockfd, newsockfd, portno, pid;
int clientNumberArray [10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
bool duplicateUser = false;
bool initUserCheck = false;
int n;
int totalClients = 0;
bool readRequest = false;
bool writeRequest = false;
bool finished = false;
std::stack<string> toFile;

typedef struct myStruct{
    char* name;
    myStruct *next;
    myStruct *prev;
} myStruct;

myStruct* lockedFiles;

char buffer[256];//server reads characters from the socket connection into this buffer.

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void addLock(myStruct* node, char* fileName){
    myStruct *current;
    current = node;
    myStruct *previous = (myStruct*) malloc(sizeof(myStruct));
    while(current != NULL){
        previous = current;
        current = current->next;
    }
    current->name = fileName;
    current->next = NULL;
    current->prev = previous;
}

void removeLock(myStruct* node, char* fileName){
    myStruct *current;
    current = node;
    myStruct *previous = (myStruct*) malloc(sizeof(myStruct));
    while(current->name != fileName){
        previous = current;
        current = current->next;
    }
    previous->next = current->next;
    current = NULL;
}



void processSocket(int newsockfd)
{
    bzero(buffer,256); //After a client has successfully connected init buffer using bzero()
    
    //read call uses new file descriptor, the one returned by accept()
    /*this call reads r/w the command from user */
    n = read(newsockfd,buffer,255);
    if (n < 0) error("ERROR reading from socket");
    
    if (n == 0) //if client exits program
    {
        totalClients--;
        printf("Client Connection Closed.\n");
        close(newsockfd);
        exit(0);
    }
    
    //split arguments from client for processing
    string filename = strtok(buffer, ", ");
    string mode = strtok(NULL, "");
	char * convertedFilename = new char[(filename.length() + 1)];
	strcpy(convertedFilename, filename.c_str());
    
    string ifReading = "r";
    if(mode.find(ifReading) != string::npos) {
        readRequest = true;
    }
    string ifWriting = "w";
    if(mode.find(ifWriting) != string::npos) {
        writeRequest = true;
    }
    
    bzero(buffer,256);
    
    /* Begin handling request. */
    if(readRequest == true)
    {
        printf("Client request reading.\n");
        ifstream fileReader (convertedFilename); //Stream class to read from file
        string line;
//        addLock(lockedFiles, convertedFilename);
        
        if(fileReader.is_open()) //check that file successfully opened
        {
            while(getline(fileReader, line))
            {
                usleep(1*100);
//                sleep(1);
                n = write(newsockfd, &line, sizeof(line));
                if (n < 0) error("ERROR writing to socket");
            }
            fileReader.close();
            n = write(newsockfd, "end", strlen("end")); //send an end of file message to client
            if (n < 0) error("ERROR writing to socket");
            
            printf("Client requesting read finished. Closing request..\n");
            close(sockfd);
        }
        else //if user is requesting to read a file that does not exist
        {
            usleep(1*100);
//            sleep(1);
            n = write(newsockfd,"File not found.\n", strlen("File not found.\n"));
            if (n < 0) error("ERROR writing to socket");
            
            printf("Client requesting read for file not available. Closing request..\n");
            fileReader.close();
        }
        
        readRequest = false;
//        removeLock(lockedFiles, convertedFilename);
    }
    else if(writeRequest == true)
    {
        printf("Client requesting write.\n");
        ofstream fileWriter;                            //Stream class to write on files
        fileWriter.open(convertedFilename);             //open this file or create if it doesnt exist
        
//        addLock(lockedFiles, convertedFilename);
        while(finished == false)
        {
            bzero(buffer,256);
            
            n = read(newsockfd, buffer, sizeof(buffer));
            
            if (n < 0) error("ERROR reading from socket");
            
            else if(strncmp(buffer, "end", 3) == 0)
            {
                printf("Writing finished.\n");
                finished = true;
            }
            else
            {
                string temp = buffer;
                toFile.push(temp);
                cout << buffer << endl;
//                fileWriter << buffer << endl;
//                fileWriter.flush();
            }
        }
        
        //utilize C stack to store file and then read it out. FILO.
        while(toFile.empty() == false) {
            string temp;
            temp = toFile.top();
            fileWriter << temp << endl;
            fileWriter.flush();
            toFile.pop();
        }
        
//        while(finished == false)                        //read then write line by line to server stored file
//        {
//            bzero(buffer,256);
//            
//            n = read(newsockfd,buffer,sizeof(buffer));
//            if (n < 0) error("ERROR reading from socket");
//            if (n == 0)
//                finished = true;
//            
//            if(strncmp(buffer, "end", 3) == 0)
//            {
//                printf("%s\n", buffer);
//                printf("ELSE IF END\n");
//                finished = true;
//            }
//            else
//            {
//                cout << buffer << endl;
//                fileWriter << buffer << endl;
//                fileWriter.flush();
//                bzero(buffer,256);
//            }
//        }
        
        bzero(buffer,256);
        finished = false;
        printf("Client done writing to server. Closing request..\n");
//        removeLock(lockedFiles, convertedFilename);
        fileWriter.close();
        writeRequest = false;
        finished = false;
    }
}

int main(int argc, char *argv[])
{
//    lockedFiles = (myStruct*) mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

    
    //clilen stores the size of the address of the client. This is required for the accept system call.
    socklen_t clilen;

    struct sockaddr_in serv_addr, cli_addr;
    
    //Step 1: create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    
    //set server address buffer with zeros using bzero or memset
    //two arguments - pointer to buffer and sizeof buffer
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    if(argc == 2) // verify port #
    {
        portno = atoi(argv[1]);
    }
    else
        error("ERROR starting server");
    
    serv_addr.sin_family = AF_INET;             //contains a code for the address family
    serv_addr.sin_addr.s_addr = INADDR_ANY;     //contains the IP address of the host
    serv_addr.sin_port = htons(portno);         //contain the port number
    
    // Step 2: Bind socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    
    // Step 3: Listen for connections
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
        for(i = 0; i < 10; i++) // cycle array to search for duplicate
        {
            if(clientNumberArray[i] == submittedClientID)
            {
                duplicateUser = true;
            }
        }
            
        if(duplicateUser == false && totalClients < 5)
        {
            int i = 0;
            while(clientNumberArray[i] != -1 && i < 5)
            {
                i++;
            }
            
            //add user to stored client array
            totalClients++;
            clientNumberArray[i] = submittedClientID;
            printf("\nClient: %d Connected. Total Clients: %d\n\n", submittedClientID, totalClients);
            
            n = write(newsockfd,"Connection established.\n", strlen("Connection established.\n"));
            if (n < 0) error("ERROR writing to socket in dupe user True");
            
            //create child process
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
        else if (duplicateUser == false && totalClients >= 5)
        {
            n = write(newsockfd,"Max Clients Connected.\n", strlen("Max Clients Connected.\n"));
            printf("Client attempting connection, max clients connected.");
            if (n < 0) error("ERROR writing to socket in dupe user True");
            close(newsockfd);
        }
        else if (duplicateUser == true)
        {
            // client already registered, deny request to server.
            write(newsockfd, "Duplicate user.", strlen("duplicate user."));
            cout << (totalClients);
            if (n < 0) error("ERROR writing to socket Dupe user False");
            close(newsockfd);
        }   /* End Check */
    } /* end while loop */
    
    //close connections using file descriptors
    close(newsockfd);
    close(sockfd);
    
    return 0;
}

