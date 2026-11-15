/*
    MOOD-BASED SONG RECOMMENDATION SYSTEM
    --------------------------------------
    - Reads song data from a CSV file (songs.csv)
      CSV columns: SongName,Artist,Mood,FilePath
    - User selects a mood from a menu
    - Shows all songs matching that mood
    - User picks a song to play -> plays it DIRECTLY from the command prompt
      itself using the Windows Multimedia Control Interface (mciSendString),
      no external player window opens. You can type p/r/s/q while it plays.
    - Then recommends:
         1) Other songs by the SAME ARTIST
         2) Other songs of the SAME MOOD (similar type)

    Compile on Windows (MinGW / gcc) - note the -lwinmm flag, required for
    the MCI audio functions:
        gcc song_recommender.c -o song_recommender.exe -lwinmm

    Run:
        song_recommender.exe
    (make sure songs.csv is in the same folder, or edit CSV_FILE below)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <mmsystem.h>  /* for mciSendString - plays audio in-process, no external player window */

#define MAX_SONGS 500
#define MAX_LEN   260

#define CSV_FILE  "songs.csv"

typedef struct {
    char name[MAX_LEN];
    char artist[MAX_LEN];
    char mood[64];
    char path[MAX_LEN];
} Song;

Song songs[MAX_SONGS];
int songCount = 0;

/* ---------- Consume rest of the current input line (stops safely at EOF) ---------- */
void clearInputLine(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* ---------- Utility: trim newline / spaces ---------- */
void trim(char *str) {
    int len = (int)strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' || str[len - 1] == ' ')) {
        str[len - 1] = '\0';
        len--;
    }
    int start = 0;
    while (str[start] == ' ') start++;
    if (start > 0) memmove(str, str + start, strlen(str) - start + 1);
}

/* ---------- Case-insensitive compare ---------- */
int strcasecmp_custom(const char *a, const char *b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* ---------- Load songs from CSV ---------- */
int loadSongs(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("ERROR: Could not open '%s'. Make sure the CSV file exists.\n", filename);
        return 0;
    }

    char line[1024];
    int isHeader = 1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        trim(line);
        if (strlen(line) == 0) continue;

        /* Skip header row (SongName,Artist,Mood,FilePath) */
        if (isHeader) {
            isHeader = 0;
            if (strstr(line, "SongName") != NULL || strstr(line, "Artist") != NULL) {
                continue;
            }
        }

        if (songCount >= MAX_SONGS) break;

        char *token;
        char lineCopy[1024];
        strcpy(lineCopy, line);

        token = strtok(lineCopy, ",");
        if (!token) continue;
        strncpy(songs[songCount].name, token, MAX_LEN - 1);

        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(songs[songCount].artist, token, MAX_LEN - 1);

        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(songs[songCount].mood, token, 63);

        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(songs[songCount].path, token, MAX_LEN - 1);

        songCount++;
    }

    fclose(fp);
    return 1;
}

