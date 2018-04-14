#include<iostream>
#include <iomanip>
#include <random>
#include <cmath>
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

struct orderList
{
    orderBoard data;
    orderList *next;
};

static int *ReadCount=0,*WriterCount=0,*firstflag;
int rmutex, wmutex , readTry, resource;
static orderList *OB;
static float *Ptime;

void printBoardList(orderList *head){
    orderList *tmp = head;
    while(tmp->next != NULL)
        cout << tmp->data.CustomerId << ", " << tmp->data.ItemId << ", " << tmp->data.Amount << ", " << tmp->data.Done << "\n";
}

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



orderList* BuildAlist(orderList *func, orderList *item)
{
 func = new orderList;
 func->next=NULL;
 orderList *tmp=func;
 orderList *Iptr=item;
 while(Iptr!=NULL)
 {
     orderList *newSnode = new orderList;
     newSnode->next=NULL;
     newSnode->data.CustomerId = item->data.CustomerId;
     newSnode->data.ItemId = item->data.ItemId;
     newSnode->data.Amount = item->data.Amount;
     newSnode->data.Done = item->data.Done;
     tmp->next=newSnode;
     tmp=tmp->next;
     Iptr=Iptr->next;
     tmp->next = NULL;
 }
     orderList *newSnode = new orderList;
     newSnode->next=NULL;
     newSnode->data.CustomerId = item->data.CustomerId;
     newSnode->data.ItemId = item->data.ItemId;
     newSnode->data.Amount = item->data.Amount;
     newSnode->data.Done = item->data.Done;
     tmp->next=newSnode;
     tmp=tmp->next;
     tmp->next = NULL;

  return func->next;
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

int randNum(int smallV, int highV){
    /*static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(smallV, highV);
    return distribution(generator);*/
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_dist(smallV, highV);
    return uniform_dist(e1);
}

void* Customer(int temp, int id, Menu *menu, int numOfDishes)
{
  int SleepTime=randNum(3, 6);
  sleep(randNum(3, 6));
  int orderAmount = randNum(1,4), order = randNum(0,2), menuOrder = randNum(1, numOfDishes);
  //cout<<"im the pid in here:"<<temp<<"\n";
  p(wmutex);
  (*WriterCount)++;
  if((*WriterCount) == 1){
	p(readTry);
    }
  sleep(1);
  if(order == 0)
    cout << *Ptime << " Customer ID " << id << ": reads about " << menu[menuOrder].Name << " (doesn't want to order)\n";
  else
    cout << *Ptime << " Customer ID " << id << ": reads about " << menu[menuOrder].Name << " (ordered, amount " << orderAmount << ")\n";

  v(wmutex);
  p(resource);
	//cout<<"Critical section with pid: "<<temp<<" and time " << *Ptime << "\n";
	 // sleep(3);
	if((*firstflag)==1)
  	{
		OB->data.CustomerId = id;
  		OB->data.ItemId = menuOrder;
  		OB->data.Amount = orderAmount;
  		OB->data.Done = 0;
  		OB->next = NULL;
	}
	else
	{
		orderList *nextnode=new orderList;
 	 	nextnode->data.CustomerId = id;
 	 	nextnode->data.ItemId = menuOrder;
 	 	nextnode->data.Amount = orderAmount;
 	 	nextnode->data.Done = 0;
		OB=BuildAlist(OB,nextnode);
		//nextnode->next=NULL;
 	 	//OB->next = nextnode;
		//OB=OB->next;
	}
	if((*firstflag)==1)
		(*firstflag)=0;
  v(resource);
  //cout<<"im out with pid: "<<temp<<"\n";
  
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

    firstflag=static_cast<int*>(mmap(NULL,sizeof *ReadCount, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *firstflag=1;

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
    int mem_id=shmget(IPC_PRIVATE, 10*sizeof(orderList), SHM_R | SHM_W);
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
    OB = NULL;
    OB=(orderList*)shmat(mem_id,NULL,0);
    orderList *head = OB;
    pid_t pid, wpid,fpid;
    fpid=getpid();
    int status=0,temp=0;
    for(int i=0;i<2;i++)
    {
    if((pid=fork())==0)
    {
        temp=getpid();
        //t = t+ (float)t1 / CLOCKS_PER_SEC;
       // cout<<"im t: "<< t <<"\n";
        while(*Ptime<15)
        {

            //cout<<"im t before: "<< *Ptime << " and pid: " <<temp<<"\n";
            Customer(temp, i, menu, numOfItems);
	    //cout<<"im t after: "<< *Ptime << " and pid: " <<temp<<"\n";

        } 
 
    }
    else if(pid<0) 
    {
       perror("fork error");
    }
    }
     while(*Ptime<15)
            {
            t1 = clock();
            *Ptime=(float)t1 / CLOCKS_PER_SEC;
            }
            while((wpid = wait(&status))>0);
            //cout<<"ended\n im temp:"<<pid<<"\n";
            //waitpid(pid,&status, 0);
            //kill(pid,SIGTERM);
		    //perror("wait error");
    //printBoardList(head);
     
     int j=1;
     sleep(2);
     if(getpid()==fpid)
     {
     while(head!=NULL)
	{
	   cout<<j<<")\n";
	   cout<<"Customer id: "<<head->data.CustomerId<<"\n";
  	   cout<<"CustomerMenuOrder: "<<head->data.ItemId<<"\n";
  	   cout<<"CustomerOrderAmount: "<<head->data.Amount<<"\n";
  	   cout<<"CustomerOrderDone: "<<head->data.Done<<"\n\n";
	    head=head->next;
	   j++;
	}
     }
   return 1;
}
