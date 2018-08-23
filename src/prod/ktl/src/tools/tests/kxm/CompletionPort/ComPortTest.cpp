/*++

Copyright (c) Microsoft Corporation

Module Name:

    Comport.c

Abstract:

    Completion Port Unit Test

--*/
#define _GNU_SOURCE             /* See feature_test_macros(7) */
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
#include <sys/time.h>
#include "KUShared.h"
#include "KXMApi.h"
#include "KXMKApi.h"
#include "strsafe.h"
#include "ktlpal.h"
#include "paldef.h"

#define SEC_TO_NSEC(x) ((unsigned long long)(x)*1000*1000*1000)
#define MSEC_TO_NSEC(x) ((unsigned long long)(x)*1000*1000)

#define Message(a, b...) printf("[%s@%d] " a "\n", __FUNCTION__, __LINE__, ##b)

#define ERR_FD(fd) (fd == -1)
#define ERR_IOCTL(status)   (status != 0)
#define ERR_NULLPTR(ptr)    (ptr == NULL)
#define EXIT_ON_ERROR(condition, msg, args...)  if((condition)) { printf("[%s@%d]:" msg "\n", __FUNCTION__, __LINE__, ##args); exit(-1);}

void dump_usage()
{
    printf("CompPort - Validates completion port implementation of KXM driver\n\n");
    printf("CompPort\n");
    printf("             -n:<Number of completion port to open.>\n");
    printf("             -c:<Number of threads waiting per completion port.>\n");
    printf("             -t:<Number of threads per completion port posting event.>\n");
    printf("             -m:<Maximum number of outstanding requests per completion port.>\n");
    printf("             -a:<Total # of per completion port operations.\n");
}

typedef struct CompTestConfig 
{
    HANDLE DevFD;
    int ID;
    int NumGetEventThreads;
    int NumPostEventThreads;
    int MaxOutstanding;
    unsigned long MaxOps;
    int MaxThreadsAllowed;
}CompTestConfig;

typedef enum _Command
{
    ResetCommand = 0,
    CallGetEvent,
    SleepThread,
    Exit
}MaxThreadCommand;

typedef enum _State
{
    CallingGetEvent=1,
    AfterGetEvent,
    Sleeping,
    AfterSleep,
    Exitting
}MaxThreadState;

typedef struct _MaxThreadConfig
{
    HANDLE DevFD;
    HANDLE CompHandle;
    pthread_barrier_t *Exitbarrier;
    volatile int Command;
    volatile int State;
}MaxThreadConfig;

typedef struct _EventCount
{
    OVERLAPPED Overlapped;
    volatile unsigned long NumEventsProcessed;
    volatile unsigned long NumEventsSubmitted;
    int exit;
}EventCount;

typedef struct _GetEventThreadArgs
{
    int ID;
    HANDLE DevFD;
    HANDLE CompHandle;
    EventCount Events;
}GetEventThreadArgs;

typedef struct _PostEventThreadArgs
{
    int ID;
    HANDLE DevFD;
    HANDLE CompHandle;
    EventCount Events;
    unsigned long MaxOutstanding;
    unsigned long MaxOps;
}PostEventThreadArgs;

void *PostEventThread(void *Args)
{
    PostEventThreadArgs *postEventArgs = (PostEventThreadArgs *)Args;
    unsigned long outstanding;
    EventCount *event;
    int count = 0;
    bool result;

    event = &postEventArgs->Events;

    while(1)
    {
retry:
        if(event->NumEventsSubmitted >= postEventArgs->MaxOps)
        {
            Message("PostEventThread Done with requests! Exiting");
            return (void *)0L;
        }

        outstanding = event->NumEventsSubmitted - event->NumEventsProcessed;
        if(outstanding > postEventArgs->MaxOutstanding)
        {
            count++;
            usleep(5*1000);
            if(count == 10000)
            {
                printf("PostEvent couldn't progress due to MaxOutStanding limit.\n");
                count = 0;
            }
            goto retry;
        }
        __sync_fetch_and_add(&event->NumEventsSubmitted, 1);
        result = KXMPostQueuedCompletionStatus(postEventArgs->DevFD, 
                                    postEventArgs->CompHandle, 0, 0,
                                    (LPOVERLAPPED)event);
        if(result != true)
        {
            Message("KXMPostQueuedCompletionStatus failed!");
            return (void *)-1L;
        }
    }
    return (void *)-1L;
}

