/*++

Copyright (c) Microsoft Corporation

Module Name:

    KDelegateTests.h

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in SampleTests.h.
    2. Add an entry to the gs_KuTestCases table in SampleTestCases.cpp.
    3. Implement your test case routine. The implementation can be in 
        this file or any other file.

--*/
#pragma once
#include "KDelegateScaleTests.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ktl.h"
#include "ktrace.h"
#include "CommandLineParser.h"

#define TEST_NUMBER 7
#ifndef MAX_PATH
#define MAX_PATH 256
#endif

DWORD StaticMethod1_0()
{
	return 0;
}
DWORD StaticMethod1_1(DWORD D0)
{
	return D0;
}
DWORD StaticMethod1_2(DWORD D0, DWORD D1)
{
	return (D0 + D1); 
}
DWORD StaticMethod1_3(DWORD D0, DWORD D1, DWORD D2)
{
	return (D0 + D1 + D2); 
}
DWORD StaticMethod1_4(DWORD D0, DWORD D1, DWORD D2, DWORD D3)
{
	return (D0 + D1 + D2 + D3); 
}
DWORD StaticMethod1_5(DWORD D0, DWORD D1, DWORD D2, DWORD D3, DWORD D4)
{
	return (D0 + D1 + D2 + D3 + D4); 
}
DWORD StaticMethod1_6(DWORD D0, DWORD D1, DWORD D2, DWORD D3, DWORD D4, DWORD D5)
{
	return (D0 + D1 + D2 + D3 + D4 + D5); 
}
DWORD StaticMethod1_7(DWORD D0, DWORD D1, DWORD D2, DWORD D3, DWORD D4, DWORD D5, DWORD D6)
{
	return (D0 + D1 + D2 + D3 + D4 + D5 + D6); 
}
DWORD StaticMethod1_8(DWORD D0, DWORD D1, DWORD D2, DWORD D3, DWORD D4, DWORD D5, DWORD D6, DWORD D7)
{
	return (D0 + D1 + D2 + D3 + D4 + D5 + D6 + D7); 
}

// Scale Tests with free functions

