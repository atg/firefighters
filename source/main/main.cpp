#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool IS_CLIENT, IS_SERVER;
static void check_if_server(int argc, char *argv[]) {
    IS_CLIENT = true;
    IS_SERVER = false;
    
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--server")) {
            IS_CLIENT = false;
            IS_SERVER = true;
        }
    }
}

int main(int argc, char *argv[]) {
    
    // Are we a server?
    check_if_server(argc, argv);
    
    // Set up run loop
    
    if (IS_CLIENT) {
        // Set up rendering, keyboard and mouse
    }
    
    // Set up networking
    
    return 0;
}
