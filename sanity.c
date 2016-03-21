#include "types.h"
#include "stat.h"
#include "user.h"

void mod0(){
	int x;
	for (int i = 0; i < 100; ++i)
	{
		for (int j = 0; j < 1000000; ++j)
		{
			x++;
		}
	}
	exit();
}

void mod1(){
	int x;
	for (int i = 0; i < 100; ++i)
	{
		for (int j = 0; j < 1000000; ++j)
		{
			x++;
		}
		yield();
	}
	exit();
}

void mod2(){
	
	for (int i = 0; i < 100; ++i)
	{
		sleep(1);
	}
	exit();
}

char proc_types[3][64] = {"CPU", "S-CPU", "I/O"};

int
main(int argc, char *argv[])
{
	if (argc!=2){
		printf(1, "Not enough parameters!\nUsage:\n%s <num of forks/3>\n", argv[0]);
		exit();
	}

	int i;
	int pid, pid2;
	int stime, retime, rutime;
	int av_stime[3] = {0};
	int av_retime[3] = {0};
	int av_rutime[3] = {0};
	int n = atoi(argv[1]);

	printf(1, "Starting %d*3 = %d tests:\n",n,n*3);

	for (i = 0; i < 3*n; ++i) {
		pid = fork();
		if (!pid) { // child
			switch(getpid()%3){
				case 0:
					mod0();
				break;
				case 1:
					mod1();
				break;
				case 2:
					mod2();
				break;
			}	
		}
	}

	for (i = 0; i < 3*n; ++i)
	{
		pid2 = wait2(&retime, &rutime, &stime);
		printf(1, "Proc-ter! PID=%d, type=%s, wait=%d, runtime=%d, i/o=%d\n", pid2, proc_types[pid2%3], retime, rutime, stime);

		av_stime[pid2%3] += stime;
		av_retime[pid2%3] += retime;
		av_rutime[pid2%3] += rutime + stime + retime; // == Turnaround time -> total time
	}
	for (i=0; i<3; i++){
		printf(1, "%s Statistics:\n", proc_types[i]);

		av_stime[i] = av_stime[i] / n;
		printf(1, "Average Sleep-Time == %d\n", av_stime[i]);

		av_retime[i] = av_retime[i] / n;
		printf(1, "Average Ready-Time == %d\n", av_retime[i]);

		av_rutime[i] = av_rutime[i] / n;
		printf(1, "Average Turnaround-Time == %d\n", av_rutime[i]);

		printf(1, "************\n");
	}

  exit();
}