NTSTATUS
FunctionScaleTests(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);
	
	EventRegisterMicrosoft_Windows_KTL();
	KDbgEntry();
	NTSTATUS status = STATUS_SUCCESS;
	
	{
		KDelegate<DWORD (VOID)> callback;	
		callback.Bind(StaticMethod1_0);		 
		if((TEST_NUMBER * 0) != callback())
		{
			KTestPrintf("Free function scale test 1 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}
	{
		KDelegate<DWORD (DWORD)> callback;	
		callback.Bind(StaticMethod1_1);		 
		if((TEST_NUMBER * 1) != callback(TEST_NUMBER))
		{
			KTestPrintf("Free function scale test 2 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}	
	{
		KDelegate<DWORD (DWORD, DWORD)> callback;	
		callback.Bind(StaticMethod1_2);		 
		if((TEST_NUMBER * 2) != callback(TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Free function scale test 3 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}	
	{
		KDelegate<DWORD (DWORD, DWORD, DWORD)> callback;	
		callback.Bind(StaticMethod1_3);		 
		if((TEST_NUMBER * 3) != callback(TEST_NUMBER, TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Free function scale test 4 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}	
	{
		KDelegate<DWORD (DWORD, DWORD, DWORD, DWORD)> callback;	
		callback.Bind(StaticMethod1_4);		 
		if((TEST_NUMBER * 4) != callback(TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Free function scale test 5 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}	
	{
		KDelegate<DWORD (DWORD, DWORD, DWORD, DWORD, DWORD)> callback;	
		callback.Bind(StaticMethod1_5);		 
		if((TEST_NUMBER * 5) != callback(TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Free function scale test 6 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}
	{
		KDelegate<DWORD (DWORD, DWORD, DWORD, DWORD, DWORD, DWORD)> callback;	
		callback.Bind(StaticMethod1_6);		 
		if((TEST_NUMBER * 6) != callback(TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Free function scale test 7 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}
	{
		KDelegate<DWORD (DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD)> callback;	
		callback.Bind(StaticMethod1_7);		 
		if((TEST_NUMBER * 7) != callback(TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Free function scale test 8 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}

ret:
	KDbgExit(status, "Exiting");
	EventUnregisterMicrosoft_Windows_KTL();
	return status;
}

class CallbackClass1
{
public:
	DWORD CallbackMethod1_0()
	{
		return 0;
	}
	DWORD CallbackMethod1_1(DWORD D0)
	{
		return D0;
	}
	DWORD CallbackMethod1_2(DWORD D0, DWORD D1)
	{
		return (D0 + D1); 
	}
	DWORD CallbackMethod1_3(DWORD D0, DWORD D1, DWORD D2)
	{
		return (D0 + D1 + D2); 
	}
	DWORD CallbackMethod1_4(DWORD D0, DWORD D1, DWORD D2, DWORD D3)
	{
		return (D0 + D1 + D2 + D3); 
	}
	DWORD CallbackMethod1_5(DWORD D0, DWORD D1, DWORD D2, DWORD D3, DWORD D4)
	{
		return (D0 + D1 + D2 + D3 + D4); 
	}
	DWORD CallbackMethod1_6(DWORD D0, DWORD D1, DWORD D2, DWORD D3, DWORD D4, DWORD D5)
	{
		return (D0 + D1 + D2 + D3 + D4 + D5); 
	}
	DWORD CallbackMethod1_7(DWORD D0, DWORD D1, DWORD D2, DWORD D3, DWORD D4, DWORD D5, DWORD D6)
	{
		return (D0 + D1 + D2 + D3 + D4 + D5 + D6); 
	}
	DWORD CallbackMethod1_8(DWORD D0, DWORD D1, DWORD D2, DWORD D3, DWORD D4, DWORD D5, DWORD D6, DWORD D7)
	{
		return (D0 + D1 + D2 + D3 + D4 + D5 + D6 + D7); 
	}
};

// Scale Tests with object functions

NTSTATUS
FunctionObjectScaleTests(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

	EventRegisterMicrosoft_Windows_KTL();
	KDbgEntry();
	NTSTATUS status = STATUS_SUCCESS;
	
	CallbackClass1 callbackObj1;

	{
		KDelegate<DWORD (VOID)> callback;	
		callback.Bind(&callbackObj1, &CallbackClass1::CallbackMethod1_0);		 
		if((TEST_NUMBER * 0) != callback())
		{
			KTestPrintf("Object function scale test 1 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}
	{
		KDelegate<DWORD (DWORD)> callback;	
		callback.Bind(&callbackObj1, &CallbackClass1::CallbackMethod1_1);		 
		if((TEST_NUMBER * 1) != callback(TEST_NUMBER))
		{
			KTestPrintf("Object function scale test 2 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}	
	{
		KDelegate<DWORD (DWORD, DWORD)> callback;	
		callback.Bind(&callbackObj1, &CallbackClass1::CallbackMethod1_2);		 
		if((TEST_NUMBER * 2) != callback(TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Object function scale test 3 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}	
	{
		KDelegate<DWORD (DWORD, DWORD, DWORD)> callback;	
		callback.Bind(&callbackObj1, &CallbackClass1::CallbackMethod1_3);		 
		if((TEST_NUMBER * 3) != callback(TEST_NUMBER, TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Object function scale test 4 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}	
	{
		KDelegate<DWORD (DWORD, DWORD, DWORD, DWORD)> callback;	
		callback.Bind(&callbackObj1, &CallbackClass1::CallbackMethod1_4);		 
		if((TEST_NUMBER * 4) != callback(TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Object function scale test 5 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}	
	{
		KDelegate<DWORD (DWORD, DWORD, DWORD, DWORD, DWORD)> callback;	
		callback.Bind(&callbackObj1, &CallbackClass1::CallbackMethod1_5);		 
		if((TEST_NUMBER * 5) != callback(TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Object function scale test 6 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}
	{
		KDelegate<DWORD (DWORD, DWORD, DWORD, DWORD, DWORD, DWORD)> callback;	
		callback.Bind(&callbackObj1, &CallbackClass1::CallbackMethod1_6);		 
		if((TEST_NUMBER * 6) != callback(TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Object function scale test 7 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}
	{
		KDelegate<DWORD (DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD)> callback;	
		callback.Bind(&callbackObj1, &CallbackClass1::CallbackMethod1_7);		 
		if((TEST_NUMBER * 7) != callback(TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER, TEST_NUMBER))
		{
			KTestPrintf("Object function scale test 8 failed");
			status = STATUS_UNSUCCESSFUL;
			goto ret;
		}
	}

ret:
	KDbgExit(status, "Exiting");
	EventUnregisterMicrosoft_Windows_KTL();
	
	return status;
}

