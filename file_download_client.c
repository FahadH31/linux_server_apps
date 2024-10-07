#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SERVER_TCP_PORT 3000    /* Default port */
#define BUFLEN        256       /* Buffer length */
#define ERROR_PREFIX  "Error:"  /* Error message prefix */

int main(int argc, char **argv)
{
    int n, sd, port;
    struct hostent *hp;
    struct sockaddr_in server;
    char *host, rbuf[BUFLEN], sbuf[BUFLEN];
    FILE *file;

    switch (argc) {
        case 2:
            host = argv[1];
            port = SERVER_TCP_PORT;
            break;
        case 3:
            host = argv[1];
            port = atoi(argv[2]);
            break;
        default:
            fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
            exit(1);
    }

    /* Create a stream socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Can't create a socket\n");
        exit(1);
    }

    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if ((hp = gethostbyname(host)) != NULL) {
        bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
    } else if (inet_aton(host, (struct in_addr *)&server.sin_addr) == 0) {
        fprintf(stderr, "Can't get server's address\n");
        exit(1);
    }

    /* Connecting to the server */
    if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        fprintf(stderr, "Can't connect\n");
        exit(1);
    }

    // Get the filename to download from the user
    printf("Enter the filename to download: ");
    fgets(sbuf, BUFLEN, stdin);  // Get filename from user
    sbuf[strcspn(sbuf, "\n")] = 0;  // Remove the newline character

    // Send filename to the server
    write(sd, sbuf, strlen(sbuf));

    // Read the first chunk of data from the server
    if ((n = read(sd, rbuf, BUFLEN)) > 0) {
        // Check if the response is an error message by checking for "Error:" prefix
        if (strncmp(rbuf, ERROR_PREFIX, strlen(ERROR_PREFIX)) == 0) {
            write(1, rbuf, n);  // Print error to terminal
            close(sd);          
            return 1;           
        }
    }

    // If no error, proceed to ask for local file to save the content
    printf("Enter the local filename to save the downloaded file: ");
    char local_filename[BUFLEN];
    fgets(local_filename, BUFLEN, stdin);
    local_filename[strcspn(local_filename, "\n")] = 0;  // Remove newline character

    file = fopen(local_filename, "w");  // Open the file in write mode
    if (file == NULL) {
        perror("Error opening file for writing");
        close(sd);
        exit(1);
    }

    // Write the first chunk of valid data to the file
    fwrite(rbuf, 1, n, file);

    // Read the remaining data from the server and write it to the file
    while ((n = read(sd, rbuf, BUFLEN)) > 0) {
        fwrite(rbuf, 1, n, file);  // Write data to file
    }

    if (n < 0) {
        fprintf(stderr, "Error reading from server\n");
    }

    fclose(file);
    close(sd);
    printf("File downloaded successfully and saved as '%s'\n", local_filename);
    return 0;
}

