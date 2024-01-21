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
sproxy is a program that acts as the proxy on the telenet daemon side of the 
project.
Authors: Yazid and Chris
*/
int MAX_LEN=1024;
int main(int argc, char *argv[])
{
    //run command on shell, with 2 arguments ./server sport
    //if argc != 2
    if(argc != 2)
    {
        fprintf(stderr, "Invalid Number of Arguments\n\n");
        exit(1);
    }
        /**
         * Create connection to cproxy
        */
        int sproxy_socket, cproxy_socket;
        //creates the sproxy's address
        struct sockaddr_in sproxy_addr;
        sproxy_socket = socket(PF_INET, SOCK_STREAM,0);
        if(sproxy_socket<0)
        {
            perror("ERROR creating socket");
            exit(1);
        }
        memset(&sproxy_addr, 0, sizeof(sproxy_addr));
        //set the internet family domain for cproxy main socket
        sproxy_addr.sin_family = PF_INET;
        //set the internet address for sproxy main
        sproxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //set the port that the sproxy main and cproxy must bind
        sproxy_addr.sin_port = htons(atoi(argv[1]));
        //Binds a unique local name to the socket with descriptor socket
        if(bind (sproxy_socket, (struct sockaddr *) &sproxy_addr, sizeof(sproxy_addr))<0)
        {
            perror("ERROR binding");
            exit(1);
        }
        while(1)
        {
            //listening to new connections
            if(listen(sproxy_socket,5)<0)
            {
                perror("Error listenning");
                exit(1);
            }
            socklen_t AddrLen;
            AddrLen = sizeof(struct sockaddr_in);
            //connect to the cproxy socket
            cproxy_socket = accept(sproxy_socket, (struct sockaddr*)&sproxy_addr,&AddrLen);
            //ERROR accepting a connection to a cproxy
            if (cproxy_socket < 0)
            {
                perror("ERROR on acccepting c");
                exit(1);
            }
            /**
             * Create connection to telenet daemon
             * 
            */
            int daemon_socket;
            //creates the daemon address variable
            struct sockaddr_in daemon_addr;
            //creates daemon socket
            daemon_socket = socket(PF_INET, SOCK_STREAM,0);
            if (daemon_socket<0)
            {
                perror("ERROR creating sproxy socket");
                exit(1);
            }
            //allocate space to daemon_addr
            bzero((struct sockaddr*) &daemon_addr, sizeof(daemon_addr));
            //set the internet family domain for daemon
            daemon_addr.sin_family = PF_INET;
            //sets the daemon's addresses
            daemon_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            //set the port that the daemon connects on sproxy
            daemon_addr.sin_port = htons(atoi("23"));
            if (connect(daemon_socket, (struct sockaddr*) &daemon_addr, sizeof(daemon_addr))<0)
            {                
                perror("ERROR connecting");
                exit(1);
            }
            //information transfer
            printf("Connected sproxy");
            int n;
            fd_set readfds;
            int rv= -99;
            int heartbeat_timer = 0;
            bool connection_lost=0;
            //infinite loop for selecting which socket, unless socket returns 0
             while(1)
            {
                FD_ZERO(&readfds);
                FD_SET(cproxy_socket, &readfds);
                FD_SET(daemon_socket, &readfds);

                //settimeout
                struct timeval timeout;
                timeout.tv_sec = 3;
                timeout.tv_usec = 0;

                if (cproxy_socket > daemon_socket) 
                {
                    n = cproxy_socket + 1;
                } 
                else{
                    n = daemon_socket +1;
                }
                rv = select(n,&readfds,NULL,NULL,&timeout);

                if (rv == -1) {
                    //perror("select");
                    break;
                }

                else if (rv==0){
                    //when time out, close sockets
                    close(cproxy_socket);

                    //creates the sproxy's address
                    struct sockaddr_in sproxy_addr2;
                    int sproxy_socket2 = socket(PF_INET, SOCK_STREAM,0);
                    if(sproxy_socket2<0)
                    {
                        perror("ERROR creating socket");
                        exit(1);
                    }
                    memset(&sproxy_addr2, 0, sizeof(sproxy_addr2));
                    //set the internet family domain for cproxy main socket
                    sproxy_addr2.sin_family = PF_INET;
                    //set the internet address for sproxy main
                    sproxy_addr2.sin_addr.s_addr = htonl(INADDR_ANY);
                    //set the port that the sproxy main and cproxy must bind
                    sproxy_addr2.sin_port = htons(atoi(argv[1]));
                    //Binds a unique local name to the socket with descriptor socket
                    if(bind (sproxy_socket2, (struct sockaddr *) &sproxy_addr2, sizeof(sproxy_addr2))<0)
                    {
                        perror("ERROR binding");
                        exit(1);
                   }
                   
                   if(listen(sproxy_socket2,5)<0)
                    {
                        perror("Error listenning");
                        exit(1);
                    }

                        socklen_t AddrLen2;
                        AddrLen2 = sizeof(struct sockaddr_in);
                        //connect to the cproxy socket
                        int cproxy_socket2 = accept(sproxy_socket2, (struct sockaddr*)&sproxy_addr2, &AddrLen2);
                        //ERROR accepting a connection to a cproxy
                        if (cproxy_socket2 < 0)
                        {
                            perror("ERROR on acccepting c");
                            exit(1);
                        }
                   
                }

                else
                {
                    //if receiving from the sproxy
                    if (FD_ISSET(cproxy_socket, &readfds)) 
                    {
                        char buff1[MAX_LEN];
                        char buff2[MAX_LEN];
                        int bytes;

                        if((bytes= recv(cproxy_socket , buff1 , MAX_LEN , 0))<=0){
                            //perror("Error reading from sproxy socket");
                            break;
                        }

                        if (send(cproxy_socket, buff1, bytes, 0)<=0){
                            //perror("Error writing to telnet socket");
                            break;
                        }

                        if (send(cproxy_socket, HEARTBEAT_MSG, strlen(HEARTBEAT_MSG), 0) <= 0) {
                            //perror("Error sending heartbeat message");
                            break;
                        }

                    }

                    //if receiving from the telenet
                    if (FD_ISSET(daemon_socket, &readfds)) 
                    {
                        char buff2[1024];
                        int bytes;
                        if ((bytes = recv(daemon_socket,buff2,MAX_LEN,0))<= 0) {
                            //perror("Error reading from telnet socket");
                            break;
                        }
                        if (send(cproxy_socket, buff2, bytes,0) <= 0) {
                            //perror("Error writing to sproxy socket");
                            break;
                        }

                    }
                }
          
            }
            //close sockets
            close(cproxy_socket);
            close(daemon_socket);
        }
        close(sproxy_socket);
}



                                     