/*++

Copyright (c) Microsoft Corporation

Module Name:
    EventFileTest.c

Abstract:
    Tests KXM EventFile functionality.

--*/
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <ctype.h>
#include <stdarg.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include "KUShared.h"
#include "KXMApi.h"
#include "strsafe.h"
#include "ktlpal.h"
#include "paldef.h"
#include <sys/types.h>
#include <sys/wait.h>

#if !__has_builtin(__int2c)
#ifdef __aarch64__
extern "C" void __int2c();
#endif
#endif

#define Message(a, b...) printf("[%s@%d] " a "\n", __FUNCTION__, __LINE__, ##b)

//Exit if condition evalutes to true.
#define EXIT_ON_ERROR(condition, msg, args...)  if((condition)) { printf("[%s@%d]:" msg "\n", __FUNCTION__, __LINE__, ##args); __int2c(); exit(-1);}

#define ERR_FD(fd) (fd == -1)
#define ERR_IOCTL(status)   (status != 0)
#define ERR_NULLPTR(ptr)    (ptr == NULL)

#define EVENT_FILE_NAME (char *)"d4534ef1-bff5-4610-91c0-5358129ab838"
//#define STANDALONE_TEST
#ifdef STANDALONE_TEST
void dump_usage()
{
    printf("EventFileTest - Runs EventFile unit tests.\n\n");
    printf("EventFileTest\n");
    printf("             -n:<Number of event files to create.\n");
    printf("             -c:<Maximum number of ops.\n");
    printf("             -t:<Test case index.\n");
}
#endif

static int CreateEventFile(HANDLE DevFD, char *Name,
                            FileMode Mode, HANDLE *EventHandle, HANDLE *Comp, 
                            void *CompletionKey, pid_t pid, int noexit)
{
    HANDLE filehandle;
    HANDLE comphandle;

    ULONG flags = 0;
    filehandle = KXMCreateFile(DevFD, -1, Name, NULL, TRUE, Mode, pid, flags);

    if(noexit && filehandle == INVALID_HANDLE_VALUE)
    {
        *EventHandle = NULL;
        return -1;
    }

    EXIT_ON_ERROR(filehandle == INVALID_HANDLE_VALUE, "Couldn't create KXM file %s in Mode %d\n", 
                    Name, Mode);

    if(Comp)
    {
        //Associate completion port for prod1
        comphandle = KXMCreateIoCompletionPort(DevFD, filehandle, *Comp,
                                (unsigned long long)CompletionKey, 0);
        EXIT_ON_ERROR(comphandle == NULL, "Couldn't create and associate completion port");
        *Comp = comphandle;
    }
    *EventHandle = filehandle;
    return 0;
}


//TestCase:1 Create Event Files in Producer Mode
static NTSTATUS EventFileTest1(HANDLE DevFD, int NumEventFiles)
{
    HANDLE *prod;
    int i;
    char filename[MAX_FILE_NAME_LEN]; 

    prod = (HANDLE *)malloc(sizeof(HANDLE)*NumEventFiles); 
    if(!prod)
        return STATUS_INSUFFICIENT_RESOURCES;

    for(i=0; i<NumEventFiles; i++)
    {
        sprintf(filename, "%s-%d", EVENT_FILE_NAME, i);     
        ULONG flags = 0;
        prod[i] = KXMCreateFile(DevFD, -1, filename, NULL, TRUE, EVENT_PRODUCER, getpid(), flags);
        if(prod[i] != INVALID_HANDLE_VALUE)
        {
            printf("Created EventFile %s with handle %p\n", filename, prod[i]);
        }else
        {
            printf("EventFile %d creation failed!\n", i);
            exit(-1);
        }
    }

    for(i=0; i<NumEventFiles; i++)
    {
        //Closing event file.
        KXMCloseFile(DevFD, prod[i]);
    }
    return STATUS_SUCCESS;
}

