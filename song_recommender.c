
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <conio.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

#define MAX_SONGS 500
#define MAX_TEXT 256
#define MAX_PATH_LEN 512

typedef struct {
    char name[MAX_TEXT];
    char artist[MAX_TEXT];
    char mood[MAX_TEXT];
    char path[MAX_PATH_LEN];
    int favorite;
    int playCount;
} Song;

typedef struct Node {
    int songIndex;
    struct Node *next;
} Node;

Song songs[MAX_SONGS];
int songCount = 0;
Node *playlist = NULL;

void loadSongs(void);
void saveFavorites(void);
void loadFavorites(void);
void savePlaylist(void);
void loadPlaylist(void);
void displaySongs(void);
void playSong(void);
void playInternally(int index);
void playbackControlLoop(void);
void playlistManager(void);
void favoritesMenu(void);
void showStatistics(void);
void addToPlaylist(int index);
void removeFromPlaylist(void);
void displayPlaylist(void);
void askToPlay(void);
void clearInput(void);

void clearInput(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

void loadSongs(void) {
    FILE *file = fopen("songs.csv", "r");

    if (file == NULL) {
        printf("\nError: songs.csv not found!\n");
        printf("Keep songs.csv in the same folder as the program.\n");
        return;
    }

    char line[1500];

    /* Skip CSV header */
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file) && songCount < MAX_SONGS) {
        line[strcspn(line, "\r\n")] = '\0';

        char *name = strtok(line, ",");
        char *artist = strtok(NULL, ",");
        char *mood = strtok(NULL, ",");
        char *path = strtok(NULL, ",");

        if (name && artist && mood && path) {
            strncpy(songs[songCount].name, name, MAX_TEXT - 1);
            strncpy(songs[songCount].artist, artist, MAX_TEXT - 1);
            strncpy(songs[songCount].mood, mood, MAX_TEXT - 1);
            strncpy(songs[songCount].path, path, MAX_PATH_LEN - 1);

            songs[songCount].favorite = 0;
            songs[songCount].playCount = 0;

            songCount++;
        }
    }

    fclose(file);
    printf("\n%d songs loaded successfully!\n", songCount);
}

 
void displaySongs(void) {
    if (songCount == 0) {
        printf("\nNo songs available.\n");
        return;
    }

    printf("\n");
    printf("+------+------------------------------+----------------------+----------------------+\n");
    printf("| No.  | Song Title                   | Artist               | Mood                 |\n");
    printf("+------+------------------------------+----------------------+----------------------+\n");

    for (int i = 0; i < songCount; i++) {
        printf("| %-4d | %-28.28s | %-20.20s | %-20.20s |\n",
               i + 1,
               songs[i].name,
               songs[i].artist,
               songs[i].mood);
    }

    printf("+------+------------------------------+----------------------+----------------------+\n");
}


void playbackControlLoop(void) {
    char command[100];
    char mode[50];
    int paused = 0;

    printf("\n---------------------------------------\n");
    printf("NOW PLAYING\n");
    printf("Press P to Pause / Resume\n");
    printf("Press S to Stop and return to menu\n");
    printf("---------------------------------------\n");

    while (1) {
        if (_kbhit()) {
            int key = _getch();

            if (key == 'p' || key == 'P') {
                if (!paused) {
                    mciSendStringA("pause songPlayer", NULL, 0, NULL);
                    paused = 1;
                    printf("\nPaused. Press P to resume.\n");
                } else {
                    mciSendStringA("resume songPlayer", NULL, 0, NULL);
                    paused = 0;
                    printf("\nResumed.\n");
                }
            }

            else if (key == 's' || key == 'S') {
                mciSendStringA("stop songPlayer", NULL, 0, NULL);
                printf("\nSong stopped.\n");
                break;
            }
        }

        mciSendStringA(
            "status songPlayer mode",
            mode,
            sizeof(mode),
            NULL
        );

        if (strcmp(mode, "stopped") == 0) {
            printf("\nSong finished.\n");
            break;
        }

        Sleep(100);
    }

    mciSendStringA("close songPlayer", NULL, 0, NULL);
}

void playInternally(int index) {
    char command[800];

    sprintf(
        command,
        "open \"%s\" type mpegvideo alias songPlayer",
        songs[index].path
    );

    if (mciSendStringA(command, NULL, 0, NULL) != 0) {
        printf("\nUnable to open this song.\n");
        printf("Check the file path and audio format.\n");
        return;
    }

    if (mciSendStringA("play songPlayer", NULL, 0, NULL) != 0) {
        printf("\nUnable to play this song.\n");
        mciSendStringA("close songPlayer", NULL, 0, NULL);
        return;
    }

    songs[index].playCount++;
    playbackControlLoop();
}