void *GetEventThread(void *Args)
{
    GetEventThreadArgs *getEventArgs = (GetEventThreadArgs *)Args;
    EventCount *event;
    NTSTATUS status;
    unsigned long completionKey;
    DWORD numbytes;
    long eventprocessed = 0;
    bool result;

    //KXMRegister
    result = KXMRegisterCompletionThread(getEventArgs->DevFD, 
                            getEventArgs->CompHandle);
    if(result != true)
    {
        Message("Couldn't register thread!\n");
        return (void *)-1L;
    }   
 
    while(1)
    {
        result = KXMGetQueuedCompletionStatus(getEventArgs->DevFD,
                                getEventArgs->CompHandle, &numbytes,
                                &completionKey, (LPOVERLAPPED *)&event, 
                                INFINITE);

        if(result == false && GetLastError() == ERROR_ABANDONED_WAIT_0)
        {
            Message("CompletionPort closed..");
            //Incase CompletionPort is closed, don't need to Unregister the
            //thread.
            return (void *)eventprocessed;
        }
        else if(result != true)
        {
            Message("GetEventThread: Exiting with error %#x\n", GetLastError());
            //Unrgister
            result = KXMUnregisterCompletionThread(getEventArgs->DevFD, 
                                        getEventArgs->CompHandle);

            if(result != true)
            {
                Message("Couldn't unregister thread!\n");
                return (void *)-1L;
            }   

            return (void *)-1L;
        }
        else
        {
            if(event->exit  == 1)
            {
                Message("GetEventThread: Rcvd Exit Event!");
                //Unrgister
                result = KXMUnregisterCompletionThread(getEventArgs->DevFD, 
                                            getEventArgs->CompHandle);

                if(result != true)
                {
                    Message("Couldn't unregister thread!\n");
                    return (void *)-1L;
                }   
                return (void *)eventprocessed;
            }
            else
            {
                __sync_fetch_and_add((unsigned long *)(&event->NumEventsProcessed), 1);
                eventprocessed++;
            }
        }
    }
    return (void *)-1L;
}

