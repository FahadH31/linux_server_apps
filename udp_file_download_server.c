#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define DATA_SIZE 100
#define PDU_TYPE_FILENAME 'C'
#define PDU_TYPE_DATA 'D'
#define PDU_TYPE_FINAL 'F'
#define PDU_TYPE_ERROR 'E'

void send_file_data(int sock, struct sockaddr_in *client_addr, socklen_t addr_len, FILE *file);

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    char buf[DATA_SIZE + 1];  	// 1 byte for PDU type + 100 bytes for data
    int sock, port = 3000;	// Default port is 3000
    struct stat file_stat;	// To get file size
    socklen_t addr_len = sizeof(client_addr);

    // Handling command line arguments
    switch (argc) {
        case 1:
            break;
        case 2:
            port = atoi(argv[1]);
            break;
        default:
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            exit(1);
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sock);
        exit(1);
    }

    printf("Server is running on port %d...\n", port);

    while (1) {
        memset(buf, 0, sizeof(buf));
        // Receive filename request (C PDU)
        if (recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &addr_len) < 0) {
            perror("recvfrom error");
            continue;
        }

        if (buf[0] == PDU_TYPE_FILENAME) {
            char *filename = buf + 1;
            printf("Client requested file: %s\n", filename);

            FILE *file = fopen(filename, "rb");
            if (file == NULL) {
                // Send ERROR PDU if file doesn't exist
                buf[0] = PDU_TYPE_ERROR;
                strcpy(buf + 1, "File not found");
                sendto(sock, buf, strlen(buf + 1) + 1, 0, (struct sockaddr *)&client_addr, addr_len);
                perror("File not found");
            }
            else {
                // Send file data to the client
                send_file_data(sock, &client_addr, addr_len, file);
                fclose(file);
            }
        }
    }

    close(sock);
    return 0;
}


// Function to send file data
void send_file_data(int sock, struct sockaddr_in *client_addr, socklen_t addr_len, FILE *file) {
    char buf[DATA_SIZE + 1]; 
    size_t nread;

    // Send file in chunks (D PDU)
    while ((nread = fread(buf + 1, 1, DATA_SIZE, file)) > 0) {
        buf[0] = PDU_TYPE_DATA;
        sendto(sock, buf, nread + 1, 0, (struct sockaddr *)client_addr, addr_len);
    }

    // Send final PDU (F PDU)
    buf[0] = PDU_TYPE_FINAL;
    sendto(sock, buf, 1, 0, (struct sockaddr *)client_addr, addr_len);
    printf("File transfer complete.\n");
}