//TestCase:2 Create Event File in Producer Mode with same name twice.
static NTSTATUS EventFileTest2(HANDLE DevFD)
{
    HANDLE prod1, prod2;
    int i;

    ULONG flags = 0;
    prod1 = KXMCreateFile(DevFD, -1, EVENT_FILE_NAME, NULL, TRUE, EVENT_PRODUCER, getpid(), flags);
    EXIT_ON_ERROR(prod1 == INVALID_HANDLE_VALUE, "EventFile creation failed!");
    
    //This should fail.
    flags = 0;  
    prod2 = KXMCreateFile(DevFD, -1, EVENT_FILE_NAME, NULL, TRUE, EVENT_PRODUCER, getpid(), flags);
    EXIT_ON_ERROR(prod2 != INVALID_HANDLE_VALUE, 
        "Multiple event files with same name created!");

    //Closing event file.
    KXMCloseFile(DevFD, prod1);
    return STATUS_SUCCESS;
}


//TestCase:3 Create Event File in Producer & Consumer Mode with same name.
static NTSTATUS EventFileTest3(HANDLE DevFD, int NumEventFiles)
{
    HANDLE *prod, *cons;
    int i;
    char filename[MAX_FILE_NAME_LEN];
	ULONG flags;

    prod = (HANDLE *)malloc(sizeof(HANDLE)*NumEventFiles); 
    EXIT_ON_ERROR(prod == NULL, "Couldn't create memory for producer handles.");

    cons = (HANDLE *)malloc(sizeof(HANDLE)*NumEventFiles); 
    EXIT_ON_ERROR(cons == NULL, "Couldn't create memory for consumer handles.");

    for(i=0; i<NumEventFiles; i++)
    {
        sprintf(filename, "%s-%d", EVENT_FILE_NAME, i);
        ULONG flags = 0;        
        prod[i] = KXMCreateFile(DevFD, -1, filename, NULL, TRUE, EVENT_PRODUCER, getpid(), flags);
        if(prod[i] != INVALID_HANDLE_VALUE)
        {
            printf("Created EventFile %s with handle %p\n", filename, prod[i]);
        }else
        {
            printf("EventFile creation failed!");
            exit(-1);
        }
    }

    for(i=0; i<NumEventFiles; i++)
    {
        sprintf(filename, "%s-%d", EVENT_FILE_NAME, i);
        flags = 0;
        cons[i] = KXMCreateFile(DevFD, -1, filename, NULL, TRUE, EVENT_CONSUMER, getpid(), flags);
        if(cons[i] != INVALID_HANDLE_VALUE)
        {
            printf("Created EventFile %s with handle %p\n", filename, cons[i]);
        }else
        {
            printf("EventFile creation failed!");
            exit(-1);
        }
    }

    for(i=0; i<NumEventFiles; i++)
    {
        //Closing event file.
        KXMCloseFile(DevFD, prod[i]);
        KXMCloseFile(DevFD, cons[i]);
    }
    return STATUS_SUCCESS;
}


//TestCase:4 Create Event File in Consumer Mode without File created in
//Producer mode.
static int EventFileTest4(HANDLE DevFD)
{
    HANDLE cons;

    ULONG   flags = 0;
    cons = KXMCreateFile(DevFD, -1, EVENT_FILE_NAME, NULL, TRUE, EVENT_CONSUMER, getpid(), flags);
    EXIT_ON_ERROR(cons != INVALID_HANDLE_VALUE, 
        "Creating EventFile in consumer mode without producer test failed");
    return STATUS_SUCCESS;
}


