/*
 * The MIT License
 *
 * Copyright 2018 Arda 'Arc' Akgur.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "utilities.h"


/*
 * Replaces the carriage return '\r' character
 * with Null character
 */
void change_carriage_return(char *data) {
    for (int i = 0; i < strlen(data); i++) {
        if (data[i] == '\r') data[i] = '\0';
    }
}

/*
 * Counts how many times the Delimeter appears in 
 * the given data
 */
u_int how_many_lines(char *data, char delim) {
    u_int count = 0;
    for (int i = 0; i < strlen(data); i++) {
        if (data[i] == delim) count++;
    }
    return count;
}

/*
 * Splits the given string into array of strings
 * Split occurs via delimeter character
 * Return value needs to be freed after usage
 */
char **split_string(char *str, char delim) {
    char **out = (char**) malloc(sizeof(char*) * how_many_lines(str, delim));
    for (u_int i = 0; i < how_many_lines(str, delim); i++)
        out[i] = (char*) malloc(sizeof(char) * 1024);
    int line = 0;
    int pos = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == delim) {
            out[line][pos] = '\0';
            line++;
            pos = 0;
            continue;
        }
        out[line][pos++] = str[i];
    }
    return out;
}

/*
 * Writes the given results to a file
 * it asks user for a output file name
 */
void save_results(char *results) {
    
    char filename[100];
    FILE *file = NULL;
    while (file == NULL) {
        printf("Please enter output file name(ex: out.txt): ");
        if (!fgets(filename, 99, stdin)) {
            puts("End of input from user, saving output to output.txt");
            strcpy(filename, "output.txt");
        }
        filename[strlen(filename) - 1] = '\0';
        file = fopen(filename, "w");
        if (!file) puts("unable to open file for save");
    }
    
    fputs(results, file);
    
    fclose(file);
    
}

char **split_url(char *url) {
    // www.abc.com/news
    // /news/sport
    char **ret = (char**) malloc(sizeof(char*) * 2);
    for (int i = 0; i < 2; i++) {
        ret[i] = (char*) malloc(sizeof(char) * 300);
    }
    int k = 0;
    int z = 0;
    BOOL ok = TRUE;
    for (int i = 0; i < strlen(url); i++) {
        if (ok && url[i] == '/') {
            ret[k][z] = '\0';
            k++;
            z = 0;
            ok = FALSE;
            continue;
        }
        ret[k][z++] = url[i];
    }
    ret[k][z] = '\0';
    
    return ret;
}

/*
 * Gets and returns the Code from the given data
 * Data input must be like 302 Found, 404 Not Found, 200 OK etc..
 * Return value is a new string pointer
 */
char *get_code(char *data) {
    // 302 Found
    char *out = (char*) malloc(4);
    for (int i = 0; i < 3; i++) {
        out[i] = data[i];
    }
    out[3] = '\0';
    return out;
}

/*
 * Duplicates the given string
 * Returns a brand new String pointer
 */
char *strdup(const char *data) {
    
    char *out = (char*) malloc(strlen(data) + 1);
    int i;
    for (i = 0; i < strlen(data); i++) {
        out[i] = data[i];
    }
    out[i] = '\0';
    return out;
}