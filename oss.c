/*
 ============================================================================
 Name        : simulator.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <time.h>
# include <stdlib.h>
# include <dirent.h>
# include <stdio.h>
# include <string.h>
# include <getopt.h>
# include <stdbool.h>
# include <ctype.h>
# include <sys/wait.h>
# include <signal.h>
# include <sys/mman.h>
# include <sys/time.h>
# include <stdint.h>
# include <fcntl.h>
# include <sys/shm.h>
# include <semaphore.h>
# include "shmemsem.h"
# include <stdbool.h>
# include <sys/ipc.h>
# include <sys/msg.h>
//global variables
int billion=1000000000;

SharedData *shmem;
int shmid;
int msgid;

//Indicates if cleanup() was called due to alarm(), if this is false it means cleanup() was called because of other termination criteria
bool timeup=false;
//constants to determine when next process will be generated
int maxtimes=1;
int maxtimen=400000;

//stores the PID of the dispatched process
int currentprocessPID;
//set to true when message is sent from oss to user process
bool currentprocessstarted;


//threshold after which a process will be transfered from blocked queue to queue0,queue0 to queue1 and queue1 to queue2
int maxblockedn=1500;
int maxwaitqueue1=1250000;
int maxwaitqueue2=2050000;


int totalran;
int key= 11223344;
//random time when next process muxt be generated
int times;
int timen;

//stores the work that the last generated process picked
int lastpicked=0;

char *logfile1="log1.dat";

char *currentlogfile;

//to store average wait times for processes in queues
long avgwaittimequeue0=0;
long avgwaittimequeue1=0;
long avgwaittimequeue2=0;
long avgturnaround=0;

//the average time that work=0,1,2,3 were chosen
int avg0=0;
int avg1=0;
int avg2=0;
int avg3=0;
int totalchosen=0;

//total number of processes that ended, used to calculate average turn around time
int processended=0;
//set to true when work chosen by user =0(terminate) and =2(blocked) if this is false we compare with threshold values and assign queues respectively
bool queuedecided=false;

int emptyPCBcounter;//holds array index for empty row in PCB
void cleanup();
void catch_alarm(int sig);
void startprocess();
int emptyPCB();//is there a row empty in PCB?
void printPCB();
void printreadyqueue();
void dequeueready(int queue);
void dequeueblocked();
void emptyqueues();
int queue_for_traversal();
void printblockedqueue();
void adjustseconds(int n, int counter, int value);

struct mesg_buffer {
	long mesg_type;
    int mesg_text[4];
} message;
/*mesg.text[0]-Stores PID
mesg.text[1]= if (=1) process terminated, if (=2) process ran its time interval, if (=3) process ran till little time and went in blocked queue
*/

void cleanup()
{
	float p0=(float)avg0/totalchosen;
	p0=p0*100.0;
	float p1=(float)avg1/totalchosen;
	p1=p1*100.0;
	float p2=(float)avg2/totalchosen;
	p2=p2*100.0;
	float p3=(float)avg3/totalchosen;
	p3=p3*100.0;

	float avgturnaround=(float)processended/shmem->pidcounter;
	avgturnaround=avgturnaround*100.0;

	printf("Total user processes =%d\n",shmem->pidcounter);
	printf("Average wait time for processes in queue0 = %dns queue1 = %dns queue2 = %dns\n",avgwaittimequeue0,avgwaittimequeue1,avgwaittimequeue2);

	printf("work=0 was chosen %.2f%% of the time, work=1 was chosen %.2f%% of the time,work=2 was chosen %.2f%% of the time, work=3 was chosen %.2f%% of the time\n",p0,p1,p2,p3);
	printf("Average turn around=%.2f%%\n",avgturnaround);
	printf("printing all the queues\n");
	printreadyqueue();
	printblockedqueue();
	printPCB();
	if(currentprocessstarted==true)
	{
		sleep(1);
	}
	emptyqueues();
	if(timeup==true)
		printf("time is up due to -t\n");
	emptyqueues();
	printf("Doing cleanup\n");
	//killready();//kill all processes in ready queue
	msgctl(msgid, IPC_RMID, NULL);
	shmdt(shmem);// Free up shared memory
	shmctl(shmid, IPC_RMID,NULL);

	exit(0);
}
int queue_for_traversal()
{
	if(shmem->queue0[shmem->front0]!=0)
		return 0;
	else if(shmem->queue1[shmem->front1]!=0)
		return 1;
	else if(shmem->queue2[shmem->front2]!=0)
		return 2;
	else
		return -1;
}
void emptyqueues()
{
	int i;
	for(i=0;i<100;i++)
	{
		if(shmem->queue0[i]!=0)
		{
			kill(shmem->queue0[i],SIGKILL);
		}
		if(shmem->queue1[i]!=0)
		{
			kill(shmem->queue1[i],SIGKILL);
		}
		if(shmem->queue2[i]!=0)
		{
			kill(shmem->queue2[i],SIGKILL);
		}
		if(shmem->blockedqueue[i][0]!=0)
		{
			kill(shmem->blockedqueue[i][0],SIGKILL);
		}
	}

}