//TestCase:5 Create Event File in Consumer Mode with invalid process ID.
static NTSTATUS EventFileTest5(HANDLE DevFD, int NumEventFiles)
{
    HANDLE *prod, *cons;
    int i;
    char filename[MAX_FILE_NAME_LEN]; 
    ULONG   flags = 0;

    prod = (HANDLE *)malloc(sizeof(HANDLE)*NumEventFiles); 
    EXIT_ON_ERROR(prod == NULL, "Couldn't create memory for producer handles.");

    cons = (HANDLE *)malloc(sizeof(HANDLE)*NumEventFiles); 
    EXIT_ON_ERROR(cons == NULL, "Couldn't create memory for consumer handles.");

    for(i=0; i<NumEventFiles; i++)
    {
        sprintf(filename, "%s-%d", EVENT_FILE_NAME, i);
        flags = 0;
        prod[i] = KXMCreateFile(DevFD, -1, filename, NULL, TRUE, EVENT_PRODUCER, 0, flags);
        if(prod[i] != INVALID_HANDLE_VALUE)
        {
            printf("Created EventFile %s with handle %p\n", filename, prod[i]);
        }else
        {
            printf("EventFile creation failed!");
            exit(-1);
        }
    }

    for(i=0; i<NumEventFiles; i++)
    {
        sprintf(filename, "%s-%d", EVENT_FILE_NAME, i);
        flags = 0;
        cons[i] = KXMCreateFile(DevFD, -1, filename, NULL, TRUE, EVENT_CONSUMER, getpid(), flags);
        if(cons[i] != INVALID_HANDLE_VALUE)
        {
            printf("Create CONSUMER with PID %d, where as PRODUCER has set"\
                    "PID to %d\n", getpid(), 0);
            exit(-1);
        }
    }

    for(i=0; i<NumEventFiles; i++)
    {
        //Closing event file.
        KXMCloseFile(DevFD, prod[i]);
    }
    return STATUS_SUCCESS;
}


//TestCase:6 Create Event File in prod/cons mode and exit from consumer, this
//should not release any File resources and producer should be still able to
//write.
static NTSTATUS EventFileTest6(HANDLE DevFD)
{
    HANDLE prod = NULL, cons = NULL;
    NTSTATUS status;
    OVERLAPPED overlap;
    HANDLE comp = NULL;

    CreateEventFile(DevFD, EVENT_FILE_NAME, EVENT_PRODUCER, 
                        &prod, &comp, 0, -1, 0);

    CreateEventFile(DevFD, EVENT_FILE_NAME, EVENT_CONSUMER, 
                        &cons, &comp, 0, -1, 0);

    //Close consumer.
    KXMCloseFile(DevFD, cons);

    status = KXMWriteFileAsync(DevFD, prod, NULL, 0, &overlap);
    EXIT_ON_ERROR(status != STATUS_SUCCESS, "WritefileAsync failed!");

    //Close producer
    KXMCloseFile(DevFD, prod);
    return STATUS_SUCCESS;
}


//TestCase:7 Create Event file in prod/cons mode and exit from producer, this
//should casue pending consumer read to complete with STATUS_FILE_CLOSED
static NTSTATUS EventFileTest7(HANDLE DevFD)
{
    HANDLE prod = NULL, cons = NULL;
    NTSTATUS status;
    OVERLAPPED overlap;
    HANDLE comp = NULL;
    LPOVERLAPPED poverlap;
    unsigned long completionKey;
    DWORD numbytes;
    bool result;

    CreateEventFile(DevFD, EVENT_FILE_NAME, EVENT_PRODUCER, 
                        &prod, &comp, 0, -1, 0);
    
    CreateEventFile(DevFD, EVENT_FILE_NAME, EVENT_CONSUMER, 
                        &cons, &comp, 0, -1, 0);

    //Issue Read on consumer endpoint.
    KXMReadFileAsync(DevFD, cons, NULL, 0, &overlap);

    //Close producer endpoint.
    KXMCloseFile(DevFD, prod);

    //KXMRegister Thread Before calling KXMGetQueued
    result = KXMRegisterCompletionThread(DevFD, comp);
    EXIT_ON_ERROR(result != true, 
                "Couldn't register thread!");

    //Completion port should have read completed with STATUS_FILE_CLOSED.
    result = KXMGetQueuedCompletionStatus(DevFD, comp, &numbytes, 
                            &completionKey, &poverlap, INFINITE);

    EXIT_ON_ERROR(result != false, 
        "KXMGetQueueCompletion post consumer close test failed!");

    //KXMUnregister Thread.
    result = KXMUnregisterCompletionThread(DevFD, comp);
    EXIT_ON_ERROR(result != true, 
                "Couldn't unregister thread!");

    //Close producer
    KXMCloseFile(DevFD, cons);
    return STATUS_SUCCESS;
}

