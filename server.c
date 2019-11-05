#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include<sys/socket.h>


#define RANDOM_NUMBER_SEED 42
#define NUM_TILES_X 9
#define NUM_TILES_Y 9
#define NUM_MINES 10
#define MAX 256
int num_conn = 0;


typedef struct
{ 
    int adjacent_mines;
    bool revealed;
    bool is_mine;
    bool flagged;
    char identifier;
} Tile;

struct GameState
{
    Tile tiles[NUM_TILES_X][NUM_TILES_Y];
};
typedef struct GameState GameState;

typedef struct
{
    int games_played;
    int games_won;
    int time;
    bool player;
    char *name;
} Client;

struct Leaderboard
{
    Client clients[MAX];
};
typedef struct Leaderboard Leaderboard;
Leaderboard leaderboard;

int player_count = 0;
pthread_mutex_t mutexcount;

pthread_mutex_t mutexboard;
pthread_mutex_t mutexrand;

void error(const char *msg)
{
    perror(msg);
    pthread_exit(NULL);
}

/* Reads a message from the server socket. */
char *recv_msg(int sockfd)
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
    //printf("\n[DEBUG] Received message: %s\n", msg);


    if (msg[5] == '\0'){
        printf("5 Characters");
        msg4 = msg;
        return msg4;
    }
    else if (msg[6] == '\0'){
        printf("6 Characters");
        msg3 = msg;
        return msg3;
    }
    else if (msg[7] == '\0'){
        printf("7 Characters");
        msg2 = msg;
        return msg2;
    }
    else {
        printf("Message Not of correct character size");
        return msg;
    }
    
}

/* Authenticates the user */
bool authenticate_login(char *username, char *passwords)
{
    FILE *file;
    char *file_username;
    char *file_password;
    file_username = (char*) malloc (7);
    file_password = (char*) malloc (6);
    bool match;
    
    if ((file = fopen("Authentication.txt", "r")) == NULL) {
        perror("fopen");
        return false;
    }
    
    while (fgetc(file) != '\n'); /* Move past the column titles in the file. */

    /* Each line in the file contains the username and password separated by a white space. */
    while (fscanf(file, "%s %s\n", file_username, file_password) > 0) {
        /* Check if the username matches one in the file, and if the password matches for that username. */
        if (strcmp(file_username, username) == 0 && strcmp(file_password, passwords) == 0) {
            match = true;
            break;
        }
    }
    
    //printf("\n[DEBUG] Returned: %d", match);
    
    fclose(file);
    return match;
}

/* Authenticates the user */ 
bool authenticate_process(int cli_sockfd){
    
    
    /* Authentication Process */
    write(cli_sockfd, "USN", 3);
    char *username;
    username = recv_msg(cli_sockfd);
    printf("[DEBUG] Client username is %s.\n", username);
    
    write(cli_sockfd, "PSW", 3);
    char *password;
    password = recv_msg(cli_sockfd);
    printf("[DEBUG] Client Password is %s.\n", password);
    
    bool approval = authenticate_login(username, password);

    if (approval == 1){
        write(cli_sockfd, "APV", 3);
        pthread_mutex_lock(&mutexboard);
        leaderboard.clients[cli_sockfd].player = true;
        leaderboard.clients[cli_sockfd].name = username;
        pthread_mutex_unlock(&mutexboard);
        return true;
    }
    else {
        write(cli_sockfd, "ERR", 3);
        return false;
    }
}



/*
 * Socket Read Functions
 */

/* Reads an int from a client socket. */
int recv_int(int cli_sockfd)
{
    int msg = 0;
    int n = read(cli_sockfd, &msg, sizeof(msg));

    if (n < 0 || n != sizeof(int)){
        
        fflush(stdout);
    } /* Not what we were expecting. Client likely disconnected. */
    
    
   // printf("[DEBUG] Received int: %d\n", msg);
    
    return msg;
}

/*
 * Socket Write Functions
 */