void catch_alarm (int sig)//SIGALRM handler
{
	timeup=true;
	cleanup();
}
int emptyPCB()
{
	int i;
	for(i=0;i<20;i++)//make <5 to see oss running in queue1. uncomment user code rand()%4 to see oss running in queue2
	{
		if(shmem->pcb[i][0]==0)
		{
			//printf("%d is empty\n",i);
			return (i);
		}
	}
	//printf("No empty spot in PCB\n");
	return (-1);

}
void printPCB()
{
	printf("\ncontents of PCB:\n");
	int i;
	for(i=0;i<20;i++)
	{
		//if(shmem->pcb[i][0]!=0)
		printf("%d-> %d %d %d %d %d %d %d %d %d\n",i,shmem->pcb[i][0],shmem->pcb[i][1],shmem->pcb[i][2],shmem->pcb[i][3],shmem->pcb[i][4],shmem->pcb[i][5],shmem->pcb[i][6],shmem->pcb[i][7],shmem->pcb[i][8]);
	}
}
void printreadyqueue()
{
	printf("\nContents of ready queue:\n");
	int i;
	printf("\nqueue0:\n");
	for(i=0;i<100;i++)
	{
		if(shmem->queue0[i]!=0)
		printf(" %d ",shmem->queue0[i]);
		//else
			//break;
	}
	printf("\n");
	printf("\nqueue1:\n");
	for(i=0;i<100;i++)
	{
		//if(shmem->queue0[i]!=0)
		printf(" %d ",shmem->queue1[i]);
		//else
			//break;
	}
	printf("\n");
	printf("\nqueue2:\n");
	for(i=0;i<100;i++)
	{
		//if(shmem->queue0[i]!=0)
		printf(" %d ",shmem->queue2[i]);
		//else
			//break;
	}
	printf("\n");
}
void printblockedqueue()
{
	printf("\ncontents of blockedqueue:\n");
	int i,j;
	for(i=0;i<100;i++)
	{
		if(shmem->blockedqueue[i][0]!=0)
		{
			for(j=0;j<3;j++)
			{
				printf(" %d ",shmem->blockedqueue[i][j]);
			}
			printf("\n");
		}

	}
	printf("\n");

}
void adjustseconds(int n,int counter,int value)
{
	if(n==8)
	{
		//printf("before adjusting %d:%d\n",shmem->pcb[counter][7],shmem->pcb[counter][8]);
		if(value>billion)
		{
			do
			{
				shmem->pcb[counter][7]++;
				shmem->pcb[counter][8]=shmem->pcb[counter][8]-billion;
			}while(shmem->pcb[counter][8]>billion);
			//printf("After adjusting %d:%d\n",shmem->pcb[counter][7],shmem->pcb[counter][8]);
		}
		//printf("Adjusted seconds for wait time value %d:%d\n",shmem->pcb[counter][7],shmem->pcb[counter][8]);
	}
	else if(n==4)
	{
		//printf("before adjusting %d:%d\n",shmem->pcb[counter][3],shmem->pcb[counter][4]);
		if(value>billion)
		{
			do
			{
				shmem->pcb[counter][3]++;
				shmem->pcb[counter][4]=shmem->pcb[counter][4]-billion;
			}while(shmem->pcb[counter][4]>billion);
		}
		//printf("Adjusted seconds for CPU time value %d:%d\n",shmem->pcb[counter][3],shmem->pcb[counter][4]);
	}

}
void dequeueready(int queue)
{
	int i;
	if(queue==0)
	{
		for(i=0;i<100;i++)
		{
			if(shmem->queue0[i]!=0)
				shmem->queue0[i]=shmem->queue0[i+1];
			else
				break;
		}
		shmem->tail0--;
	}
	if(queue==1)
	{
		for(i=0;i<100;i++)
		{
			if(shmem->queue1[i]!=0)
				shmem->queue1[i]=shmem->queue1[i+1];
			else
				break;
		}
		shmem->tail1--;
	}
	if(queue==2)
	{
		for(i=0;i<100;i++)
		{
			if(shmem->queue2[i]!=0)
				shmem->queue2[i]=shmem->queue2[i+1];
			else
				break;
		}
		shmem->tail2--;
	}
}
void dequeueblocked()
{
	int i,j;
		for(i=0;i<100;i++)
		{
			if(shmem->blockedqueue[i][0]!=0)
			{
				for(j=0;j<3;j++)
				{
					shmem->blockedqueue[i][j]=shmem->blockedqueue[i+1][j];
				}
			}
		}
	shmem->blockedtail--;
}
void updatePCB(int process)
{
	int i;
	for(i=0;i<100;i++)
	{
		if(shmem->pcb[i][0]==process)
		{
			shmem->pcb[i][0]=0;
			shmem->pcb[i][1]=0;
			shmem->pcb[i][2]=0;
			shmem->pcb[i][3]=0;
			shmem->pcb[i][4]=0;
			shmem->pcb[i][5]=0;
			shmem->pcb[i][6]=0;
			shmem->pcb[i][7]=0;
			shmem->pcb[i][8]=0;
		}
	}
}
void startprocess()
{
	signal (SIGALRM, catch_alarm);
	alarm (20);

	int i;

	while(1)
	{
		if(shmem->pidcounter>=100)
		{
			printf("Forked 100 processes\n");
			cleanup();
			exit(0);
		}

		if((emptyPCBcounter=emptyPCB())>=0)
		{
			//printf("PCB empty\n");
			//printf("times=%d timen=%d clocks=%d clockn=%d\n",times,timen,shmem->seconds,shmem->nanoseconds);
			if(shmem->seconds>=times || (shmem->seconds==times && shmem->nanoseconds>=timen))
			{
				//printf("Future time reached going to fork\n");
				pid_t pID = fork();
				if (pID < 0)
				{
					perror("oss: Failed to fork:");
					exit(EXIT_FAILURE);
				}
				//launch user process and send a message to it using PID as msg type
				else if (pID == 0)
				{
					static char *args[]={"./user",NULL};
					int status;
					if(( status= (execv(args[0], args)))==-1)
					{
						perror("oss:failed to execv");
						exit(EXIT_FAILURE);
					}
					//else
						//printf("execv to user sucessfully\n");
				}
				FILE *fp;
				fp = fopen(currentlogfile, "a+");
	            if (fp == NULL)
	            {
	                perror("oss: Unable to open the output file:");
	            }
	            else
                {
                	//printf("\nentering data in file");
                	fprintf(fp,"Generating process %d and putting it in queue 0 at time %d:%d\n",pID,shmem->seconds,shmem->nanoseconds);
                }
                fclose(fp);


				printf("just forked process PID=%d\n",pID);
				shmem->pidcounter++;
				shmem->queue0[shmem->tail0]=pID;
				shmem->tail0++;
				//printf("emptypcbcounter=%d",emptyPCBcounter);

				shmem->pcb[emptyPCBcounter][0]=pID;
				shmem->pcb[emptyPCBcounter][1]=shmem->seconds;//Since when is it in the system?
				shmem->pcb[emptyPCBcounter][2]=shmem->nanoseconds;

				//printreadyqueue();
				//printPCB();

				//setting up time for next process to run
				int rann = (rand() % maxtimen);
				int rans = (rand() % maxtimes);
				//printf("\n for future rann: %d rans: %d",rann,rans);
				//printf("\n clock %d:%d",shmem->seconds,shmem->nanoseconds);
				times= shmem->seconds+rans;
				timen=shmem->nanoseconds+rann;
				//printf("future process time= %d:%d\n",times,timen);
				//printf("ready front %d ready tail %d\n",shmem->readyfront,shmem->readytail);
			}
		}//endif(emptyPCB)
		//if PCB is not empty but the future process time has been reached. We generate another time in future when a process must be forked.
		else if(emptyPCB()==false && (shmem->seconds>=times || (shmem->seconds==times && shmem->nanoseconds>=timen)))
		{
			//printf("Future time was reached but the PCB is full\n");
			//setting up time for next process to run
			int rann = (rand() % maxtimen);
			int rans = (rand() % maxtimes);
			//printf("\n for future rann: %d rans: %d\n",rann,rans);
			//printf("\n clock %d:%d",shmem->seconds,shmem->nanoseconds);
			times= shmem->seconds+rans;
			timen=shmem->nanoseconds+rann;

		}

		//sleep(1);
		//SCHEDULING ALGO
		if(currentprocessstarted==false)
		{
			//checking to see if we have anything in blocked queue
			if(shmem->blockedtail!=shmem->blockedfront)
			{
				bool removeprocess=false;
				//printf("There is a process in blocked queue\n");
				int timespentblockeds=(shmem->seconds)-(shmem->blockedqueue[shmem->blockedfront][1]);
				int timespentblockedn=(shmem->nanoseconds)-(shmem->blockedqueue[shmem->blockedfront][2]);
				//printf(" time spent in blocked queue by process s=%d n=%d \n",timespentblockeds,timespentblockedn);
				//checks if time spent by process in blocked queue is more than threshold maxblockedn
				if(shmem->seconds==timespentblockeds)
					if(timespentblockedn>=maxblockedn)
						removeprocess=true;
				if(shmem->seconds>timespentblockeds)
					removeprocess=true;

				if(removeprocess==true)
				{
						//printf("process in blocked queue has reached maximum time\n");

						int blockedPID=shmem->blockedqueue[shmem->blockedfront][0];
						shmem->queue0[shmem->tail0]=shmem->blockedqueue[shmem->blockedfront][0];
						shmem->tail0++;
						dequeueblocked();
						//printblockedqueue();

						FILE *fp;
						fp = fopen(currentlogfile, "a+");
			            if (fp == NULL)
			            {
			                perror("oss: Unable to open the output file:");
			            }
			            else
		                {
			            	//printf("process %d moved from blocked queue and put in queue 0 \n",blockedPID);
			            	fprintf(fp,"process %d moved from blocked queue and put in queue 0 at time %d:%d\n",blockedPID,shmem->seconds,shmem->nanoseconds);
		                }
		                fclose(fp);

				}//endif
			}//endif something in blocked queue
			//checks if there is something in ready queue and if we are not running anything from blocked queue
			int currentqueue=queue_for_traversal();
			//printf("currentqueue=%d\n",currentqueue);
			if(currentqueue<0)
			{
				printf("All the queues are empty\n");
				cleanup();
			}
			if(currentqueue==0)
			{
				currentprocessPID=shmem->queue0[shmem->front0];
			}
			else if(currentqueue==1)
			{
				currentprocessPID=shmem->queue1[shmem->front1];
			}
			else if(currentqueue==2)
			{
				currentprocessPID=shmem->queue2[shmem->front2];
			}
			FILE *fp;
			fp = fopen(currentlogfile, "a+");
            if (fp == NULL)
            {
                perror("oss: Unable to open the output file:");
            }
            else
            {
            	fprintf(fp,"Dispatching process %d from queue %d at time %d:%d\n",currentprocessPID,currentqueue,shmem->seconds,shmem->nanoseconds);
            }
            fclose(fp);
			//printf("front of queue process: %d\n",currentprocessPID);
			dequeueready(currentqueue);
			//printf("printing ready queue after dequeue()\n");
			//printreadyqueue();
			//printf("tail=%d",shmem->readytail);

			message.mesg_type = currentprocessPID;
			message.mesg_text[0]=getppid();
			message.mesg_text[1]=lastpicked;
			//printf("BEFORE msgsnd is sent from oss\n");
			int sendm=msgsnd(msgid, &message, sizeof(message), 0);
			//delay
			int sum = 0;
			for (i = 0; i < 10000000; i++)
			 sum += i;

			if( sendm==-1)
			{	perror("error in sending message to user process");
				fprintf(stderr,"error: %d\n",currentprocessPID);
				exit(0);
			}

			else
			{
				printf("message sent to process %d\n",currentprocessPID);

				//printf("\n clock %d:%d",shmem->seconds,shmem->nanoseconds);
				currentprocessstarted=true;

				//updating avg wait time for the process in the system
				for(i=0;i<20;i++)
				{	if(shmem->pcb[i][0]==currentprocessPID)
					{
						int lastbursts=shmem->pcb[i][5];
						int lastburstn=shmem->pcb[i][6];
						//new wait time=last time it was scheduled- time it was scheduled now + overhead
						int newwaittime=((shmem->seconds*billion)+shmem->nanoseconds)-((lastbursts*billion)+lastburstn)+100;//100 for overhead
						shmem->pcb[i][5]=shmem->seconds;//When was the last time it ran?
						shmem->pcb[i][6]=shmem->nanoseconds;

						//Checks if process was scheduled as soon as it entered the system
						if(shmem->pcb[i][1]==shmem->pcb[i][5] && shmem->pcb[i][2]==shmem->pcb[i][6])
							shmem->pcb[i][8]=100;//wait time=little overhead from scheduling
						//Checks if the values are empty, this means that the process is running for the first time
						if(shmem->pcb[i][7]==0 && shmem->pcb[i][8]==0)
						{
							int enteredsystemnano=(shmem->pcb[i][1]*billion)+shmem->pcb[i][2];
							int gotschedulednano=(shmem->seconds*billion)+shmem->nanoseconds;
							shmem->pcb[i][8]=gotschedulednano-enteredsystemnano+100;//avg wait time for process in the system in nanoseconds
						}
						else if(shmem->pcb[i][7]!=0 || shmem->pcb[i][8]!=0)//compare the wait time with previous wait time
						{
							int previouswaittime=(shmem->pcb[i][7]*billion)+shmem->pcb[i][8];//in nanoseconds
							//taking average of both wait times
							shmem->pcb[i][8]=(previouswaittime+newwaittime)/2;//avg wait time for process in the system in nanoseconds
						}
						if(currentqueue==0)
							avgwaittimequeue0=(avgwaittimequeue0+shmem->pcb[i][8])/2;
						if(currentqueue==1)
							avgwaittimequeue1=(avgwaittimequeue1+shmem->pcb[i][8])/2;
						if(currentqueue==2)
							avgwaittimequeue2=(avgwaittimequeue2+shmem->pcb[i][8])/2;
						if(shmem->pcb[i][8]>billion)
							adjustseconds(8,i,shmem->pcb[i][8]);
					}
				}
			}
			sleep(1);

			//receive message from user
			if(msgrcv(msgid, &message, sizeof(message), getppid(), 0)==-1)
			{
				perror("error in recieving message from user process");
				exit(0);
			}
			else
			{
				int timeused=0;
				totalran++;
				printf("message received from %d \n",message.mesg_text[0]);
				lastpicked=message.mesg_text[1];
				if(lastpicked==0)
					avg0++;
				if(lastpicked==1)
					avg1++;
				if(lastpicked==2)
					avg2++;
				if(lastpicked==3)
					avg3++;
				totalchosen++;
				//printf("lastpicked from user to oss %d\n",lastpicked);
				//adding time to clock according to work (1-3) selected by user process
				if(message.mesg_text[1]==0)
				{
					queuedecided=true;
					processended++;
					//printf("process %d wants to terminate\n",currentprocessPID);
					int corpse,status;
					FILE *fp;
					fp = fopen(currentlogfile, "a+");
		            if (fp == NULL)
		            {
		                perror("oss: Unable to open the output file:");
		            }
		            else
	                {
		            	fprintf(fp,"process %d wants to terminate at time %d:%d\n",currentprocessPID,shmem->seconds,shmem->nanoseconds);
	                }
	                fclose(fp);
					while ((corpse = waitpid(message.mesg_text[0], &status, 0)) != message.mesg_text[0] && corpse != -1)
					{
						char pmsg[64];
						snprintf(pmsg, sizeof(pmsg), "logParse: PID %d exited with status 0x%.4X", corpse, status);
						perror(pmsg);
					}
					updatePCB(currentprocessPID);
				}
				else if(message.mesg_text[1]==1)
				{
					if(currentqueue==0)
					{
						timeused=2000000;
						shmem->nanoseconds=shmem->nanoseconds+2000000;//adding 2ms to clock
						if(shmem->nanoseconds>=billion)
						{
							shmem->seconds++;
							shmem->nanoseconds=0;
						}

					}
					else if(currentqueue==1)
					{
						timeused=4000000;
						shmem->nanoseconds=shmem->nanoseconds+4000000;//adding 4ms to clock
						if(shmem->nanoseconds>=billion)
						{
							shmem->seconds++;
							shmem->nanoseconds=0;
						}
					}
					else if(currentqueue==2)
					{
						timeused=8000000;
						shmem->nanoseconds=shmem->nanoseconds+8000000;//adding 8ms to clock
						if(shmem->nanoseconds>=billion)
						{
							shmem->seconds++;
							shmem->nanoseconds=0;
						}
					}
					FILE *fp;
					fp = fopen(currentlogfile, "a+");
		            if (fp == NULL)
		            {
		                perror("oss: Unable to open the output file:");
		            }
		            else
	                {
		            	fprintf(fp,"process %d ran for its complete time quantum %d nanoseconds\n",currentprocessPID,timeused);
	                }
	                fclose(fp);

					queuedecided=false;
				}

				else if(message.mesg_text[1]==2)
				{
					FILE *fp;
					fp = fopen(currentlogfile, "a+");
		            if (fp == NULL)
		            {
		                perror("oss: Unable to open the output file:");
		            }
		            else
	                {
		            	fprintf(fp,"process %d is being put in blocked queue, ran for %d:%d at time %d:%d\n",currentprocessPID,message.mesg_text[2],message.mesg_text[3],shmem->seconds,shmem->nanoseconds);
	                }
	                fclose(fp);
	                int r=message.mesg_text[2]*billion;
	                timeused=r+message.mesg_text[3];
					shmem->seconds=shmem->seconds+message.mesg_text[2];//r
					shmem->nanoseconds=shmem->nanoseconds+message.mesg_text[3];//s
					if(shmem->nanoseconds>=billion)
					{
						shmem->seconds++;
						shmem->nanoseconds=0;
					}
					//printf("r=%d s=%d\n",r,s);
					shmem->blockedqueue[shmem->blockedtail][0]=currentprocessPID;
					shmem->blockedqueue[shmem->blockedtail][1]=shmem->seconds;
					shmem->blockedqueue[shmem->blockedtail][2]=shmem->nanoseconds;
					shmem->blockedtail++;
					queuedecided=true;
					//including overhead for queues, 1000 for putting in queue 1000 for removing from blocked queue and putting in ready queue
					timeused=2000;
					//printf("added process to blocked queue\n");

				}
				else if(message.mesg_text[1]==3)
				{
					//adding p% of q to clock
					int p=message.mesg_text[2];
					if(currentqueue==0)
					{
						int temp=(p*2000000)/100;
						timeused=2000000-temp;
					}
					else if(currentqueue==1)
					{
						int temp=(p*4000000)/100;
						timeused=4000000-temp;
					}
					else if(currentqueue==2)
					{
						int temp=(p*8000000)/100;
						timeused=8000000-temp;
					}
					FILE *fp;
					fp = fopen(currentlogfile, "a+");
		            if (fp == NULL)
		            {
		                perror("oss: Unable to open the output file:");
		            }
		            else
	                {
		            	fprintf(fp,"process %d ran for %d percent of its time\n",currentprocessPID,p);
	                }
	                fclose(fp);

					shmem->nanoseconds=shmem->nanoseconds+timeused;//p
					if(shmem->nanoseconds>=billion)
					{
						shmem->seconds++;
						shmem->nanoseconds=0;
					}

					queuedecided=false;
				}
				//adding time for moving processes in different queues and scheduling overhead
				timeused=timeused+1000;
				//printf("Total time used by process=%d\n",timeused);
				FILE *fp;
				fp = fopen(currentlogfile, "a+");
	            if (fp == NULL)
	            {
	                perror("oss: Unable to open the output file:");
	            }
	            else
                {
	            	fprintf(fp,"Total time this dispatch lasted %d nanoseconds (including overhead)\n",timeused);
                }
                fclose(fp);

				shmem->nanoseconds=shmem->nanoseconds+1000;
				if(shmem->nanoseconds>=billion)
				{
					shmem->seconds++;
					shmem->nanoseconds=0;
				}
				int i_in_pcb;
				//printf("%d process is done with its work\n",message.mesg_text[0]);
				for(i=0;i<20;i++)
				{
					if(shmem->pcb[i][0]==message.mesg_text[0])
					{
						//if the values are 0 means this is the first time it ran, so we just store the values
						if(shmem->pcb[i][4]==0)
						{
							shmem->pcb[i][4]=timeused;
						}
						else//Avg CPU time used by the process in nanoseconds
						{
							shmem->pcb[i][4]=(shmem->pcb[i][4]+timeused)/2;
						}
						if(shmem->pcb[i][4]>billion)
							adjustseconds(4,i,shmem->pcb[i][4]);
						i_in_pcb=i;
					}
				}
				int newqueue;
				if(queuedecided==false)
				{
					int totalwaitnano=(shmem->pcb[i_in_pcb][3]*billion)+shmem->pcb[i_in_pcb][4];
					//printf("total wait time for process =%d \n",totalwaitnano);
					if(currentqueue==0)
					{
						if(totalwaitnano>maxwaitqueue1)
						{
							//printf("Putting in queue1: %d>%d\n",totalwaitnano,maxwaitqueue1);
							FILE *fp;
							fp = fopen(currentlogfile, "a+");
				            if (fp == NULL)
				            {
				                perror("oss: Unable to open the output file:");
				            }
				            else
			                {
				            	fprintf(fp,"Process %d being moved from queue0 to queue 1 since total wait %d > %d maximum wait threshold \n",currentprocessPID,totalwaitnano,maxwaitqueue1);
			                }
			                fclose(fp);
							newqueue=1;
							shmem->queue1[shmem->tail1]=currentprocessPID;
							shmem->tail1++;
						}
						else
						{

							newqueue=0;
							FILE *fp;
							fp = fopen(currentlogfile, "a+");
				            if (fp == NULL)
				            {
				                perror("oss: Unable to open the output file:");
				            }
				            else
			                {
				            	fprintf(fp,"Process %d stays in queue0 since total wait %d < %d maximum wait threshold \n",currentprocessPID,totalwaitnano,maxwaitqueue1);
			                }
			                fclose(fp);
							//printf("Putting in queue0: %d<%d\n",totalwaitnano,maxwaitqueue1);
							shmem->queue0[shmem->tail0]=currentprocessPID;
							shmem->tail0++;
						}

					}
					else if(currentqueue==1)
					{
						if(totalwaitnano>maxwaitqueue2)
						{
							newqueue=2;
							FILE *fp;
							fp = fopen(currentlogfile, "a+");
				            if (fp == NULL)
				            {
				                perror("oss: Unable to open the output file:");
				            }
				            else
			                {
				            	fprintf(fp,"Process %d being moved from queue1 to queue 2 since total wait %d > %d maximum wait threshold \n",currentprocessPID,totalwaitnano,maxwaitqueue2);
			                }
			                fclose(fp);

							//printf("Putting in queue2: %d>%d\n",totalwaitnano,maxwaitqueue2);
							shmem->queue2[shmem->tail2]=currentprocessPID;
							shmem->tail2++;
						}
						else
						{
							newqueue=1;
							FILE *fp;
							fp = fopen(currentlogfile, "a+");
				            if (fp == NULL)
				            {
				                perror("oss: Unable to open the output file:");
				            }
				            else
			                {
				            	fprintf(fp,"Process %d stays in queue1 since total wait %d < %d maximum wait threshold \n",currentprocessPID,totalwaitnano,maxwaitqueue2);
			                }
			                fclose(fp);
							//printf("Putting in queue1: %d<%d\n",totalwaitnano,maxwaitqueue2);
							shmem->queue1[shmem->tail1]=currentprocessPID;
							shmem->tail1++;
						}
					}
					else if(currentqueue==2)
					{
						newqueue=2;
						FILE *fp;
						fp = fopen(currentlogfile, "a+");
			            if (fp == NULL)
			            {
			                perror("oss: Unable to open the output file:");
			            }
			            else
		                {
			            	fprintf(fp,"Process %d stays in queue2 \n",currentprocessPID);
		                }
		                fclose(fp);
						//printf("Putting in queue2\n");
						shmem->queue2[shmem->tail2]=currentprocessPID;
					    shmem->tail2++;

					}
				}
				queuedecided=false;
				//printf("printing queues at the end of process");
				currentprocessPID=0;
				currentprocessstarted=false;

				//printreadyqueue();
				//printblockedqueue();
				//printPCB();
				//printf("Clock after finishing with process : %d %d\n",shmem->seconds,shmem->nanoseconds);

			}//endelse

		}//endif(currentprocessstarted)
}//endwhile
}
int main(int argc, char **argv)
{
	//printf("\nHello from oss");
	//initializing everything
	timeup=false;
	signal (SIGALRM, catch_alarm);
	int i,j;

	// Get our shm id
	shmid = shmget(key, sizeof(SharedData), 0600 | IPC_CREAT);
	if (shmid == -1) {
		perror("oss:Failed to allocate shared memory region");
		exit(-1);
	}

	// Attach to our shared memory
	shmem = (SharedData*)shmat(shmid, (void *)0, 0);
	if (shmem == (void *)-1) {
		perror("oss:Failed to attach to shared memory region");
		exit(-1);
	}


	for(i=0;i<20;i++)
	{
		for(j=0;j<9;j++)
		{
			shmem->pcb[i][j]=0;
		}
	}
	for(i=0;i<100;i++)
	{
		shmem->queue0[i]=0;
		shmem->queue1[i]=0;
		shmem->queue2[i]=0;
	}
	shmem->front0=0;
	shmem->front1=0;
	shmem->front2=0;

	shmem->tail0=0;
	shmem->tail1=0;
	shmem->tail2=0;

	shmem->pidcounter=-1;
	shmem->nanoseconds=0;
	shmem->seconds=0;

	shmem->blockedfront=0;
	shmem->blockedtail=0;
	int rann = (rand() % maxtimen);
	int rans = (rand() % maxtimes);
	//printf("\nFirst process generation at time: rann:%d rans:%d\n",rann,rans);
	//jumping to time
	shmem->seconds=rans;
	shmem->nanoseconds=rann;
	currentprocessstarted=false;

	msgid = msgget(key, 0666 | IPC_CREAT);

	emptyPCBcounter=-1;

	currentlogfile=logfile1;
	totalran=0;
	//printreadyqueue();
	startprocess();

    return 0;
}
