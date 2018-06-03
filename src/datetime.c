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
#include "datetime.h"

#define AEST "AEST"
//Date: Wed, 21 Oct 2015 07:28:00 GMT

typedef struct {
    char *day;
    int nday;
    char *month;
    int year;
    int hour;
    int min;
    int sec;
}ARCDATE;

static ARCDATE *arc_date = NULL; // arc_date to be used in this file

static BOOL next_day = FALSE; // is it next_day after 10 hrs

static BOOL first_time = TRUE; // is it first time this file runs

static const char *days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", (char*) 0};

static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", 
                            "Aug", "Sep", "Oct", "Nov", "Dec", (char*) 0};


static const int max_dates[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/*
 * Initializes the arc_date
 */
static void initialize_date(void) {
    if (arc_date == NULL) {
        arc_date = (ARCDATE*) malloc(sizeof(ARCDATE));
    }
}

/*
 * Populates the static arc_date struct with given info
 */
static void populate_date(const char *date) {
    //Wed, 21 Oct 2015 07:28:00 GMT
    char d[5], nd[5], mo[5], y[6], h[5], mi[5], s[5];
    int w = 0;
    int c = 0;
    BOOL go = TRUE;
    for (int i = 0; i < strlen(date); i++) {
    
        if (c == 0) {
            if (date[i] == ',') {
                    d[w] = '\0';
                    w = 0;
                    c++;
                    continue;
                }
                d[w++] = date[i];
        } 
        else if (c == 1) {
            if (date[i] == ' ') {
                if (go) {
                    go = FALSE;
                    continue;
                } else {
                    go = TRUE;
                    nd[w] = '\0';
                    c++;
                    w = 0;
                    continue;
                }
            }
            nd[w++] = date[i];
            
        }
        else if (c == 2) {
            if (date[i] == ' ') {
                mo[w] = '\0';
                c++;
                w = 0;
                continue;
            }
            mo[w++] = date[i];
        }
        else if (c == 3) {
            if (date[i] == ' ') {
                y[w] = '\0';
                c++;
                w = 0;
                continue;
            }
            y[w++] = date[i];
        }
        else if (c == 4) {
            if (date[i] == ':') {
                h[w] = '\0';
                c++;
                w = 0;
                continue;
            }
            h[w++] = date[i];
        }
        else if (c == 5) {
            if (date[i] == ':') {
                mi[w] = '\0';
                c++;
                w = 0;
                continue;
            }
            mi[w++] = date[i];
        }
        else if (c == 6) {
            if (date[i] == ' ') {
                s[w] = '\0';
                c++;
                break;
            }
            s[w++] = date[i];
        }
        else break;
    }
    if (!first_time) free(arc_date->day);
    if (!first_time) free(arc_date->month);
    
    arc_date->day = strdup((char*)&d[0]);
    arc_date->month = strdup((char*)&mo[0]);
    
    arc_date->hour = atoi((char*)&h[0]);
    arc_date->min = atoi((char*)&mi[0]);
    arc_date->sec = atoi((char*)&s[0]);
    arc_date->year = atoi((char*)&y[0]);
    arc_date->nday = atoi((char*)&nd[0]);
    
    first_time = FALSE;
    
}

/* 
 * Checks if current year in the arcdate is leap or not 
 */
static BOOL is_it_leap_year(void) {
    if (arc_date->year % 4 != 0) return FALSE;
    else if (arc_date->year % 100 != 0) return TRUE;
    else if (arc_date->year % 400 != 400) return FALSE;
    else return TRUE;
}

/*
 * Increments hour by 10 and returns new hour
 */
static int get_new_hour(void) {
    int new_hour = arc_date->hour;
    for (int i = 0; i < 10; i++) {
        if (new_hour == 24) {
            new_hour = 0;
            next_day = TRUE;
        }
        new_hour++;    
    }
    return new_hour;
}

/*
 * Returns the next days index
 */
static int find_next_day(void) {
    int i;
    for (i = 0; days[i]; i++) {
        if (strcmp(days[i], arc_date->day) == 0) {
            break;
        }
    }
    if (i == 6) return 0;
    else return (i + 1);
}

/*
 * Returns current months index
 */
static int get_current_month(void) {
    int i;
    for (i = 0; months[i]; i++) {
        if (strcmp(arc_date->month, months[i]) == 0) {
            break;
        }
    }
    return i;
}

/*
 * Returns next months index
 */
static int get_next_month_index(int current) {
    if (current == 11) return 0;
    else return (current + 1);
}

/*
 * This function takes HTTP Date formatted text as param
 * Initializes and populates the static ARCDATE struct
 * Converts this date from GMT to AEST
 * returns same formatted string
 */
char *convert_GMT_to_AEST(char *date) {
    //Wed, 21 Oct 2015 07:28:00 GMT
    
    initialize_date();
    populate_date(date);
    arc_date->hour = get_new_hour();
    
    int next_index = (-1);
    int current_month = get_current_month();
    
    int max_day = max_dates[current_month];
    
    
    if (strcmp(arc_date->month, "Feb") == 0 && is_it_leap_year()) {
        max_day = 29;
    }
    
    if (next_day) {
       
        next_index = find_next_day();
        
        arc_date->day = strdup(days[next_index]);
        
        if (arc_date->nday == max_day) {
            arc_date->nday = 1;
            int next_mon = get_next_month_index(current_month);
            arc_date->month = strdup(months[next_mon]);
            if (next_mon == 0) {
                arc_date->year++;
            }
        }
    }
    //char *ret = (char*) malloc(sizeof(char) * 100);
    char r[100];
    
    
    /*printf("Date: %s converted to ", date);
    printf("%s, %d %s %d %d:%d:%d AEST\n", arc_date->day, 
            arc_date->nday, arc_date->month, arc_date->year, 
            arc_date->hour, arc_date->min, arc_date->sec); */
    
    sprintf(r, "%s, %d %s %d %d:%d:%d AEST", 
            arc_date->day, arc_date->nday, arc_date->month, 
            arc_date->year, arc_date->hour, arc_date->min, arc_date->sec);
    
    
    return strdup((char*)&r[0]);
    
}