void playSong(void) {
    if (songCount == 0) {
        printf("\nNo songs available.\n");
        return;
    }

    displaySongs();

    int choice;

    printf("\nEnter song number to play: ");

    if (scanf("%d", &choice) != 1) {
        clearInput();
        printf("\nInvalid input.\n");
        return;
    }

    if (choice < 1 || choice > songCount) {
        printf("\nInvalid song number.\n");
        return;
    }

    int index = choice - 1;

    printf("\nPlaying: %s by %s\n",
           songs[index].name,
           songs[index].artist);

    playInternally(index);
}

void addToPlaylist(int index) {
    Node *newNode = (Node *)malloc(sizeof(Node));

    if (newNode == NULL) {
        printf("\nMemory allocation failed.\n");
        return;
    }

    newNode->songIndex = index;
    newNode->next = NULL;

    if (playlist == NULL) {
        playlist = newNode;
    } else {
        Node *temp = playlist;

        while (temp->next != NULL) {
            temp = temp->next;
        }

        temp->next = newNode;
    }

    printf("\nSong added to playlist.\n");
}

void displayPlaylist(void) {
    if (playlist == NULL) {
        printf("\nYour playlist is empty.\n");
        return;
    }

    printf("\n=============== YOUR PLAYLIST ===============\n");

    Node *temp = playlist;
    int count = 1;

    while (temp != NULL) {
        int index = temp->songIndex;

        printf("\n%d. %s - %s",
               count,
               songs[index].name,
               songs[index].artist);

        temp = temp->next;
        count++;
    }

    printf("\n");
}

void removeFromPlaylist(void) {
    if (playlist == NULL) {
        printf("\nPlaylist is empty.\n");
        return;
    }

    displayPlaylist();

    int position;

    printf("\nEnter playlist position to remove: ");

    if (scanf("%d", &position) != 1) {
        clearInput();
        printf("\nInvalid input.\n");
        return;
    }

    if (position < 1) {
        printf("\nInvalid position.\n");
        return;
    }

    Node *temp = playlist;
    Node *previous = NULL;

    for (int i = 1; temp != NULL && i < position; i++) {
        previous = temp;
        temp = temp->next;
    }

    if (temp == NULL) {
        printf("\nInvalid position.\n");
        return;
    }

    if (previous == NULL) {
        playlist = temp->next;
    } else {
        previous->next = temp->next;
    }

    free(temp);
    printf("\nSong removed from playlist.\n");
}

void playlistManager(void) {
    int choice;

    do {
        printf("\n========== PLAYLIST MANAGER ==========\n");
        printf("1. View Playlist\n");
        printf("2. Add Song to Playlist\n");
        printf("3. Remove Song from Playlist\n");
        printf("0. Back to Main Menu\n");
        printf("Enter choice: ");

        if (scanf("%d", &choice) != 1) {
            clearInput();
            printf("\nInvalid input.\n");
            continue;
        }

        switch (choice) {
            case 1:
                displayPlaylist();
                break;

            case 2: {
                displaySongs();

                int songChoice;
                printf("\nEnter song number: ");

                if (scanf("%d", &songChoice) != 1) {
                    clearInput();
                    printf("\nInvalid input.\n");
                    break;
                }

                if (songChoice >= 1 && songChoice <= songCount) {
                    addToPlaylist(songChoice - 1);
                } else {
                    printf("\nInvalid song number.\n");
                }
                break;
            }

            case 3:
                removeFromPlaylist();
                break;

            case 0:
                break;

            default:
                printf("\nInvalid choice.\n");
        }

    } while (choice != 0);
}

void saveFavorites(void) {
    FILE *file = fopen("favorites.txt", "w");

    if (file == NULL) return;

    for (int i = 0; i < songCount; i++) {
        if (songs[i].favorite) {
            fprintf(file, "%d\n", i);
        }
    }

    fclose(file);
}

void loadFavorites(void) {
    FILE *file = fopen("favorites.txt", "r");

    if (file == NULL) return;

    int index;

    while (fscanf(file, "%d", &index) == 1) {
        if (index >= 0 && index < songCount) {
            songs[index].favorite = 1;
        }
    }

    fclose(file);
}

