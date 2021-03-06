//
//  client.c
//
//  Created by Eddie Schweitzer & Chris Ward on 9/27/16.
//  Course CSE434, Computer Networks
//  Semester: Fall 2016
//  Project Part: 2
//  Time Spent: 50+ hours

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <iostream> 
#include <fstream>
#include <sys/types.h> //contains definitions of a number of data types used in system calls
#include <sys/socket.h> //definitions of structures needed for sockets
#include <netinet/in.h> //in.h contains constants and structures needed for internet domain addresses
#include <netdb.h> //netdb.h defines the structure hostent,

using namespace std;

//This function is called when a system call fails. It displays a message about the error on stderr and then aborts the program.
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    //variable declarations
    int sockfd, portno, n; //file descriptor, portnumber on which server accepts connections
    int clientNumber;
    int i = 0;
    char buffer[256];
    
    //serv_addr will contain the address of the server
    struct sockaddr_in serv_addr;

    struct hostent *server;
    
    if(argc == 4) // check to verify port number was entered
    {
        clientNumber = atoi(argv[2]);
        portno = atoi(argv[3]);
    }
    else
        error("Error in Command Line Input(incorrect # of arguments).\n");
    
    // Step 1: create socket
    //it take three arguments - address domain, type of socket, protocol (zero allows the OS to choose thye appropriate protocols based on type of socket)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    
    //funcion gethostbyname() takes name as an argument from user and returns a pointer to a hostent containing information about that host
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
    
    bcopy((char *)server->h_addr, //copies length bytes from s1 to s2
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    
    serv_addr.sin_port = htons(portno); //contain the port number
    
    // Step 2: Connect socket to address of server / (Step 4 on Server)
    //connect function is called by the client to establish a connection to the server.
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
        
        //After a connection and the client has successfully connected to the server
        //initialize the buffer using the bzero() function
        bzero(buffer,256);
        
        /* Step 2.5: Attempt to establish connection with client ID */
        write(sockfd, &clientNumber, sizeof(int));
        read(sockfd, buffer, 255);
    
        if(strncmp(buffer, "Duplicate user.", 15) == 0)
        {
            printf("..Connection error..Client already connected.\n");
            exit(0);
        }
        else if(strncmp(buffer, "Max Clients Connected.", 22) == 0)
        {
            printf("Max Clients Connected.\n");
            exit(0);
        }
    
        printf("%s\n",buffer);
        /* End Step 2.5 */
    
    while(1)
    {
        bool readRequest = false;
        bool writeRequest = false;
        bool finished = false;
        bool initMakeFile = false;
        bool userInputCorrect = false;
        
        printf("Please enter the request: \n");
        
        bzero(buffer,256);
        fgets(buffer,255,stdin);                                //user input into buffer
        
        string filename = strtok(buffer, ", ");                 //parse buffer into 2 arguments
        string mode = strtok(NULL,"");
        string command = filename + ", " + mode;                //concatenate user input to send to server
        
        char * commandToPass = new char[(command.length() + 1)];
        strcpy(commandToPass, command.c_str());
        
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
        
        // Step 3: Send and Receive data using Read/Write system calls.
        //this write call is sent and received inside the Servers ProcessSocket()
        usleep(1*100);
//        sleep(1);
        n = write(sockfd, commandToPass, strlen(commandToPass)); //send request of either r/w of/to a file.
        if (n < 0) error("ERROR writing to socket");
        
        if(readRequest == true)
        {
            printf("Client is requesting reading:\n");
            ofstream fileWriter;                                //Stream class to write on files
            
            while(finished == false)
            {
                bzero(buffer,256);
                
                n = read(sockfd,buffer,sizeof(buffer));
                if (n < 0) error("ERROR reading from socket");
                if (n == 0)
                    finished = true;
                else if(strncmp(buffer, "end", 3) == 0)
                {
                    printf("Reading finished.\n");
                    finished = true;
                }
                else if(strncmp(buffer, "File not found.\n", 17) == 0)
                {
                    cout << buffer << endl;
                    finished = true;
                }
                else                                            //server found file & is sending content
                {
                    if(initMakeFile == false)
                    {
                        fileWriter.open(convertedFilename);     //open a file 'filename' for processing
                        initMakeFile = true;
                    }
                    fileWriter << buffer << endl;               //writes to local file that is "open"
                    fileWriter.flush();
                }
            }
            bzero(buffer,256);
            fileWriter.close();
            initMakeFile = false;
            readRequest = false;
            finished = false;
        }
        else if(writeRequest == true)
        {
            printf("Beginning writing.\n");
            ifstream fileReader;                                //Stream class to read from files
            string line;
            fileReader.open(convertedFilename);
            
            while(getline(fileReader, line))                    //write line by line to server
            {
                char * lineArray = new char[(line.length() + 1)];
                strcpy(lineArray, line.c_str());
                
                usleep(1*100);
//                sleep(1);
                n = write(sockfd, lineArray, strlen(lineArray));
                if (n < 0) error("ERROR writing to socket");
                cout << lineArray << endl;
            }
            
            n = write(sockfd, "end", strlen("end"));            //send an end of file message to client
            if (n < 0) error("ERROR writing to socket");
            
            fileReader.close();
            printf("Writing finished.\n");
            writeRequest = false;
        }
        
        bzero(buffer,256);
        char response;
        
        printf("Press Y to continue or N to exit.\n");
        response = getchar();
            
        if(response == 'n' || response == 'N') {
                close(sockfd);
                exit(0);
        }
        if(response == 'y' || response == 'Y') {
            getchar();
        }
        else{
            printf("Error: must be Y or N. Exiting...\n");
            exit(1);
        }

    } // End of while loop
    
    //close connections using file descriptors
    close(sockfd);
    return 0;
}
