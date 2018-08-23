/*++

Copyright (c) Microsoft Corporation

Module Name:

    DescTest.c

Abstract:

    KXM Descriptor tests.

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

#define EXIT_ON_ERROR(condition, msg, args...)  if((condition)) { printf("[%s@%d]:" msg "\n", __FUNCTION__, __LINE__, ##args); exit(-1);}

void dump_usage()
{
    printf("DescriptorTest - Opens specified number of descriptor simultaneously.\n");
    printf("                 This is to test the KXM dynamic descriptor management layer.\n\n");
    printf("DescriptorTest\n");
    printf("             -n:<Number of descriptors port to open.>\n");
}

void DoDescOpenCloseTest(HANDLE DevFD, int NumFD)
{
    NTSTATUS status;
    HANDLE *handle;
    int i;

    handle = (HANDLE *)malloc(sizeof(HANDLE)*NumFD);
    EXIT_ON_ERROR(handle == NULL, "Couldn't allocate memory for handle.");

    printf("Opening %d descriptors..\n", NumFD);
    for(i=0; i<NumFD; i++)
    {
        handle[i] = KXMCreateIoCompletionPort(DevFD, INVALID_HANDLE_VALUE, 
                                            NULL, 0ULL, 0);
        EXIT_ON_ERROR(handle[i] == NULL, "Couldn't open descriptor %d", i);
    }
    printf("Opened %d descriptors..\n", NumFD);

    printf("Closing %d descriptors..\n", NumFD);
    for(i=0; i<NumFD; i++)
    {
        KXMCloseIoCompletionPort(DevFD, handle[i]);
    }
    printf("Closed %d descriptors..\n", NumFD);

    printf("Test Passed!\n");
    return;
}


int main(int argc, char *argv[])
{
    int numCompPorts=(1<<16);   //64K ports
    HANDLE dev_fd = INVALID_HANDLE_VALUE;
    int option = 0;

    //Parse parameter.
    while((option = getopt(argc, argv, "hn:")) != -1) {
        switch (option) {
            case 'n':
                numCompPorts = atoi(optarg);
                break;
            case 'h':
            default:
                dump_usage();
                return 0;
        }
    }
   
    printf("Running test with numDescOpen %d\n", numCompPorts);

    //Open device file.
    dev_fd = KXMOpenDev();
    EXIT_ON_ERROR((long)dev_fd <= (long)INVALID_HANDLE_VALUE, "Unable to open device file %s", KXM_DEVICE_FILE);

    DoDescOpenCloseTest(dev_fd, numCompPorts); 

    KXMCloseDev(dev_fd);
    printf("TEST COMPLETED SUCCESSFULLY\n");
    return 0;
}
