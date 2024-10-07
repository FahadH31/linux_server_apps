#include <stdio.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>

#define SERVER_TCP_PORT 3000	/* Default port */
#define BUFLEN		256	/* Buffer length */
#define PACKETLENGTH	100	/* Packet length */

int echod(int);			
void reaper(int);

int main(int argc, char **argv)
{
	int 	sd, new_sd, client_len, port;
	struct	sockaddr_in server, client;

	switch(argc){
	case 1:
		port = SERVER_TCP_PORT;
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	/* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	/* queue up to 5 connect requests  */
	listen(sd, 5);

	(void) signal(SIGCHLD, reaper);

	while(1) {
	  client_len = sizeof(client);
	  new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
	  if(new_sd < 0){
	    fprintf(stderr, "Can't accept client \n");
	    exit(1);
	  }
	  switch (fork()){
	  case 0:		/* child */
		(void) close(sd);
		exit(echod(new_sd));
	  default:		/* parent */
		(void) close(new_sd);
		break;
	  case -1:
		fprintf(stderr, "fork: error\n");
	  }
	}
}

/*	echod function	*/
int echod(int sd)
{
    char buf[BUFLEN];       // Buffer to store incoming data
    int n;                  // Number of bytes read

    // Read filename from client
    if ((n = read(sd, buf, BUFLEN)) > 0) {
        buf[n] = '\0';      // Null-terminate buffer content to make it a string

        // Open file with the name received from the client
        FILE *file = fopen(buf, "r"); // Open in read mode
        if (file == NULL) {
            perror("Error opening file");
            write(sd, "Error: Could not open file.\n", 28);
        } 
        else {
            // Get file size
            struct stat file_stat;
            if (stat(buf, &file_stat) == 0) {
                // File size should be a minimum of 100 bytes
                if (file_stat.st_size >= 100) {

                    // Read file in chunks (of length 100 bytes each) and send back to client
                    while ((n = fread(buf, 1, PACKETLENGTH, file)) > 0) {
                        write(sd, buf, n);
                    }
                } 
                else {
                    write(sd, "Error: File size is less than 100 bytes.\n", 41);
                }
            } 
            else {
                write(sd, "Error: Could not retrieve file information.\n", 44);
            }
            fclose(file);
        }
    }

    close(sd);
    return 0;
}


/*	reaper		*/
void	reaper(int sig)
{
	int	status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}
