#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <errno.h>
extern int errno;
#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define MAX 256


void error(const char *msg)
{
    #ifdef DEBUG
    perror(msg);
    #else
    printf("\nEither the server shut down or the other player disconnected.\nGame over.\n");
    #endif 

    exit(0);
}
/* Handles shutdowns */
void shutdown_cli(int sockfd)
{
    printf("\nYou choose to quit! Goodbye.\n");
    close(sockfd);
    exit(EXIT_SUCCESS);
}

/*
 * Socket Read Functions
 */

/* Reads a message from the server socket. */
void recv_msg(int sockfd, char * msg)
{
    /* All messages are 3 bytes. */
    //memset(msg, 0, 3);
    int n = read(sockfd, msg, 3);
    
    if (n < 0 || n != 3) /* Not what we were expecting. Server got killed or the other client disconnected. */ 
        error("ERROR reading message from server socket.");

    //printf("[DEBUG] Received message: %s\n", msg);
}

/* Reads a message from the server socket. */
char *recv_name(int sockfd)
{
    int length;
    char *msg;
    char *msg2;
    char *msg3;
    char *msg4;
    msg = (char*) malloc (7);
    msg2 = (char*) malloc (6);
    msg3 = (char*) malloc (5);
    msg4 = (char*) malloc (4);
    
    
    /* All messages are 7 or less bytes. */
    int n = read(sockfd, msg, sizeof(msg));
    //printf("%d", n);
   // printf("\n[DEBUG] Received message: %s\n", msg);
    
    
    if (msg[5] == '\0'){
        msg4 = msg;
        return msg4;
    }
    else if (msg[6] == '\0'){
        msg3 = msg;
        return msg3;
    }
    else if (msg[7] == '\0'){
        msg2 = msg;
        return msg2;
    }
    else {
        printf("Message Not of correct character size");
        return msg;
    }
    
}

/* Reads an int from the server socket. */
int recv_int(int sockfd)
{
    int msg = 0;
    int n = read(sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int)) 
        error("ERROR reading int from server socket");
    
    //printf("[DEBUG] Received int: %d\n", msg);
    
    return msg;
}

/*
 * Socket Write Functions
 */

/* Writes an int to the server socket. */
void write_server_int(int sockfd, int msg)
{
    int n = write(sockfd, &msg, sizeof(msg));
    if (n < 0)
        printf( "Error Value is : %s\n", strerror(errno) );
    
    //printf("[DEBUG] Wrote int to server: %d\n", msg);
}

/* Writes a message to a client socket. */
void write_server_msg(int sockfd, char *msg)
{
    int n = write(sockfd, msg, 7);
    if (n < 0){
        error("ERROR writing msg to client socket");
    }
    
}

/*
 * Connect Functions
 */

/* Sets up the connection to the server. */
int connect_to_server(char * hostname, int portno)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
 
    /* Get a socket. */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
    if (sockfd < 0) 
        error("ERROR opening socket for server.");
	
    /* Get the address of the server. */
    server = gethostbyname(hostname);
	
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
	
	/* Zero out memory for server info. */
	memset(&serv_addr, 0, sizeof(serv_addr));

	/* Set up the server info. */
    serv_addr.sin_family = AF_INET;
    memmove(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno); 

	/* Make the connection. */
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting to server");

    //printf("[DEBUG] Connected to server.\n");
    
    return sockfd;
}

/*
 * Game Functions
 */

/* Draws the game board to stdout. */
void draw_board(char board[][9], int remainingMines)
{
    printf("Remaining Mines: %d\n", remainingMines);
    printf("    1 2 3 4 5 6 7 8 9 \n");
    printf("--------------------- \n");
    printf("A | %c %c %c %c %c %c %c %c %c \n", board[0][0], board[0][1], board[0][2], board[0][3], board[0][4], board[0][5], board[0][6], board[0][7], board[0][8]);
    printf("B | %c %c %c %c %c %c %c %c %c \n", board[1][0], board[1][1], board[1][2], board[1][3], board[1][4], board[1][5], board[1][6], board[1][7], board[1][8]);
    printf("C | %c %c %c %c %c %c %c %c %c \n", board[2][0], board[2][1], board[2][2], board[2][3], board[2][4], board[2][5], board[2][6], board[2][7], board[2][8]);
    printf("D | %c %c %c %c %c %c %c %c %c \n", board[3][0], board[3][1], board[3][2], board[3][3], board[3][4], board[3][5], board[3][6], board[3][7], board[3][8]);
    printf("E | %c %c %c %c %c %c %c %c %c \n", board[4][0], board[4][1], board[4][2], board[4][3], board[4][4], board[4][5], board[4][6], board[4][7], board[4][8]);
    printf("F | %c %c %c %c %c %c %c %c %c \n", board[5][0], board[5][1], board[5][2], board[5][3], board[5][4], board[5][5], board[5][6], board[5][7], board[5][8]);
    printf("G | %c %c %c %c %c %c %c %c %c \n", board[6][0], board[6][1], board[6][2], board[6][3], board[6][4], board[6][5], board[6][6], board[6][7], board[6][8]);
    printf("H | %c %c %c %c %c %c %c %c %c \n", board[7][0], board[7][1], board[7][2], board[7][3], board[7][4], board[7][5], board[7][6], board[7][7], board[7][8]);
    printf("I | %c %c %c %c %c %c %c %c %c \n", board[8][0], board[8][1], board[8][2], board[8][3], board[8][4], board[8][5], board[8][6], board[8][7], board[8][8]);
}

