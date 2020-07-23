#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h>

#include <ktl.h>
#include <ktrace.h>

struct MyMessage
{
    ULONG Val1;
    ULONG Val2;
};

typedef KDelegate<ULONG(MyMessage*)> MyMessage_Handler;

class KNetworkServer
{
public:
     ULONG Register(MyMessage_Handler& Deleg);

};


class MyClass
{
    int _x;

public:
    ULONG Handler(MyMessage *Msg)
    {
        UNREFERENCED_PARAMETER(Msg);

        printf("Callback\n");
    }
};



int testFunc2(int a, int b)
{
    KDbgEntry();

    // Custom checkpoint example

    TRACE_EID_MY_MODULE_CHECKPOINTEXAMPLE(1, ERROR_SUCCESS, "Sample custom checkpoint with message");

    return a + b;
}

void testfunc()
{
    KDbgEntry();

    KDbgCheckpoint(1, "About to call testFunc2");

    int total = testFunc2(10, 20);

    KDbgCheckpoint(2, "Finished with testFunct2");

    KDbgPrintf("testFunc2 returned %u\n", total);

    KDbgExit(0, "Success");
}

void main()
{
    EventRegisterMicrosoft_Windows_KTL();
    KDbgEntry();

    testfunc();

    EventUnregisterMicrosoft_Windows_KTL();
}
