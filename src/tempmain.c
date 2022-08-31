#define _GNU_SOURCE

#include <ncurses.h>
#include <form.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "../include/events.h"
#include "../include/helper.h"

enum mode {
    Normal,
    Search,
    Debug
};

typedef struct state
{ 
    int m;
} state_t;

typedef struct pokemon
{
    char *name;
    char url[128];
    int hp, atk, def, spA, spD, spe, bst;
} pokemon_t;

typedef struct winSize 
{
    int x, y;
} winSize_t;

struct queue q;
pthread_mutex_t kPokemonMutex;
int fd[2];
bool run = true;
state_t kState = {.m = Normal};
pokemon_t kPokemon = {.name = "\0"};

void pokemonGet()
{
    CURL *curl;
    CURLcode res;
    char url[55] = "https://pokeapi.co/api/v2/pokemon/";
    memcpy(&url[34], kPokemon.name, 20);
    
    curl = curl_easy_init();

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

void *stateListener()
{
    initscr();
    refresh();

    WINDOW *win = newwin(5, 5, 5, 5);
    int prevState = Normal;
    int state;
    while(1)
    {
        read(fd[1], &state, state);
        
        if (state != prevState)
        {
            switch (state)
            {
                default:
                case Normal:
                    box(win, 5, 3);
                    break;
                case Search:
                    box(win, 3, 5);
                    break;
                case Debug:
                    break;
            }
            wrefresh(win);
            refresh();
        }
        prevState = state;
    }
    delwin(win);
}

void *inputScan(void *ptr)
{
    keypad(stdscr, true);
    intrflush(stdscr, false);
    clear();
    refresh();

    while (run)
    {
        int input = getch();
        write(fd[0], &kState.m, kState.m);

        switch (kState.m)
        {
            case Normal:
                switch (input) {
                case 113:
                    run = false;
                    break;
                case 115:
                    kState.m = Search;
                    break;
                case 116:
                    kState.m = Debug;
                default:
                    break;
                }
                break;
            case Search:
                FIELD *field[2];
                FORM *form;
                int ch;

                field[0] = new_field(1, 20, 1, 11, 0, 20);
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
                    printf("Could not take mutex!");
                }
                pthread_mutex_unlock(&kPokemonMutex);

                unpost_form(form);
                free_form(form);
                free_field(field[0]);
               
                kState.m = Normal;

                break;
            case Debug:
                break;
        }
    }
    return NULL;
}

void *eventQueue(void *ptr)
{
    while (run)
    {
        if (q.front != NULL)
        {
            executeEvent(&q);
            refresh();
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t thread1, thread2, thread3;

    int results = pipe(fd);
    if (results > 0)
    {
        perror("pipe");
        exit(1);
    }

    initscr();
    refresh();

    pthread_create(&thread1, NULL, inputScan, NULL);
    pthread_create(&thread2, NULL, eventQueue, (void *) argv[1]);
    pthread_create(&thread3, NULL, stateListener, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread2, NULL);

    endwin();
    return 0;
}

