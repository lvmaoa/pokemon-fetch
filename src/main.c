#define _GNU_SOURCE

#include <ncurses.h>
#include <string.h>
#include <form.h>
#include <pthread.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "../include/events.h"
#include "../include/helper.h"

#define CHOICES_MAX 3
#define CHUNK_SIZE 281600

typedef struct
{
    char *name;
    char url[128];
    int hp, atk, def, spA, spD, spe, bst;
} pokemon_t;

typedef struct
{
    char *buffer;
    size_t len;
    size_t buflen;
} get_request;

// global variables
bool run = true;
struct queue q;
pthread_mutex_t kPokemonMutex;
pokemon_t kPokemon = {.name = "\0"};
int fd[2];

void parseJSON(char *inBuf)
{
    char name[] = "base_stat";
    inBuf = strstr(inBuf, name);
    if (pthread_mutex_trylock(&kPokemonMutex) == 0)
    {
        // TODO: fix kPokemon.name is being malformed 
        // workaround
        inBuf = strstr(inBuf, name);
        kPokemon.hp = parseFirstNum(inBuf);
        inBuf = strstr(inBuf + 90, name);
        kPokemon.atk = parseFirstNum(inBuf);
        inBuf = strstr(inBuf + 90, name);
        kPokemon.def = parseFirstNum(inBuf);
        inBuf = strstr(inBuf + 90, name);
        kPokemon.spA = parseFirstNum(inBuf);
        inBuf = strstr(inBuf + 90, name);
        kPokemon.spD = parseFirstNum(inBuf);
        inBuf = strstr(inBuf + 90, name);
        kPokemon.spe = parseFirstNum(inBuf);
        kPokemon.bst = kPokemon.hp + kPokemon.atk + kPokemon.def + kPokemon.spA + kPokemon.spD + kPokemon.spe;
    }
    else
    {
        printf("Could not take mutex");
    }
    pthread_mutex_destroy(&kPokemonMutex);
}

void printPokemon(WINDOW *win)
{
    mvwprintw(win, 1, 2, "Name: %s", kPokemon.name);
    mvwprintw(win, 2, 2, "HP: %d", kPokemon.hp);
    mvwprintw(win, 3, 2, "Atk: %d", kPokemon.atk);
    mvwprintw(win, 4, 2, "Def: %d", kPokemon.def);
    mvwprintw(win, 5, 2, "SpA: %d", kPokemon.spA);
    mvwprintw(win, 6, 2, "SpD: %d", kPokemon.spD);
    mvwprintw(win, 7, 2, "Spe: %d", kPokemon.spe);
    mvwprintw(win, 8, 2, "Bst: %d", kPokemon.bst);
    // TODO: Add more info to print out here
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb; 
    get_request *req = (get_request *) userdata;

    printw("receive chunk of %zu bytes\n", realsize);

    while (req->buflen < req->len + realsize + 1)
    {
        req->buffer = realloc(req->buffer, req->buflen + CHUNK_SIZE);
        req->buflen += CHUNK_SIZE;
    }
    memcpy(&req->buffer[req->len], ptr, realsize);
    req->len += realsize;
    req->buffer[req->len] = 0;

    return realsize;
}

void pokemonGet()
{
    CURL *curl;
    char url[55] = "https://pokeapi.co/api/v2/pokemon/";
    memcpy(&url[34], kPokemon.name, 20);

    curl = curl_easy_init();
    get_request req = {.buffer = NULL, .len = 0, .buflen = 0};

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        req.buffer = malloc(CHUNK_SIZE);
        req.buflen = CHUNK_SIZE;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&req);
        
        curl_easy_perform(curl);
        
        parseJSON(req.buffer);

        free(req.buffer);
        curl_easy_cleanup(curl);
    }
}

