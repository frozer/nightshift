#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Function to convert hex string to byte array
void hexStringToByteArray(const char* hexString, unsigned char* byteArray, size_t* byteArrayLength) {
    size_t hexLength = strlen(hexString);
    *byteArrayLength = hexLength / 2;
    for (size_t i = 0; i < *byteArrayLength; i++) {
        sscanf(hexString + 2*i, "%2hhx", &byteArray[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hex_string>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* hexString = argv[1];
    size_t byteArrayLength;
    unsigned char byteArray[1024];  // Buffer to hold the byte array

    hexStringToByteArray(hexString, byteArray, &byteArrayLength);

    int sockfd;
    struct sockaddr_in serverAddr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return EXIT_FAILURE;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(1111);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return EXIT_FAILURE;
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        return EXIT_FAILURE;
    }

    // Send the byte array to the server
    send(sockfd, byteArray, byteArrayLength, 0);
    printf("Data sent to server\n");

    // Close the socket
    close(sockfd);

    return EXIT_SUCCESS;
}