//TestCase:8, Producer writes twice before consumer reads. Second write from
//producer would fail until consumer reads first write. 
static NTSTATUS EventFileTest8(HANDLE DevFD)
{
    char filename[MAX_FILE_NAME_LEN]; 
    HANDLE prod = NULL, cons = NULL, comp = NULL;
    OVERLAPPED overlap;
    LPOVERLAPPED poverlap;
    NTSTATUS status;
    unsigned long completionKey;
    DWORD numbytes;
    bool result;

    sprintf(filename, "%s-%d", EVENT_FILE_NAME, 0);
    CreateEventFile(DevFD, filename, EVENT_PRODUCER, &prod, 
                            &comp, NULL, getpid(), 0);

    CreateEventFile(DevFD, filename, EVENT_CONSUMER, &cons, 
                            &comp, NULL, getpid(), 0);

    //This must return STATUS_SUCCES.
    status = KXMWriteFileAsync(DevFD, prod, NULL, 0, &overlap);
    EXIT_ON_ERROR(status != STATUS_SUCCESS, "WritefileAsync failed!");

    //KXMRegister Thread Before calling KXMGetQueued
    result = KXMRegisterCompletionThread(DevFD, comp);
    EXIT_ON_ERROR(result != true, 
                "Couldn't register thread!");

    //Drain completion queue.
    result = KXMGetQueuedCompletionStatus(DevFD, comp, &numbytes,
                        &completionKey, &poverlap, INFINITE);
    EXIT_ON_ERROR(result != true || poverlap != &overlap,
                    "KXMGetQueuedCompletionStatus Returned failure!");

    //This must return STATUS_ENCOUNTERED_WRITE_IN_PROGRESS since consumer 
    //hasn't read the previously written value.
    status = KXMWriteFileAsync(DevFD, prod, NULL, 0, &overlap);
    EXIT_ON_ERROR(status != STATUS_ENCOUNTERED_WRITE_IN_PROGRESS, 
                        "WritefileAsync shouldn't have succeeded!");

    //Now read the posted write from consumer endpoint, after which a new write
    //should succeed.
    status = KXMReadFileAsync(DevFD, cons, NULL, 0, &overlap);
    EXIT_ON_ERROR(status != STATUS_SUCCESS, "ReadfileAsync failed!");

    result = KXMGetQueuedCompletionStatus(DevFD, comp, &numbytes, &completionKey,
                                    &poverlap, INFINITE);
    EXIT_ON_ERROR(result != true || poverlap != &overlap,
                    "KXMGetQueuedCompletionStatus Returned failure!");

    //KXMUnregister Thread.
    result = KXMUnregisterCompletionThread(DevFD, comp);
    EXIT_ON_ERROR(result != true, 
                "Couldn't unregister thread!");

    //This must return STATUS_SUCCESS since consumer has read the previous
    //write.
    status = KXMWriteFileAsync(DevFD, prod, NULL, 0, &overlap);
    EXIT_ON_ERROR(status != STATUS_SUCCESS, "WritefileAsync failed!");

    KXMCloseFile(DevFD, prod);
    KXMCloseFile(DevFD, cons);
    KXMCloseIoCompletionPort(DevFD, comp);
    return STATUS_SUCCESS;
}

