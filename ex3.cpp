#include<iostream>
#include <iomanip>
#include <random>
//#include <time.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include <semaphore.h>
#include <sys/mman.h>
//#include <sys/types.h>
//#include <sys/ipc.h>
//#include <fcntl.h>
#include <sys/sem.h>
#include <sys/shm.h>
using namespace std;

#define SEMPERM 0600

// Our menu struct.
struct Menu {
    int Id;
    char Name[16];
    float Price;
    int TotalOrdered;
};

// Our order board struct.
struct orderBoard {
    int CustomerId;
    int ItemId;
    int Amount;
    int Done = -1;
};

// Our global variables.
static int *ReadCount=0,*WriterCount=0,*CustomerNumber,*WaitersNumber,*WaitersCount,*BreakCond;
int rmutex, wmutex , readTry, resource,ReadBlock,CN,WN,TimeBlock;
struct orderBoard *OB = NULL;
struct Menu *menu;
static float *Ptime;

// This function prints the menu,
void printMenu(int length){
    cout << setfill('=') << setw(25) << "Menu list" << setfill('=') << setw(25) << "\n";
    cout << "Id" << setfill(' ') << setw(15) << "Name" << setfill(' ') << setw(15) << "Price" << setfill(' ') << setw(15) << "Orders\n";
    for (int i = 0; i < length; i++)
        cout << menu[i].Id << setfill(' ') << setw(15) << menu[i].Name << setfill(' ') << setw(15) << menu[i].Price << setfill(' ') << setw(15) << menu[i].TotalOrdered << "\n";
    cout << setfill('=') << setw(50) << "\n";
}

// This is our random function, it returns a random float number between smallV and highV.
float randNum(float smallV, float highV){
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_real_distribution<float> uniform_dist(smallV, highV);
    return uniform_dist(e1);
}

// The function that checks the arguments given at the running of the program.
int checkArgs(int argc, char* argv[]){
    if(argc == 5){
	int Time=atoi(argv[1]), MenuItem=atoi(argv[2]) ,Cust=atoi(argv[3]), Waiter=atoi(argv[4]);
        if(Time >= 30 || Time < 0)
	{	 cerr << "Time is invalid!\n";
			return 0;
	}
	if(Cust > 10 || Cust < 0)
	{	 cerr << "Invalid customer number!\n";
			return 0;
	}
	if(Waiter > 3 || Waiter < 0)	
	{
	 cerr << "invalid waiter number!\n";
         return 0;
	}
	if(MenuItem <0 || MenuItem > 10)
	{
	 cerr << "Menu items number is not valid!\n";
            	 return 0;
	}
        return 1;
    }
    cerr<<"invalid num of arguments\n";
    return 0;
}

// The function for the initialization of the semaphores.
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

// Our down function for the semaphores.
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

// Our up function for the semaphores.
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

