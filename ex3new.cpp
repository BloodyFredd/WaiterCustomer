#include<iostream>
#include <iomanip>
#include <time.h>
#include<string>
#include<vector>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/shm.h>
using namespace std;

//#define SNAME1 "/srmutex"
//#define SNAME2 "/swmutex"
//#define SNAME3 "/sreadtry"
//#define SNAME4 "/sresource"
#define SEMPERM 0600

struct Menu {
    int Id;
    char Name[16];
    float Price;
    int TotalOrdered;
};

struct orderBoard {
    int CustomerId;
    int ItemId;
    int Amount;
    int Done;
};

static int *ReadCount=0,*WriterCount=0;
int rmutex, wmutex , readTry, resource;
static orderBoard *OB;
static float *Ptime;


void printMenu(Menu *menu, int length){
    cout << setfill('=') << setw(25) << "Menu list" << setfill('=') << setw(25) << "\n";
    cout << "Id" << setfill(' ') << setw(15) << "Name" << setfill(' ') << setw(15) << "Price" << setfill(' ') << setw(15) << "Orders\n";
    for (int i = 0; i < length; i++)
        cout << menu[i].Id << setfill(' ') << setw(15) << menu[i].Name << setfill(' ') << setw(15) << menu[i].Price << setfill(' ') << setw(15) << menu[i].TotalOrdered << "\n";
    cout << setfill('=') << setw(50) << "\n";
}

int checkArgs(int argc, char* argv[]){
    if(argc == 5){
        if(atoi(argv[2]) > 7 || atoi(argv[3]) > 10 || atoi(argv[4]) > 3)
            return 0;
        return 1;
    }
    return 0;
}

int initsem(key_t semkey, int initval)
{
	int status = 0, semid;
	union semun 
	{
		int val;
		struct semid_ds *stat;
		ushort *array;
	} ctl_arg;

	if ( ( semid = semget(semkey, 1, SEMPERM | IPC_CREAT | IPC_EXCL) ) == -1 )
	{
		if( errno ==  EEXIST )
		{
			if ( (semid = semget(semkey, 1, 0) ) == -1)
			{
				printf("Error creating semaphores\n");
				exit(0);			
			}
		}
		else
		{
			printf("Error creating semaphores\n");
			exit(0);
		}					
	}		
	ctl_arg.val = initval;
	status = semctl(semid, 0, SETVAL, ctl_arg);
	if(status == -1)
	{
		perror("initsem failed");
		exit(0);
	}	
	return semid;	
}

void p(int semid)
{
	struct sembuf p_buf;
	p_buf.sem_num = 0;
	p_buf.sem_op = -1;
	p_buf.sem_flg = SEM_UNDO;
	if( semop( semid, &p_buf, 1) == -1 )
	{
		perror("p(semid) failed");
		exit(0);
	}
}

void v(int semid)
{
	struct sembuf v_buf;
	v_buf.sem_num = 0;
	v_buf.sem_op = 1;
	v_buf.sem_flg = SEM_UNDO;

	if( semop( semid, &v_buf, 1 ) == -1 )
	{
		perror("v(semid) failed");
		exit(0);
	}
} 


void Waiter()
{
  p(readTry);
  p(rmutex);
  (*ReadCount)++;
  if((*ReadCount) == 1)
	p(resource);
  v(rmutex);
  v(readTry);

  sleep(3);
  cout<<"Critical section\n";
 

  p(rmutex);
  (*ReadCount)--;
  if((*ReadCount) == 0)
	v(resource);	
  v(rmutex);
}

void* Customer(int temp)
{
  int SleepTime=rand()%6+3;
  sleep(SleepTime);
  cout<<"im the pid in here:"<<temp<<"\n";
  p(wmutex);
  (*WriterCount)++;
  if((*WriterCount) == 1)
	p(readTry);
  v(wmutex);
  p(resource);
	//cout<<"Critical section with pid: "<<temp<<" and time " << *Ptime << "\n";
	 // sleep(3);
	
  v(resource);
  cout<<"im out with pid: "<<temp<<"\n";
  
  p(wmutex);
  (*WriterCount)--;
  if( (*WriterCount) == 0)
	v(readTry);
  v(wmutex);
}

int main(int argc, char* argv[])
{
    clock_t t1;
    Ptime=static_cast<float*>(mmap(NULL,sizeof *Ptime, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *Ptime=0;

    WriterCount=static_cast<int*>(mmap(NULL,sizeof *WriterCount, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *WriterCount=0;

    ReadCount=static_cast<int*>(mmap(NULL,sizeof *ReadCount, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *ReadCount=0;

    key_t semkey1   = ftok(".", 1);
    key_t semkey2   = ftok(".", 2);
    key_t semkey3   = ftok(".", 3);
    key_t semkey4   = ftok(".", 4);
    rmutex = initsem(semkey1, 1);   
    wmutex = initsem(semkey2, 1);   
    readTry = initsem(semkey3, 1);   
    resource = initsem(semkey4, 1);   

    if(checkArgs(argc, argv) == 0)
    {
        cout << "Input arguments are not valid!\n";
        exit(0);
    }

    int numOfItems = atoi(argv[2]);
    int mem_id=shmget(IPC_PRIVATE, 10*sizeof(orderBoard), SHM_R | SHM_W);
    t1 = clock();
    cout << setfill('=') << setw(25) << "Simulation arguments" << setfill('=') << setw(25) << "\n";
    cout << "Simulation time: " << argv[1];
    cout << "\nMenu items count: " << argv[2];
    cout << "\nCustomers count: " << argv[3];
    cout << "\nWaiters count: " << argv[4] << "\n";
    cout << setfill('=') << setw(50) << "\n";
    printf("%.2f", (float)t1 / CLOCKS_PER_SEC);
    cout << " Main process ID " << getpid() << " start\n";
    Menu menu[] = {{ 0 , "Pizza" , 10.5 , 0 }, { 1 , "Salad", 7.50, 0}, { 2, "Hamburger", 12.00, 0}, { 3, "Spaghetti", 9.00, 0}, {4, "Pie", 9.50, 0}, {5, "Milkshake", 6.00, 0} , {6, "Vodka", 12.25, 0}};
    printMenu(menu, numOfItems);
    
    OB=(orderBoard*)shmat(mem_id,NULL,0);
    
    cout<<"loldsfdsfdsfs\n";
    pid_t pid, wpid;
    int status=0,temp=0;
    for(int i=0;i<3;i++)
    {
    if((pid=fork())==0)
    {
        temp=getpid();
        //t = t+ (float)t1 / CLOCKS_PER_SEC;
       // cout<<"im t: "<< t <<"\n";
        while(*Ptime<7)
        {

            cout<<"im t before: "<< *Ptime << " and pid: " <<temp<<"\n";
            Customer(temp);
	    cout<<"im t after: "<< *Ptime << " and pid: " <<temp<<"\n";

        } 
 
    }
    else if(pid<0) 
    {
       perror("fork error");
    }
    }
       while(*Ptime<7)
            {
            t1 = clock();
            *Ptime=(float)t1 / CLOCKS_PER_SEC;
            }
            while((wpid = wait(&status))>0);
            //cout<<"ended\n im temp:"<<pid<<"\n";
            //waitpid(pid,&status, 0);
            //kill(pid,SIGTERM);
		    //perror("wait error");

   return 1;
}