void *StartCompPortTest(void *Args)
{
    CompTestConfig *config = (CompTestConfig *)Args;
    pthread_t *postEventThreads = NULL;
    pthread_t *getEventThreads = NULL;
    int i = 0;
    NTSTATUS statsus;
    int ret = -1;
    void *threadExitCode;
    long error = 0;
    HANDLE handle = INVALID_HANDLE_VALUE;
    PostEventThreadArgs postEventArgs;
    GetEventThreadArgs getEventArgs;
    unsigned long completedOps = 0;
    EventCount exitEvent; 
    NTSTATUS status;
    bool result;

    postEventThreads = (pthread_t *)malloc(config->NumPostEventThreads*sizeof(pthread_t));
    EXIT_ON_ERROR(postEventThreads == NULL, "postEventThreads memory allocation failure.");

    getEventThreads = (pthread_t *)malloc(config->NumGetEventThreads*sizeof(pthread_t));
    EXIT_ON_ERROR(getEventThreads == NULL, "getEventThreads memory allocation failure.");

    //OpenCompletionPort
    handle = KXMCreateIoCompletionPort(config->DevFD, INVALID_HANDLE_VALUE,
                                NULL, 0ULL, config->MaxThreadsAllowed);
    EXIT_ON_ERROR(handle == NULL, "KXMCreateIo completion failed.");

    //Setup PostEvent Args.
    postEventArgs.DevFD = config->DevFD;
    postEventArgs.CompHandle = handle;
    postEventArgs.MaxOutstanding = config->MaxOutstanding; 
    postEventArgs.MaxOps = config->MaxOps;
    postEventArgs.Events.exit = 0;
    postEventArgs.Events.NumEventsSubmitted = 0;
    postEventArgs.Events.NumEventsProcessed = 0;

    //Start threads to post events.
    for(i=0; i<config->NumPostEventThreads; i++)
    {
        ret = pthread_create(&postEventThreads[i], NULL, PostEventThread, &postEventArgs);
        EXIT_ON_ERROR(ret != 0, "CompThread-%d SubmitThread create failed for %d", config->ID, i);
    }

    //Setup GetEvent Args.
    getEventArgs.DevFD = config->DevFD;
    getEventArgs.CompHandle = handle;
    getEventArgs.Events.exit = 0;
    //Start threads to process events.
    for(i=0; i<config->NumGetEventThreads; i++)
    {
        ret = pthread_create(&getEventThreads[i], NULL, GetEventThread, &getEventArgs);
        EXIT_ON_ERROR(ret != 0, "CompThread-%d WaitThread create failed for %d", config->ID, i);
    }

    //Wait for PostEventThreads. 
    for(i=0; i<config->NumPostEventThreads; i++)
    {
        pthread_join(postEventThreads[i], &threadExitCode);
        EXIT_ON_ERROR(threadExitCode != 0L, 
                "CompTestThread-%d SubmitThread %d failed!\n", config->ID, i);
    }

    printf("PostEventThread Exitted..\n");

    //Send exit request to all GetEventThreads.
    exitEvent.exit = 1;
    for(i=0; i<config->NumGetEventThreads; i++)
    {
        result = KXMPostQueuedCompletionStatus(postEventArgs.DevFD, 
                                    postEventArgs.CompHandle, 0, 0,
                                    (LPOVERLAPPED)&exitEvent);
        EXIT_ON_ERROR(result != true, 
            "CompTestThread-%d Failed submitting exit event!\n", config->ID);
    }

    printf("Waiting For GetEventThreads to Exit..\n");

    //Wait for GetEventThreads. 
    for(i=0; i<config->NumGetEventThreads; i++)
    {
        pthread_join(getEventThreads[i], &threadExitCode);
        EXIT_ON_ERROR((long)threadExitCode < 0L, 
            "CompTestThread-%d, WaitThread %d failed with exitcode %#lx\n", config->ID, i, (long)threadExitCode);
        printf("Thread%d, Processed %ld events\n", i, (long)threadExitCode);
    }

    if(handle != NULL)
    {
        KXMCloseIoCompletionPort(config->DevFD, handle);
        handle = NULL;
    }

    Message("NumberOfEventsSubmitted %ld, NumberOfEventsProcessed %ld\n", 
                        postEventArgs.Events.NumEventsSubmitted,
                        postEventArgs.Events.NumEventsProcessed);

    KXMCloseIoCompletionPort(config->DevFD, handle);
    free(postEventThreads);
    free(getEventThreads);
    return (void *)error;
}

NTSTATUS KXMCreateIoCompletionPortTest(HANDLE DevFD)
{
    HANDLE handle;

    //
    // TESTCASE 1 : KXMCreateIoCompletion
    // Try to create a completion port with Invalid Device Handle DevFD.
    // DevFD This should fail
    //
    handle = CreateIoCompletionPortK(INVALID_HANDLE_VALUE, NULL, NULL, 0ULL, 0);
    EXIT_ON_ERROR(handle != NULL, "Create completion port with invalid KXMDev handl test failed.");

    //
    // TESTCASE 2 : KXMCreateIoCompletion
    // Try to create an IO completion port with Invalid compHandle and +ve Value for FileHandle
    // This should fail.
    //
    handle = CreateIoCompletionPortK(DevFD, (HANDLE)10, INVALID_HANDLE_VALUE, 0ULL, 0);
    EXIT_ON_ERROR(handle != NULL, "Create completion port with invalid comphandle test failed.");

    //
    // TESTCASE 3 : KXMCreateIoCompletion
    // Try to create IO completion port with Invalid CompHandle and FileHandle values.
    // This should fail.
    //
    handle = CreateIoCompletionPortK(DevFD, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0ULL, 0);
    EXIT_ON_ERROR(handle != NULL, "Create completion port with invalid comphandle and filehandle test failed.");

    //
    // TESTCASE 4 : KXMCreateIoCompletion
    // Try to create IO completion port with Invalid CompHandle and 'NULL' FileHandle.
    // This should succeed.
    //
    handle = CreateIoCompletionPortK(DevFD, INVALID_HANDLE_VALUE, NULL, 0ULL, 0);
    EXIT_ON_ERROR(handle == NULL, "Creating completion port failed.");

    //
    // TESTCASE 5 : KXMCreateIoCompletion
    // Try to setup association between two completion port.
    // This should fail.
    //
    handle = CreateIoCompletionPortK(DevFD, handle, NULL, 0ULL, 0);
    EXIT_ON_ERROR(handle != NULL, "Setting up association between two completion Ports failed.");
	
    return STATUS_SUCCESS; 
}

