#include "top_proc.h"

#include <proc/readproc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

int proc_comp_tid(const void* e1, const void* e2) {
    proc_t* p1 = *(proc_t**)e1;
    proc_t* p2 = *(proc_t**)e2;
    
    if (p1->tid < p2->tid) {
        return -1;
    } else if (p1->tid > p2->tid) {
        return 1;
    } else {
        return 0;
    }
}


int myproc_comp_pcpu(const void* e1, const void* e2) {
    myproc_t* p1 = (myproc_t*)e1;
    myproc_t* p2 = (myproc_t*)e2;
    
    if (p1->pcpu < p2->pcpu) {
        return 1;
    } else if (p1->pcpu > p2->pcpu) {
        return -1;
    } else {
        return 0;
    }
}


int myproc_comp_rss(const void* e1, const void* e2) {
    myproc_t* p1 = (myproc_t*)e1;
    myproc_t* p2 = (myproc_t*)e2;
    
    if (p1->vm_rss < p2->vm_rss) {
        return 1;
    } else if (p1->vm_rss > p2->vm_rss) {
        return -1;
    } else {
        return 0;
    }
}


unsigned long long get_total_cpu_time() {
    FILE* file = fopen("/proc/stat", "r");
    if (file == NULL) {
        perror("Could not open stat file");
        return 0;
    }
    
    char buffer[1024];
    unsigned long long user = 0, nice = 0, system = 0, idle = 0;
    unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0, guest = 0, guestnice = 0;
    
    char* ret = fgets(buffer, sizeof(buffer) - 1, file);
    if (ret == NULL) {
        perror("Could not read stat file");
        fclose(file);
        return 0;
    }
    fclose(file);
    
    sscanf(buffer,
           "cpu  %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guestnice);
    

    return user + nice + system + idle + iowait + irq + softirq + steal;
}


void freeproctab(proc_t** procs) {
    proc_t** p;
    for(p = procs; *p; ++p) {
	freeproc(*p);
    }
    free(procs);
}


void sample_processes(
        myproc_t** myprocs,
        unsigned int* myprocs_size,
        struct timespec sample_time) {
    unsigned long long total_cpu_time = get_total_cpu_time();
    
    long num_cpus = sysconf(_SC_NPROCESSORS_ONLN);


    proc_t** procs1 = readproctab(PROC_FILLCOM | PROC_FILLSTAT | PROC_FILLSTATUS);
    if (!procs1) {
        perror("readproctab() failed");
        return;
    }
    unsigned int procs1_len;
    for (procs1_len = 0; procs1[procs1_len]; ++procs1_len) {}
    

    qsort(procs1, procs1_len, sizeof(procs1[0]), proc_comp_tid);
    

    nanosleep(&sample_time, NULL);


    total_cpu_time = get_total_cpu_time() - total_cpu_time;


    proc_t** procs2 = readproctab(PROC_FILLCOM | PROC_FILLSTAT | PROC_FILLSTATUS);
    if (!procs2) {
        perror("readproctab() failed");
        return;
    }
    unsigned int procs2_len;
    for (procs2_len = 0; procs2[procs2_len]; ++procs2_len) {}
    

    qsort(procs2, procs2_len, sizeof(procs2[0]), proc_comp_tid);
    

    *myprocs_size = (procs1_len < procs2_len ? procs1_len : procs2_len);
    *myprocs = calloc(*myprocs_size * sizeof(myproc_t), sizeof(myproc_t));
    unsigned int pos1 = 0, pos2 = 0;
    unsigned int newpos = 0;
    while (pos1 < procs1_len && pos2 < procs2_len) {
        if (procs1[pos1]->tid < procs2[pos2]->tid) {
            ++pos1;
        } else if (procs1[pos1]->tid > procs2[pos2]->tid) {
            ++pos2;
        } else {

            (*myprocs)[newpos].tid = procs2[pos2]->tid;
            (*myprocs)[newpos].vm_rss = procs2[pos2]->vm_rss;
            strncpy((*myprocs)[newpos].cmd, procs2[pos2]->cmd, 15);


            unsigned long long cpu_time = (
                (procs2[pos2]->utime + procs2[pos2]->stime) - 
                (procs1[pos1]->utime + procs1[pos1]->stime));
            (*myprocs)[newpos].pcpu = (cpu_time / (float)total_cpu_time) * 100.0 * num_cpus;

            ++pos1;
            ++pos2;
            ++newpos;
        }
    }
    
    freeproctab(procs1);
    freeproctab(procs2);
}
