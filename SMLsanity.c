#include "types.h"
#include "stat.h"
#include "user.h"


// Sets the priority to 'prio', and starts working:
void mod0(int prio){
	int i,j;
	printf(1, "set_prio(%d)==%d\n", prio, set_prio(prio));
	for ( i = 0; i < 100; ++i)
	{
		for ( j = 0; j < 1000000; ++j)
		{
			// x++;
		}
	}
	exit();
}

char proc_types[3][64] = {"Low", "High"};

int
main(int argc, char *argv[])
{

	if (argc!=2){
		printf(1, "Not enough parameters!\nUsage:\n%s <num of forks>\n", argv[0]);
		exit();
	}

	int i;
	int pid, pid2;
	int stime, retime, rutime;
	int av_stime[2] = {0};
	int av_retime[2] = {0};
	int av_rutime[2] = {0};
	int n = atoi(argv[1]);

	printf(1, "Starting %d forks:\n",n);

	// fork and split them to lowest and highest priority:
	for (i = 0; i < n; ++i) {
		pid = fork();
		if (pid==0) { // child
			switch(getpid()%2){
				case 0:
					printf(1, "Creating low child");
					mod0(1);
				break;
				case 1:
					printf(1, "Creating high child");
					mod0(3);
				break;
			}	
		}	
	}
	
	for (i = 0; i < n; ++i)
	{
		pid2 = wait2(&retime, &rutime, &stime);
		printf(1, "Proc-ter! PID=%d, type=%s, wait=%d, runtime=%d, i/o=%d\n", pid2, proc_types[pid2%2], retime, rutime, stime);

		av_stime[pid2%2] += stime;
		av_retime[pid2%2] += retime;
		av_rutime[pid2%2] += rutime + stime + retime; // == Turnaround time -> total time
	}
	n = n/2;
	printf(1, "LOW priority Statistics:\n");

	av_stime[0] = av_stime[0] / n;
	printf(1, "Average Sleep-Time == %d\n", av_stime[0]);

	av_retime[0] = av_retime[0] / n;
	printf(1, "Average Ready-Time == %d\n", av_retime[0]);

	av_rutime[0] = av_rutime[0] / n;
	printf(1, "Average Turnaround-Time == %d\n", av_rutime[0]);

	printf(1, "************\n");


	printf(1, "HIGH priority Statistics:\n");
	av_stime[1] = av_stime[1] / n;
	printf(1, "Average Sleep-Time == %d\n", av_stime[1]);

	av_retime[1] = av_retime[1] / n;
	printf(1, "Average Ready-Time == %d\n", av_retime[1]);

	av_rutime[1] = av_rutime[1] / n;
	printf(1, "Average Turnaround-Time == %d\n", av_rutime[1]);

	printf(1, "************\n");
	exit();
}






// ########################################################################
// ########################################################################
// ########################## HAYMI AND NIR TEST ##########################
// ########################################################################
// ########################################################################
// ########################################################################




// #include "types.h"
// #include "stat.h"
// #include "user.h"

// typedef struct mod_stats{
// 	int counter;
// 	int tot_rutime;
// 	int tot_retime;
// 	int tot_stime;
// } mod_stats;

// mod_stats all_stats[3];

// void printTotalStatistics()
// {
// 	int i;
// 	for(i = 0; i < 3; i++){
// 		printf(1, "Average sleep time for Priority %d is - %d\n", i+1, all_stats[i].tot_stime / all_stats[i].counter);
// 		printf(1, "Average ready time for Priority %d is - %d\n", i+1, all_stats[i].tot_retime / all_stats[i].counter);
// 		printf(1, "Average turnaround time for Priority %d is - %d\n", i+1, (all_stats[i].tot_retime + all_stats[i].tot_stime + all_stats[i].tot_rutime) / all_stats[i].counter);
// 	}
// }

// void update_stats(int idx, int retime, int rutime, int stime)
// {
// 	all_stats[idx].counter++;
// 	all_stats[idx].tot_rutime += rutime;
// 	all_stats[idx].tot_retime += retime;
// 	all_stats[idx].tot_stime += stime;
// }

// void
// printAndUpdateStatistics(int priority, int retime, int rutime, int stime)
// {
// 	// printf(1, "PID = %d\n", pid);
// 	// printf(1, "Came From Func - %d\n", pid % 3);
// 	printf(1, "Priority = %d:\n\tCPU TIME = %d, S-CPU TIME = %d, IO TIME = %d\n",
// 		priority, rutime, retime, stime);
// 	update_stats(priority - 1, rutime, retime, stime);
// }

// void
// cpu_work()
// {
// 	int i, j;
// 	for(i = 0; i < 100; i++){
// 		for(j = 0; j < 1000000; j++)
// 		{
// 		}
// 	}
// 	printf(1, "0: i'm done\n");
// }

// void
// ready_work()
// {
// 	int i, j;
// 	for(i = 0; i < 100; i++){
// 		for(j = 0; j < 1000000; j++)
// 		{
// 		}
// 		yield();
// 	}
// 	printf(1, "1: i'm done\n");
// }

// void
// sleep_work()
// {
// 	int i;
// 	for(i = 0; i < 100; i++){
// 		sleep(1);
// 	}
// 	printf(1, "2: i'm done\n");
// }



// int
// main(int argc, char *argv[])
// {
// 	int i, pid, ppid;
// 	int n = atoi(argv[1]);
// 	for(i = 0; i < n; i++){
// 		pid = fork();
// 		if (pid == 0){
// 			ppid = getpid();
// 			set_prio((ppid % 3) + 1);
	    	
// 	    	cpu_work();
// 	    	ready_work();
// 	    	sleep_work();
// 	    	exit();
// 		} else // parent node
// 			continue;    
// 	}
// 	uint priority;
// 	int retime;
// 	int rutime;
// 	int stime;
// 	while ((pid=wait2(&retime, &rutime, &stime)) > 0) {
// 		//printf(1, "i waited for: %d\n", pid);
// 		priority = (pid%3) + 1;
// 		printAndUpdateStatistics(priority, retime, rutime, stime);
// 	}
// 	printTotalStatistics();	
// 	printf(1, "should be printed last...\n");
//   exit();
// }
