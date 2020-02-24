Functions and flow of the program :

void cleanup();
void catch_alarm(int sig);
void startprocess();
int emptyPCB();
void printPCB();
void printreadyqueue();
void dequeueready(int queue);
void dequeueblocked();
void emptyqueues();
int queue_for_traversal();
void printblockedqueue();
void adjustseconds(int n, int counter, int value);

The program starts with oss initializing all the global variables making the fields in queues and PCB =0. setting up the front and tail values, picking a random number when the oss should generate the first process (timen,times) and we just jump to that time and call startprocess(). The startprocess() is where the actual work begins. The oss checks timen and times and calls emptyPCB() to check is an empty spot is present in the PCB and forks a process. Then the process checks if there is anything in the blocked queue that has reached its threshold value (maxblockedn) and if yes it puts it in queue0. Next the oss calls queue_for_traversal() which goes through the front for each queue and returns 0,1,2 for queue0,queue1,queue2 respectively. We then take the process at the front of this queue and dequeueready() and send a message to that process with message type=PID of the process. We update the wait time for the process in PCB. We then immediately receive a message back from the process, take note of the work it selected and add to the clock, we update the PCBs by calling adjustseconds() and put the process in respective queue depending on the time used by the process. In user the message is received, the user sends back a message with message type = PID of oss, with the work that it has selected(0-4) along with respective r,s,p values if it applicable. The project terminates when there are 100 processes created or when alarm sounds (Mostly the case for my program). Just before cleanup() I print all the queues, PCB, average some statistics and total number of used processes generated.

Problems during the project
1. I had a lot of difficulty in understanding what the project was supposed to look like. I wrote nearly 5 different codes before finally submitting because I wasn't sure, how the multi level queues were supposed to be setup, How the threshold for queues need to be setup, the clock continously incrementing or not etc etc. Which is why I met professor mark nearly 10 times and emailed nearly 25 times in two weeks (sorry). I'm still not sure if the code is actually what was asked of us. Nevertheless I will keep working on perfecting the code more in my free time. 

2. The biggest problem of my program is that it is very slow. If you increased the alarm() to 40 then around 28 user processes get generated. This is why the alarm() is currently 20 in my program even when the description says '3 real life seconds'. The reason for this is, the user processes were running too fast before the oss had a chance to send the message and vice versa. I had to fix all these by putting a sleep(1) as suggested by professor mark. 

3. As a result of the slowness the project is only able to run through the first queue (queue0). It barely runs the second queue. But if we limit the user processes to 5 (make the for loop in function emptypcb() i<5) then we can see the processes in queue1 and queue2 get to run. Also, because of this the average wait time for processes in queue1 and queue2 is 0 most of the time, Since the oss is not able to reach this queue. 

4. Another big problem my project had was that the user processes kept picking either 3 or 1 randomly when I did rand()%4. This was funny and exhuasting as I tried for 2 days to get around it. I finally made a pattern where the processes pick numbers in the order:
if lastpicked= 0->this process picks 3
if lastpicked= 1->this process picks 2
if lastpicked= 3->this process picks 1
if lastpicked= 2->this process picks 0
I know this is terrible and completely different from the description. But the only reason I did this was so you could see the processes terminating and going into blocked queue. However if you want, you can uncomment the work =rand()%4 line in user code and notice the randomness which doesn't look that random (pun intended).

5. The threshold values are constants that I fine tuned so there can be something in all queues at all times. If we limit the user processes to say 5 and make work=rand()%4 in user code, then we can notice the the oss running in queue1 and queue2. 

6. Sometimes when the alarm is called after message has been sent to user process we get an error (identifier removed), we can also notice that at this time the queues will have one less process than the PCB. This was another problem that I tried to fix by adding delays but sometimes the error does get displayed.

7. Since there aren't a lot of processes generated I did not put the check for log file exceeding 1000 lines.

8. I've never turned in a project with so many problems. I am truely embarrased. But in all honesty I started working on the project from the first day and I put in a lot of hardwork in this project. I am happy that something is working but I could do better. Thank you for your support professor mark.