// Our waiter, from the reader-writer problem.
void Waiter(int id,int NOC)
{
  p(readTry);
  usleep(1000000 * randNum(1, 2));
  p(rmutex);
  (*ReadCount)++;
  if((*ReadCount) == 1)
	p(resource);
  v(rmutex);
  v(readTry);
  
 p(ReadBlock);
    // Run on all the customers to check if someone ordered something.
    for(int i = 0; i < NOC; i++)
	 {
	   if(OB[i].Done == 0)
		{
		 p(TimeBlock);
          	    printf("%.3f", (float)*Ptime);
		 v(TimeBlock);
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

// Our customer, from the reader-writer problem.
void Customer(int id, int numOfDishes)
{
  // All the randoms.
  usleep(1000000 * randNum(3, 6));
  int orderAmount = randNum(1,4), order = randNum(0,2), menuOrder = randNum(0, numOfDishes - 1);
  p(wmutex);
  (*WriterCount)++;
  if((*WriterCount) == 1){
	p(readTry);
    }

  v(wmutex);
  p(resource);
  usleep(1000000);
  // If not ordered yet.
 if(OB[id].Done != 0)
 { // If the random is 0.
  if(order == 0){
    p(TimeBlock);
    printf("%.3f", (float)*Ptime);
     v(TimeBlock);
    cout << " Customer ID " << id << ": reads about " << menu[menuOrder].Name << " (doesn't want to order)\n";
  }
  else
    { // If the random is 1.
	p(TimeBlock);
        	printf("%.3f", (float)*Ptime);
	v(TimeBlock);
	    cout << " Customer ID " << id << ": reads about " << menu[menuOrder].Name << " (ordered, amount " << orderAmount << ")\n";
		OB[id].CustomerId = id;
  		OB[id].ItemId = menuOrder;
  		OB[id].Amount = orderAmount;
  		OB[id].Done = 0;
    }	
  }
  v(resource);
  
  p(wmutex);
  (*WriterCount)--;
  if( (*WriterCount) == 0)
	v(readTry);
  v(wmutex);
}

// initialize the menu.
void initMenu(){
	menu[0].Id = 1;
	strcpy(menu[0].Name, "Pizza");
	menu[0].Price = 10.50;
	menu[0].TotalOrdered = 0;
	
	menu[1].Id = 2;
	strcpy(menu[1].Name, "Salad");
	menu[1].Price = 7.50;
	menu[1].TotalOrdered = 0;	
	
	menu[2].Id = 3;
	strcpy(menu[2].Name, "Hamburger");
	menu[2].Price = 12.00;
	menu[2].TotalOrdered = 0;	
	
	menu[3].Id = 4;
	strcpy(menu[3].Name, "Spaghetti");
	menu[3].Price = 9.00;
	menu[3].TotalOrdered = 0;
	
	menu[4].Id = 5;
	strcpy(menu[4].Name, "Pie");
	menu[4].Price = 9.50;
	menu[4].TotalOrdered = 0;	
	    
	menu[5].Id = 6;
	strcpy(menu[5].Name, "Milkshake");
	menu[5].Price = 6.00;
	menu[5].TotalOrdered = 0;	
	        
	menu[6].Id = 7;
	strcpy(menu[6].Name, "Vodka");
	menu[6].Price = 12.25;
	menu[6].TotalOrdered = 0;	

    menu[7].Id = 8;
	strcpy(menu[7].Name, "Schnitzel");
	menu[7].Price = 14.50;
	menu[7].TotalOrdered = 0;

    menu[8].Id = 9;
	strcpy(menu[8].Name, "Croissant");
	menu[8].Price = 3.5;
	menu[8].TotalOrdered = 0;

    menu[9].Id = 10;
	strcpy(menu[9].Name, "Coffee");
	menu[9].Price = 2.50;
	menu[9].TotalOrdered = 0;
}	  

// Check if all the orders are done.
int checkDone(int numOfCustomers){
	for(int i = 0; i < numOfCustomers; i++){
		if(OB[i].Done == 0)
			return 0;
	}
	return 1;
}

// Our main function that runs everything.
int main(int argc, char* argv[])
{
	struct timespec begin, end;
	clock_gettime(CLOCK_MONOTONIC, &begin);
    int totalOrders = 0;
    float amount = 0.0;
    Ptime=(float *)mmap(NULL,sizeof *Ptime, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0);
    *Ptime=0;

    WriterCount=static_cast<int*>(mmap(NULL,sizeof *WriterCount, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *WriterCount=0;

    ReadCount=static_cast<int*>(mmap(NULL,sizeof *ReadCount, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *ReadCount=0;

    CustomerNumber=static_cast<int*>(mmap(NULL,sizeof *CustomerNumber, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *CustomerNumber=atoi(argv[3]);

    WaitersNumber=static_cast<int*>(mmap(NULL,sizeof *WaitersNumber, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *WaitersNumber=atoi(argv[4]);

    WaitersCount=static_cast<int*>(mmap(NULL,sizeof *WaitersCount, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *WaitersCount=-1;

    BreakCond=static_cast<int*>(mmap(NULL,sizeof *BreakCond, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *BreakCond=0;
    
    int totalnum=(*CustomerNumber)+(*WaitersNumber);
    key_t semkey1   = ftok(".", 1);
    key_t semkey2   = ftok(".", 2);
    key_t semkey3   = ftok(".", 3);
    key_t semkey4   = ftok(".", 4);
    key_t semkey5   = ftok(".", 5);
    key_t semkey6   = ftok(".", 6);
    key_t semkey7   = ftok(".", 7);
    key_t semkey8   = ftok(".", 8);
    rmutex = initsem(semkey1, 1);   
    wmutex = initsem(semkey2, 1);   
    readTry = initsem(semkey3, 1);   
    resource = initsem(semkey4, 1); 
    ReadBlock= initsem(semkey5, 1);  
    CN = initsem(semkey6, 1); 
    WN = initsem(semkey7, 1); 
    TimeBlock=initsem(semkey8, 1);

	// Check the arguments.
    if(checkArgs(argc, argv) == 0)
    {
        exit(0);
    }

    int numOfItems = atoi(argv[2]),NumOfCustProc=atoi(argv[3]);
    int mem_id=shmget(IPC_PRIVATE, 10*sizeof(orderBoard), SHM_R | SHM_W);
    int menu_id=shmget(IPC_PRIVATE, 10*sizeof(Menu), SHM_R | SHM_W);
    cout << setfill('=') << setw(25) << "Simulation arguments" << setfill('=') << setw(25) << "\n";
    cout << "Simulation time: " << argv[1];
    cout << "\nMenu items count: " << argv[2];
    cout << "\nCustomers count: " << argv[3];
    cout << "\nWaiters count: " << argv[4] << "\n";
    cout << setfill('=') << setw(50) << "\n";
    p(TimeBlock);
	clock_gettime(CLOCK_MONOTONIC, &end);
	*Ptime = end.tv_sec - begin.tv_sec;
	*Ptime += (end.tv_nsec - begin.tv_nsec) / 1000000000.0;
    printf("%.3f", (float)*Ptime);
    cout << " Main process ID " << getpid() << " start\n"; 
    v(TimeBlock);   
    menu=(Menu*)shmat(menu_id,0,0);
    initMenu();
    printMenu(numOfItems);
    OB=(orderBoard*)shmat(mem_id,NULL,0);
    pid_t pid, Mainpid=getpid();
    p(TimeBlock);
    printf("%.3f", (float)*Ptime);
    cout << " Main process start creating sub-process\n";
    v(TimeBlock);
    int k = 0;
    while(k < atoi(argv[3])){
	    OB[k].Done = -1;
	    k++;
    }

	// Our main loop that builds the waiters and customers.
    for(int i=0;i<(totalnum+1);i++)
    {
    if((pid=fork())==0)
    {

	if(i==totalnum)
         {
	   while(true)
            {
	     p(TimeBlock);
		 	clock_gettime(CLOCK_MONOTONIC, &end);
			*Ptime = end.tv_sec - begin.tv_sec;
			*Ptime += (end.tv_nsec - begin.tv_nsec) / 1000000000.0;
             v(TimeBlock);
	   if(*BreakCond==1){
		exit(0);
	   }
	    }
	}
	else
	{
	if(i<NumOfCustProc)      
	{
		
		p(CN);
		 p(TimeBlock);
            		printf("%.3f", (float)*Ptime);
		 v(TimeBlock);
			cout << " Customer ID " << i << ": created PID " << getpid() << " PPID " << getppid() << "\n";
			(*CustomerNumber)--;
		v(CN);
		while(*Ptime<atoi(argv[1]))
        	{
            		Customer(i,numOfItems);
        	} 
	p(WN);
	 p(TimeBlock);
        printf("%.3f", (float)*Ptime);
	  v(TimeBlock);
        cout << " Customer ID " << i << ": PID " << getpid() << " end work PPID " << getppid() << "\n";
	v(WN);
	}
	else if(i>=NumOfCustProc && i<totalnum)     
	{
		
		p(CN);
			(*WaitersNumber)--;
			(*WaitersCount)++;
			int Wid=(*WaitersCount);
			p(TimeBlock);
            			printf("%.3f", (float)*Ptime);
		 	v(TimeBlock);	
			cout << " Waiter ID " << *WaitersCount << ": created PID " << getpid() << " PPID " << getppid() << "\n";
		v(CN);
		while(*Ptime<atoi(argv[1]) || checkDone(NumOfCustProc) == 0)
        	{
            		Waiter(Wid,NumOfCustProc);
        	} 
	p(WN);
	   p(TimeBlock);
        printf("%.3f", (float)*Ptime);
	   v(TimeBlock);
        cout << " Waiter ID " << Wid << ": PID " << getpid() << " end work PPID " << getppid() << "\n";
	v(WN);
	
	}
 		exit(0);
	}
    }
    else if(pid<0) 
    {
       perror("fork error");
    }
    }
	   for(int i=0;i<totalnum;i++){
    	   	wait(NULL);
	   }
		if(getpid()==Mainpid)
		{	
			*BreakCond=1;
			 wait(NULL);
		}
   printMenu(numOfItems);
   for(int j = 0; j < numOfItems; j++){
       totalOrders += menu[j].TotalOrdered;
       amount += menu[j].TotalOrdered * menu[j].Price;
   }

   // Print the sikum of everything.
   cout << "Total orders " << totalOrders << ", for an amount " << amount << " NIL\n";
   printf("%.3f", (float)*Ptime);
   cout << " Main ID " << getpid() << " end work\n";
   printf("%.3f", (float)*Ptime);
   cout << " End of simulation\n";
   
   // Free semaphores and shared memory.
   shmctl(mem_id, IPC_RMID, 0);
   shmctl(menu_id, IPC_RMID, 0);
   munmap(WriterCount, 5000);
   munmap(ReadCount, 5000);
   munmap(CustomerNumber, 5000);
   munmap(WaitersNumber, 5000);
   munmap(WaitersCount, 5000);
   semctl(semget(semkey1, 1, 0), 0, IPC_RMID);
   semctl(semget(semkey2, 1, 0), 0, IPC_RMID);
   semctl(semget(semkey3, 1, 0), 0, IPC_RMID);
   semctl(semget(semkey4, 1, 0), 0, IPC_RMID);
   semctl(semget(semkey5, 1, 0), 0, IPC_RMID);
   semctl(semget(semkey6, 1, 0), 0, IPC_RMID);
   semctl(semget(semkey7, 1, 0), 0, IPC_RMID);
   semctl(semget(semkey8, 1, 0), 0, IPC_RMID);
   munmap(Ptime, 100);
   return 0;
}
