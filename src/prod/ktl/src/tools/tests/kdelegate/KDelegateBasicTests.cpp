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
#include "KDelegateBasicTests.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ktl.h"
#include "ktrace.h"
#include "CommandLineParser.h"

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

#define TEST_NUMBER 7
#ifndef MAX_PATH
#define MAX_PATH 256
#endif

int StaticMethod2(KWString Str1, KWString& Str2, KWString* Str3)
{
    KDbgEntry();

    Str2;  //  Avoid Error C4100 (Unreferenced formal parameter)
    Str3;  //  Avoid Error C4100 (Unreferenced formal parameter)
    return sizeof(Str1) + sizeof(Str2) + sizeof(*Str3);
}

int StaticMethodWithContext2(PVOID Context, KWString Str1, KWString& Str2, KWString* Str3)
{
    KDbgEntry();
    KTestPrintf("%s\n", Context);
    return StaticMethod2(Str1, Str2, Str3);
}

class CallbackClassB2
{
public:
    virtual int CallbackMethodB2(KWString Str1, KWString& Str2, KWString* Str3)
    {
        KDbgEntry();
        return StaticMethod2(Str1, Str2, Str3);
    }
};

class CallbackClassD2 : public CallbackClassB2
{
public:
    int CallbackMethodD2(KWString Str1, KWString& Str2, KWString* Str3)
    {
        KDbgEntry();
        return this->CallbackClassB2::CallbackMethodB2(Str1, Str2, Str3);
    }
};

// Constructor Tests

