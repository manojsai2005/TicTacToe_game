#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 2048

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    socklen_t addr_len = sizeof(serv_addr);

    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "192.168.104.225", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    // Send an initial message to connect to the server
    const char *connect_message = "Connect me to the game";
    sendto(sock, connect_message, strlen(connect_message), 0, (struct sockaddr *)&serv_addr, addr_len);

    // Main game loop
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));

        // Receive board and message from the server
        int bytes_read = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&serv_addr, &addr_len);
        if (bytes_read <= 0)
        {
            printf("\nConnection closed by server.\n");
            break; // Exit the loop if the connection is closed
        }

        // Print the board and any additional messages
        printf("%s", buffer);

        if (strstr(buffer, "Do you want"))
        {
            char response[10];
            scanf("%s", response);
            // printf("%s\n",response);
            sendto(sock, response, strlen(response), 0, (struct sockaddr *)&serv_addr, addr_len);

            // Break the loop if the response is "no"
            if (strcmp(response, "no") == 0)
            {
                printf("Thank you for playing!\n");
                break; // Exit the loop if the player doesn't want to play again
            }

            // Continue to next iteration if they want to play again
            continue;
        }
        if (strstr(buffer, "Your opponent did not"))
        {
            break;
        }
        // Determine if it's this player's turn
        if (strstr(buffer, "Wait for Player"))
        {
            printf("Waiting for the other player to make a move...\n");
            continue; // Wait for the other player's move
        }
        if (strstr(buffer, "your turn") || strstr(buffer, "Invalid move"))
        {
            int row, col;
            printf("Enter row and column (e.g., 1 1 for top-left): ");
            scanf("%d %d", &row, &col);

            // Send move to the server
            char move[10];
            snprintf(move, sizeof(move), "%d %d", row, col);
            sendto(sock, move, strlen(move), 0, (struct sockaddr *)&serv_addr, addr_len);
        }
    }

    // Close the socket
    close(sock);
    return 0;
}
