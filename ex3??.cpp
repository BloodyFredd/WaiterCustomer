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
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
using namespace std;

//#define SNAME1 "/srmutex"
//#define SNAME2 "/swmutex"
//#define SNAME3 "/sreadtry"
//#define SNAME4 "/sresource"

int ReadCount=0,WriterCount=0;
int rm, wm, rt, r;
sem_t *rmutex, *wmutex , *readTry, *resource;
static float *Ptime;
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

void P(int semid, int id){
    struct sembuf p_buf;
    if(id == 0)
        p_buf.sem_num = 1;
    else p_buf.sem_num = 0;
    p_buf.sem_op = 0;
    p_buf.sem_flg = SEM_UNDO;
    if(semop(semid, &p_buf, 1) == -1)
        perror("P(semid) failed\n");
}

void V(int semid, int id){
    struct sembuf v_buf;
    if(id == 0)
        v_buf.sem_num = 1;
    else v_buf.sem_num = 0;
    v_buf.sem_op = 1;
    v_buf.sem_flg = SEM_UNDO;
    if(semop(semid, &v_buf, 1) == -1)
        perror("V(semid) failed\n");
}

void Waiter(sem_t *rmutex,sem_t *wmutex ,sem_t *readTry,sem_t *resource)
{
  sem_wait(readTry);
  sem_wait(rmutex);
  ReadCount++;
  if(ReadCount == 1)
	sem_wait(resource);
  sem_post(rmutex);
  sem_post(readTry);

  sleep(3);
  cout<<"Critical section\n";
 

  sem_wait(rmutex);
  ReadCount--;
  if(ReadCount == 0)
	sem_post(resource);	
  sem_post(rmutex);
}

void* Customer(int temp, int id, int semid)
{
  int wmutexval;
  cout<<"im the pid in here:"<<temp<<"\n";
  //sem_getvalue(wmutex,&wmutexval);
  //cout<<"im the wmutexval: "<<wmutexval<<"\n";
  //V(wm);
  WriterCount++;
  //if(WriterCount == 1)
	//V(rt);
  //P(wm);
  cout << semid << " IMMM hereee\n";
  P(semid, id);
	cout<< *Ptime << " Customer ID " << id << ": reads a menu about...\n";
	  sleep(3);
	cout<< *Ptime << " Customer ID " << id << ": after sleep...\n";
  V(semid, id);
cout<<"im out with pid: "<<temp<<"\n";
  
  //V(wm);
  WriterCount--;
  //if( WriterCount == 0)
	//P(rt);
  //P(wm);
}

int main(int argc, char* argv[])
{
    clock_t t1;
    
    Ptime=static_cast<float*>(mmap(NULL,sizeof *Ptime, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *Ptime=0;
    int semid = shmget(IPC_PRIVATE, 1, IPC_CREAT | SHM_R | SHM_W);
    int id = semget(IPC_PRIVATE, 2, SHM_R | SHM_W);
    void* addr = shmat(semid, NULL, 0);
    *((int*) addr) = id;

    /*key_t k2 = ftok("/HOME", 'b');
    key_t k3 = ftok("/HOME", 'c');
    key_t k4 = ftok("/HOME", 'd');

    int shmid1 = shmget(k1, 1024, 0644 | IPC_CREAT);
    int shmid2 = shmget(k2, 1024, 0644 | IPC_CREAT);
    int shmid3 = shmget(k3, 1024, 0644 | IPC_CREAT);
    int shmid4 = shmget(k4, 1024, 0644 | IPC_CREAT);

    readTry = shmat(shmid1, (void *)0, 0);
    rmutex = shmat(shmid2, (void *)0, 0);
    wmutex = shmat(shmid3, (void *)0, 0);
    resource = shmat(shmid4, (void *)0, 0);

    /*readTry=sem_open("readtry", O_CREAT|O_EXCL, 0644, 3);
    if(readTry == SEM_FAILED)
	perror("readTry sem failed");
    rmutex=sem_open("rmutex", O_CREAT|O_EXCL, 0644, 3);
    if(rmutex == SEM_FAILED)
	perror("rmutex sem failed");
    wmutex=sem_open("wmutex", O_CREAT|O_EXCL, 0644, 3);
    if(wmutex == SEM_FAILED)
	perror("wmutex sem failed");
    resource=sem_open("resource", O_CREAT|O_EXCL, 0644, 3);
    if(resource == SEM_FAILED)
	*/perror("resource sem failed");
    if(checkArgs(argc, argv) == 0)
    {
        cout << "Input arguments are not valid!\n";
        exit(0);
    }
    /*readTry = new sem_t;
    rmutex = new sem_t;
    wmutex = new sem_t;
    resource = new sem_t;

    sem_init(readTry, 0, 1);
    sem_init(rmutex, 0, 1);
    sem_init(wmutex, 0, 1);
    sem_init(resource, 0, 1);*/
    int numOfItems = atoi(argv[2]);
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
    
    pid_t pid, wpid;
    int status=0,temp=0;
    for(int i=0;i<2;i++)
    {
    if((pid=fork())==0)
    {
	 //readTry=sem_open("/readtry",1);
       	 //rmutex=sem_open("/rmutex",1);
         //wmutex=sem_open("/wmutex",1);
         //resource=sem_open("/resource",1);
        temp=getpid();
        //t = t+ (float)t1 / CLOCKS_PER_SEC;
       // cout<<"im t: "<< t <<"\n";
        while(*Ptime<7)
        {

            cout<<"im t before: "<< *Ptime << " and pid: " <<temp<<"\n";
            Customer(temp, i, id);
	    cout<<"im t after: "<< *Ptime << " and pid: " <<temp<<"\n";

        } 
 
    }
    else if(pid<0) 
    {
       perror("fork error");
    }
    }
     /*    sem_close(readTry);
	 sem_close(rmutex);
	 sem_close(wmutex);
	 sem_close(resource);*/ 
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