void favoritesMenu(void) {
    int choice;

    do {
        printf("\n============== FAVORITES ==============\n");
        printf("1. View Favorites\n");
        printf("2. Add Favorite\n");
        printf("3. Remove Favorite\n");
        printf("0. Back to Main Menu\n");
        printf("Enter choice: ");

        if (scanf("%d", &choice) != 1) {
            clearInput();
            printf("\nInvalid input.\n");
            continue;
        }

        if (choice == 1) {
            int found = 0;

            printf("\nYour Favorite Songs:\n");

            for (int i = 0; i < songCount; i++) {
                if (songs[i].favorite) {
                    printf("\n%s - %s",
                           songs[i].name,
                           songs[i].artist);
                    found = 1;
                }
            }

            if (!found) {
                printf("\nNo favorite songs yet.\n");
            }
        }

        else if (choice == 2) {
            displaySongs();

            int index;
            printf("\nEnter song number: ");

            if (scanf("%d", &index) != 1) {
                clearInput();
                printf("\nInvalid input.\n");
                continue;
            }

            if (index >= 1 && index <= songCount) {
                songs[index - 1].favorite = 1;
                saveFavorites();
                printf("\nAdded to favorites.\n");
            } else {
                printf("\nInvalid song number.\n");
            }
        }

        else if (choice == 3) {
            int index;
            printf("\nEnter song number to remove: ");

            if (scanf("%d", &index) != 1) {
                clearInput();
                printf("\nInvalid input.\n");
                continue;
            }

            if (index >= 1 && index <= songCount &&
                songs[index - 1].favorite) {
                songs[index - 1].favorite = 0;
                saveFavorites();
                printf("\nRemoved from favorites.\n");
            } else {
                printf("\nSong is not in favorites or number is invalid.\n");
            }
        }

        else if (choice != 0) {
            printf("\nInvalid choice.\n");
        }

    } while (choice != 0);
}

void showStatistics(void) {
    int totalPlays = 0;
    int favoriteCount = 0;

    for (int i = 0; i < songCount; i++) {
        totalPlays += songs[i].playCount;

        if (songs[i].favorite) {
            favoriteCount++;
        }
    }

    printf("\n============= MUSIC STATISTICS =============\n");
    printf("\nTotal songs: %d", songCount);
    printf("\nTotal plays this session: %d", totalPlays);
    printf("\nFavorite songs: %d", favoriteCount);

    printf("\n\nSong Play Counts:\n");

    for (int i = 0; i < songCount; i++) {
        printf("%s - %d plays\n",
               songs[i].name,
               songs[i].playCount);
    }
}

void savePlaylist(void) {
    FILE *file = fopen("playlist.txt", "w");

    if (file == NULL) return;

    Node *temp = playlist;

    while (temp != NULL) {
        fprintf(file, "%d\n", temp->songIndex);
        temp = temp->next;
    }

    fclose(file);
}

void loadPlaylist(void) {
    FILE *file = fopen("playlist.txt", "r");

    if (file == NULL) return;

    int index;

    while (fscanf(file, "%d", &index) == 1) {
        if (index >= 0 && index < songCount) {
            addToPlaylist(index);
        }
    }

    fclose(file);
}

void askToPlay(void) {
    char choice;

    printf("\nWould you like to play a song now? (Y/N): ");
    scanf(" %c", &choice);

    if (choice == 'Y' || choice == 'y') {
        playSong();
    }
}

int main(void) {
    int choice;

    printf("============================================\n");
    printf("       MOOD BASED SONG RECOMMENDATION\n");
    printf("============================================\n");

    loadSongs();

    if (songCount == 0) {
        printf("\nPlease add songs to songs.csv first.\n");
        return 0;
    }

    loadFavorites();
    loadPlaylist();

    do {
        printf("\n\n=============== MAIN MENU ===============\n");
        printf("1. Browse All Songs\n");
        printf("2. Play a Song\n");
        printf("3. Playlist Manager\n");
        printf("4. Favorites\n");
        printf("5. Music Statistics\n");
        printf("0. Save and Exit\n");
        printf("=========================================\n");
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) {
            clearInput();
            printf("\nInvalid input.\n");
            continue;
        }

        switch (choice) {
            case 1:
                displaySongs();
                askToPlay();
                break;

            case 2:
                playSong();
                break;

            case 3:
                playlistManager();
                askToPlay();
                break;

            case 4:
                favoritesMenu();
                askToPlay();
                break;

            case 5:
                showStatistics();
                askToPlay();
                break;

            case 0:
                saveFavorites();
                savePlaylist();

                printf("\nData saved successfully!\n");
                printf("Thank you for using the Song Recommendation System!\n");
                break;

            default:
                printf("\nInvalid choice. Try again.\n");
        }

    } while (choice != 0);

    while (playlist != NULL) {
        Node *temp = playlist;
        playlist = playlist->next;
        free(temp);
    }

    return 0;
}
