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
#include <sys/mman.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
using namespace std;

int ReadCount=0,WriterCount=0;
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

void V(sem_t *sem){
    int n = sem_wait(sem);
    if(n != 0)
        perror("sem_wait failed");
}

void P(sem_t *sem){
    int n = sem_post(sem);
    if(n != 0)
        perror("sem_post failed");
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

void Waiter()
{
  sem_wait(readTry);
  sem_wait(rmutex);
  ReadCount++;
  if(ReadCount == 1)
	sem_wait(resource);
  sem_post(rmutex);
  sem_post(readTry);

  cout<<"Critical section\n";

  sem_wait(rmutex);
  ReadCount--;
  if(ReadCount == 0)
	sem_post(resource);	
  sem_post(rmutex);
}

void* Customer()
{
  sem_wait(wmutex);
  WriterCount++;
  if(WriterCount == 1)
	sem_wait(readTry);
  sem_post(wmutex);

  sem_wait(resource);
    sleep(3);
	cout<<"Critical section\n";
  sem_post(resource);
  
  sem_wait(wmutex);
  WriterCount--;
  if( WriterCount == 0)
	sem_post(readTry);
  sem_post(wmutex);
}

int main(int argc, char* argv[])
{
    clock_t t1;
    Ptime=static_cast<float*>(mmap(NULL,sizeof *Ptime, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0));
    *Ptime=0;
    int fd1 = shm_open("readTry", O_CREAT, O_RDWR);
    ftruncate(fd1, sizeof(sem_t));
    readTry = static_cast<sem_t*>(mmap(NULL,sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0));
    int fd2 = shm_open("rmutex", O_CREAT, O_RDWR);
    ftruncate(fd2, sizeof(sem_t));
    rmutex = static_cast<sem_t*>(mmap(NULL,sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0));
    int fd3 = shm_open("wmutex", O_CREAT, O_RDWR);
    ftruncate(fd3, sizeof(sem_t));
    wmutex = static_cast<sem_t*>(mmap(NULL,sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd3, 0));
    int fd4 = shm_open("resource", O_CREAT, O_RDWR);
    ftruncate(fd4, sizeof(sem_t));
    resource = static_cast<sem_t*>(mmap(NULL,sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd4, 0));
    readTry = new sem_t;
    rmutex = new sem_t;
    wmutex = new sem_t; 
    resource = new sem_t;;
    sem_init(readTry, 1, 1);
    sem_init(rmutex, 1, 1);
    sem_init(wmutex, 1, 1);
    sem_init(resource, 1, 1);

    if(checkArgs(argc, argv) == 0){
        cout << "Input arguments are not valid!\n";
        exit(0);
    }
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
    for(int i=0;i<3;i++)
    {
    if((pid=fork())==0)
    {
        temp=getpid();
        //t = t+ (float)t1 / CLOCKS_PER_SEC;
       // cout<<"im t: "<< t <<"\n";
        while(*Ptime<7)
        {
            
            cout<<"im t: "<< *Ptime <<"\n";
            Customer();
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