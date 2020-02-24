#ifndef SHMEMSEM_H
typedef struct {
	int seconds;
	int nanoseconds;

	int pcb[20][9];
	int pidcounter;

	int queue0[100];//q=2ms
	int front0;
	int tail0;

	int queue1[100];//q=4ms
	int front1;
	int tail1;

	int queue2[100];//q=8ms
	int front2;
	int tail2;

	int blockedfront;
	int blockedtail;
	int blockedqueue[100][3];

} SharedData;

#define SHMEMSEM_H
#endif
/* PCB:
 0-local PID
 1-TimeInSystemseconds
 2-TimeInSystemnanoseconds
 3-Avg CpuTimeUsedByProcessInSeconds;
 4-Avg CpuTimeUsedByProcessInNanoSeconds;
 5-LastBurts;
 6-LastBurstn;
 7-Avg WaitTimeForProcessInSeconds
 8-Avg WaitTimeForProcessInNanoseconds
*/
/* Blocked queue
 0-PID
 1-when it was put in queue seconds
 2- when it was put in queue nanoseconds
 */
