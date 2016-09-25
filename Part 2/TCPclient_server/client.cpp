//
//  client.c
//
//  Created by Eddie Schweitzer & Chris Ward on 9/6/16.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <iostream> 
#include <fstream>

//contains definitions of a number of data types used in system calls
#include <sys/types.h>

//definitions of structures needed for sockets
#include <sys/socket.h>

//in.h contains constants and structures needed for internet domain addresses
#include <netinet/in.h>

//netdb.h defines the structure hostent,
#include <netdb.h>

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
    
    //sockfd is a file descriptor
    //portno stores the port number on which the server accepts connections.
    int sockfd, portno, n;
    int clientNumber;
    int i = 0;
    char buffer[256];
    
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
    
    //atoi() function can be used to convert port number from a string of digits to an integer, if your input is in the form of a string.
    //    portno = 8888;
    
    // check to verify port number was entered
    if(argc == 4)
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
        write(sockfd, &clientNumber, sizeof(int));
        read(sockfd, buffer, 255);
    
        if(strncmp(buffer, "Duplicate user.", 15) == 0)
        {
            printf("..Connection error..Client already connected.\n");
            exit(1);
        }
    
        printf("%s\n",buffer);
/* End Step 2.5 */
    
    while(1)
    {
        bool readRequest = false;
        bool writeRequest = false;
        bool finished = false;
        bool initMakeFile = false;
        
        printf("Please enter the request: \n");
        
        bzero(buffer,256);
        fgets(buffer,255,stdin); //set buffer to console input for a maximum of 255 characters
        
        //parse buffer into 2 arguments
        string filename = strtok(buffer, ",");//filename
        string mode = strtok(NULL,"");
        string command = filename + "," + mode; //concatenate user input to send to server
        
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
        
//        if (stringMode == " r")
//        {
//            readRequest = true;
//        }
        
// Step 3: Send and Receive data using Read/Write system calls.
        //this write call is sent and received inside the Servers ProcessSocket()
        n = write(sockfd, &command, sizeof(command)); //send 2 arguments to server to request either r/w of/to a file.
        if (n < 0) error("ERROR writing to socket");
        
        bzero(buffer,256);
        
        if(readRequest == true) //if user wants to read from server
        {
            ofstream fileWriter; //Stream class to write on files
            
            printf("Client is now waiting for Response:\n");
            
            while(finished == false)//read response from server
            {
                n = read(sockfd,buffer,255);
                if (n < 0) error("ERROR reading from socket");
                if (n == 0) finished = true; //if server has finished sending file contents, it sends 0
                
//                ofstream fw ("readthis.txt");
//                fw << "it does work.\n"; //writes to local file that is "open"
                
                if(strncmp(buffer, "File not found.\n", 17) == 0)
                {
//                    cout << buffer << endl;
                    finished = true;
                }
                else //server found file and has begun sending content
                {
                    if(initMakeFile == false)
                    {
                        fileWriter.open("readthis.txt"); //open a file 'filename' for processing
                        initMakeFile = true;
                    }
                    if(fileWriter.is_open())
                    {
                        printf("writing to file\n");
                        fileWriter << "it does work.\n"; //writes to local file that is "open"
                    }
                }
            }
            
            fileWriter.close(); //close local file
        }
        else if(writeRequest == true) //if user wants to write(transfer) to server
        {
            ifstream fileReader; //Stream class to read from files
            
            //Client must READ one more time before writing data to server!
            n = read(sockfd,buffer,255);
            
            if(strncmp(buffer, "Begin writing.\n", 16) == 0)
            {
                printf("Beginning file writing.\n");
            }
            else exit(1); //if server not rdy to write. Unexpected error, abort.
            
            string line;
            fileReader.open(filename);
            
            while(getline(fileReader, line)) //write line by line to server
            {
                write(sockfd,"Writing to server..\n", strlen("Writing to server..\n"));
                if (n < 0) error("ERROR writing to socket");
            }
            
        }
        
        bzero(buffer,256);
        printf("Press enter to continue or N to exit.\n");
        char response;
        response = (char)getc(stdin);

        if(response == 'n' || response == 'N')
        {
            close(sockfd);
            exit(0);
        }

    } // End of while loop
    
    //close connections using file descriptors
    close(sockfd);
    return 0;
}