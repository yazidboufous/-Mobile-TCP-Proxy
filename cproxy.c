#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define HEARTBEAT_MSG "heartbeat"

/*
cproxy is a program that acts as the proxy on the telenet program side of the 
project.
Authors: Yazid and Chris
*/
int MAX_LEN=1024;
int main (int argc, char* argv[])
{
    //run command on shell, with 3 arguments 
    //./cproxy tport sip sport
    //if argc != 4
    if (argc<4)
    {
        perror("Invalid Number of Arguments\n");
        exit(1);
    }
    
        /**
         * Establishing connection to telenet program
        */
        //creates telenet's socket
        int cproxy_socket, telenet_socket;
        //creates the server's and client's addresses
        struct sockaddr_in cproxy_addr, cli_addr;
        cproxy_socket = socket(PF_INET, SOCK_STREAM,0);
        if(cproxy_socket<0)
        {
            perror("ERROR creating cproxy main socket");
            exit(1);
        }
        memset(&cproxy_addr, 0, sizeof(cproxy_addr));
        //set the internet family domain for cproxy main socket
        cproxy_addr.sin_family = PF_INET;
        //set the port that the cproxy main and telnet must bind
        cproxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //set the internet address for cproxy main
        cproxy_addr.sin_port = htons(atoi(argv[1]));
        //Binds a unique local name to the cproxy main with telenet socket
        if(bind(cproxy_socket, (struct sockaddr *) &cproxy_addr, sizeof(cproxy_addr))<0)
        {
            perror("ERROR binding");
            exit(1);
        }
        //Infinite while loop for accepting connections
        while(1)
        {
            //listening to new connections
            if(listen(cproxy_socket,5)<0)
            {
                perror("Error listening");
                exit(1);
            }

            socklen_t AddrLen;
            AddrLen = sizeof(struct sockaddr_in);
            //connect to the telenet socket
            telenet_socket = accept( cproxy_socket, (struct sockaddr*) &cproxy_addr,&AddrLen );
            //ERROR accepting a connection to a client
            if (telenet_socket < 0)
            {
                perror("ERROR on acccepting telenet");
                exit(1);
            }
            /**
             * Create connection to sproxy
             */
            //creates sproxy socket
            int sproxy_socket;
            //creates the sproxy address variable
            struct sockaddr_in sproxy_addr;
            //creates sproxy socket
            sproxy_socket = socket(PF_INET, SOCK_STREAM,0);
            if (sproxy_socket<0)
            {
                perror("ERROR creating sproxy socket");
                exit(1);
            }
            //allocate space to sockaddr
            bzero((struct sockaddr*) &sproxy_addr, sizeof(sproxy_addr));
            //set the internet family domain for sproxy
            sproxy_addr.sin_family = PF_INET;
            //sets the sproxy's addresses
            sproxy_addr.sin_addr.s_addr = inet_addr(argv[2]);
            //set the port that the sproxy socket connects to
            sproxy_addr.sin_port = htons(atoi(argv[3]));
            if (connect(sproxy_socket, (struct sockaddr*) &sproxy_addr, sizeof(sproxy_addr))<0)
            {                
                perror("ERROR connecting");
                exit(1);
            }
            //information transfer
            printf("Connected cproxy");
            int n;
            fd_set readfds;
            int rv= -99;
            //infinite loop for selecting which socket, unless socket returns 0

            while(1)
            {
                FD_ZERO(&readfds);
                FD_SET(sproxy_socket, &readfds);
                FD_SET(telenet_socket, &readfds);

                //settimeout
                struct timeval timeout;
                timeout.tv_sec = 3;
                timeout.tv_usec = 0;

                if (sproxy_socket > telenet_socket) 
                {
                    n = sproxy_socket + 1;
                } 
                else{
                    n = telenet_socket +1;
                }
                rv = select(n,&readfds,NULL,NULL,&timeout);

                if (rv == -1) {
                    perror("error select");
                    break;
                }

                else if (rv==0){
                    //when time out, close sockets
                    close(sproxy_socket);

                    //reconnect again
                    
                        int sproxy_socket2;
                        //creates the sproxy address variable
                        struct sockaddr_in sproxy_addr2;
                        //creates sproxy socket
                        sproxy_socket2 = socket(PF_INET, SOCK_STREAM,0);
                        if (sproxy_socket2<0)
                        {
                            perror("ERROR creating sproxy socket");
                            exit(1);
                        }

                        //allocate space to sockaddr
                        bzero((struct sockaddr*) &sproxy_addr2, sizeof(sproxy_addr2));
                        //set the internet family domain for sproxy
                        sproxy_addr2.sin_family = PF_INET;
                        //sets the sproxy's addresses
                        sproxy_addr2.sin_addr.s_addr = inet_addr(argv[2]);
                        //set the port that the sproxy socket connects to
                        sproxy_addr2.sin_port = htons(atoi(argv[3]));
                        if (connect(sproxy_socket2, (struct sockaddr*) &sproxy_addr2, sizeof(sproxy_addr2))<0)
                        {                
                            perror("ERROR connecting");
                            exit(1);
                        }

                }

                else
                {
                    //if receiving from the sproxy
                    if (FD_ISSET(sproxy_socket, &readfds)) 
                    {
                        char buff1[MAX_LEN];
                        char buff2[MAX_LEN];
                        int bytes;

                        if((bytes= recv(sproxy_socket , buff1 , MAX_LEN , 0))<=0){
                            perror("Error reading from sproxy socket");
                        }

                        if (send(sproxy_socket, buff1, bytes, 0)<=0){
                            perror("Error writing to telnet socket");
                        }

                        if (send(sproxy_socket, HEARTBEAT_MSG, strlen(HEARTBEAT_MSG), 0) <= 0) {
                            perror("Error sending heasrtbeat message");
                        }

                    }

                    //if receiving from the telenet
                    if (FD_ISSET(telenet_socket, &readfds)) 
                    {
                        char buff2[1024];
                        int bytes;
                        if ((bytes = recv(telenet_socket,buff2,MAX_LEN,0))<= 0) {
                            perror("Error reading from telnet socket");
                        }

                        if (send(telenet_socket, buff2, bytes,0) <= 0) {
                            perror("Error writing to sproxy socket");
                        }

                        if (send(telenet_socket, HEARTBEAT_MSG, strlen(HEARTBEAT_MSG), 0) <= 0) {
                            perror("Error sending heartbeat message");
                        }

                    }
                }

            }
            //close sockets
            close(telenet_socket);
             close(sproxy_socket);
         }
             
        close(cproxy_socket);
   
    }

    




    
