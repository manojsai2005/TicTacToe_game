#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define ROWS 3
#define COLS 3

char board[ROWS][COLS];
int player_turn = 1; // 1 for Player 1 (X), 2 for Player 2 (O)

// Function to initialize the game board
void init_board()
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            board[i][j] = ' ';
        }
    }
}

// Function to create the board state string
void create_board_string(char *buffer)
{
    snprintf(buffer, 2048, "Current board:\n");
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            snprintf(buffer + strlen(buffer), 2048 - strlen(buffer), " %c ", board[i][j]);
            if (j < COLS - 1)
                strcat(buffer, "|");
        }
        strcat(buffer, "\n");
        if (i < ROWS - 1)
            strcat(buffer, "---+---+---\n");
    }
}

// Function to check if a player has won the game
int check_winner()
{
    for (int i = 0; i < ROWS; i++)
    {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
        {
            return player_turn;
        }
    }
    for (int j = 0; j < COLS; j++)
    {
        if (board[0][j] == board[1][j] && board[1][j] == board[2][j] && board[0][j] != ' ')
        {
            return player_turn;
        }
    }
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ')
    {
        return player_turn;
    }
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
    {
        return player_turn;
    }
    return 0;
}

// Function to check if the board is full (for a draw)
int is_draw()
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            if (board[i][j] == ' ')
            {
                return 0;
            }
        }
    }
    return 1;
}

int main()
{
    int server_fd;
    struct sockaddr_in address, player_addr[2];
    int addrlen = sizeof(address);
    char buffer[2048] = {0};
    socklen_t player_addr_len[2] = {sizeof(player_addr[0]), sizeof(player_addr[1])};

    // Create server socket
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("192.168.104.225");
    address.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    // Accept connections from two clients (UDP, we receive their first message to identify them)
    for (int i = 0; i < 2; i++)
    {
        recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&player_addr[i], &player_addr_len[i]);
        printf("Player %d connected\n", i + 1);
        snprintf(buffer, sizeof(buffer), "You are Player %d (%c)\n", i + 1, (i == 0) ? 'X' : 'O');
        sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&player_addr[i], player_addr_len[i]);
    }

    // Game loop
    int play_again = 1;
    int game_over = 0;
    while (play_again == 1)
    {
        init_board();
        player_turn = 1;
        int sh = 1;

        while (1)
        {
            memset(buffer, 0, sizeof(buffer));
            int row, col, winner = 0;
            char move[10];

            // Inform players whose turn it is
            if (sh == 1)
            {
                snprintf(buffer, sizeof(buffer), "It's your turn\n");
                sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&player_addr[player_turn - 1], player_addr_len[player_turn - 1]);
                snprintf(buffer, sizeof(buffer), "Wait for Player %d's turn...\n", player_turn);
                sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&player_addr[2 - player_turn], player_addr_len[2 - player_turn]);
            }
            else
            {
                sh = 0;
            }
            // Receive move from current player
            recvfrom(server_fd, move, sizeof(move), 0, (struct sockaddr *)&player_addr[player_turn - 1], &player_addr_len[player_turn - 1]);
            sscanf(move, "%d %d", &row, &col);

            // Adjust input to array indexing (1-based to 0-based)
            row--;
            col--;

            // Validate move and update the board
            if (row >= 0 && row < ROWS && col >= 0 && col < COLS && board[row][col] == ' ')
            {
                sh = 1;
                board[row][col] = (player_turn == 1) ? 'X' : 'O';

                // Check if current player has won
                winner = check_winner();
                if (winner)
                {
                    game_over = 1;
                    snprintf(buffer, sizeof(buffer), "Player %d Wins!\n", winner);
                    for (int i = 0; i < 2; i++)
                    {
                        sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&player_addr[i], player_addr_len[i]);
                    }
                    break;
                }

                // Check for draw
                if (is_draw())
                {
                    snprintf(buffer, sizeof(buffer), "It's a Draw!\n");
                    game_over = 1;
                    for (int i = 0; i < 2; i++)
                    {
                        sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&player_addr[i], player_addr_len[i]);
                    }
                    break;
                }

                // Send updated board state to both players
                create_board_string(buffer);
                for (int i = 0; i < 2; i++)
                {
                    sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&player_addr[i], player_addr_len[i]);
                }

                // Switch player turn
                if (player_turn == 1)
                {
                    player_turn = 2;
                }
                else if (player_turn == 2)
                {
                    player_turn = 1;
                }
            }
            else
            {
                sh = 0;
                snprintf(buffer, sizeof(buffer), "Invalid move, try again.\n");
                sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&player_addr[player_turn - 1], player_addr_len[player_turn - 1]);
            }
        }

        if (game_over)
        {
            for (int i = 0; i < 2; i++)
            {
                sendto(server_fd, "Do you want to play again? (yes/no)\n", strlen("Do you want to play again? (yes/no)\n"), 0, (struct sockaddr *)&player_addr[i], player_addr_len[i]);
            }
            char response[10], response1[10];
            recvfrom(server_fd, response, sizeof(response), 0, (struct sockaddr *)&player_addr[0], &player_addr_len[0]);
            recvfrom(server_fd, response1, sizeof(response1), 0, (struct sockaddr *)&player_addr[1], &player_addr_len[1]);
            printf("%s\n", response);
            printf("%s\n", response1);
            if (strstr(response, "yes") != NULL && strstr(response1, "yes") != NULL)
            {
                play_again = 1;
            }
            else
            {
                play_again = 0;
                if (strstr(response, "yes") != NULL)
                {
                    sendto(server_fd, "Your opponent did not wish to play again.\n", strlen("Your opponent did not wish to play again.\n"), 0, (struct sockaddr *)&player_addr[0], player_addr_len[0]);
                }
                if (strstr(response1, "yes") != NULL)
                {
                    sendto(server_fd, "Your opponent did not wish to play again.\n", strlen("Your opponent did not wish to play again.\n"), 0, (struct sockaddr *)&player_addr[1], player_addr_len[1]);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
