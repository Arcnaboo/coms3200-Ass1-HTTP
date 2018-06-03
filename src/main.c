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

/* 
 * File:   main.c
 * Author: Arda 'Arc' Akgur
 * 
 * Program meant to compile on x64 based windows systems
 * 
 * Known issues:
 *      * There is a slight bug that crashes to program if requested https
 *          and then if user re-runs the program without exiting
 *      * Running program more than several times without exiting could cause
 *          memory issues that result program to crash unexpectedly
 * 
 * How to compile and run:
 *      * Program can compile with either MinGW or CyWin basic Gcc
 *      * Link ws2_32.a library when compiling
 * 
 * Created on March 12, 2018, 3:02 PM AST
 */


#include "utilities.h" // utilities for this program
#include "datetime.h" // time convert

// use Winsocket library
#pragma comment(lib,"ws2_32.lib")


// pointer to hostent struct in Winsock dll
struct hostent *he; 

char *missing; // reference if new location missing url

// Struct that holds address data
typedef struct {
    char *hostname;
    char *file;
    BOOL protocol;
    char ip[100];
    int port;
}ADDRESS;

// Map for storing HTTP response package data
typedef struct {
    char **key;
    char **value;
    u_int len;
    u_int max_size;
}ARCMAP;

// Struct that holds pointer to address and response map
typedef struct {
    ADDRESS *server;
    ADDRESS *client;
    ARCMAP *arcmap;
    int code;
    char *code_meaning;
}ANALYSER;


/*
 * Creates and returns pointer to a ARCMAP
 * map size given as param
 */
ARCMAP *get_blank_map(u_int size) {
    if (!size) return NULL;
    
    ARCMAP *res = (ARCMAP*) malloc(sizeof(ARCMAP));
    res->key = (char**) malloc(sizeof(char*) * size);
    res->value = (char**) malloc(sizeof(char*) * size);
    res->len = 0;
    for (int i = 0; i < size; i++) {
        res->key[i] = (char*) malloc(sizeof(char) * 1024);
        res->value[i] = (char*) malloc(sizeof(char) * 1024);
    }
    res->max_size = size;
    return res;
}

/*
 * Attempts to free the memory usage of
 * given ARCMAP
 */
void free_map(ARCMAP *map) {
    for (u_int i = 0; i < map->max_size; i++) {
        free(map->key[i]);
        free(map->value[i]);
    }
    free(map->key);
    free(map->value);
    free(map);
}

/*
 * Return corresponding value from the map
 * depending on the given key
 * returns NULL if map is NULL
 * returns NULL if key is not in map
 * returns corresponding value if successful
 */
char *get_from_map(ARCMAP *map, char *key) {
    if (map == NULL) return NULL;
    
    for (int i = 0; i < map->len; i++) {
        if (strcmp(key, map->key[i]) == 0) {
            return map->value[i];
        }
    }
    return NULL;
}

/*
 * Attempts to put given key-value
 * pairing into the given map
 * on success returns True
 * returns False if map is NULL
 * returns False if map is full
 */
BOOL put_to_map(ARCMAP *map, char *key, char*value) {
    if (map == NULL) return FALSE;
    if (map->len == map->max_size) return FALSE;
    int i;
    for (i = 0; i < strlen(key); i++) {
        map->key[map->len][i] = key[i];
    }
    map->key[map->len][i] = '\0';
    
    for (i = 0; i < strlen(value); i++) {
        map->value[map->len][i] = value[i];
    }
    map->value[map->len][i] = '\0';
    
    
    map->len++;
    return TRUE;
}

/*
 * Initializes windows socket library
 * Exits program if fails
 */
void initialise_winsock(WSADATA *wsa) {
    printf("Attempting to initialize Winsock...");
    if (WSAStartup(MAKEWORD(2,2), wsa) != 0) {
        printf("Failed. Error Code : %d",WSAGetLastError());
        exit(1);
    }
    printf("Successfully initialized\n");
}

/*
 * Creates Socket and returns it
 * Exits program if fails to create socket
 */
