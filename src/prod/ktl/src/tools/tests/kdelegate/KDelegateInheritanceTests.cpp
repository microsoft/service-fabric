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
#include "KDelegateInheritanceTests.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ktl.h"
#include "ktrace.h"
#include "CommandLineParser.h"

typedef KDelegate< size_t (char *) > CallBack;

class CallBackClassB1 
{
public:
	virtual size_t CallBackMethod(char *str) 
	{
        KTestPrintf("%s  %s\n", __FUNCTION__, str);
		return strlen(str);
    }
};

class CallBackClassB2 
{
public:
	virtual size_t CallBackMethod(char *str) 
	{
        KTestPrintf("%s  %s\n", __FUNCTION__, str);
		return strlen(str);
    }
};

class CallBackClassD1 : public CallBackClassB1
{
public:
	virtual size_t CallBackMethod(char *str) 
	{
		KTestPrintf("%s  %s\n", __FUNCTION__, str);
		return strlen(str);
    }
};

//class CallBackClassD2 : public CallBackClassB1, public CallBackClassB2
//{
//	public:
//	int CallBackMethod(char *str) 
//	{
//		KTestPrintf("%s  %s\n", __FUNCTION__, str);
//		return strlen(str);
//    }
//};

//class CallBackClassD3 : public CallBackClassD1
//{
//	public:
//	int CallBackMethodD3(char *str) 
//	{
//		KTestPrintf("%s  %s\n", __FUNCTION__, str);
//		return strlen(str);
//    }
//};
//
//class CallBackClassD4 : public CallBackClassB1
//{
//	public:
//	virtual int CallBackMethodD4(char *str) 
//	{
//		KTestPrintf("%s  %s\n", __FUNCTION__, str);
//		return strlen(str);
//    }
//};
//
//class CallBackClassD5 : public CallBackClassD1, public CallBackClassD4
//{
//	public:
//	int CallBackMethodD5(char *str) 
//	{
//		KTestPrintf("%s  %s\n", __FUNCTION__, str);
//		return strlen(str);
//    }
//};

NTSTATUS
InheritanceTests(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

	EventRegisterMicrosoft_Windows_KTL();
	KDbgEntry();

	char* str = "rvdrvdrvd";
	size_t length = strlen(str);
	NTSTATUS status = STATUS_SUCCESS;

    CallBack callback;
    CallBackClassB1 callbackObjB1;
	CallBackClassD1 callbackObjD1;
    //CallBackClassD2 callbackObjD2;
	//CallBackClassD3 callbackObjD3;
	//CallBackClassD5 callbackObjD5;

	// B1
	callback.Bind(&callbackObjB1, &CallBackClassB1::CallBackMethod);
	if(length != callback(str))
	{
		KTestPrintf("Inheritance test 1 failed");
		status = STATUS_UNSUCCESSFUL;
		goto ret;
	}

	// D1
	callback.Bind(&callbackObjD1, &CallBackClassD1::CallBackMethod);
	if(length != callback(str))
	{
		KTestPrintf("Inheritance test 2 failed");
		status = STATUS_UNSUCCESSFUL;
		goto ret;
	}

	callback.Bind(&callbackObjD1, &CallBackClassB1::CallBackMethod);
	if(length != callback(str))
	{
		KTestPrintf("Inheritance test 3 failed");
		status = STATUS_UNSUCCESSFUL;
		goto ret;
	}
	
	// D2
	//callback.Bind(&callbackObjD2, &CallBackClassD2::CallBackMethod);
	//	if(length != callback(str))
	//{
	//	status = STATUS_UNSUCCESSFUL;
	//	goto ret;
	//}

	//callback.Bind(&callbackObjD2, &CallBackClassB1::CallBackMethod);
	//	if(length != callback(str))
	//{
	//	status = STATUS_UNSUCCESSFUL;
	//	goto ret;
	//}

	//callback.Bind(&callbackObjD2, &CallBackClassB2::CallBackMethod);
	//	if(length != callback(str))
	//{
	//	status = STATUS_UNSUCCESSFUL;
	//	goto ret;
	//}

	//D3
	//callback.Bind(&callbackObjD3, &CallBackClassD3::CallBackMethodD3);
	//	if(length != callback(str))
	//{
	//	status = STATUS_UNSUCCESSFUL;
	//	goto ret;
	//}

	//callback.Bind(&callbackObjD3, &CallBackClassD3::CallBackMethodD1);
	//	if(length != callback(str))
	//{
	//	status = STATUS_UNSUCCESSFUL;
	//	goto ret;
	//}

	//callback.Bind(&callbackObjD3, &CallBackClassD3::CallBackMethodB1);
	//	if(length != callback(str))
	//{
	//	status = STATUS_UNSUCCESSFUL;
	//	goto ret;
	//}

	////D5
	//callback.Bind(&callbackObjD5, &CallBackClassD5::CallBackMethodD5);
	//	if(length != callback(str))
	//{
	//	status = STATUS_UNSUCCESSFUL;
	//	goto ret;
	//}
	
	//callback.Bind(&callbackObjD5, &CallBackClassD5::CallBackMethodD1);
	//	if(length != callback(str))
	//{
	//	status = STATUS_UNSUCCESSFUL;
	//	goto ret;
	//}

	//callback.Bind(&callbackObjD5, &CallBackClassD5::CallBackMethodD4);
	//	if(length != callback(str))
	//{
	//	status = STATUS_UNSUCCESSFUL;
	//	goto ret;
	//}

	// Does not compile due to ambiguous access
	// callback.Bind(&callbackObjD5, &CallBackClassD5::CallBackMethodB1);
	//	if(length != callback(str))
	//{
	//	status = STATUS_UNSUCCESSFUL;
	//	goto ret;
	//}

ret:
	KDbgExit(status, "");
	EventUnregisterMicrosoft_Windows_KTL();
	return status;
}

