#include "types.h"
#include "stat.h"
#include "user.h"


// Sets the priority to 'prio', and starts working:
void mod0(int prio){
	int x;
	printf(1, "set_prio==%d\n",set_prio(prio));
	for (int i = 0; i < 100; ++i)
	{
		for (int j = 0; j < 1000000; ++j)
		{
			x++;
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

	printf(1, "LOW priority Statistics:\n");

	av_stime[0] = av_stime[0] / n / 2;
	printf(1, "Average Sleep-Time == %d\n", av_stime[0]);

	av_retime[0] = av_retime[0] / n / 2;
	printf(1, "Average Ready-Time == %d\n", av_retime[0]);

	av_rutime[0] = av_rutime[0] / n / 2;
	printf(1, "Average Turnaround-Time == %d\n", av_rutime[0]);

	printf(1, "************\n");


	printf(1, "HIGH priority Statistics:\n");
	av_stime[1] = av_stime[1] / n / 2;
	printf(1, "Average Sleep-Time == %d\n", av_stime[1]);

	av_retime[1] = av_retime[1] / n / 2;
	printf(1, "Average Ready-Time == %d\n", av_retime[1]);

	av_rutime[1] = av_rutime[1] / n / 2;
	printf(1, "Average Turnaround-Time == %d\n", av_rutime[1]);

	printf(1, "************\n");
	exit();
}