SOCKET create_sock(void) {
    SOCKET s;
    printf("Creating Socket...");
    if ((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET) {
        printf("Could not create socket : %d" , WSAGetLastError());
        exit(2);
    }
    printf("Successfully created\n");
    return s;
}

/*
 * Prompts user for hostname and returns as string pointer
 * Exits program if end of input from user
 */
char *get_hostname(void) {
    char *hostname = (char*) malloc(sizeof(char) * 1024);
    
    while (TRUE) {
        printf("Please Enter website address: ");
        if (!fgets(hostname, 1024, stdin)) {
            printf("End of input from user\n");
            exit(3);
        }
        if (hostname[0] != '\n') break;
    }
    
    hostname[strlen(hostname) - 1] = '\0';
    return hostname;
}

/*
 * Populates server struct with given ip address
 * param server - OUT - pointer to sockaddr_in
 * param ip - IN - String representation of IP to store
 * param port - IN - Port to use for connection
 */
void populate_server_info(struct sockaddr_in *server, char *ip, int port) {
    server->sin_addr.s_addr = inet_addr(ip);
    server->sin_family = AF_INET;
    server->sin_port = htons(port);
}

/*
 * Analyzes hostname
 * checks protocol
 * splits hostname from path
 * Param IN/OUT address that holds web address
 */
void analyze_hostname_input(ADDRESS *address) {
    printf("Analyzing: %s\n", address->hostname);
    char *temp = address->hostname;
    
    if (strncmp(address->hostname, "https://", 8) == 0) address->protocol = TRUE;
    else address->protocol = FALSE;
        
    
    char *protocol;
    int size;
    
    if (address->protocol) {
        protocol = strdup(HTTPS);
        size = 8;
    } else {
        protocol = strdup(HTTP);
        size = 7;
    }
    
    //printf("%s %d\n", protocol, size);
    // double check free call of this function, there might be memory leak
    if (strncmp(address->hostname, protocol, size) == 0) {
        // http://www.example.com/about
        // 01234567
        address->hostname = (char*)&address->hostname[size];
    }
    //puts(address->hostname);
    BOOL yes = FALSE;
    int i;
    char host[((int)strlen(address->hostname)) + 1];
    for (i = 0; i < strlen(address->hostname); i++) {
        if (address->hostname[i] == '/') {
            yes = TRUE;
            break;
        }
        host[i] = address->hostname[i];
    }
    host[i] = '\0';
    if (yes) address->file = strdup((char*)&address->hostname[i]);
    else address->file = strdup("/");
    
    free(temp);
    address->hostname = strdup((char*) &host[0]);
    
    free(protocol);
    
}

/*
 * Prompt user for hostname via get_hostname()
 * Populates pointer to a ADDRESS struct
 * with hostname and ip address
 * Exits program if user inputs 5 invalid hostname
 * Returns ADDRESS struct if successful
 */
ADDRESS *get_host_ip(void) {
    ADDRESS *res = (ADDRESS*) malloc(sizeof(ADDRESS));
    res->hostname = get_hostname();
    puts(res->hostname);
    analyze_hostname_input(res);
    
    printf("User Input: %s%s\n", res->hostname, res->file);
    int tries = 0;
    
    while ((he = gethostbyname(res->hostname)) == NULL) {
        printf("gethostbyname() failed : %d\n" , WSAGetLastError());
        free(res->hostname);
        res->hostname = get_hostname();
        analyze_hostname_input(res);
        printf("User Input: %s%s\n", res->hostname, res->file);
        tries++;
        if (tries == 5) {
            printf("Too many invalid hostname tries.. Exiting.\n");
            exit(5);
        }
    }
    return res;
}

/*
 * Attempts to get the host and ip of next address Location
 * This function should run if the initial website request 
 * moved to a different location with code 301 or similar
 * Returns the new pointer to a Address struct if successful
 * Returns NULL on fail
 */
ADDRESS *get_ip_from_prev(ANALYSER *prev) {
    ADDRESS *res = (ADDRESS*) malloc(sizeof(ADDRESS));
    char *host = get_from_map(prev->arcmap, "Location");
    char **splitted = split_url(host);
    
    if (strlen(splitted[0]) == 0 || strlen(splitted[0]) == 1) {
        char temp[200];
        strcpy(temp, missing);
        strcat(temp, "/");
        strcat(temp, splitted[1]);
        host = strdup((char*)&temp[0]);
    }
    for (int i = 0; i < 2; i++) {
        free(splitted[i]);
    }
    free(splitted);
    if (host) {
        res->hostname = strdup(host);
    } else {
        puts("Can not locate new Location");
        return NULL;
    }
    analyze_hostname_input(res);
    printf("New Host: %s # New Path: %s\n", res->hostname, res->file);
    if ((he = gethostbyname(res->hostname)) == NULL) {
        printf("gethostbyname() failed : %d\n" , WSAGetLastError());
        return NULL;
    }
    return res;
}

/*
 * Resolves hostname into IP address
 * param address - IN/OUT 
 * param addr_list - IN/OUT
 */
void resolve_ip(ADDRESS *address) {
    //Cast the h_addr_list to in_addr , since h_addr_list also has the ip address in long format only
    struct in_addr **addr_list = (struct in_addr **) he->h_addr_list;

    for(int i = 0; addr_list[i] != NULL; i++) {
        //Return the first one;
        strcpy(address->ip, inet_ntoa(*addr_list[i]));
        //address->ip = strdup(inet_ntoa(*addr_list[i]));
    }
     
    printf("%s resolved to : %s\n" , address->hostname , address->ip);
}

/*
 * Sends HTTP HEAD request to given SOCKET
 * param s - SOCKET to send request to
 * param file - Requested file default '/'
 * param hostname - website hostname
 */
void send_HTTP_request(SOCKET s, char *file, char *hostname) {
    char *request = (char*) malloc(strlen(file) + strlen(hostname) + 26);
    int done = sprintf(request, "HEAD %s HTTP/1.1\r\nHost: %s\r\n\r\n", 
            file, hostname);
    if (!done) {
        printf("Error writing to request string\n");
        exit(8);
    }
    printf("Sending: %s", request);
    if (send(s, request, strlen(request), 0) < 0) {
        printf("Send() failed\n");
        exit(6);
    }
    printf("HTTP Request Send to server\n");
}

/*
 * Attempts to get this client's ip and port
 * stores and returns in ADDRESS struct if successful
 * if fails returns NULL
 */
ADDRESS *get_client_info(SOCKET s) {
    struct sockaddr_in client;
    ADDRESS *ret = (ADDRESS*) malloc(sizeof(ADDRESS));
    int client_len = (int) sizeof(client);
    if (getsockname(s, (struct sockaddr*)&client, &client_len) == SOCKET_ERROR) {
        printf("Can't get client ip\n");
        return NULL;
    }
    printf("Client IP/PORT: %s/%d\n", inet_ntoa(client.sin_addr), (int)ntohs(client.sin_port));
    
    sprintf(ret->ip, "%s", inet_ntoa(client.sin_addr));
    ret->port = (int)ntohs(client.sin_port);
    return ret;
}


/*
 * Splits the given data properly
 * Adds the data to analyser->arcmap
 */
void split_and_add(ANALYSER *analyser, char *data) {
    // Encoding: blah blah
    // Location: http:
    char key[1024];
    char value[1024];
    int control = 0, k = 0, v = 0;
    
    for (int i = 0; i < strlen(data); i++) {
        if (data[i] == ':' && !control) {
            control++;
            continue;
        }
        if (data[i] == ' ' && control == 1) {
            control++;
            continue;
        }
        if (!control) {
            key[k++] = data[i];
        }
        if (control == 2) {
            value[v++] = data[i];
        }
    }
    key[k] = '\0';
    value[v] = '\0';
    if (!put_to_map(analyser->arcmap, (char*)&key[0], (char*)&value[0])) {
        printf("Unable to update arcmap.. Exiting\n");
        exit(21);
    }
    
}

/*
 * Populates the given analyser depending on the response
 */
void populate_analyser(ANALYSER *analyser, char *response) {

    u_int lines = how_many_lines(response, '\n');
    analyser->arcmap = get_blank_map((lines + 3));
    char **split = split_string(response, '\n'); 
    analyser->code_meaning = (char*) malloc(strlen(split[0]));
    change_carriage_return(split[0]);   
    char *code_n_mean = strdup((char*)&split[0][9]);   
    char *code = get_code(code_n_mean);
    char *meaning = strdup((char*)&code_n_mean[4]);
    
    if (!put_to_map(analyser->arcmap, "code", code) ||
            !put_to_map(analyser->arcmap, "meaning", meaning)) {
        printf("Unable to update arcmap.. Exiting\n");
        exit(21);
    }

    free(meaning);
    free(code);
    free(code_n_mean);
    
    for (u_int i = 1; i < lines; i++) {
        if (i != lines - 1) {
            change_carriage_return(split[i]);
            split_and_add(analyser, split[i]);
        }
        free(split[i]);
    }
    free(split[0]);
    free(split);
}

/*
 * Main Interact loop for connecting webserver
 * Recursively continues if requested page moved to new location
 * Returns number of times it jumps
 */
int interact(ANALYSER **analysers, BOOL mode, int jump) {
    SOCKET s = create_sock();
    struct sockaddr_in server;
    char *response;
    analysers[jump] = (ANALYSER*) malloc(sizeof(ANALYSER));
    
   
    
    if (mode) analysers[jump]->server = get_host_ip();
    else analysers[jump]->server = get_ip_from_prev(analysers[jump - 1]);
    
    if (analysers[jump]->server == NULL && !mode) {
        puts("can not continue jumping");
        return jump;
    }
    
    missing = analysers[jump]->server->hostname;
    resolve_ip(analysers[jump]->server);
    analysers[jump]->server->port = 80;
    if (analysers[jump]->server->protocol) {
        printf("SSL connection not implemented yet, cannot connect to: %s%s%s\n",
                HTTPS,
                analysers[jump]->server->hostname,
                analysers[jump]->server->file);
        
        analysers[jump]->server->port = 443;
        analysers[jump]->arcmap = get_blank_map(4);
        analysers[jump]->code_meaning = (char*) malloc(sizeof(char) * 100);
        strcpy(analysers[jump]->code_meaning, "SSL not implemented");
        put_to_map(analysers[jump]->arcmap, "code", "999");
        put_to_map(analysers[jump]->arcmap, "meaning", "SSL not implemented");
        strcpy(analysers[jump]->client->ip, "Did not connected to socket");
        analysers[jump]->client->port = 0;
        return jump; //remove this after implementing SSL
    }
    
    populate_server_info(&server, analysers[jump]->server->ip, 
            analysers[jump]->server->port);
    
    printf("Trying to connect to %s... ", analysers[jump]->server->ip);
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
        puts("Connection error");
        return jump;
    }
    puts("Connected.");
    
    analysers[jump]->client = get_client_info(s);
    send_HTTP_request(s, analysers[jump]->server->file, analysers[jump]->server->hostname);
    response = (char*) malloc(9999);
    int size;
    if ((size = recv(s , response , 8192 , 0)) == SOCKET_ERROR) {
        puts("recv() failed");
        return jump;
    }
    printf("Response received from server\n\n"); 
    response[size] = '\0';
    //puts(response);
    populate_analyser(analysers[jump], response);
    free(response);
    
    char *location = get_from_map(analysers[jump]->arcmap, "Location");
    closesocket(s);
    if (location) return interact(analysers, FALSE, (jump + 1));
    else return jump;
}

