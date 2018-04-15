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
    int Done = -1;
};

struct orderList
{
    orderBoard data;
    orderList *next;
};

static int *ReadCount=0,*WriterCount=0,*firstflag,*CustomerNumber,*WaitersNumber,*WaitersCount;
int rmutex, wmutex , readTry, resource,ReadBlock,CN,WN;
struct orderBoard *OB = NULL;
struct Menu *menu;
static float *Ptime;

void printMenu(int length){
    cout << setfill('=') << setw(25) << "Menu list" << setfill('=') << setw(25) << "\n";
    cout << "Id" << setfill(' ') << setw(15) << "Name" << setfill(' ') << setw(15) << "Price" << setfill(' ') << setw(15) << "Orders\n";
    for (int i = 0; i < length; i++)
        cout << menu[i].Id << setfill(' ') << setw(15) << menu[i].Name << setfill(' ') << setw(15) << menu[i].Price << setfill(' ') << setw(15) << menu[i].TotalOrdered << "\n";
    cout << setfill('=') << setw(50) << "\n";
}

int randNum(int smallV, int highV){
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_dist(smallV, highV);
    return uniform_dist(e1);
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
 //func = new orderList;
 //func->next=NULL;
 orderList *tmp=func;
 orderList *Iptr=item;
 while(tmp!=NULL)
 {
     /*orderList *newSnode = new orderList;
     newSnode->next=NULL;
     newSnode->data.CustomerId = Iptr->data.CustomerId;
     newSnode->data.ItemId = Iptr->data.ItemId;
     newSnode->data.Amount = Iptr->data.Amount;
     newSnode->data.Done = Iptr->data.Done;
     tmp->next=newSnode;
     tmp=tmp->next;
     Iptr=Iptr->next;
     tmp->next = NULL;
     */
    tmp=tmp->next;
 }
     orderList *newSnode;// = new orderList;
     newSnode->next=NULL;
     newSnode->data.CustomerId = item->data.CustomerId;
     newSnode->data.ItemId = item->data.ItemId;
     newSnode->data.Amount = item->data.Amount;
     newSnode->data.Done = item->data.Done;
     tmp = newSnode;
     tmp=tmp->next;
     tmp->next = NULL;

  return func->next;
}



void Waiter(int id,int NOC, Menu *menu)
{
  
  p(readTry);
  sleep(randNum(1, 2));
  p(rmutex);
  (*ReadCount)++;
  if((*ReadCount) == 1)
	p(resource);
  v(rmutex);
  v(readTry);
  
 p(ReadBlock);
    for(int i = 0; i < NOC; i++)
	 {
	   if(OB[i].Done == 0)
		{
           printf("%.3f", (float)*Ptime);
		   cout << " Waiter ID " << id << ": performs order of Customer ID: " << i << " (" << menu[OB[i].ItemId].Name << ", " << OB[i].Amount << ")\n";
		   OB[i].Done=1;
	   	   menu[OB[i].ItemId].TotalOrdered = menu[OB[i].ItemId].TotalOrdered + OB[i].Amount;
           break;
		}
	 }
  v(ReadBlock);
 
  p(rmutex);
  (*ReadCount)--;
  if((*ReadCount) == 0)
	v(resource);	
  v(rmutex);
}

void* Customer(int id, int numOfDishes)
{
  sleep(randNum(3, 6));
  int orderAmount = randNum(1,4), order = randNum(0,2), menuOrder = randNum(0, numOfDishes - 1);
  p(wmutex);
  (*WriterCount)++;
  if((*WriterCount) == 1){
	p(readTry);
    }

  v(wmutex);
  p(resource);
    sleep(1);
 if(OB[id].Done != 0)
 {
  if(order == 0){
    printf("%.3f", (float)*Ptime);
    cout << " Customer ID " << id << ": reads about " << menu[menuOrder].Name << " (doesn't want to order)\n";
  }
  else
    {
        printf("%.3f", (float)*Ptime);
	    cout << " Customer ID " << id << ": reads about " << menu[menuOrder].Name << " (ordered, amount " << orderAmount << ")\n";
		OB[id].CustomerId = id;
  		OB[id].ItemId = menuOrder;
  		OB[id].Amount = orderAmount;
  		OB[id].Done = 0;
		cout<<"CustID: "<< OB[id].CustomerId<<"  Item id: "<< OB[id].ItemId <<"  Amount: "<<  OB[id].Amount <<"\n";
    }	
  }
  v(resource);
  
  p(wmutex);
  (*WriterCount)--;
  if( (*WriterCount) == 0)
	v(readTry);
  v(wmutex);
}