NTSTATUS KXMGetQueuedCompletionStatusTest(HANDLE DevFD)
{
    HANDLE handle;
    DWORD  numBytes;
    unsigned long completionKey;
    LPOVERLAPPED poverlap;
    OVERLAPPED overlap;
    volatile unsigned long long delta, pre_tv_in_mill, post_tv_in_mill;
    struct timespec ts_start, ts_end;
    NTSTATUS status;
    int timeout_in_ms = 50;
    //There is a ~10% drift in time between clock_gettime and kernel time calculation.
    float factor = 0.90;
    bool result;

    overlap.Offset = 15;
    overlap.OffsetHigh = 57;
    overlap.Internal = STATUS_SUCCESS;
 
    handle = KXMCreateIoCompletionPort(DevFD, INVALID_HANDLE_VALUE, NULL, 0ULL, 0);
    EXIT_ON_ERROR(handle == NULL, "Create completion port failed.");

    result = KXMPostQueuedCompletionStatus(DevFD, handle, 7, 0, &overlap);
    EXIT_ON_ERROR(result != true, "Post Queue operation failed.");

    //
    // TESTCASE 1 : KXMGetQueuedCompletionStatusTest
    // Try to get queued completion status from an Invalid Device Handle.
    // This should return STATUS_INVALID_HANDLE
    //
    result = KXMGetQueuedCompletionStatus(INVALID_HANDLE_VALUE, handle, &numBytes, &completionKey, &poverlap, INFINITE); 
    EXIT_ON_ERROR(result != false || GetLastError() != ERROR_INVALID_HANDLE, "KXMGetQueued completion with invalid KXMDev handle failed.");

    //
    // TESTCASE 2 : KXMGetQueuedCompletionStatusTest
    // Try with invalid compHandle.
    // This should fail.
    //
    result = KXMGetQueuedCompletionStatus(DevFD, INVALID_HANDLE_VALUE, &numBytes, &completionKey, &poverlap, INFINITE);
    EXIT_ON_ERROR(result != false || GetLastError() != ERROR_INVALID_HANDLE, "KXMGetQueued completion with invalid CompHandle failed.\n");

    //
    // TESTCASE 3 : KXMGetQueuedCompletionStatusTest
    // Check correctness of number of Bytes, completion key and poverlap
    //

    //KXMRegister Thread.
    result = KXMRegisterCompletionThread(DevFD, handle);
    EXIT_ON_ERROR(result != true, "Registeration of Thread failed!\n");

    result = KXMGetQueuedCompletionStatus(DevFD, handle, &numBytes, &completionKey, &poverlap, INFINITE);
    EXIT_ON_ERROR(result != true, "KXMGetQueued completion operation failed \n");
    EXIT_ON_ERROR((numBytes !=7 || poverlap->Offset != 15 || poverlap->OffsetHigh != 57), 
        "KXMGetQueued and KXMPostQueued parameter mismatch. Received numBytes %d, Offset %d, OffsetHigh %d", 
        numBytes, poverlap->Offset, poverlap->OffsetHigh);

    //
    // TESTCASE 4 : KXMGetQueuedCompletionStatusTest 
    // Test the timeout setting.
    // Diff between clock time before and after KXMGetQueuedCompletionStatus should be greater than Timeout value.
    //
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
 
    result = KXMGetQueuedCompletionStatus(DevFD, handle, &numBytes, &completionKey, &poverlap, timeout_in_ms);
    EXIT_ON_ERROR(result != false || GetLastError() != ERROR_TIMEOUT, "KXMGetQueued completion operation failed \n");
   
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    
    delta = SEC_TO_NSEC(ts_end.tv_sec - ts_start.tv_sec) + 
               (ts_end.tv_nsec>ts_start.tv_nsec? (ts_end.tv_nsec-ts_start.tv_nsec):(ts_start.tv_nsec - ts_end.tv_nsec));

    EXIT_ON_ERROR(delta < MSEC_TO_NSEC(timeout_in_ms)*factor, 
                "KXMGetQueuedCompletionStatusTest timeout failed : detla %ld, expected %ld", delta, MSEC_TO_NSEC(timeout_in_ms));

    //KXMUnregister Thread.
    result = KXMUnregisterCompletionThread(DevFD, handle);
    EXIT_ON_ERROR(result != true, "Unregisteration of Thread failed!\n");

    return STATUS_SUCCESS; 
}