/* Screen display CSS */
void space_screen(){
    printf("\n\n\n\n");
    printf("Going back to Main Menu!");
}

/* Draws Menu Screen */
int menu_screen(int sockfd){
    int option;
    printf("\nPlease Enter a selection:\n");
    printf("<1> Play Minesweeper\n");
    printf("<2> Show Leaderboard\n");
    printf("<3> Quit\n");
    printf("Selection option (1-3): ");
    while (1){
        char buffer[1];
        scanf("%s", buffer);
        
        if (strcmp(buffer, "1") == 0){
            option = 1;
            break;
        }
        else if (strcmp(buffer, "2") == 0){
            option = 2;
            break;
        }
        else if (strcmp(buffer, "3") == 0){
            option = 3;
            break;
        }
        else {
            printf("Please Enter a Vaild Menu Option (1-3):");
        }
        
    }
    if (option == 1 || option == 2 || option == 3){
        write_server_int(sockfd, option);
        return option;
    }
    else {
        return -1;
    }
}

/* Get's the players turn and sends it to the server. */
int take_turn_member(int sockfd)
{
    char buffer[1];
    printf("Choose an Option:\n");
    printf("<R> Reveal Tile\n");
    printf("<P> Place Flag\n");
    printf("<Q> Quit Game\n");
    printf("Option (R,P,Q): ");
    while (1){
        scanf("%s", buffer);
        if (strcmp(buffer, "R") == 0){
            /* Send players move to the server. */
            return 0;
            break;
        }
        else if (strcmp(buffer, "P") == 0){
            /* Send players move to the server. */
            return 1;
            break;
        }
        else if (strcmp(buffer, "Q") == 0){
            return 2; // Return to main menu
            break;
        }
        else {
            printf("\nPlease Enter a Vaild Option (R,P,Q):");
        }
    }
    
}

/* Grab x input */
int take_turn_coord_x(){
    char buffer[1];
    while (1){
        printf("Enter Row Tile Coordinates A-I: ");
        scanf("%s", buffer);
        if (strcmp(buffer, "A") == 0){
            return 0;
            break;
        }
        else if (strcmp(buffer, "B") == 0){
            return 1;
            break;
        }
        else if (strcmp(buffer, "C") == 0){
            return 2;
            break;
        }
        else if (strcmp(buffer, "D") == 0){
            return 3;
            break;
        }
        else if (strcmp(buffer, "E") == 0){
            return 4;
            break;
        }
        else if (strcmp(buffer, "F") == 0){
            return 5;
            break;
        }
        else if (strcmp(buffer, "G") == 0){
            return 6;
            break;
        }
        else if (strcmp(buffer, "H") == 0){
            return 7;
            break;
        }
        else if (strcmp(buffer, "I") == 0){
            return 8;
            break;
        }
        else {
            printf("Please enter a valid choice!\n");
        }
    }
}