void initMenu(){
	//menu = new Menu[10];
	menu[0].Id = 0;
	strcpy(menu[0].Name, "Pizza");
	menu[0].Price = 10.50;
	menu[0].TotalOrdered = 0;
	
	menu[1].Id = 1;
	strcpy(menu[1].Name, "Salad");
	menu[1].Price = 7.50;
	menu[1].TotalOrdered = 0;	
	
	menu[2].Id = 2;
	strcpy(menu[2].Name, "Hamburger");
	menu[2].Price = 12.00;
	menu[2].TotalOrdered = 0;	
	
	menu[3].Id = 3;
	strcpy(menu[3].Name, "Spaghetti");
	menu[3].Price = 9.00;
	menu[3].TotalOrdered = 0;
	
	menu[4].Id = 4;
	strcpy(menu[4].Name, "Pie");
	menu[4].Price = 9.50;
	menu[4].TotalOrdered = 0;	
	    
	menu[5].Id = 5;
	strcpy(menu[5].Name, "Milkshake");
	menu[5].Price = 6.00;
	menu[5].TotalOrdered = 0;	
	        
	menu[6].Id = 6;
	strcpy(menu[6].Name, "Vodka");
	menu[6].Price = 12.25;
	menu[6].TotalOrdered = 0;	
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

    CustomerNumber=static_cast<int*>(mmap(NULL,sizeof *CustomerNumber, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *CustomerNumber=atoi(argv[3]);

    WaitersNumber=static_cast<int*>(mmap(NULL,sizeof *WaitersNumber, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *WaitersNumber=atoi(argv[4]);

    WaitersCount=static_cast<int*>(mmap(NULL,sizeof *WaitersCount, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *WaitersCount=-1;
    
    int totalnum=(*CustomerNumber)+(*WaitersNumber);
    key_t semkey1   = ftok(".", 1);
    key_t semkey2   = ftok(".", 2);
    key_t semkey3   = ftok(".", 3);
    key_t semkey4   = ftok(".", 4);
    key_t semkey5   = ftok(".", 5);
    key_t semkey6   = ftok(".", 6);
    key_t semkey7   = ftok(".", 7);
    rmutex = initsem(semkey1, 1);   
    wmutex = initsem(semkey2, 1);   
    readTry = initsem(semkey3, 1);   
    resource = initsem(semkey4, 1); 
    ReadBlock= initsem(semkey5, 1);  
    CN = initsem(semkey6, 1); 
    WN = initsem(semkey7, 1); 

    if(checkArgs(argc, argv) == 0)
    {
        cout << "Input arguments are not valid!\n";
        exit(0);
    }

    int numOfItems = atoi(argv[2]),NumOfCustProc=atoi(argv[3]);
    int mem_id=shmget(IPC_PRIVATE, 10*sizeof(orderBoard), SHM_R | SHM_W);
    int menu_id=shmget(IPC_PRIVATE, 10*sizeof(Menu), SHM_R | SHM_W);
    t1 = clock();
    cout << setfill('=') << setw(25) << "Simulation arguments" << setfill('=') << setw(25) << "\n";
    cout << "Simulation time: " << argv[1];
    cout << "\nMenu items count: " << argv[2];
    cout << "\nCustomers count: " << argv[3];
    cout << "\nWaiters count: " << argv[4] << "\n";
    cout << setfill('=') << setw(50) << "\n";
    *Ptime=(float)t1 / CLOCKS_PER_SEC;
    printf("%.3f", (float)*Ptime);
	
    cout << " Main process ID " << getpid() << " start\n";
        
    menu=(Menu*)shmat(menu_id,0,0);
    initMenu();
    printMenu(numOfItems);
    //OB = new orderBoard[atoi(argv[3])];
    OB=(orderBoard*)shmat(mem_id,NULL,0);
    int k = 0;
    while(k < atoi(argv[3])){
	    OB[k].Done = -1;
	    k++;
    }
    pid_t pid, wpid,fpid;
    fpid=getpid();
    int status=0,temp=0,timepid;
    printf("%.3f", (float)*Ptime);
    cout << " Main process start creating sub-process\n";
    for(int i=0;i<(totalnum+1);i++)
    {
    if((pid=fork())==0)
    {

	if(i==totalnum)
         {
	   timepid=getpid();
	   while(true)
            {
            t1 = clock();
            *Ptime=(float)t1 / CLOCKS_PER_SEC;
	    }
	}
	else
	{
        temp=getpid();
	if(i<NumOfCustProc)//(*CustomerNumber)!=0)        
	{
		
		p(CN);
            printf("%.3f", (float)*Ptime);
			cout << " Customer ID " << i << ": created PID " << getpid() << " PPID " << getppid() << "\n";
			(*CustomerNumber)--;
		v(CN);
		while(*Ptime<15)
        	{
            		Customer(i,numOfItems);
        	} 
	}
	else if(i>=NumOfCustProc && i<totalnum)     
	{
		
		p(WN);
            printf("%.3f", (float)*Ptime);
			
			(*WaitersNumber)--;
			(*WaitersCount)++;
			cout << " Waiter ID " << *WaitersCount << ": created PID " << getpid() << " PPID " << getppid() << "\n";
		v(WN);
		while(*Ptime<15)
        	{
            		Waiter(*WaitersCount,NumOfCustProc, menu);
        	} 
	}
 		exit(0);
	}
    }
    else if(pid<0) 
    {
       perror("fork error");
    }
    }

	   for(int i=0;i<totalnum;i++) // loop will run n times (n=5)
    	   	wait(NULL);
	   //kill(timepid,SIGKILL);

   printMenu(numOfItems);
   return 1;
}