/* Writes a message to a client socket. */
void write_client_msg(int cli_sockfd, char * msg)
{
    int n = write(cli_sockfd, msg, strlen(msg));
    if (n < 0)
        error("ERROR writing msg to client socket");
    //printf("[DEBUG] Message sent to client %s", msg);

}

/* Writes a message to a client socket. */
void write_client_name(int sockfd, char *msg)
{
    int n = write(sockfd, msg, strlen(msg));
    if (n < 0){
        error("ERROR writing msg to client socket");
    }
    
}

/* Writes an int to a client socket. */
void write_client_int(int cli_sockfd, int msg)
{
    int n = write(cli_sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to client socket");
}

/*
 * Game Functions
 */
/* If a tile contains a mine it returns true*/
bool tile_contains_mine(int x, int y, GameState *tileInfo){
    
    if (tileInfo->tiles[x][y].is_mine){
        return true;
    }
    else {
        return false;
    }
}

/* Place the mines upon the random 42 seed algorithm */
void place_mines(GameState *tileInfo){
    for (int i =0; i < NUM_MINES; i++){
        int x,y;
        do {
            pthread_mutex_lock(&mutexrand);
            x = rand() % NUM_TILES_X;
            y = rand() % NUM_TILES_Y;
            pthread_mutex_unlock(&mutexrand);

        }while(tile_contains_mine(x,y, tileInfo));
        //place mine at (x,y)
        tileInfo->tiles[x][y].is_mine = true;
        tileInfo->tiles[x][y].identifier = '*';
    }
}

/* Intialize all the tiles for a new game */
void intialize_tiles(GameState *tileInfo){
    for (int i =0; i < NUM_TILES_X; i++){
        for (int j =0; j < NUM_TILES_Y; j++){
            tileInfo->tiles[i][j].is_mine = false;
            tileInfo->tiles[i][j].flagged = false;
            tileInfo->tiles[i][j].adjacent_mines = 0;
            tileInfo->tiles[i][j].revealed = false;
        }
    }
}

/* Check how man mines are remaining */
int mines_remain(GameState *tileInfo){
    int num_mines_found = 0;
    for (int i =0; i < NUM_TILES_X; i++){
        for (int j =0; j < NUM_TILES_Y; j++){
            if (tileInfo->tiles[i][j].is_mine && tileInfo->tiles[i][j].flagged){
                num_mines_found++;
            }
        }
    }
    return NUM_MINES - num_mines_found;
}

/* Check Adjacent and set */
void check_adj(GameState *tileInfo){
    for (int i =0; i < NUM_TILES_X; i++){
        for (int j =0; j < NUM_TILES_Y; j++){
            if (tileInfo->tiles[i][j].is_mine){
                if (!((i - 1) < 0)){
                    tileInfo->tiles[i-1][j].adjacent_mines++;
                }
                if (!((i + 1) > NUM_TILES_X)){
                    tileInfo->tiles[i+1][j].adjacent_mines++;
                }
                if (!((j - 1) < 0)){
                    tileInfo->tiles[i][j-1].adjacent_mines++;
                }
                if (!((j + 1) > NUM_TILES_Y)){
                    tileInfo->tiles[i][j+1].adjacent_mines++;
                }
                if (!((i - 1) < 0) && !((j - 1) < 0)){
                    tileInfo->tiles[i-1][j-1].adjacent_mines++;
                }
                if (!((i - 1) < 0) && !((j + 1) > NUM_TILES_Y)){
                    tileInfo->tiles[i-1][j+1].adjacent_mines++;
                }
                if (!((i + 1) > NUM_TILES_X) && !((j - 1) < 0)){
                    tileInfo->tiles[i+1][j-1].adjacent_mines++;
                }
                if (!((i + 1) > NUM_TILES_X) && !((j + 1) > NUM_TILES_Y)){
                    tileInfo->tiles[i+1][j+1].adjacent_mines++;
                }
            }
        }
    }
}

/* Draws the board */
void build_board(char board[][9], GameState *tileInfo){
    for (int i =0; i < NUM_TILES_X; i++){
        for (int j =0; j < NUM_TILES_Y; j++){
            
            // INT NOT CONVERTING TO CHAR HERE
            if (!tileInfo->tiles[i][j].is_mine){
                char converter;
                if (tileInfo->tiles[i][j].adjacent_mines == 0){
                    converter = '0';
                }
                if (tileInfo->tiles[i][j].adjacent_mines == 1){
                    converter = '1';
                }
                if (tileInfo->tiles[i][j].adjacent_mines == 2){
                    converter = '2';
                }
                if (tileInfo->tiles[i][j].adjacent_mines == 3){
                    converter = '3';
                }
                if (tileInfo->tiles[i][j].adjacent_mines == 4){
                    converter = '4';
                }
                if (tileInfo->tiles[i][j].adjacent_mines == 5){
                    converter = '5';
                }
                tileInfo->tiles[i][j].identifier = converter;
             }
        }
    }
}

/* Updates the board */
void update_board(char board[][9], GameState *tileInfo){
    for (int i =0; i < NUM_TILES_X; i++){
        for (int j =0; j < NUM_TILES_Y; j++){
            // Print the board identifiers
            board[i][j] = tileInfo->tiles[i][j].identifier;
        }
    }
}



/* Draws the game board to stdout. */
void draw_board(char board[][9], GameState *tileInfo)
{
    int remainingMines = mines_remain(tileInfo);
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


/* Get users choice to place or reveal tile */
int place_reveal(int cli_sockfd){
    write_client_msg(cli_sockfd, "TRN");
    int place_reveals = 0;
    place_reveals = recv_int(cli_sockfd);
    return place_reveals;
}

/* Get users x coords */
int get_cords_x(int cli_sockfd){
    write_client_msg(cli_sockfd, "COX"); // Grab x position
    int user_cords_x = 0;
    user_cords_x = recv_int(cli_sockfd);
    return user_cords_x;
}

/* Get users y coords */
int get_cords_y(int cli_sockfd){
    write_client_msg(cli_sockfd, "COY"); // Grab y position
    int user_cords_y = 0;
    user_cords_y = recv_int(cli_sockfd);
    return user_cords_y;
}

/* Place Flag */
void place_flag(int user_cords_x, int user_cords_y, int cli_sockfd, GameState *tileInfo){
    
    tileInfo->tiles[user_cords_x][user_cords_y].flagged = true;
    write_client_msg(cli_sockfd, "UPD");
    write_client_int(cli_sockfd, 10);
    int mines_left = mines_remain(tileInfo); // Send client mines remaining
    write_client_int(cli_sockfd, mines_left);
    write_client_int(cli_sockfd, 98); // No tiles to reveal

}

/* Checks which tiles around a '0' tile needs to be revealed */
void recusion_check(GameState *tileInfo, int user_cords_x, int user_cords_y){
    
    int tiles_with_0  = 0;
    int tilesx[8];
    int tilesy[8];

    for (int i = -1; i < 2; i++){
        for (int j = -1; j < 2; j++){
            if (i == 0 && j == 0){
                continue;
            }
            else {
                int cordx = user_cords_x + i;
                int cordy = user_cords_y + j;
                if (cordx >= 0 && cordx < NUM_TILES_X && cordy >= 0 && cordy < NUM_TILES_Y){
                    if (!tileInfo->tiles[cordx][cordy].is_mine && !tileInfo->tiles[cordx][cordy].revealed){
                        tileInfo->tiles[cordx][cordy].flagged = false;
                        tileInfo->tiles[cordx][cordy].revealed = true;
                        //printf("[DEBUG] Tile Revealed\n");
                        if(tileInfo->tiles[cordx][cordy].identifier == '0'){
                            tiles_with_0++;
                            tilesx[tiles_with_0-1] = cordx;
                            tilesy[tiles_with_0-1] = cordy;
                        }
                    }
                }
            }
        }
    }
    
    for (int i = 0; i < tiles_with_0; i++){
        recusion_check(tileInfo, tilesx[i], tilesy[i]);
    }
    
}

/* Checks and converts the tiles object to an int */
int check_identifier(GameState *tileInfo, int x_tile, int y_tile){
    int converter;
    if (tileInfo->tiles[x_tile][y_tile].identifier == '0'){
        converter = 0;
    }
    if (tileInfo->tiles[x_tile][y_tile].identifier == '1'){
        converter = 1;
    }
    if (tileInfo->tiles[x_tile][y_tile].identifier == '2'){
        converter = 2;
    }
    if (tileInfo->tiles[x_tile][y_tile].identifier == '3'){
        converter = 3;
    }
    if (tileInfo->tiles[x_tile][y_tile].identifier == '4'){
        converter = 4;
    }
    if (tileInfo->tiles[x_tile][y_tile].identifier == '5'){
        converter = 5;
    }
    if (tileInfo->tiles[x_tile][y_tile].identifier == '*'){
        converter = 12;
    }
    return converter;
}

/* Revealing a tile */
void reveal_tile(int user_cords_x, int user_cords_y, int cli_sockfd, GameState *tileInfo){
    
    tileInfo->tiles[user_cords_x][user_cords_y].flagged = false;
    tileInfo->tiles[user_cords_x][user_cords_y].revealed = true;

    write_client_msg(cli_sockfd, "UPD");
    int converter;
    if (tileInfo->tiles[user_cords_x][user_cords_y].identifier == '0'){
        converter = 0;
    }
    if (tileInfo->tiles[user_cords_x][user_cords_y].identifier == '1'){
        converter = 1;
    }
    if (tileInfo->tiles[user_cords_x][user_cords_y].identifier == '2'){
        converter = 2;
    }
    if (tileInfo->tiles[user_cords_x][user_cords_y].identifier == '3'){
        converter = 3;
    }
    if (tileInfo->tiles[user_cords_x][user_cords_y].identifier == '4'){
        converter = 4;
    }
    if (tileInfo->tiles[user_cords_x][user_cords_y].identifier == '5'){
        converter = 5;
    }
    if (tileInfo->tiles[user_cords_x][user_cords_y].identifier == '*'){
        converter = 12;
    }
    write_client_int(cli_sockfd, converter); // Send Client tile they revealed
    int mines_left = mines_remain(tileInfo);
    write_client_int(cli_sockfd, mines_left); // Send client mines remaining

    /* Reveal tiles that are around when a 0 is clicked */
    if (converter == 0){
        write_client_int(cli_sockfd, 99); // Notify Client more tile to come
        
        recusion_check(tileInfo, user_cords_x, user_cords_y); // Uncover
        
        int tiles_to_reveal = 0;
        for (int i =0; i < NUM_TILES_X; i++){
            for (int j =0; j < NUM_TILES_Y; j++){
                if (tileInfo->tiles[i][j].revealed){
                    tiles_to_reveal++;
                }
            }
        }
        

        write_client_int(cli_sockfd, tiles_to_reveal); // Notify Client how many to reveal
        
        printf("\nTile to be revealed %d\n", tiles_to_reveal);
        
        
        for (int i =0; i < NUM_TILES_X; i++){
            for (int j =0; j < NUM_TILES_Y; j++){
                if (tileInfo->tiles[i][j].revealed){
                    write_client_int(cli_sockfd, i); // Send x
                    write_client_int(cli_sockfd, j); // Send y
                    int value_of_tile = check_identifier(tileInfo, i, j);
                    write_client_int(cli_sockfd, value_of_tile); // Send identifier
                }
            }
        }
    
    }
    else {
        write_client_int(cli_sockfd, 98); // Notify Client no more tiles to come
    }
}

/* Debugging */
void print_struct_details (GameState *tileInfo){
    for (int i =0; i < NUM_TILES_X; i++){
        for (int j =0; j < NUM_TILES_Y; j++){
            printf("The identifier: %c\n", tileInfo->tiles[i][j].identifier);
        }
    }
}

/* Check if mine is hit */
bool mine_ishit(GameState *tileInfo){
    
    bool ishit = false;
    
    for (int i =0; i < NUM_TILES_X; i++){
        for (int j =0; j < NUM_TILES_Y; j++){
            if (tileInfo->tiles[i][j].is_mine && tileInfo->tiles[i][j].revealed){
                ishit = true;
            }
        }
    }
    return ishit;
}

/* Check if game is over */
bool check_win_loss(char board[][9], GameState *tileInfo){
    if(mines_remain(tileInfo) == 0){
        return true;
    }
    else if (mine_ishit(tileInfo)){
        return true;
    }
    else {
        return false;
    }
}

/* Send board if game is won */
void send_board_win(GameState *tileInfo, int cli_sockfd){
    for (int i =0; i < NUM_TILES_X; i++){
        for (int j =0; j < NUM_TILES_Y; j++){
            tileInfo->tiles[i][j].flagged = false;
            tileInfo->tiles[i][j].revealed = true;
            int converter;
            if (tileInfo->tiles[i][j].identifier == '0'){
                converter = 0;
            }
            if (tileInfo->tiles[i][j].identifier == '1'){
                converter = 1;
            }
            if (tileInfo->tiles[i][j].identifier == '2'){
                converter = 2;
            }
            if (tileInfo->tiles[i][j].identifier == '3'){
                converter = 3;
            }
            if (tileInfo->tiles[i][j].identifier == '4'){
                converter = 4;
            }
            if (tileInfo->tiles[i][j].identifier == '5'){
                converter = 5;
            }
            if (tileInfo->tiles[i][j].identifier == '*') {
                    converter = 10;
            }

            write_client_int(cli_sockfd, converter);
        }
    }
    
}

/* Send board if game is lost */
void send_board_loss(GameState *tileInfo, int cli_sockfd){
    for (int i =0; i < NUM_TILES_X; i++){
        for (int j =0; j < NUM_TILES_Y; j++){
            tileInfo->tiles[i][j].flagged = false;
            tileInfo->tiles[i][j].revealed = true;
            int converter;
            if (tileInfo->tiles[i][j].identifier == '0'){
                converter = 0;
            }
            if (tileInfo->tiles[i][j].identifier == '1'){
                converter = 1;
            }
            if (tileInfo->tiles[i][j].identifier == '2'){
                converter = 2;
            }
            if (tileInfo->tiles[i][j].identifier == '3'){
                converter = 3;
            }
            if (tileInfo->tiles[i][j].identifier == '4'){
                converter = 4;
            }
            if (tileInfo->tiles[i][j].identifier == '5'){
                converter = 5;
            }
            if (tileInfo->tiles[i][j].identifier == '*') {
                converter = 12;
            }
            
            write_client_int(cli_sockfd, converter);
        }
    }
    
}

/* Main game logic */
void running_game(int cli_sockfd){
    char board[9][9] = { {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}, /* Game board */
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}};
    
    printf("Game on!\n");
    pthread_mutex_lock(&mutexboard);
    leaderboard.clients[cli_sockfd].games_played++;
    pthread_mutex_unlock(&mutexboard);

    // Creating And Drawing Board Here
    GameState tileInfo;
    intialize_tiles(&tileInfo);
    place_mines(&tileInfo);
    check_adj(&tileInfo);
    build_board(board, &tileInfo);
    
    /* Send the start message. */

    bool game_over = false;
    //Timer
    int time_taken = 0;
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);

    while (!game_over){
        update_board(board, &tileInfo);
        build_board(board, &tileInfo);
        draw_board(board, &tileInfo);
        
        int place_reveals = place_reveal(cli_sockfd); // Grab clients choice
        if (place_reveals == 0){ // Reveal Tile
            int user_cords_x = get_cords_x(cli_sockfd);
            int user_cords_y = get_cords_y(cli_sockfd);
            if (tileInfo.tiles[user_cords_x][user_cords_y].revealed){
                write_client_msg(cli_sockfd, "ERR"); // Tile already revealed
            }
            else {
                reveal_tile(user_cords_x, user_cords_y, cli_sockfd, &tileInfo);
            }
        }
        if (place_reveals == 1){ // Place Flag
            int user_cords_x = get_cords_x(cli_sockfd);
            int user_cords_y = get_cords_y(cli_sockfd);
            if (tileInfo.tiles[user_cords_x][user_cords_y].revealed){
                write_client_msg(cli_sockfd, "ERR"); // Tile already revealed
            }
            else {
                place_flag(user_cords_x, user_cords_y, cli_sockfd, &tileInfo);
            }
        }
        if (place_reveals == 2){ // Place Flag
            break;
        }
        
        game_over = check_win_loss(board, &tileInfo);
    }
    
    if( gettimeofday(&end, NULL) != 0){
        printf("Getting time failed!\n");
    }
    time_taken = (int) end.tv_sec-start.tv_sec + ( end.tv_usec - start.tv_usec ) / 1000000.0;
    
    // Loss
    if (mine_ishit(&tileInfo)){
        pthread_mutex_lock(&mutexboard);
        leaderboard.clients[cli_sockfd].time = leaderboard.clients[cli_sockfd].time + time_taken;
        pthread_mutex_unlock(&mutexboard);

        write_client_msg(cli_sockfd, "LOS"); // Game Lost
        send_board_loss(&tileInfo, cli_sockfd);

    }
    // Win
    if (mines_remain(&tileInfo) == 0){
        pthread_mutex_lock(&mutexboard);
        leaderboard.clients[cli_sockfd].time = leaderboard.clients[cli_sockfd].time + time_taken;
        pthread_mutex_unlock(&mutexboard);
        write_client_msg(cli_sockfd, "WIN"); // Game Won
        write_client_int(cli_sockfd, time_taken); // Game Won
        
        pthread_mutex_lock(&mutexboard);
        leaderboard.clients[cli_sockfd].games_won++;
        pthread_mutex_unlock(&mutexboard);
        
        send_board_win(&tileInfo, cli_sockfd);
    }

    
}

