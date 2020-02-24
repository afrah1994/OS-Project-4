
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
# include "shmemsem.h"
# include <sys/ipc.h>
# include <semaphore.h>
# include <stdbool.h>
# include <sys/ipc.h>
# include <sys/msg.h>
SharedData *shmem;
int key= 11223344;

int shmid;
struct mesg_buffer {
    long mesg_type;
    int mesg_text[4];
} message;
char *logfile1="log1.dat";
char *logfile2="log2.dat";
int work=0;
void main()
{
		int pid=getpid();
		int error1,work;

		//fflush(stdout);
		//fprintf(stderr,"STARTING USER PROCESS %d\n",getpid());
		//fflush(stderr);
		shmid = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);
		if (shmid == -1) {
			perror("user:Failed to allocate shared memory region");
			exit(-1);
		}

		// Attach to our shared memory
		shmem = (SharedData*)shmat(shmid, (void *)0, 0);
		if (shmem == (void *)-1) {
			perror("user:Failed to attach to shared memory region");
			exit(-1);
		}
		//shmem->lastpicked=0;
		int msgid = msgget(key, 0666 | IPC_CREAT);

		if(msgid==-1)
			{
			perror("User:error in message get");
			exit(0);
			}
		//else
			//printf("msgid=%d\n",msgid);
		while(1)
		{
				error1=msgrcv(msgid, &message, sizeof(message), pid, 0);
				if(error1==-1)
				{
					fprintf(stderr,"error: %d\n",getpid());
					perror("User: Error in receieving message from OSS ");
					exit(0);
				}
				else
				{
					int osspid=message.mesg_text[0];
					int lastpicked=message.mesg_text[1];
					if(lastpicked==0)
						work=3;
					if(lastpicked==1)
						work=2;
					if(lastpicked==3)
						work=1;
					if(lastpicked==2)
						work=rand()%1;
					//work=rand()%4;


					FILE *fp;
					fp = fopen("log1.dat", "a+");
		            if (fp == NULL)
		            {
		                perror("User: Unable to open the output file:");
		            }
		            else
	                {
		            	fprintf(fp,"User %d chose work=%d\n",getpid(),work);
	                }
		            fclose(fp);
		            fflush(stdout);
					fprintf(stderr,"%d chose work=%d\n",getpid(),work);
					fflush(stderr);


					if(work==0)
					{
						//printf("the process is ending\n");
						message.mesg_type=osspid;
						message.mesg_text[0]=pid;
						message.mesg_text[1]=0;

						if( msgsnd(msgid, &message, sizeof(message), 0)==-1)
							perror("error in sending message back to OSS");
						//else
							//	printf("message sent to oss now I can die in peace\n");
						exit(0);
					}

					if(work==1)
					{
						//printf("the process finished its full time quantom\n");
						message.mesg_type=osspid;
						message.mesg_text[0]=pid;
						message.mesg_text[1]=1;

						if( msgsnd(msgid, &message, sizeof(message), 0)==-1)
							perror("error in sending message back to OSS");
						//else
							//	printf("message sent to oss now I can die in peace\n");
					}
					if(work==2)
					{
						int r=(rand() % 1);//random number between 0-5
						int s= (rand() % 1001);//random number between 0-1000
						message.mesg_type=osspid;
						message.mesg_text[0]=pid;
						message.mesg_text[1]=2;
						message.mesg_text[2]=r;
						message.mesg_text[3]=s;
						//printf("r=%d s=%d\n",r,s);
						//fflush(stdout);
						//fprintf(stderr,"r=%d s=%d\n",r,s);
						//fflush(stderr);

						if( msgsnd(msgid, &message, sizeof(message), 0)==-1)
							perror("error in sending message back to OSS");
						//else
							//printf("message sent to oss now I can die in peace\n");
					}
					if(work==3)
					{
						int p =(rand() % (99 - 1 + 1)) + 1;
						message.mesg_type=osspid;
						message.mesg_text[0]=pid;
						message.mesg_text[1]=3;
						message.mesg_text[2]=p;
						//fflush(stdout);
						//fprintf(stderr,"p=%d\n",p);
						//fflush(stderr);

						if( msgsnd(msgid, &message, sizeof(message), 0)==-1)
							perror("error in sending message back to OSS");
						//else
							//printf("message sent to oss now I can die in peace\n");
					}
				}//endif
		}
}

