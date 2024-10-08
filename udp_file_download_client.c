/* udp_file_client.c - main */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>

#define DATA_SIZE 100
#define PDU_TYPE_FILENAME 'C'
#define PDU_TYPE_DATA 'D'
#define PDU_TYPE_FINAL 'F'
#define PDU_TYPE_ERROR 'E'

void receive_file_data(int sock, const char *filename);

int main(int argc, char **argv) {
    char *host = "localhost";
    int port = 3000;
    struct sockaddr_in server_addr;
    struct hostent *phe;
    int sock;
    char filename[256];

    // Command line argument for host and port
    switch (argc) {
        case 1:
            break;
        case 2:
            host = argv[1];
            break;
        case 3:
            host = argv[1];
            port = atoi(argv[2]);
            break;
        default:
            fprintf(stderr, "Usage: %s [host [port]]\n", argv[0]);
            exit(1);
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Resolve host
    if ((phe = gethostbyname(host)) != NULL) {
        memcpy(&server_addr.sin_addr, phe->h_addr, phe->h_length);
    } else if ((server_addr.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        perror("Invalid host");
        exit(1);
    }

    // Main loop to request files or quit
    while (1) {
        printf("Enter the filename to download (or 'quit' to exit): ");
        fgets(filename, sizeof(filename), stdin);
        filename[strcspn(filename, "\n")] = 0;  // Remove newline

        if (strcmp(filename, "quit") == 0) {
            break;
        }

        // Send FILENAME PDU
        char buf[DATA_SIZE + 1];
        buf[0] = PDU_TYPE_FILENAME;
        strncpy(buf + 1, filename, DATA_SIZE);
        sendto(sock, buf, strlen(filename) + 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        // Receive file data
        receive_file_data(sock, filename);
    }

    close(sock);
    return 0;
}

void receive_file_data(int sock, const char *filename) {
    char buf[DATA_SIZE + 1];
    ssize_t n;
    FILE *file = fopen(filename, "wb");

    if (file == NULL) {
        perror("Error opening file for writing");
        return;
    }

    // Receive file data
    while ((n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL)) > 0) {
        if (buf[0] == PDU_TYPE_ERROR) {
            printf("Server error: %s\n", buf + 1);
            fclose(file);
            remove(filename);  // Delete incomplete file
            return;
        } else if (buf[0] == PDU_TYPE_FINAL) {
            printf("File download complete.\n");
            break;
        } else if (buf[0] == PDU_TYPE_DATA) {
            fwrite(buf + 1, 1, n - 1, file);
        }
    }

    fclose(file);
}

