#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int
wmain(int argc, wchar_t** args)
{

    ULONG maxSleepSeconds;
    ULONG sleepSeconds;

    if (argc < 2)
    {
        printf("SleepRand - Sleep for a random number of seconds\n");
        printf("Usage:\n"
               "    SleepRand [max number of seconds to sleep]\n"
            "\n");
        return -1;
    }


    srand( (unsigned)time( NULL ) );

    maxSleepSeconds = _wtoi(args[1]);
    sleepSeconds = rand() % maxSleepSeconds;

    printf("Sleeping %d seconds\n", sleepSeconds);
    Sleep(sleepSeconds * 1000);
    

    return(0);
}



