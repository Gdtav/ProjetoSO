#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *cfg = fopen("config.txt","r");
    int *n_threads = (int*) malloc(sizeof(int));
    if (cfg) {
        start_server(cfg);
        fclose(cfg);
    } else {
        printf("Config file not accessible. Exiting...");
        return 1;
    }
    return 0;
}