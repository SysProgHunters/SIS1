#include "top_proc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <proc/readproc.h>

int main() {
    struct timespec tm;
    tm.tv_sec  = 0;
    tm.tv_nsec = 500 * 1000 * 1000; 

    myproc_t* myprocs = NULL;
    unsigned int myprocs_len = 0;
    sample_processes(&myprocs, &myprocs_len, tm);
    
    
    qsort(myprocs, myprocs_len, sizeof(myprocs[0]), myproc_comp_pcpu);
    
    printf("%-10s %-10s %-10s %-10s\n", "PID", "<CPU>", "RAM", "CMD");
    for (unsigned int i = 0; i < myprocs_len && i < 10; ++i) {
        if (strlen(myprocs[i].cmd) == 0) {
            break;
        }
        printf("%-10d %-10.2f %-10lu %-10s\n", myprocs[i].tid, myprocs[i].pcpu, myprocs[i].vm_rss, myprocs[i].cmd);
    }

    printf("\n");
    
    free(myprocs);
}
