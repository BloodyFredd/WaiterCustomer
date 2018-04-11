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
using namespace std;

int ReadCount=0,WriterCount=0;
sem_t rmutex, wmutex , readTry, resource;

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

void Waiter()
{
  sem_wait(&readTry);
  sem_wait(&rmutex);
  ReadCount++;
  if(ReadCount == 1)
	sem_wait(&resource);
  sem_post(&rmutex);
  sem_post(&readTry);

  cout<<"Critical section\n";

  sem_wait(&rmutex);
  ReadCount--;
  if(ReadCount == 0)
	sem_post(&resource);	
  sem_post(&rmutex);
}

void* Customer()
{
  sem_wait(&wmutex);
  WriterCount++;
  if(WriterCount == 1)
	sem_wait(&readTry);
  sem_post(&wmutex);

  sem_wait(&resource);
	cout<<"Critical section\n";
  sem_post(&resource);
  
  sem_wait(&wmutex);
  WriterCount--;
  if( WriterCount == 0)
	sem_post(&readTry);
  sem_post(&wmutex);
}

int main(int argc, char* argv[])
{
    clock_t t1;
    if(checkArgs(argc, argv) == 0){
        cout << "Input arguments are not valid!\n";
        exit(0);
    }
    sem_init(&readTry, 0, 1);
    sem_init(&rmutex, 0, 1);
    sem_init(&wmutex, 0, 1);
    sem_init(&resource, 0, 1);
    int numOfItems = atoi(argv[2]);
    t1 = clock();
    cout << setfill('=') << setw(25) << "Simulation arguments" << setfill('=') << setw(25) << "\n";
    cout << "Simulation time: " << argv[1];
    cout << "\nMenu items count: " << argv[2];
    cout << "\nCustomers count: " << argv[3];
    cout << "\nWaiters count: " << argv[4] << "\n";
    cout << setfill('=') << setw(50) << "\n";
    printf("%.2f", (float)t1 / CLOCKS_PER_SEC);
    cout << " Main process ID " << "PUT PID HERE" << " start\n";
    Menu menu[] = {{ 0 , "Pizza" , 10.5 , 0 }, { 1 , "Salad", 7.50, 0}, { 2, "Hamburger", 12.00, 0}, { 3, "Spaghetti", 9.00, 0}, {4, "Pie", 9.50, 0}, {5, "Milkshake", 6.00, 0} , {6, "Vodka", 12.25, 0}};
    printMenu(menu, numOfItems);
    return 1;
}