/* Grab Y input */
int take_turn_coord_y(){
    int coords = 0;
    while (1){
        char buffer[1];
        printf("Enter Column Tile Coordinates 1-9: ");
        scanf("%s", buffer);
        if (strcmp(buffer, "1") == 0){
            coords = 1;
            break;
        }
        else if (strcmp(buffer, "2") == 0){
            coords = 2;
            break;
        }
        else if (strcmp(buffer, "3") == 0){
            coords = 3;
            break;
        }
        else if (strcmp(buffer, "4") == 0){
            coords = 4;
            break;
        }
        else if (strcmp(buffer, "5") == 0){
            coords = 5;
            break;
        }
        else if (strcmp(buffer, "6") == 0){
            coords = 6;
            break;
        }
        else if (strcmp(buffer, "7") == 0){
            coords = 7;
            break;
        }
        else if (strcmp(buffer, "8") == 0){
            coords = 8;
            break;
        }
        else if (strcmp(buffer, "9") == 0){
            coords = 9;
            break;
        }
        else {
            printf("Please enter a vaild choice!:\n");
        }
        
    }
    coords = coords -1; // Must take off last number as array goes from 0-8 not 1-9
    return coords;
}

/* Shows the leaderboard */
void show_leaderboard(int sockfd){
    printf("\n\n==================================================================== \n\n");
    int numer_of_players = recv_int(sockfd);
    int games_won_pass = recv_int(sockfd);
    
//    /* Recieve Names for loop */
//
//    for (int i = 0; i < numer_of_players; i++){
//        char *name = NULL;
//        name = recv_name(sockfd);
//
//        printf("This is the names recieved: %s\n", name);
//    }
    
    for (int i = 0; i < numer_of_players; i++){
        int games_played = recv_int(sockfd);
        int games_won = recv_int(sockfd);
        int time = recv_int(sockfd);
        if (games_won_pass == 0){
            printf("There is no information currently stored in the leaderboard. Try again later.\n");
        }
        else{
            printf("          %d seconds       %d games won, %d games played\n", time, games_won, games_played);
        }
    }
    printf("\n===================================================================\n\n");
}

/* Recieve the entire board */
void recieve_board(char board[][9], int sockfd){
    for (int i =0; i < NUM_TILES_X; i++){
        for (int j =0; j < NUM_TILES_Y; j++){
            int identity = recv_int(sockfd);
            char identifier;
            if (identity == 10){ // 10 is to pass an int and convert to + flag symbol
                identifier = '+';
            }
            else {
                if (identity == 0){
                    identifier = '0';
                }
                if (identity == 1){
                    identifier = '1';
                }
                if (identity == 2){
                    identifier = '2';
                }
                if (identity == 3){
                    identifier = '3';
                }
                if (identity == 4){
                    identifier = '4';
                }
                if (identity == 5){
                    identifier = '5';
                }
                if (identity == 12){ // Mine
                    identifier = '*';
                }
                if (identity == 13){ // Error Tile
                    identifier = '$';
                }
                
            }
            board[i][j] = identifier;
        }
    }
}

/* Rest the board */
void reset_board(char board[][9]){
    for (int i =0; i < NUM_TILES_X; i++){
        for (int j =0; j < NUM_TILES_Y; j++){
            board[i][j] = ' ';
        }
    }
}

/*
 * Main Program
 */
void INThandler(int);