/*
 * Attempts to free the memory usage of the 
 * given address struct
 */
void free_address(ADDRESS *address) {
    if (address) {
        if (address->file) free(address->file);
        if (address->hostname) free(address->hostname);
        free(address);
    }
}

/*
 * Attempts to free the memory usage of the
 * given Analyser array
 */
void free_analysers(ANALYSER **analysers, int jump) {
    while (analysers != NULL && jump >= 0) {
        free_map(analysers[jump]->arcmap);

        free_address(analysers[jump]->client);

        free_address(analysers[jump]->server);
        
        if (analysers[jump]->code_meaning) free(analysers[jump]->code_meaning);
        
        free(analysers[jump]);
        jump--;
    }
    if (analysers) free(analysers);
    
}

/*
 * Generates and returns a Results string
 * using analyser array jump times
 */
char *get_results(ANALYSER **analysers, int jump) {
    char *results = (char*) malloc(sizeof(char) * 9999);
    
    strcpy(results, "HTTP Protocol Analyzer, Written by Arda Akgur, 43829114\n\n");
   
    
    for (int i = 0; i <= jump; i++) {
        strcat(results, "#####################################\n\n");
        char url[200], server[200], client[200], code[200], reply[200];
        
        snprintf(url, 200, "Url requested: %s%s\n\n", analysers[i]->server->hostname,
                analysers[i]->server->file);
        strcat(results, url);
  
        snprintf(server, 200, "Ip address, # Port of the server: %s,%d\n\n", 
                analysers[i]->server->ip,
                analysers[i]->server->port);
        
        strcat(results, server);
        
        snprintf(client, 200, "Ip address, # Port of the client: %s,%d\n\n",
                analysers[i]->client->ip,
                analysers[i]->client->port);
        
        strcat(results, client);
        
        snprintf(code, 200, "Reply code: %s\n\n", 
                get_from_map(analysers[i]->arcmap, "code"));
        
        strcat(results, code);
        
        snprintf(reply, 200, "Reply code meaning: %s\n\n", 
                get_from_map(analysers[i]->arcmap, "meaning"));
        
        strcat(results, reply);
        
        char dat[200], la[200], con[200], loc[200];
        
        char *date = get_from_map(analysers[i]->arcmap, "Date");
        if (date) {
            date = convert_GMT_to_AEST(date);
            snprintf(dat, 200, "Date: %s\n\n", date);
        }
        else snprintf(dat, 200, "Date: Not Included\n\n");
        
        strcat(results, dat);
        
        char *last = get_from_map(analysers[i]->arcmap, "Last-Modified");
        if (last) {
            last = convert_GMT_to_AEST(last);
            snprintf(la, 200, "Last-Modified: %s\n\n", last);
            
        }
        else snprintf(la, 200, "Last-Modified: Not Included\n\n");
        
        strcat(results, la);
        
        char *content = get_from_map(analysers[i]->arcmap, "Content-Encoding");
        if (content) snprintf(con, 200, "Content-Encoding: %s\n\n", content);
        else snprintf(con, 200, "Content-Encoding: Not Included\n\n");
        
        strcat(results, con);
        
        char *location = get_from_map(analysers[i]->arcmap, "Location");
        if (location) {
            snprintf(loc, 200, "Moved to: %s\n\n", location);
            strcat(results, loc);
        }
    }
    return results;
}

