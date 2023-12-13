#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_WIDTH 
#define MAX_HEIGHT 24
#define MAX_ROW 34

int max_x, max_y;    // dimensiuni maxime ale ferestrei
int bx, by;          // poziția mingii
int px = 0;          // poziția paletei
int dx = 1, dy = 1;  // diferența mingii pe cadru
int sx = 3, sy = 3;  // spațiu între blocuri și pereți laterali

int score = 0; 
int blockHeight = 5; //5 randuri de blocuri
int total_bricks = 0;
const char paddle[] = "--------";
bool blocks[5][MAX_ROW];

pthread_t game_thread;  // Identificatorul firului de execuție pentru joc
pthread_mutex_t draw_mutex = PTHREAD_MUTEX_INITIALIZER;// Mutex pentru sincronizare


void draw();// desenar e a stării curente a jocului
bool move_ball();// mutarea mingii pe cadru și verificarea coliziunilor
void check_block();//verificarea și eliminarea blocurilor lovite de minge
bool is_block(int x, int y); //dacă o poziție specificată este ocupată de un bloc

//functie pentru firul de executie al jocului
void* game_loop(void* arg) {
    while (1) {
        pthread_mutex_lock(&draw_mutex);
        draw();
        pthread_mutex_unlock(&draw_mutex);

        if (!move_ball()) {
            break;
        }
        if (score == total_bricks) {
            char msg[] = "YOU WIN! (Press q to quit)";
            mvprintw(max_y / 2, max_x / 2 - (sizeof(msg) / 2), "%s", msg);
            refresh();

            pthread_cancel(game_thread);//oprim bucla de joc
            break;
    }

        usleep(80000); // pentru controlarea vitezei jocului
    }
    
    
   if (by > max_y) {
        char msg[] = "GAME OVER! (Press q to quit)";
        mvprintw(max_y / 2 - 1, max_x / 2 - (sizeof(msg) / 2), "%s", msg);
        mvprintw(max_y / 2, max_x / 2 - 5, "Your Score: %d", score);
        refresh();
    }
    

    return NULL;
}
//miscarea stanga/dreapta pentru paleta
void check_key() {
    int key;

    while ((key = getch()) != 'q') {
        switch (key) {
            case KEY_LEFT: if (px > 1) px--; break;
            case KEY_RIGHT: if (px < max_x - sizeof(paddle)) px++; break;
            //case 'x': pthread_cancel(game_thread); return;
        }
    }
}



void initialize_blocks() {
    int blocksPerRow = max_x;

    for (int i = 0; i < blockHeight; i++) {
        for (int j = 0; j < max_x - sx * 2; j++) {
           
            int trapezWidth = max_x - sx * 2;  
            int trapezTop = 0; 

            if (i >= trapezTop && j > (2 * i - trapezTop) && j < (trapezWidth - (2 * i - trapezTop))) {
                blocks[i][j] = true;
                total_bricks++; 
            } else {
                blocks[i][j] = false;
            }
        }
    }
}
//initializarea ferestrei inaintea inceperii jocului
void initialize_game() {
    initscr(); // initializeaza modul ncurses
    noecho(); 
    curs_set(0);
    keypad(stdscr, TRUE); // poti sa citesti taste

    // Setează dimensiunile maxime ale ferestrei
    //getmaxyx(stdscr, max_y, max_x);
    max_x = MAX_ROW;
    max_y = MAX_HEIGHT;
    resize_term(max_y, max_x);

    // Setează pozițiile inițiale pentru mingea și paletă
    bx = max_x / 2 + 1;
    by = max_y - 1;
    px = max_x / 2 - (sizeof(paddle) / 2);
    

    // Inițializează blocurile
    initialize_blocks();

    // Afișează starea inițială a jocului
    pthread_mutex_lock(&draw_mutex);
    draw();
    pthread_mutex_unlock(&draw_mutex);

    //apăsarea tastei 's' pentru a începe jocul
    int key;
    while ((key = getch()) != 's') {
        // Dacă se apasă 'q', ieși din program
        if (key == 'q') {
            endwin();
            exit(EXIT_SUCCESS);
        }
    }
}

void draw() {
    clear();
    getmaxyx(stdscr, max_y, max_x);//ia valorile maxime pentru fereastra

    // Desenează peretele stâng
    for (int i = 0; i < max_y; i++) {
        mvprintw(i, 0, "|");
    }

    // Desenează peretele drept
    for (int i = 0; i < max_y; i++) {
        mvprintw(i, max_x - 1, "|");
    }

    // Desenează peretele superior
    for (int i = 0; i < max_x; i++) {
        mvprintw(0, i, "-");
    }

    mvprintw(max_y - 1, px, "%s", paddle);
    mvprintw(by, bx, "O");
    mvprintw(0, 2, "Score: %d", score);//coltul stanga sus
    //afiseaza blocurile
    for (int i = 0; i < blockHeight; i++) {
        for (int j = 0; j < max_x - sx * 2; j++) {
            if (blocks[i][j]) {
                mvprintw(i + sy, j + sx, "#");
            }
        }
    }

    refresh();
}

bool is_block(int x, int y) {
    x -= sx;
    y -= sy;

    return x >= 0 && x < max_x - sx * 2 && y >= 0 && y < blockHeight && blocks[y][x];
}

void check_block() {
    int x = bx + dx;
    int y = by + dy;

    if (is_block(x, y)) {
        blocks[y - sy][x - sx] = false;
        dy = -dy;  // Inversăm doar direcția pe axa Y
        score++;
    }
}


bool move_ball() {
    check_block();

    bx += dx;
    by += dy;

    if (bx < 0) {
        bx = 0;
        dx = abs(dx);
    }
    if (by < 0) {
        by = 0;
        dy = abs(dy);
    }
    if (bx > max_x - 1) {
        bx = max_x - 1;
        dx = -abs(dx);
    }

    if (by > max_y - 2 && bx > px - 1 && bx < px + sizeof(paddle) + 1) {
        by = max_y - 2;
        dy = -abs(dy);

        // Ajustează direcția pe axa X în funcție de locul de impact pe paletă
        int paddleCenter = px + sizeof(paddle) / 2;
        int offset = bx - paddleCenter;
        dx = offset / 2; //ajusteaza aceasta valoare pentru a schimba unghiul de ricoseu
    }

    if (by > max_y) {
        return false;
    }
    return true;
}


int main() {
    // ncurses settings
    initialize_game();
     // Crearea firului de execuție pentru joc
    if (pthread_create(&game_thread, NULL, game_loop, NULL) != 0) {
        fprintf(stderr, "Error creating game thread\n");
        endwin();
        return EXIT_FAILURE;
    }

    check_key();

    // incheierea procesarii
    pthread_cancel(game_thread);
    endwin();

    return EXIT_SUCCESS;
}