/* ---------- Collect distinct moods present in the CSV ---------- */
int getDistinctMoods(char moodList[][64]) {
    int count = 0;
    for (int i = 0; i < songCount; i++) {
        int found = 0;
        for (int j = 0; j < count; j++) {
            if (strcasecmp_custom(moodList[j], songs[i].mood) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            strcpy(moodList[count], songs[i].mood);
            count++;
        }
    }
    return count;
}

/* ---------- Play a song directly in the console via MCI (no external player window) ---------- */
void playSong(const char *path) {
    char command[600];
    char errorBuf[256];

    /* Close any previous MCI session using the same alias, just in case */
    mciSendStringA("close song", NULL, 0, NULL);

    /* Open the file as an MCI device under the alias "song" */
    sprintf(command, "open \"%s\" type mpegvideo alias song", path);
    MCIERROR err = mciSendStringA(command, NULL, 0, NULL);
    if (err != 0) {
        mciGetErrorStringA(err, errorBuf, sizeof(errorBuf));
        printf("\n   Could not open the file: %s\n", errorBuf);
        printf("   Check that the path in the CSV is correct: %s\n", path);
        return;
    }

    printf("\n>> Now playing: %s\n", path);
    mciSendStringA("play song", NULL, 0, NULL);

    printf("   Controls: [p]ause  [r]esume  [s]top & show recommendations  [q]uit controls (song keeps playing)\n");

    int controlling = 1;
    while (controlling) {
        printf("   > ");
        int ch = getchar();

        if (ch == EOF) {
            controlling = 0; /* no more input available, stop waiting */
            break;
        }

        /* consume rest of line */
        if (ch != '\n') {
            clearInputLine();
        } else {
            continue; /* empty line, just show prompt again */
        }

        switch (tolower(ch)) {
            case 'p':
                mciSendStringA("pause song", NULL, 0, NULL);
                printf("   (paused)\n");
                break;
            case 'r':
                mciSendStringA("resume song", NULL, 0, NULL);
                printf("   (resumed)\n");
                break;
            case 's':
                mciSendStringA("stop song", NULL, 0, NULL);
                mciSendStringA("close song", NULL, 0, NULL);
                controlling = 0;
                break;
            case 'q':
                controlling = 0; /* leaves song playing in background */
                break;
            default:
                printf("   Unknown command. Use p / r / s / q.\n");
        }
    }
}

/* ---------- Recommend songs after one is played ---------- */
void recommendSongs(int playedIndex) {
    Song played = songs[playedIndex];
    int shown = 0;

    printf("\n==================== RECOMMENDATIONS ====================\n");

    /* 1) Same artist */
    printf("\n-- More from %s --\n", played.artist);
    int artistMatches = 0;
    for (int i = 0; i < songCount; i++) {
        if (i == playedIndex) continue;
        if (strcasecmp_custom(songs[i].artist, played.artist) == 0) {
            printf("  [%d] %s - %s (%s)\n", ++shown, songs[i].name, songs[i].artist, songs[i].mood);
            artistMatches++;
        }
    }
    if (artistMatches == 0) {
        printf("  (No other songs by this artist in your library)\n");
    }

    /* 2) Similar mood/type */
    printf("\n-- Similar %s songs --\n", played.mood);
    int moodMatches = 0;
    for (int i = 0; i < songCount; i++) {
        if (i == playedIndex) continue;
        if (strcasecmp_custom(songs[i].mood, played.mood) == 0 &&
            strcasecmp_custom(songs[i].artist, played.artist) != 0) {
            printf("  [%d] %s - %s (%s)\n", ++shown, songs[i].name, songs[i].artist, songs[i].mood);
            moodMatches++;
        }
    }
    if (moodMatches == 0) {
        printf("  (No other songs with this mood in your library)\n");
    }

    printf("===========================================================\n");
}

/* ---------- Main menu flow ---------- */
int main(void) {
    printf("====================================================\n");
    printf("      MOOD-BASED SONG RECOMMENDATION SYSTEM\n");
    printf("====================================================\n");

    if (!loadSongs(CSV_FILE)) {
        printf("\nPress Enter to exit...");
        getchar();
        return 1;
    }

    if (songCount == 0) {
        printf("No songs found in %s.\n", CSV_FILE);
        return 1;
    }

    printf("Loaded %d songs from %s\n", songCount, CSV_FILE);

    int running = 1;
    while (running) {
        /* Step 1: show mood menu */
        char moods[50][64];
        int moodCount = getDistinctMoods(moods);

        printf("\n---------- Select your mood ----------\n");
        for (int i = 0; i < moodCount; i++) {
            printf("  %d. %s\n", i + 1, moods[i]);
        }
        printf("  0. Exit\n");
        printf("----------------------------------------\n");
        printf("Enter choice: ");

        int moodChoice;
        int scanResult = scanf("%d", &moodChoice);
        if (scanResult == EOF) {
            break; /* no more input available (e.g. automated/non-interactive run) */
        }
        if (scanResult != 1) {
            clearInputLine(); /* clear bad input */
            continue;
        }

        if (moodChoice == 0) {
            running = 0;
            break;
        }
        if (moodChoice < 1 || moodChoice > moodCount) {
            printf("Invalid choice, try again.\n");
            continue;
        }

        char *selectedMood = moods[moodChoice - 1];

        /* Step 2: list songs matching that mood */
        int matchIndex[MAX_SONGS];
        int matchCount = 0;

        printf("\n---- Songs for mood: %s ----\n", selectedMood);
        for (int i = 0; i < songCount; i++) {
            if (strcasecmp_custom(songs[i].mood, selectedMood) == 0) {
                printf("  %d. %s - %s\n", matchCount + 1, songs[i].name, songs[i].artist);
                matchIndex[matchCount] = i;
                matchCount++;
            }
        }

        if (matchCount == 0) {
            printf("No songs found for this mood.\n");
            continue;
        }

        printf("  0. Back to mood menu\n");
        printf("Pick a song to play: ");

        int songChoice;
        int songScanResult = scanf("%d", &songChoice);
        if (songScanResult == EOF) {
            running = 0;
            break;
        }
        if (songScanResult != 1) {
            clearInputLine();
            continue;
        }

        if (songChoice == 0) continue;
        if (songChoice < 1 || songChoice > matchCount) {
            printf("Invalid choice.\n");
            continue;
        }

        int playedIndex = matchIndex[songChoice - 1];

        /* Step 3: play the song */
        playSong(songs[playedIndex].path);

        /* Step 4: recommend by artist + similar mood */
        recommendSongs(playedIndex);

        printf("\nPress Enter to go back to mood menu...");
        clearInputLine(); /* clear leftover newline */
        if (getchar() == EOF) break; /* no more input (e.g. automated run) */
    }

    printf("\nGoodbye! Enjoy your music.\n");
    return 0;
}