NTSTATUS KXMPostQueuedCompletionStatusTest(HANDLE DevFD)
{
    HANDLE handle;
    DWORD  numBytes;
    unsigned long completionKey;
    OVERLAPPED overlap;
    LPOVERLAPPED poverlap;
    NTSTATUS status;
    bool result;
 
    overlap.Offset = 15;
    overlap.OffsetHigh = 57;
    overlap.Internal = STATUS_SUCCESS;

    handle = KXMCreateIoCompletionPort(DevFD, INVALID_HANDLE_VALUE, NULL, 0ULL, 0);
    EXIT_ON_ERROR(handle == NULL, "KXMCreateIo completion failed.");

    //
    // TESTCASE 1 : KXMPostQueuedCompletionStatusTest.
    // Try with invalid KXMDev.
    // This should fail.
    //
    result = KXMPostQueuedCompletionStatus(INVALID_HANDLE_VALUE, handle, 0, 0, &overlap);
    EXIT_ON_ERROR(result != false || GetLastError() != ERROR_INVALID_HANDLE, 
                    "KXMPostQueued with invalid kxm dev handle test failed.");

    //
    // TESTCASE 2 : KXMPostQueuedCompletionStatusTest.
    // Try with invalid compHandle.
    // This should fail.
    //
    result = KXMPostQueuedCompletionStatus(DevFD, INVALID_HANDLE_VALUE, 0, 0, &overlap);
    EXIT_ON_ERROR(result != false || GetLastError() != ERROR_INVALID_HANDLE, 
                        "KXMPostQueued with invalid comphandle test failed.");

    //
    // TESTCASE 3 : KXMPostQueuedCompletionStatusTest.
    // Verify, KXMGetQueued call is not blocked after successful KXMPostQueued return.
    //
    result = KXMPostQueuedCompletionStatus(DevFD, handle, 0, 0, &overlap);
    EXIT_ON_ERROR(result != true, "KXMPostQueue operation failed.\n");

    //KXMRegister
    result = KXMRegisterCompletionThread(DevFD, handle);
    EXIT_ON_ERROR(result != true, "Registeration of Thread failed!\n");

    result = KXMGetQueuedCompletionStatus(DevFD, handle, &numBytes, &completionKey, &poverlap, INFINITE);
    EXIT_ON_ERROR(result != true,
            "KXMGetQueued test after KXMPostQueued operation failed.");

    //UnRegister
    result = KXMUnregisterCompletionThread(DevFD, handle);
    EXIT_ON_ERROR(result != true, "Unregisteration of Thread failed!\n");

    return STATUS_SUCCESS;
}

NTSTATUS KXMCloseDevTest()
{
    HANDLE handle = INVALID_HANDLE_VALUE;
    HANDLE DevFD = INVALID_HANDLE_VALUE;
    DWORD  numBytes;
    unsigned long completionKey;
    OVERLAPPED overlap;
    LPOVERLAPPED poverlap;
    NTSTATUS status;
    bool result;

    DevFD = KXMOpenDev();
    EXIT_ON_ERROR((long)DevFD <= (long)INVALID_HANDLE_VALUE, "Unable to open device file %s", KXM_DEVICE_FILE);
 
    overlap.Internal = STATUS_SUCCESS; 
    handle = KXMCreateIoCompletionPort(DevFD, INVALID_HANDLE_VALUE, NULL, 0ULL, 0);
    EXIT_ON_ERROR(handle == NULL, "KXMCreateIo completion failed.");
    //
    // TESTCASE : KXMCloseDevTest.
    // After successful KXMPostQueued, close KXMDev and again call KXMGetQueued & KXMPostQueued.
    //
    result = KXMPostQueuedCompletionStatus(DevFD, handle, 0, 0, &overlap);
    EXIT_ON_ERROR(result != true,
                "KXMPostQueued operation failed.\n");

    status = KXMCloseDev(DevFD);
    EXIT_ON_ERROR(status != STATUS_SUCCESS,
                "KXMCloseDevTest: Couldn't close KXM device\n");
    
    result = KXMGetQueuedCompletionStatus(DevFD, handle, &numBytes, &completionKey, &poverlap, INFINITE); 
    EXIT_ON_ERROR(result != false,  
                "KXMGetQueued operation after close KXMDev test failed.\n");

    result = KXMPostQueuedCompletionStatus(DevFD, handle, 0, 0, &overlap);
    EXIT_ON_ERROR(result != false || GetLastError() != ERROR_INVALID_HANDLE, 
                "KXMPostQueued operation after close KXMDev test failed.\n");

    return STATUS_SUCCESS;
}

