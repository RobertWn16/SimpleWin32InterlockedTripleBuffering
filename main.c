#include <Windows.h>
#include <stdio.h>
/*
* Simple Win32 FreeLock and 0 copy triple buffering especially for reading video frames with delay.
* A good example for learn Interlocked API
*/

unsigned char* queue[3]; //Can be extended to triple buffering
int index;
int mask = 0;
char value[3][40];

HANDLE hEvent;

void producer()
{
    for (int i = 0; i < 3; i++) {
        sprintf(value[index % 3], "Test %d", index); //The buffer will be filled with the same value

        queue[i] = malloc(sizeof(char*));
        queue[i] = value;
    }
    SetEvent(hEvent);
    while (TRUE)
    {
        sprintf(value[index % 3], "Test %d", index);

        InterlockedCompareExchangePointer((volatile PVOID*)&queue[2], (PVOID*)value[index % 3], queue[2]); //2nd Back buffer
        InterlockedCompareExchangePointer((volatile PVOID*)&queue[1], (PVOID*)queue[2], queue[1]); //1st backBuffer for reduce tearing
        InterlockedExchange(&mask, 0); //Triggers consumer to take the 1st backBuffer
        index++;
        Sleep(1000); //Slow producer
    }
}

void consumer()
{
    //Swap the 1st back buffer to be presented
    InterlockedCompareExchangePointer((volatile PVOID*)&queue[0], (PVOID*)queue[1], queue[0]);

    //Wait until 1st backBuffer is ready (If the producer is too slow for example timestamp read from a video)
    while (InterlockedBitTestAndSet(&mask, 0)) {};

    printf("%s \n", queue[0]); //Presenting the context

    Sleep(1000); //Slow consumer
}

int main() {
    hEvent = CreateEvent(NULL, TRUE, FALSE, 0, NULL);
    ResetEvent(hEvent);
    
    HANDLE hThread = CreateThread(0,
        0,
        (LPTHREAD_START_ROUTINE)producer,
        NULL,
        0,
        NULL);
    
    WaitForSingleObject(hEvent, INFINITE);
    
    while (TRUE)
    {
        consumer();
    }

    //The loop is true always. Make the loop with fixed number of steps. 
    //This is just for testing. The program will have memory leaks.
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(hEvent);
    
    free(queue);
}