/*Shows the leaderboard */
void show_leaderboard(int cli_sockfd){
    printf("\n\n============================================================ \n\n");
    int number_of_players = 0;
    int number_of_players_writer = 0;
    int games_won_pass = 0;

    int a = 0;
   
    /* Find number of players */
    for (int i = 0; i < MAX; i++){
        if (leaderboard.clients[i].player){
            number_of_players++;
            games_won_pass += leaderboard.clients[i].games_won;
        }
    }
    
    int playersindex[number_of_players];
    
    /* Find number of players */
    for (int i = 0; i < MAX; i++){
        if (leaderboard.clients[i].player){
            number_of_players_writer++;
            playersindex[number_of_players_writer-1] = i;
        }
    }


    for (int i = 0; i < number_of_players; ++i)
    {
        for (int j = i + 1; j < number_of_players; ++j)
        {
            if (leaderboard.clients[playersindex[i]].time < leaderboard.clients[playersindex[j]].time)
            {
                a = playersindex[i];
                playersindex[i] = playersindex[j];
                playersindex[j] = a;
            }
            else if (leaderboard.clients[playersindex[i]].time == leaderboard.clients[playersindex[j]].time) {
                if (leaderboard.clients[playersindex[i]].games_won < leaderboard.clients[playersindex[j]].games_won){
                    // Order by number of games won
                    a = playersindex[i];
                    playersindex[i] = playersindex[j];
                    playersindex[j] = a;
                }
                else {
                    // Alphebetical Order
                    char temp[25];
                    if(strcmp(leaderboard.clients[playersindex[i]].name,leaderboard.clients[playersindex[j]].name)>0){
                        a = playersindex[i];
                        playersindex[i] = playersindex[j];
                        playersindex[j] = a;
                    }
                }
                
            }
        }
    }
    
    for (int i = 0; i < number_of_players; i++){

            printf("%s          %d seconds       %d games won, %d games played\n", leaderboard.clients[playersindex[i]].name, leaderboard.clients[playersindex[i]].time, leaderboard.clients[playersindex[i]].games_won, leaderboard.clients[playersindex[i]].games_played);

    }
    printf("\n============================================================\n\n");
    write_client_int(cli_sockfd, number_of_players);
    write_client_int(cli_sockfd, games_won_pass);

//    /* Send names for loop */
//    for (int i = 0; i < number_of_players; i++){
//        write_client_name(cli_sockfd, leaderboard.clients[playersindex[i]].name);
//    }
    
    for (int i = 0; i < number_of_players; i++){
            write_client_int(cli_sockfd, leaderboard.clients[playersindex[i]].games_played);
            write_client_int(cli_sockfd, leaderboard.clients[playersindex[i]].games_won);
            write_client_int(cli_sockfd, leaderboard.clients[playersindex[i]].time);
    }
    
}