NTSTATUS MultiThreadCompPortTest(HANDLE DevFD, int NumCompPorts, 
        int NumGetEventThreads, int NumPostEventThreads,
        long NumMaxOutstandingOps, long NumTotalOps, int NumMaxThreads)
{
    pthread_t *compTestThreads;
    CompTestConfig config;
    long ret;
    int i;
    void *returnCode;

    Message("numCompPorts %d\n", NumCompPorts);
    Message("numGetEventThreads %d\n", NumGetEventThreads);
    Message("numPostEventThreads %d\n", NumPostEventThreads);
    Message("num_max_outstanding_ops %ld\n", NumMaxOutstandingOps);
    Message("numTotalOps %ld\n", NumTotalOps);

    compTestThreads = (pthread_t *)malloc(NumCompPorts * sizeof(pthread_t));
    EXIT_ON_ERROR(compTestThreads == NULL, "OOM for compTestThreads!");

    config.DevFD = DevFD;
    config.NumGetEventThreads = NumGetEventThreads;
    config.NumPostEventThreads = NumPostEventThreads;
    config.MaxOutstanding = NumMaxOutstandingOps;
    config.MaxOps = NumTotalOps;
    config.MaxThreadsAllowed = NumMaxThreads;

    for(i=0; i<NumCompPorts; i++)
    {
        ret = pthread_create(&compTestThreads[i], NULL, StartCompPortTest, &config);
        EXIT_ON_ERROR(ret != 0, 
            "Thread create failed for %d, num_compo_ports %d", i, NumCompPorts);
    }

    for(i=0; i<NumCompPorts; i++)
    {
        Message("Waiting for CompTestThread %d to complete.\n", i);
        pthread_join(compTestThreads[i], &returnCode);
        EXIT_ON_ERROR(returnCode != 0, "CompTestThread %d failed!\n", i);
    }

    return STATUS_SUCCESS;
}

void *WorkerThread(void *Args)
{
    MaxThreadConfig *config = (MaxThreadConfig *)Args;
    DWORD  numBytes;
    unsigned long completionKey;
    LPOVERLAPPED poverlap;
    NTSTATUS status;
    bool result;

    //KXMRegister Thread
    result = KXMRegisterCompletionThread(config->DevFD, config->CompHandle);
    EXIT_ON_ERROR(result != true, "Registeration of Thread failed!\n");

    while(1)
    {
        switch(config->Command)
        {
            case CallGetEvent:
                config->Command = ResetCommand;
                config->State = CallingGetEvent;

                printf("Calling GetEvent.\n");
                //Call GetEvent.
                result = KXMGetQueuedCompletionStatus(config->DevFD,
                                config->CompHandle, &numBytes, &completionKey,
                                &poverlap, INFINITE); 
                EXIT_ON_ERROR(result != true,  "KXMGetQueued operation failed.\n");

                //Set state.
                config->State = AfterGetEvent;
                break;

            case SleepThread:
                config->Command = ResetCommand;
                config->State = Sleeping;
                sleep(2);
                config->State = AfterSleep;
                break;

            case Exit:
                config->State = Exitting;
                goto exit;

            default:
                break;
        }
    }
exit:
    printf("Thread Exitting..\n");
    //KXMUnregister Thread
    result = KXMUnregisterCompletionThread(config->DevFD, config->CompHandle);
    EXIT_ON_ERROR(result != true, "Unregisteration of Thread failed!\n");
    return NULL;
}

