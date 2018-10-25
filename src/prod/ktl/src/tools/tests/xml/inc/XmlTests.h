/*++

Copyright (c) Microsoft Corporation

Module Name:

    XmlTests.h

Abstract:

    Contains unit test case routine declarations.

    To add a new unit test case:
    1. Declare your test case routine in XmlTests.h.
    2. Add an entry to the gs_KuTestCases table in XmlTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

    --*/

#pragma once
#if KTL_USER_MODE
#include <KmUser.h>
#else
#include <KmUnit.h>
#endif

#include <ktl.h>

class Synchronizer
{
public:
    Synchronizer() : 
        _Event(
            FALSE,  // IsManualReset
            FALSE   // InitialState
            ), 
        _CompletionStatus(STATUS_SUCCESS)
    {
        _Callback.Bind(this, &Synchronizer::AsyncCompletion);
    }
    
    ~Synchronizer() {}

    //
    // Use the returned KDelegate as the async completion callback.
    // Then wait for completion using WaitForCompletion().
    //

    KAsyncContextBase::CompletionCallback
    AsyncCompletionCallback()
    {
        return _Callback;
    }    
    
    NTSTATUS
    WaitForCompletion()
    {        
        _Event.WaitUntilSet();
        return _CompletionStatus;
    }

    virtual void
    OnCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation)
    {
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    }

private:
    VOID
    AsyncCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        )
    {
        UNREFERENCED_PARAMETER(Parent);
        
        _CompletionStatus = CompletingOperation.Status();
        if (NT_SUCCESS(_CompletionStatus))
        {
            OnCompletion(Parent, CompletingOperation);
        }
        _Event.SetEvent();
    }    
    
    KEvent _Event;
    NTSTATUS _CompletionStatus;
    KAsyncContextBase::CompletionCallback _Callback;
};



NTSTATUS
XmlBasicTest(int Argc, WCHAR* Args[]);

NTSTATUS
DomBasicTest(int Argc, WCHAR* Args[]);