NTSTATUS
ConstructorTests(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    EventRegisterMicrosoft_Windows_KTL();
    KDbgEntry();
    NTSTATUS status = STATUS_SUCCESS;

    KtlSystem::Initialize();

    {
        // Params
        KWString str1(KtlSystem::GlobalNonPagedAllocator(), L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        KWString str2(KtlSystem::GlobalNonPagedAllocator(), L"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
        KWString str3(KtlSystem::GlobalNonPagedAllocator(), L"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");

        int totalSize =  sizeof(str1) + sizeof(str2) + sizeof(str3);

        typedef KDelegate<int (KWString, KWString&, KWString*)> Callback;

        {
             Callback callback;
        }
        {
            Callback callback(StaticMethod2);
            if(totalSize != callback(str1, str2, &str3))
            {
                KTestPrintf("Constructor Test 1 failed");
                status = STATUS_UNSUCCESSFUL;
                goto ret;
            }
			
			//** Prove const override call via KDelegate<>
			if (totalSize != callback.ConstCall(str1, str2, &str3))
			{
				KTestPrintf("Constructor Test 1 failed");
				status = STATUS_UNSUCCESSFUL;
				goto ret;
			}
		}
        {
            Callback callback((PVOID)"context", StaticMethodWithContext2);
            if(totalSize != callback(str1, str2, &str3))
            {
                KTestPrintf("Constructor Test 2 failed");
                status = STATUS_UNSUCCESSFUL;
                goto ret;
            }
        }
        {
            CallbackClassB2 callbackObj2;
            Callback callback(&callbackObj2, &CallbackClassB2::CallbackMethodB2);
            if(totalSize != callback(str1, str2, &str3))
            {
                KTestPrintf("Constructor Test 3 failed");
                status = STATUS_UNSUCCESSFUL;
                goto ret;
            }
        }
        {
            CallbackClassD2 callbackObj2;
            Callback callback(&callbackObj2, &CallbackClassB2::CallbackMethodB2);
            if(totalSize != callback(str1, str2, &str3))
            {
                KTestPrintf("Constructor Test 4 failed");
                status = STATUS_UNSUCCESSFUL;
                goto ret;
            }
        }
    }

ret:
    KDbgExit(status, "");
    KtlSystem::Shutdown();
    EventUnregisterMicrosoft_Windows_KTL();
    return status;
}


// Bind Tests

NTSTATUS
BindTests(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    EventRegisterMicrosoft_Windows_KTL();
    KDbgEntry();
    NTSTATUS status = STATUS_SUCCESS;
    KtlSystem::Initialize();

    {
        // Params
        KWString str1(KtlSystem::GlobalNonPagedAllocator(), L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        KWString str2(KtlSystem::GlobalNonPagedAllocator(), L"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
        KWString str3(KtlSystem::GlobalNonPagedAllocator(), L"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");

        int totalSize =  sizeof(str1) + sizeof(str2) + sizeof(str3);

        typedef KDelegate<int (KWString, KWString&, KWString*)> Callback;

        {
            Callback callback;
            callback.Bind(StaticMethod2);
            if(totalSize != callback(str1, str2, &str3))
            {
                KTestPrintf("Bind Test 1 failed");
                status = STATUS_UNSUCCESSFUL;
                goto ret;
            }
        }
        {
            Callback callback;
            callback.Bind((PVOID)"context", StaticMethodWithContext2);
            if(totalSize != callback(str1, str2, &str3))
            {
                KTestPrintf("Bind Test 2 failed");
                status = STATUS_UNSUCCESSFUL;
                goto ret;
            }
        }
        {
            Callback callback;
            CallbackClassB2 callbackObj2;
            callback.Bind(&callbackObj2, &CallbackClassB2::CallbackMethodB2);
            if(totalSize != callback(str1, str2, &str3))
            {
                KTestPrintf("Bind Test 3 failed");
                status = STATUS_UNSUCCESSFUL;
                goto ret;
            }
        }
        {
            Callback callback;
            CallbackClassD2 callbackObj2;
            callback.Bind(&callbackObj2, &CallbackClassB2::CallbackMethodB2);
            if(totalSize != callback(str1, str2, &str3))
            {
                KTestPrintf("Bind Test 4 failed");
                status = STATUS_UNSUCCESSFUL;
                goto ret;
            }
        }
    }

ret:
    KDbgExit(status, "");
    KtlSystem::Shutdown();
    EventUnregisterMicrosoft_Windows_KTL();
    return status;
}

// Re-bind Tests

NTSTATUS
RebindTests(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    EventRegisterMicrosoft_Windows_KTL();
    KDbgEntry();
    NTSTATUS status = STATUS_SUCCESS;
    KtlSystem::Initialize();

	{
		// Params
		KWString str1(KtlSystem::GlobalNonPagedAllocator(), L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
		KWString str2(KtlSystem::GlobalNonPagedAllocator(), L"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
		KWString str3(KtlSystem::GlobalNonPagedAllocator(), L"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");

		int totalSize =  sizeof(str1) + sizeof(str2) + sizeof(str3);

		typedef KDelegate<int (KWString, KWString&, KWString*)> Callback;

		{
			Callback callback;

			callback.Bind(StaticMethod2);
			if(totalSize != callback(str1, str2, &str3))
			{
				KTestPrintf("Rebind Test 1 failed");
				status = STATUS_UNSUCCESSFUL;
				goto ret;
			}

			callback.Bind((PVOID)"context", StaticMethodWithContext2);
			if(totalSize != callback(str1, str2, &str3))
			{
				KTestPrintf("Rebind Test 2 failed");
				status = STATUS_UNSUCCESSFUL;
				goto ret;
			}

			CallbackClassB2 callbackObjB2;
			callback.Bind(&callbackObjB2, &CallbackClassB2::CallbackMethodB2);
			if(totalSize != callback(str1, str2, &str3))
			{
				KTestPrintf("Rebind Test 3 failed");
				status = STATUS_UNSUCCESSFUL;
				goto ret;
			}

			CallbackClassD2 callbackObjD2;
			callback.Bind(&callbackObjD2, &CallbackClassB2::CallbackMethodB2);
			if(totalSize != callback(str1, str2, &str3))
			{
				KTestPrintf("Rebind Test 4 failed");
				status = STATUS_UNSUCCESSFUL;
				goto ret;
			}
		}
	}

ret:
    KDbgExit(status, "");
    KtlSystem::Shutdown();
    EventUnregisterMicrosoft_Windows_KTL();
    return status;
}





//
//
//
//// Test for KDelegate<VOID (VOID)>
//
//VOID StaticMethod1()
//{
//  KTestPrintf("%s\n", __FUNCTION__);
//}
//
//VOID StaticMethodWithContext1(PVOID Context)
//{
//  KTestPrintf("%s  %s\n", __FUNCTION__, Context);
//}
//
//class CallbackClassB1
//{
//public:
//  virtual VOID CallbackMethodB1()
//  {
//      KTestPrintf("%s\n", __FUNCTION__);
//      StaticMethod1();
//  }
//};
//
//class CallbackClassD1 : public CallbackClassB1
//{
//public:
//  VOID CallbackMethodD1()
//  {
//      KTestPrintf("%s\n", __FUNCTION__);
//      this->CallbackClassB1::CallbackMethodB1();
//  }
//};
//
//NTSTATUS
//TestOne(
//    int argc, WCHAR* args[]
//    )
//{
//  typedef KDelegate<VOID (VOID)> Callback;
//
//  // Constructor Tests
//  {
//      Callback callback;
//      callback();
//  }
//  {
//      Callback callback(StaticMethod1);
//      callback();
//  }
//  {
//      Callback callback("context", StaticMethodWithContext1);
//      callback();
//  }
//  {
//      CallbackClassB1 callbackObj1;
//      Callback callback(&callbackObj1, &CallbackClassB1::CallbackMethodB1);
//      callback();
//  }
//  {
//      CallbackClassD1 callbackObj1;
//      Callback callback(&callbackObj1, &CallbackClassB1::CallbackMethodB1);
//      callback();
//  }
//
//  // Bind Tests
//  {
//      Callback callback;
//      callback.Bind(StaticMethod1);
//      callback();
//  }
//  {
//      Callback callback;
//      callback.Bind("context", StaticMethodWithContext1);
//      callback();
//  }
//  {
//      Callback callback;
//      CallbackClassB1 callbackObj1;
//      callback.Bind(&callbackObj1, &CallbackClassB1::CallbackMethodB1);
//      callback();
//  }
//  {
//      Callback callback;
//      CallbackClassD1 callbackObj1;
//      callback.Bind(&callbackObj1, &CallbackClassB1::CallbackMethodB1);
//      callback();
//  }
//
//    return STATUS_SUCCESS;
//}
//



//typedef KDelegate< BOOL (VOID) > CallBack2;
//typedef KDelegate< PBOOL (VOID) > CallBack3;
//typedef KDelegate< LARGE_STRING (VOID) > CallBack5;
//typedef KDelegate< PLARGE_STRING (VOID) > CallBack6;
//
//typedef KDelegate< NTSTATUS (BOOL) > CallBack7;
//typedef KDelegate< NTSTATUS (BOOL, BOOL, BOOL, BOOL, BOOL, BOOL, BOOL, BOOL, BOOL) > CallBack8;
//
//typedef KDelegate< NTSTATUS (BOOL) > CallBack9;
//typedef KDelegate< NTSTATUS (LARGE_STRING, LARGE_STRING, LARGE_STRING, LARGE_STRING, LARGE_STRING, LARGE_STRING, LARGE_STRING, LARGE_STRING, LARGE_STRING) > CallBack10;
//
//typedef KDelegate< NTSTATUS (PLARGE_STRING) > CallBack11;
//typedef KDelegate< NTSTATUS (LARGE_STRING&) > CallBack12;

// Test for KDelegate<NTSTATUS (DWORD, DWORD&, PDWORD, LARGE_ARRAY, LARGE_ARRAY&, PLARGE_ARRAY)>