static NTSTATUS MaxThreadUnitTest(HANDLE DevFD, int NumGetEventThreads, 
                                    int NumMaxThreads)
{
    pthread_t *threads;
    MaxThreadConfig *config;
    OVERLAPPED overlap;
    int dispatched = 0;
    int blocked = 0;
    int i, ret;
    NTSTATUS status;
    HANDLE handle;
    pthread_barrier_t exitbarrier;
    int retry, max_retry=5;
    bool result;

    threads = (pthread_t *)malloc(NumGetEventThreads * sizeof(pthread_t));
    EXIT_ON_ERROR(threads == NULL, "OOM for threads!");
    
    config = (MaxThreadConfig *)malloc(NumGetEventThreads * sizeof(MaxThreadConfig));
    EXIT_ON_ERROR(config == NULL, "OOM for threads!");

    //Create Completion Port with NumMaxThreads.
    handle = KXMCreateIoCompletionPort(DevFD, INVALID_HANDLE_VALUE, NULL, 0ULL, 
                                        NumMaxThreads);
    EXIT_ON_ERROR(handle == NULL, "KXMCreateIo completion failed.");


    //Create WorkerThreads. All threads would enter into busy wait waiting for
    //command from the master.
    for(i=0; i<NumGetEventThreads; i++)
    {
        memset(&config[i], 0, sizeof(MaxThreadConfig));
        config[i].DevFD = DevFD;
        config[i].CompHandle = handle;
        ret = pthread_create(&threads[i], NULL, WorkerThread, &config[i]);
        EXIT_ON_ERROR(ret != 0, "Thread create failed for %d", i);
    }

    //Set CallGetEvent command for all threads, since we haven't posted any
    //event yet all threads would block.
    for(i=0; i<NumGetEventThreads; i++)
    {
        config[i].Command = CallGetEvent;
    }

    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////

    //Post NumGetEventThreads CompletionEvents. Only NumMaxThreads would be
    //dispatched.
    overlap.Internal = STATUS_SUCCESS;
    for(i=0; i<NumGetEventThreads; i++)
    {
       result = KXMPostQueuedCompletionStatus(DevFD, handle, 0, 0, &overlap);
       EXIT_ON_ERROR(result != true, "KXMPostQueued failed.\n");
    }

    retry = 0;
    //Check how many threads have come out of the loop.
    while(retry++ < max_retry)
    {
        for(i=0; i<NumGetEventThreads; i++)
        {
            if(config[i].State == AfterGetEvent)
                dispatched++;
            else
                blocked++;
        }
        if(dispatched == NumMaxThreads)
            break;
        dispatched = 0;
        blocked = 0;
        sleep(1);
    }
    printf("Number of threads dispatched %d, blocked %d\n", dispatched, blocked);
    EXIT_ON_ERROR(dispatched != NumMaxThreads, "Thread count mismatch!");

    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////

    //Make one of the active thread sleep this should cause KXM to dispatch one
    //of the blocking thread.
    for(i=0; i<NumGetEventThreads; i++)
    {
        if(config[i].State == AfterGetEvent)
        {
            config[i].Command = SleepThread;
            break;
        }
    }

    //Check number of running should now have NumMaxThreads+1 threads running in tight loop. 
    retry = 0;
    while(retry++ < max_retry)
    {
        dispatched = 0;
        for(i=0, blocked=0, dispatched=0; i<NumGetEventThreads; i++)
        {
            if(config[i].State == AfterGetEvent || 
                    config[i].State == AfterSleep)
                dispatched++;
        }
        if(dispatched == (NumMaxThreads+1))
            break;
        sleep(1);
    }

    printf("Current Active Threads %d, Blocked Threads %d, Max Threads allowed %d\n", 
                        dispatched, blocked, NumMaxThreads);
    EXIT_ON_ERROR(dispatched != (NumMaxThreads + 1), "Thread count mismatch!");
    
    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    
    //Make one of the active thread to call GetEvent again, it shouldn't get
    //dispatched. 
    for(i=0; i<NumGetEventThreads; i++)
    {
        if(config[i].State == AfterGetEvent ||
                config[i].State == AfterSleep)
        {
            config[i].Command = CallGetEvent; 
            break;
        }
    }
    //Post one more event for the above thread.
    result = KXMPostQueuedCompletionStatus(DevFD, handle, 0, 0, &overlap);
    EXIT_ON_ERROR(result != true, "KXMPostQueued failed.\n");

    sleep(1);

    while(retry++ < max_retry)
    {
        for(i=0, blocked=0, dispatched=0; i<NumGetEventThreads; i++)
        {
            if(config[i].State == AfterGetEvent || 
                    config[i].State == AfterSleep)
                dispatched++;
        }
        if(dispatched == NumMaxThreads)
            break;
        dispatched = 0;
        sleep(1);
    }

    printf("Current Active Threads %d, Max Threads allowed %d\n", 
                        dispatched, NumMaxThreads);
    EXIT_ON_ERROR(dispatched != (NumMaxThreads), "Thread count mismatch!");
    
    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    
    //Set exit command for all threads.
    for(i=0; i<NumGetEventThreads; i++)
    {
        printf("Setting Thread %d for Exit\n", i);
        config[i].Command = Exit;
    }

    for(i=0; i<NumGetEventThreads; i++)
    {
        printf("Waiting for thread %d to exit.\n", i);
        pthread_join(threads[i], NULL);
    }

    return STATUS_SUCCESS;
}