/* Runs a game clients. */
void menu_option(int *cli_sockfd)
{
    srand(RANDOM_NUMBER_SEED); // Create same random number for each client
    //int *cli_sockfd = (int*)thread_data; /* Client sockets. */
    int gameoption = 0;
    while(1){
        gameoption = recv_int(*cli_sockfd);
        printf("[DEBUG] User menu option: %d\n",gameoption);
        while (gameoption == 1){
            running_game(*cli_sockfd); // Run Game Logic
            break;
        }
        while (gameoption == 2){
            show_leaderboard(*cli_sockfd);
            // Need to notify client to break and send menu option again
            break;
        }
        if (gameoption == 3){
            printf("[DEBUG] Player choose to exit!\n");
            break;
        }
    }
    
    pthread_mutex_lock(&mutexcount);
    player_count--;
    pthread_mutex_unlock(&mutexcount);
    
    //int pthread_cancel(pthread_t thread);
//    free(cli_sockfd);
//
    pthread_exit(NULL);
}

/*
 * Main Program
 */
void INThandler(int);
void *connection_handler(void *);


int main(int argc, char *argv[])
{
    //srand(RANDOM_NUMBER_SEED);
    int port;
    /* Make sure a port was specified. */
    if (argc < 2) {
        fprintf(stderr,"No port provided using port 12345\n");
        port = 12345;
    }
    else {
        port = atoi(argv[1]);
    }
    signal(SIGINT, INThandler);

    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
    
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
    
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
    
    //Listen
    listen(socket_desc , 3);
    
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    pthread_t thread_id;
    
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
        
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
        
        puts("Handler assigned");
    }
    
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    
    return 0;
    
}

/* Handles the control c exit */
void  INThandler(int sig)
{
    
    printf("\nOUCH you hit Ctrl-C?!\n" "Goodbye!\n");
    pthread_mutex_destroy(&mutexcount);
    
    pthread_mutex_destroy(&mutexboard);
    pthread_mutex_destroy(&mutexrand);

//    free(&mutexcount);
//    free(&mutexboard);

    exit(0);
}


/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    /* Send the client it's ID. */
    write(sock, &num_conn, sizeof(int));
    
    //printf("[DEBUG] Sent client %d it's ID.\n", num_conn);
    
    /* Authentication Process */
    bool isauthenticated = authenticate_process(sock);
    if (isauthenticated){
        printf("Authenticated\n");
        pthread_mutex_init(&mutexcount, NULL);
        
        pthread_mutex_init(&mutexboard, NULL);
        pthread_mutex_init(&mutexrand, NULL);

        /* Increment the player count. */
        pthread_mutex_lock(&mutexcount);
        player_count++;
        printf("Number of players is now %d.\n", player_count);
        pthread_mutex_unlock(&mutexcount);
        
        menu_option(&sock);
    }
    else {
        printf("Not Authenticated\n");
        fflush(stdout);
    }
    
    return 0;
}