int main(int argc, char *argv[])
{
    /* Make sure host and port are specified. */
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    
    
    /* Connect to the server. */
    int sockfd = connect_to_server(argv[1], atoi(argv[2]));
    /* The client Player number is the first thing we receive after connecting. */
    int id = recv_int(sockfd);

    //printf("[DEBUG] Client ID: %d\n", id);

    char msg[3];
    char board[9][9] = { {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, /* Game board */
                         {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
                         {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
                         {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
                         {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
                         {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
                         {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
                         {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
                         {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}};

    printf("\n\n===============================================\n");
    printf("Welcome to the online Minesweeper gaming system\n");
    printf("===============================================\n");
    printf("\nYou are required to log on with your registered username and password.\n");
    

    /* Wait for the Authentication to start. */
    bool aut_complete = false;
    while (!aut_complete) {
        recv_msg(sockfd, msg);
        char username[7], password[7];
        if (!strcmp(msg, "USN")){
            printf("\nUsername: ");
            scanf("%s", username);
            write_server_msg(sockfd, username);
            bool user = true;
        }

        if (!strcmp(msg, "PSW")){
            printf("\nPassword: ");
            scanf("%s", password);
            write_server_msg(sockfd, password);
            bool pass = true;
        }
        
        if (!strcmp(msg, "APV")){
            printf("\nYou have been authenticated!\n");
            aut_complete = true;
        }
        
        if (!strcmp(msg, "ERR")){
            printf("You entered either an incorrect username or password. Disconnecting\n");
            close(sockfd);
        }
    }
    

    /* The game has begun. */
    int option;
    //printf("\nWelcome to the online Minesweeper gaming system\n\n");
    while(1) {
        option = menu_screen(sockfd); // Get Menu Screen and Clients Choosen Option
        //printf("[DEBUG] User selection menu option: %d\n", option);
        reset_board(board);
        int remainingMines = 10;
        while (option == 1){ // Gameplay Option
            draw_board(board, remainingMines);
            recv_msg(sockfd, msg);
            int choice;
            int coordinates_x;
            int coordinates_y;
            if (!strcmp(msg, "TRN")) { /* Take a turn. */
                choice = take_turn_member(sockfd);
                if (choice == 2){ // Go back to main menu 
                    write_server_int(sockfd, choice);
                    break;
                }
                else {
                    write_server_int(sockfd, choice);
                }
            }
            if (!strcmp(msg, "COX")){
                coordinates_x = take_turn_coord_x();
                //printf("[DEBUG] Coordinates X is: %d\n", coordinates_x);
                write_server_int(sockfd, coordinates_x);
            }
            if (!strcmp(msg, "COY")){
                coordinates_y = take_turn_coord_y();
                //printf("[DEBUG] Coordinates Y is: %d\n", coordinates_y);
                write_server_int(sockfd, coordinates_y);
            }
            if (!strcmp(msg, "UPD")){
                char identifier;
                int identity = recv_int(sockfd);
                if (identity == 10){ // 10 is to pass an int and convert to + flag symbol
                    identifier = '+';
                }
                else {
                    if (identity == 0){
                        identifier = '0';
                    }
                    if (identity == 1){
                        identifier = '1';
                    }
                    if (identity == 2){
                        identifier = '2';
                    }
                    if (identity == 3){
                        identifier = '3';
                    }
                    if (identity == 4){
                        identifier = '4';
                    }
                    if (identity == 5){
                        identifier = '5';
                    }
                    if (identity == 12){ // Mine
                        identifier = '*';
                    }
                    if (identity == 13){ // Error Tile
                        identifier = '$';
                    }
                    
                }
                remainingMines = recv_int(sockfd);
                int more = recv_int(sockfd);
                if (more == 99){ // More tiles to reveal
                    
                    int reveal_tiles_amount = recv_int(sockfd); //Number of tiles to reveal
                    for (int i = 0; i < reveal_tiles_amount; i++){
                        int tile_x = recv_int(sockfd);
                        int tile_y = recv_int(sockfd);
                        int tile_object = recv_int(sockfd);
                        char tile_object_identifier;
                        if (tile_object == 10){ // 10 is to pass an int and convert to + flag symbol
                            tile_object_identifier = '+';
                        }
                        else {
                            if (tile_object == 0){
                                tile_object_identifier = '0';
                            }
                            if (tile_object == 1){
                                tile_object_identifier = '1';
                            }
                            if (tile_object == 2){
                                tile_object_identifier = '2';
                            }
                            if (tile_object == 3){
                                tile_object_identifier = '3';
                            }
                            if (tile_object == 4){
                                tile_object_identifier = '4';
                            }
                            if (tile_object == 5){
                                tile_object_identifier = '5';
                            }
                            if (tile_object == 12){ // Mine
                                tile_object_identifier = '*';
                            }
                            if (tile_object == 13){ // Error Tile
                                tile_object_identifier = '$';
                            }
                        }
                        board[tile_x][tile_y] = tile_object_identifier;
                    }
                }
                board[coordinates_x][coordinates_y] = identifier; // The users original selection
            }
            
            if (!strcmp(msg, "WIN")){ // Win Game
                int time_taken = recv_int(sockfd);
                recieve_board(board, sockfd);
                printf("\n\n\n\n");
                draw_board(board, remainingMines);
                printf("Congratulations! You have located all the mines. You won in: %d Seconds", time_taken);
                space_screen();
                break;
            }
            
            if (!strcmp(msg, "LOS")){ // Lost Game
                recieve_board(board, sockfd);
                printf("\n\n\n\n");
                draw_board(board, remainingMines);
                printf("Game Over! You Hit A Mine.\n");
                space_screen();
                break;
            }
            
            if (!strcmp(msg, "ERR")){ // Tile already revealed
                printf("TILE ALREADY REVEALED OR FLAGGED PICK AGAIN!\n");
            }
            
        }
        while (option == 2){ // Leaderboard Option
            show_leaderboard(sockfd);
            printf("Going Back To Menu!\n");
            break;
        }
        if (option == 3) {
            break; // Takes you outside the while loop when the game is ended
        }
    }
    
    shutdown_cli(sockfd);
    return 0;
}