void getPokemonName()
{
    keypad(stdscr, true);
    FIELD *field[2];
    FORM *form;
    int ch;
    field[0] = new_field(1, 20, 1, 1, 0, 20);
    field[1] = NULL;
    set_field_type(field[0], TYPE_ALPHA);
    set_field_opts(field[0], O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);
    form = new_form(field);
    post_form(form);
    while((ch=getch()) != 10)
    {
        switch (ch)
        {
            case KEY_LEFT:
                form_driver(form, REQ_PREV_CHAR);
                break;
            case KEY_RIGHT:
                form_driver(form, REQ_NEXT_CHAR);
                break;
            case KEY_BACKSPACE:
                form_driver(form, REQ_DEL_PREV);
                break;
            case 127:
                form_driver(form, REQ_DEL_CHAR);
                break;
            default:
                form_driver(form, ch);
                break;
        }
        refresh();
    }
    form_driver(form, REQ_NEXT_FIELD);

    if (pthread_mutex_trylock(&kPokemonMutex) == 0)
    {
        kPokemon.name = trimWhitespace(field_buffer(field[0], 0));
        if ((int) kPokemon.name[0] != 0)
        {
            // register event
            enQueueEvent(&q, pokemonGet);
        }
    }
    else
    {
        printf("Could not lock mutex!");
    }
    pthread_mutex_unlock(&kPokemonMutex);

    unpost_form(form);
    free_form(form);
    free_field(field[0]);
}

void getRandomName()
{
    if (pthread_mutex_trylock(&kPokemonMutex) == 0)
    {
        srand(time(NULL));
        // TODO: Actually finish this section
        char *temp = malloc(20 * sizeof(char));
        *temp = rand() % 10 + '0';
        // kPokemon.name = temp;
        enQueueEvent(&q, pokemonGet);
    }
    else
    {
        printf("Could not lock mutex!");
    }
    pthread_mutex_unlock(&kPokemonMutex);
}

void *getMod()
{
    keypad(stdscr, true);
    while (run)
    {
        int mod = wgetch(stdscr);
        write(fd[1], &mod, sizeof mod);
    }
    return NULL;
}

void *eventQueue()
{
    while (run)
    {
        if (q.front != NULL)
        {
            executeEvent(&q);
        }
    }
    return NULL;
}

int main()
{
    // Create threads
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, eventQueue, NULL);
    pthread_create(&thread2, NULL, getMod, NULL);

    int results = pipe(fd);
    if (results > 0)
    {
        perror("pipe");
        exit(1);
    }

    // Start ncurses
    initscr();
    noecho();
    cbreak();
    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);

    WINDOW *win = newwin(6, xMax - 10, yMax - 8, 5);
    box(win, 0, 0);
    refresh();
    wrefresh(win);
    keypad(win, true);

    char *choice[CHOICES_MAX] = {"Search", "Random", "Quit"};
    int ch;
    int mod;
    int hl = 0;

    while (run)
    {
        clear();
        refresh();
        box(win, 0, 0);
        for (int i = 0; i < 3; ++i)
        {
            if (i == hl)
                wattron(win, A_REVERSE);
            mvwprintw(win, i + 1, 1, "%s", choice[i]);
            wattroff(win, A_REVERSE);
        }
        WINDOW *pokewin = newwin(yMax - 10, xMax - 10, 2, 5);
        box(pokewin, 0, 0);
        printPokemon(pokewin);
        // print last pokemon stats here (if not null)
        refresh();
        wrefresh(pokewin);

        ch = wgetch(win);

        if (ch == 113)
        {
            read(fd[0], &mod, sizeof mod);
            if (mod == 64)
            {
                // control was pressed
                
            }
        }

        switch (ch)
        {
            case KEY_UP:
                --hl;
                if (hl == -1)
                    hl = 0;
                break;
            case KEY_DOWN:
                ++hl;
                if (hl == CHOICES_MAX)
                    hl = CHOICES_MAX - 1;
                break;
            default:
                break;
        }
        if (ch == 10)
        {
            switch (hl)
            {
            case 0:
                // search
                getPokemonName();
                break;
            case 1:
                // random
                getRandomName();
                break;
            default:
                // quit
                run = false;
                break;
            }
        }
    }

    endwin();

    pthread_join(thread1, NULL);
}