int main(int argc, char *argv[])
{
    int option = 0;
    int numCompPorts=1, numGetEventThreads=8, numPostEventThreads=8;
    unsigned long numMaxOutstandingOps=100000;
    unsigned long numTotalOps=100000;
    HANDLE dev_fd = INVALID_HANDLE_VALUE;
    int numMaxThreads = 2;

    //Parse parameter.
    while((option = getopt(argc, argv, "hn:c:t:m:a:")) != -1) {
        switch (option) {
            case 'n':
                numCompPorts = atoi(optarg);
                break;
            case 'c':
                numGetEventThreads = atoi(optarg);
                break;
            case 't':
                numPostEventThreads = atoi(optarg);
            case 'm':
                numMaxOutstandingOps = strtoul(optarg, NULL, 10);
                break;
            case 'a':
                numTotalOps = strtoul(optarg, NULL, 10);
                break;
            case 'x':
                numMaxThreads = strtoul(optarg, NULL, 10);
                break;
            case 'h':
            default:
                dump_usage();
                return 0;
        }
    }
    
    //Open device file.
    dev_fd = KXMOpenDev();
    EXIT_ON_ERROR((long)dev_fd <= (long)INVALID_HANDLE_VALUE, "Unable to open device file %s", KXM_DEVICE_FILE);

    printf("File opened with FD %d\n", (int)(long)dev_fd);

    //Create IO Completion Port Test
    printf("Running KXMCreateIoCompletionPort Test\n");
    KXMCreateIoCompletionPortTest(dev_fd);
    printf("KXMCreateIoCompletionPort test passed!\n");

    //KXMGetQueued Completion Status Test
    printf("Running KXMGetQueuedCompStatus Test\n");
    KXMGetQueuedCompletionStatusTest(dev_fd);
    printf("KXMGetQueuedCompStatus test passed!\n");

    //KXMPostQueued Completion Status Test
    printf("Running KXMPostQueuedCompStatus Test\n");
    KXMPostQueuedCompletionStatusTest(dev_fd);
    printf("KXMPostQueuedCompStatus test passed!\n");

    //Close KXMDev and try KXMGetQueuedCompletionStatus & KXMPostQueuedCompletionStatus
    printf("Running Get/Post test after closing KXM device.\n");
    KXMCloseDevTest(); 
    printf("Get/Post on Closed KXM device test passed!\n");

    //Validate CompPort Testing with multiple threads.
    printf("Running Multithread comport test with following parameters.\n");
    printf("NumComPorts %d, NumGetThreads %d NumPostThreads %d\n", 
            numCompPorts, numGetEventThreads, numPostEventThreads);
    printf("MaxOutstandingOps %d, TotalOps %d\n", numMaxOutstandingOps, numTotalOps);
    
    MultiThreadCompPortTest(dev_fd, numCompPorts, numGetEventThreads, 
                numPostEventThreads, numMaxOutstandingOps, numTotalOps, numMaxThreads);
    printf("Multithread comport test passed!\n");
 
    MaxThreadUnitTest(dev_fd, numGetEventThreads, numMaxThreads);
    printf("MaxThreadUnitTest passed!\n");

    KXMCloseDev(dev_fd);
    printf("TEST COMPLETED SUCCESSFULLY\n");
    return 0;
}