/*
 * Main loop
 */
int main(int argc, char **argv) {
    if (argc > 1) {
        printf("Usage: %s\n", argv[0]);
        return 1;
    }
    printf("COMS3200 - Assignment I - HTTP Protocol Analyser\n");
    printf("by Arda 'Arc' Akgur\n\n\n");
   
    WSADATA wsa;
    int jump = 0;
    initialise_winsock(&wsa);
    ANALYSER **analysers; 
    
    while (TRUE) {
        analysers = (ANALYSER**) malloc(sizeof(ANALYSER*) * 10);
        jump = interact(analysers, TRUE, jump);
        char *results = get_results(analysers, jump);
        
        if (!results) {
            printf("Something went wrong, can not display results\n");
            free_analysers(analysers, jump);
            goto Cleanup;
        }
        
        char *response = (char*) malloc(sizeof(char) * 124);
        BOOL done = FALSE;
        while (!done) {
            printf("Enter q/Q for quit..");
            printf("p/P for print results..");
            printf("s/S for save results to a file.\n");              
            printf("Please enter response key to continue: ");
            
        
            if (!fgets(response, 120, stdin)) {
                puts("End of input from user");
                goto Cleanup;
            }
            if (response[0] == '\n') continue;
            
            else if (response[0] == 'q' || response[0] == 'Q') {
                free(results);
                free_analysers(analysers, jump);
                goto Cleanup;
            } 
            
            else if (response[0] == 'p' || response[0] == 'P') {
                puts("Printing results:\n");
                puts(results);
                done = TRUE;
            }
            
            else if (response[0] == 's' || response[0] == 'S') {
                save_results(results);
                done = TRUE;
            } 
            
            else {
                printf("Unrecognized input: %s", response);
                continue;
            }
        }
        while (TRUE) {
            printf("Want to run the program again? Y/N ");
            if (!fgets(response, 100, stdin)) {
                puts("End of input from user");
                goto Cleanup;
            }
            if (response[0] == '\n') continue;
            
            if (response[0] == 'Y' || response[0] == 'y') {
                free_analysers(analysers, jump);
                free(response);
                jump = 0;
                break;
                
            } else if (response[0] == 'N' || response[0] == 'n') {
                printf("attempts to free analyzers..");
                free_analysers(analysers, jump);
                puts("done");
                printf("attempts to free response..");
                free(response);
                puts("done");
                goto Cleanup;
            } else {
                printf("Unrecognized input: %s", response);
                continue;
            }
        }
        
    }
    
    Cleanup:
        puts("Unloading Winsock library..");
        WSACleanup();
        puts("Thank you for using Arc's HTTP protocol analyzer");
        
    return (EXIT_SUCCESS);
}

