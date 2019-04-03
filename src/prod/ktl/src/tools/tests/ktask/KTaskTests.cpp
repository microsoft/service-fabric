/*++

Copyright (c) Microsoft Corporation

Module Name:

    KTaskTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KTaskTests.h.
    2. Add an entry to the gs_KuTestCases table in KTaskTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KTaskTests.h"
#include <CommandLineParser.h>


KAllocator* g_Allocator = nullptr;

#if KTL_USER_MODE
extern volatile LONGLONG gs_AllocsRemaining;
#endif


NTSTATUS
DomFromKStringView(
    __in  KStringView& Src,
    __in  KAllocator& Alloc,
    __out KIMutableDomNode::SPtr& Dom
    )
{
    if (Src.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    KBuffer::SPtr Tmp;
	HRESULT hr;
	ULONG result;
	hr = ULongMult(Src.Length(), 2, &result);
	KInvariant(SUCCEEDED(hr));
    NTSTATUS Res = KBuffer::Create(result, Tmp, Alloc);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KStringView Dest(PWSTR(Tmp->GetBuffer()), Src.Length());
    Dest.CopyFrom(Src);

    Res = KDom::CreateFromBuffer(Tmp, Alloc, KTL_TAG_DOM, Dom);
    return Res;
}

class MyTask : public KITask
{

public:
    MyTask(KITaskManager::SPtr& Mgr)
    {
        _Mgr = Mgr;
    }

    // ExecuteAsync
    //
    // Models the execution of a single task instance.
    //
    virtual NTSTATUS
    ExecAsync(
        __in     KIDomNode::SPtr& InParameters,
        __out    KIDomNode::SPtr& Result,
        __out    KIDomNode::SPtr& ErrorDetail,
        __in_opt KAsyncContextBase::CompletionCallback Callback,
        __in_opt KAsyncContextBase* const ParentContext = nullptr
        )
    {
        UNREFERENCED_PARAMETER(InParameters);
        UNREFERENCED_PARAMETER(Result);
        UNREFERENCED_PARAMETER(ErrorDetail);

        Start(ParentContext, Callback);
        return STATUS_PENDING;
    }


    VOID OnStart()
    {
        KIMutableDomNode::SPtr Tmp;

        KIDb::SPtr Db;
        NTSTATUS Res;

        Res = _Mgr->GetTaskDb(Db);
        if (!NT_SUCCESS(Res))
        {
            Complete(Res);
            return;
        }

        KUri::SPtr TableUri;
        Res = KUri::Create(KStringView(L"table://mytable"), *g_Allocator, TableUri);
        if (!NT_SUCCESS(Res))
        {
            Complete(Res);
            return;
        }

        KITable::SPtr TestTable;
        Res = Db->GetTable(*TableUri, TestTable);
        if (!NT_SUCCESS(Res))
        {
            Complete(Res);
            return;
        }

        KIMutableDomNode::SPtr TestDom;
        Res = TestTable->Get(KStringView(L""), TestDom);
        if (!NT_SUCCESS(Res))
        {
            Complete(Res);
            return;
        }

        KVariant v;
        TestDom->GetChildValue(KIDomNode::QName(L"b"), v);

        KVariant v2;

        v2 = KVariant::Create(KStringView(L"aa333"), *g_Allocator);
        TestDom->SetChildValue(KIDomNode::QName(L"b"), v2);

        Res = TestTable->Update(KStringView(L""), TestDom, 0);

        Complete(STATUS_SUCCESS);
    }

    virtual VOID
    Cancel()
    {

    }

    KITaskManager::SPtr _Mgr;

};

class MyTaskFactory : public KITaskFactory
{
    K_DENY_COPY(MyTaskFactory);

public:
    static KITaskFactory::SPtr
    Create(
        __in KAllocator& Alloc
        )
    {
        return _new(KTL_TAG_TASK, Alloc) MyTaskFactory(Alloc);
    }

    MyTaskFactory(
        __in KAllocator& Alloc
        )
        : _Allocator(Alloc)
    {
    }

    virtual NTSTATUS
    NewTask(
        __in   KUri&         TaskTypeId,
        __in   KSharedPtr<KITaskManager> Manager,
        __out  KITask::SPtr& NewTask
        )
    {
        UNREFERENCED_PARAMETER(TaskTypeId);

        MyTask* Tmp = _new(KTL_TAG_TASK, _Allocator) MyTask(Manager);
        if (!Tmp)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        NewTask = Tmp;
        return STATUS_SUCCESS;
    }

private:
    KAllocator& _Allocator;

};


NTSTATUS
MakeFactory(
    __in KITaskManager::SPtr TaskMan
    )
{
    NTSTATUS Res;
    KITaskFactory::SPtr Fact = MyTaskFactory::Create(*g_Allocator);

    if (!Fact)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KUri::SPtr TaskUri;
    Res = KUri::Create(KStringView(L"rvd://task/test"), *g_Allocator, TaskUri);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }
    return TaskMan->RegisterTaskFactory(*TaskUri, KITaskManager::eSerialExecution, Fact);
}


NTSTATUS
NewTask(
    __in KITaskManager::SPtr TaskMan,
    __out KITask::SPtr& NewTask
    )
{
    NTSTATUS Res;

    KUri::SPtr TaskUri;

    Res = KUri::Create(KStringView(L"rvd://task/test"), *g_Allocator, TaskUri);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    return TaskMan->NewTask(*TaskUri, NewTask);
}

KEvent* pEvent = nullptr;

VOID
CompletionCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(Op);

    pEvent->SetEvent();
}

NTSTATUS
SetupDb(
    __in KITaskManager::SPtr TaskMan
    )
{
    KIDb::SPtr Db;
    NTSTATUS Res;

    Res = TaskMan->GetTaskDb(Db);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KUri::SPtr TableUri;
    Res = KUri::Create(KStringView(L"table://mytable"), *g_Allocator, TableUri);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KITable::SPtr TestTable;
    Res = Db->CreateTable(*TableUri, KITable::eSingleton, TestTable);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KIMutableDomNode::SPtr TestDom;
    KStringView DomData(L"\xFEFF<a><b>x100</b><c>y200</c></a>");
    Res = DomFromKStringView(DomData, *g_Allocator, TestDom);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = TestTable->Insert(KStringView(L""), TestDom);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    return Res;
}

NTSTATUS
TestDomChanges(
    __in KITaskManager::SPtr TaskMan
    )
{
    KIDb::SPtr Db;
    NTSTATUS Res;

    Res = TaskMan->GetTaskDb(Db);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KUri::SPtr TableUri;
    Res = KUri::Create(KStringView(L"table://mytable"), *g_Allocator, TableUri);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KITable::SPtr TestTable;
    Res = Db->GetTable(*TableUri, TestTable);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KIMutableDomNode::SPtr TestDom;
    Res = TestTable->Get(KStringView(L""), TestDom);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KVariant v;
    TestDom->GetChildValue(KIDomNode::QName(L"b"), v);


    return Res;
}

NTSTATUS
TestSequence()
{
    NTSTATUS Status;

    KITaskManager::SPtr TaskMan;

    Status = KLocalTaskManager::Create(*g_Allocator, TaskMan);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = SetupDb(TaskMan);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }


    Status = MakeFactory(TaskMan);

    KITask::SPtr Task;

    Status = NewTask(TaskMan, Task);

    // Now execute it
    KAutoResetEvent Event;
    pEvent = &Event;

    KIDomNode::SPtr InParams;
    KIDomNode::SPtr OutParams;
    KIDomNode::SPtr ErrorDetail;

    Task->ExecAsync(
        InParams,
        OutParams,
        ErrorDetail,
        CompletionCallback
        );

    Event.WaitUntilSet();

    KNt::Sleep(500);

    // Get the DOM back out and test it

    Status = TestDomChanges(TaskMan);

    return Status;
}




NTSTATUS
KTaskTest(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
	
    KTestPrintf("KTaskTest: STARTED\n");

    NTSTATUS Result;
    KtlSystem* system;

    Result = KtlSystem::Initialize(&system);
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KtlSystem::Initializer() failure\n");
        return Result;
    }
    KFinally([&](){ KtlSystem::Shutdown(); });
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    system->SetStrictAllocationChecks(TRUE);

    EventRegisterMicrosoft_Windows_KTL();

    if (NT_SUCCESS(Result))
    {
       Result = TestSequence();
    }

    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KTaskTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
	
    return Result;
}



#if CONSOLE_TEST
int
#if defined(PLATFORM_UNIX)
main(int argc, char* args[])
#else
wmain(int argc, WCHAR* args[])
#endif
{
#if defined(PLATFORM_UNIX)
    return KTaskTest(0, NULL);
#else
    if (argc > 0)
    {
        argc--;
        args++;
    }

    return KTaskTest(argc, args);
#endif
}
#endif