//TestCase:9 Test consumer read failure scenarios.
static NTSTATUS EventFileTest9(HANDLE DevFD)
{
    char filename[MAX_FILE_NAME_LEN]; 
    HANDLE prod = NULL, cons = NULL, comp = NULL;
    OVERLAPPED overlap;
    LPOVERLAPPED poverlap;
    NTSTATUS status;
    unsigned long completionKey;

    sprintf(filename, "%s-%d", EVENT_FILE_NAME, 0);
    CreateEventFile(DevFD, filename, EVENT_PRODUCER, &prod, 
                            &comp, NULL, getpid(), 0);

    CreateEventFile(DevFD, filename, EVENT_CONSUMER, &cons, 
                            &comp, NULL, getpid(), 0);

    //Now read the posted write from consumer endpoint, this should return
    //STATUS_SUCCESS but since producer has not written anything, operation
    //won't complete.
    status = KXMReadFileAsync(DevFD, cons, NULL, 0, &overlap);
    EXIT_ON_ERROR(status != STATUS_SUCCESS, "ReadfileAsync failed!");

    //Issuing another read should fail with STATUS_RESOURCE_IN_USE as previous 
    //read is still not completed.
    status = KXMReadFileAsync(DevFD, cons, NULL, 0, &overlap);
    EXIT_ON_ERROR(status != STATUS_RESOURCE_IN_USE, "KXMReadFileAsync shouldn't have succeeded!");

    KXMCloseFile(DevFD, prod);
    KXMCloseFile(DevFD, cons);
    KXMCloseIoCompletionPort(DevFD, comp);

    return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
//TestCase:10 InProc Ensure Write from Producer wake up Consumer.
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

typedef struct 
{
    HANDLE DevFD;
    //Issue Read on This FD.
    HANDLE ConsumerFD;
    //Issue GetCompletion on This FD.
    HANDLE CompletionFD;
    volatile int *Count;
    volatile int MaxOperations;
}ThreadArgs;

static void *StartEventTest(void *Args)
{
    ThreadArgs *arg = (ThreadArgs *)Args;
    unsigned long completionKey;
    HANDLE prod;
    OVERLAPPED overlap1, overlap2;
    LPOVERLAPPED poverlap = NULL;
    NTSTATUS status;
    volatile int *count = arg->Count;
    DWORD numbytes;
    bool result;

    //Submit Async Read
    status = KXMReadFileAsync(arg->DevFD, arg->ConsumerFD, NULL, 0, &overlap1);
    EXIT_ON_ERROR(status != STATUS_SUCCESS, "ReadfileAsync failed!");

    //KXMRegister Thread.
    result = KXMRegisterCompletionThread(arg->DevFD, arg->CompletionFD);
    EXIT_ON_ERROR(result != true, 
                "Couldn't unregister thread!");

    while(1)
    {
        result = KXMGetQueuedCompletionStatus(arg->DevFD, 
                                arg->CompletionFD, &numbytes, &completionKey,
                                &poverlap, 0);

        if(result == false)
        {
            if(GetLastError() == ERROR_TIMEOUT) 
                continue;
            EXIT_ON_ERROR(1, "KXMGetQueueCompletion failed!");
        }


        prod = (void *)completionKey; 
        if(prod != NULL)
        {
            //printf("Count@%d, Cons@%p, WriteOn@%p\n", *count, arg->ConsumerFD, prod);

            if(*count < arg->MaxOperations)
                (*count)++;

            //Wakeup remote process first.
            status = KXMWriteFileAsync(arg->DevFD, prod, NULL, 0, &overlap2);
            
            EXIT_ON_ERROR(status != STATUS_SUCCESS, "WritefileAsync failed!");

            if(*count >= arg->MaxOperations)
                return 0;

            //printf("Count reached %d\n", *count);
            //Submit Async Read 
            status = KXMReadFileAsync(arg->DevFD, arg->ConsumerFD, NULL, 0, &overlap1);
            EXIT_ON_ERROR(status != STATUS_SUCCESS, "ReadfileAsync failed!");
        }
    }
    //KXMUnregister Thread.
    result = KXMUnregisterCompletionThread(arg->DevFD, arg->CompletionFD);
    EXIT_ON_ERROR(result != true, 
                "Couldn't unregister thread!");

    return (void *)STATUS_SUCCESS;
}

static NTSTATUS EventFileTest10(HANDLE DevFD, int MaxOperations)
{
    HANDLE prod1, prod2, cons1, cons2;
    HANDLE comp1 = NULL;
    HANDLE comp2 = NULL;
    char filename[MAX_FILE_NAME_LEN]; 
    ThreadArgs arg1, arg2;
    int count = 0;
    pthread_t thread1, thread2;
    int ret;
    NTSTATUS status;
    OVERLAPPED overlap;
    void *ret1, *ret2;

    /*

        Thread-1                          Thread-2
                           PIPE-0
                      |||||||||||||||
      Prod1,Comp1     |||||||||||||||    CONS1,Comp2
                      |||||||||||||||


                           PIPE-1
                      |||||||||||||||
      Cons2,Comp1     |||||||||||||||    Prod2,Comp2
                      |||||||||||||||
    
    */

    //Create Producer for first pipe.
    sprintf(filename, "%s-%d", EVENT_FILE_NAME, 0);
    CreateEventFile(DevFD, filename, EVENT_PRODUCER, &prod1, 
                            &comp1, NULL, getpid(), 0);
 
    //Create Producer second pipe.
    sprintf(filename, "%s-%d", EVENT_FILE_NAME, 1);
    CreateEventFile(DevFD, filename, EVENT_PRODUCER, &prod2, 
                            &comp2, NULL, getpid(), 0);

    //Create Consumer for second pipe.
    sprintf(filename, "%s-%d", EVENT_FILE_NAME, 1);
    CreateEventFile(DevFD, filename, EVENT_CONSUMER, &cons2, 
                            &comp1, 
                            /*This would be returned as completion key.*/prod1, 
                            0, 0);

    //GetEvent would be submitted on this FD.
    arg1.CompletionFD = comp1;
    //KXMReadFile would be submitted on this FD.
    arg1.ConsumerFD = cons2;

    //Create Consumer for first pipe.
    sprintf(filename, "%s-%d", EVENT_FILE_NAME, 0);
    CreateEventFile(DevFD, filename, EVENT_CONSUMER, &cons1, 
                            &comp2, 
                            /*This would be returned as completion key.*/prod2,
                            0, 0);

    //GetEvent would be submitted on this FD.
    arg2.CompletionFD = comp2;
    //KXMReadFile would be submitted on this FD.
    arg2.ConsumerFD = cons1;


    arg1.Count = arg2.Count = &count;
    arg1.MaxOperations = arg2.MaxOperations = MaxOperations;
    arg1.DevFD = arg2.DevFD = DevFD;

    ret = pthread_create(&thread1, NULL, StartEventTest, &arg1);
    EXIT_ON_ERROR(ret < 0 , "Thread creation failed!");

    ret = pthread_create(&thread2, NULL, StartEventTest, &arg2);
    EXIT_ON_ERROR(ret < 0 , "Thread creation failed!");

    //write into prod1, this will cause thread2 to wakeup and let the fun begin.
    status = KXMWriteFileAsync(DevFD, prod1, NULL, 0, &overlap);
    EXIT_ON_ERROR(status != STATUS_SUCCESS, "WritefileAsync failed!");

    //wait for both threads.
    pthread_join(thread1, &ret1);
    pthread_join(thread2, &ret2);
    EXIT_ON_ERROR(ret1 != (void *)STATUS_SUCCESS || ret2 != (void *)STATUS_SUCCESS,
        "ret1 %#x, ret2 %#x\n", (int)(long)ret1, (int)(long)ret2);

    //Close all descriptors.
    KXMCloseFile(DevFD, prod1);
    KXMCloseFile(DevFD, prod2);
    KXMCloseFile(DevFD, cons1);
    KXMCloseFile(DevFD, cons2);
    KXMCloseIoCompletionPort(DevFD, comp1);
    KXMCloseIoCompletionPort(DevFD, comp2);
    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//TestCase:11 Across Proc Ensure Write from Producer wake up Consumer.
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void ProcessChild(char *ProdName, char *ConsName, int Start, int MaxOps)
{
    HANDLE devFD = NULL;
    HANDLE prod = NULL;
    HANDLE cons = NULL;
    HANDLE comp = NULL;
    int ret;
    int writecount = 0;
    int readcount = 0;
    OVERLAPPED overlap1, overlap2;
    int count = 0;
    unsigned long completionKey;
    LPOVERLAPPED poverlap;
    NTSTATUS status;
    DWORD numbytes;
    bool result;

    devFD = KXMOpenDev();
    EXIT_ON_ERROR((long)devFD <= (long)INVALID_HANDLE_VALUE, "Unable to open device file %s", KXM_DEVICE_FILE);

    //Allow any process to connect to  this pipe.
    CreateEventFile(devFD, ProdName, EVENT_PRODUCER, &prod, &comp, (void *)(long)1, -1, 0);
    printf("Process%d, File [%s] Created in Producer Mode.\n", getpid(), ProdName);

    while(count < 10)
    {
        ret = CreateEventFile(devFD, ConsName, EVENT_CONSUMER, &cons, &comp, (void *)(long)2, -1, 1);
        if(ret < 0){
            sleep(1);
            count++;
            //This can happen if other process has not opened the producer file
            //yet.
            printf("CreateEventFile as Consumer [%s] failed, retrying %d time..\n", ConsName, count);
            continue;
        }
        break;
    }

    EXIT_ON_ERROR(cons == NULL, "Couldn't create consumer endpoint for file %s", ConsName);

    printf("Process%d, File [%s] Created in Consumer Mode.\n", getpid(), ConsName);

    if(Start)
    {
        //Let's write into producer endpoint.
        status = KXMWriteFileAsync(devFD, prod, NULL, 0, &overlap2);
        EXIT_ON_ERROR(status != STATUS_SUCCESS, "Write failed!");
    }

    //Issue an outstanding read on the consumer.
    status = KXMReadFileAsync(devFD, cons, NULL, 0, &overlap1);
    EXIT_ON_ERROR(status != STATUS_SUCCESS, "ReadfileAsync failed!");

    //KXMRegister Thread Before calling KXMGetQueued
    result = KXMRegisterCompletionThread(devFD, comp);
    EXIT_ON_ERROR(result != true, 
                "Couldn't register thread!");

    while(writecount < MaxOps || readcount < MaxOps)
    {
        result = KXMGetQueuedCompletionStatus(devFD, comp, &numbytes,
                                &completionKey, &poverlap, INFINITE);

        EXIT_ON_ERROR(result != true,
                            "GetqueueCompletion failed -- status %#x, Internal %#x\n",
                            status, poverlap->Internal);

        if(completionKey == 1)
        {   
            writecount++;
            continue;
        }
        else
        {
            readcount++;
            if(readcount < MaxOps)
            {
                status = KXMReadFileAsync(devFD, cons, NULL, 0, &overlap1);
                EXIT_ON_ERROR(status != STATUS_SUCCESS, "ReadfileAsync failed!");
            }

            if(writecount < MaxOps)
            {
                //Wakeup remote process first.
                status = KXMWriteFileAsync(devFD, prod, NULL, 0, &overlap2);
                EXIT_ON_ERROR(status != STATUS_SUCCESS, "WritefileAsync failed!");
            }
        }
    }
    //KXMUnregister Thread.
    result = KXMUnregisterCompletionThread(devFD, comp);
    EXIT_ON_ERROR(result != true, 
                "Couldn't unregister thread!");

    printf("TotalReadOps %d, TotalWriteOps %d\n", readcount, writecount);

    status = KXMCloseFile(devFD, prod);
    if(status != STATUS_SUCCESS)
        printf("Closing producer failed %#x\n", status);

    status = KXMCloseFile(devFD, cons);
    if(status != STATUS_SUCCESS)
        printf("Closing consumer failed status %#x\n", status);

    status = KXMCloseIoCompletionPort(devFD, comp);
    if(status != STATUS_SUCCESS)
        printf("Closing comport failed status %#x\n", status);

    KXMCloseDev(devFD);

    exit(0);
}

static NTSTATUS EventFileTest11(HANDLE DevFD, int MaxOperations)
{
    char name1[MAX_FILE_NAME_LEN];
    char name2[MAX_FILE_NAME_LEN];
    pid_t pid1, pid2;
    int ret1, ret2;

    //Setup filename.
    sprintf(name1, "%s-%d", EVENT_FILE_NAME, 0);
    sprintf(name2, "%s-%d", EVENT_FILE_NAME, 1);

    //Flush any pending prints from this process to avoid duplicate outputs in
    //child.
    fflush(stdout);

    if((pid1 = fork()) == 0){
        //Child process.
        //Don't share devFD with parent.
        KXMCloseDev(DevFD);
        ProcessChild(name1, name2, 1, MaxOperations);
        exit(0);
    }
    if((pid2 = fork()) == 0){
        //Child process.
        //Don't share devFD with parent.
        KXMCloseDev(DevFD);
        ProcessChild(name2, name1, 0, MaxOperations);
        exit(0);
    }
    waitpid(pid1, &ret1, 0);
    waitpid(pid2, &ret2, 0);

    EXIT_ON_ERROR(ret1 != 0 || ret2 != 0, "ret1 %#x, ret2 %#x\n", ret1, ret2);

    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    int option = 0;
    HANDLE dev_fd = INVALID_HANDLE_VALUE;
    int numEventFiles = 64;
    int numTestCase = 0;
    int maxops = 1000;

    //Open device file.
    dev_fd = KXMOpenDev();
    EXIT_ON_ERROR((long)dev_fd <= (long)INVALID_HANDLE_VALUE, 
        "Unable to open device file %s", KXM_DEVICE_FILE);

#ifdef STANDALONE_TEST
    while((option = getopt(argc, argv, "hn:t:c:")) != -1) {
        switch (option) {
            case 'n':
                numEventFiles = atoi(optarg);
                break;
            case 't':
                numTestCase = atoi(optarg);
                break;
            case 'c':
                maxops = atoi(optarg);
                break;
            case 'h':
            default:
                dump_usage();
                return 0;
        }
    }
#else
    //Run all tests.
    for(numTestCase=1; numTestCase<12; numTestCase++)
#endif
    {
        printf("Running Test %d\n", numTestCase);
        switch(numTestCase)
        {
            case 1:
                EventFileTest1(dev_fd, numEventFiles);
                break;
            case 2:
                EventFileTest2(dev_fd);
                break;
            case 3:
                EventFileTest3(dev_fd, numEventFiles);
                break;
            case 4:
                EventFileTest4(dev_fd);
                break;
            case 5:
                EventFileTest5(dev_fd, numEventFiles);
                break;
            case 6:
                EventFileTest6(dev_fd);
                break;
            case 7:
                EventFileTest7(dev_fd);
                break;
            case 8:
                EventFileTest8(dev_fd);
                break;
            case 9:
                EventFileTest9(dev_fd);
                break;
            case 10:
                EventFileTest10(dev_fd, maxops);
                break;
            case 11:
                EventFileTest11(dev_fd, maxops);
                break;
            default:
                break;
        }
        printf("Test %d passed\n", numTestCase);
    }
    KXMCloseDev(dev_fd);

    printf("TEST COMPLETED SUCCESSFULLY\n");
    return 0;
}
