/*++

Copyright (c) Microsoft Corporation

Module Name:

    KNetChannelTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KNetChannelTests.h.
    2. Add an entry to the gs_KuTestCases table in KNetChannelTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KNetChannelTests.h"
#include <CommandLineParser.h>

NTSTATUS
NetServiceLayerStartup(
    __in KUriView BaseUri,
    __in KAllocator& Allocator
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KNetServiceLayerStartup::SPtr startup;

    status = KNetServiceLayer::CreateStartup(Allocator, startup);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KSynchronizer syncObject;

    KNetworkEndpoint::SPtr ep;
    status = KRvdTcpNetworkEndpoint::Create(BaseUri, Allocator, ep);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = startup->StartNetwork(ep, syncObject);
    if (!K_ASYNC_SUCCESS(status))
    {
        return status;
    }

    status = syncObject.WaitForCompletion();
    return status;
}

VOID
NetServiceLayerShutdown()
{
    KNetServiceLayer::Shutdown();
}

//
// A simple synchronization object to be used with KAsyncContextBase.
// It allows an user to fire an async operation then wait for it to complete.
//

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

    //
    // Use the returned KDelegate as the async completion callback.
    // Then wait for completion using WaitForCompletion().
    //

    operator KAsyncContextBase::CompletionCallback()
    {
        return _Callback;
    }    
    
    NTSTATUS
    WaitForCompletion()
    {        
        _Event.WaitUntilSet();
        return _CompletionStatus;
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
        _Event.SetEvent();
    }    
    
    KEvent _Event;
    NTSTATUS _CompletionStatus;
    KAsyncContextBase::CompletionCallback _Callback;
};

NTSTATUS
TestWrapper(
    __in PCWSTR TestCaseName,
    __in KU_TEST_CASE TestRoutine,
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
    )

/*++

Routine Description:

    This routines is a wrapper around the real test routine. It has all the
    common infrasture.

Arguments:

    TestCaseName - Supplies the test case name.

    TestRoutine - Supplies the real test case function pointer. It may be
        different from the one registered in gs_KuTestCases[].

    argc - Supplies the number of parameters in args.

    args - Supplies an array of parameters to be passed to the test case
        routine.

Return Value:

    NTSTATUS

--*/

{
    KTestPrintf("************************\n\n");

    const LONGLONG startingAllocCount = KAllocatorSupport::gs_AllocsRemaining;

    KTestPrintf("Starting %S: Number Of allocations = %I64d\n\n",
        TestCaseName, KAllocatorSupport::gs_AllocsRemaining);

    KTestPrintf("************************\n\n");

    KtlSystem* underlyingSystem;
    NTSTATUS status = KtlSystem::Initialize(&underlyingSystem);
    KAssert(NT_SUCCESS(status));

    //
    // Turn on strict allocation tracking
    //
    
    underlyingSystem->SetStrictAllocationChecks(TRUE);    

    EventRegisterMicrosoft_Windows_KTL();

    //
    // By default, disable debug trace mirroring.
    //

    KDbgMirrorToDebugger = FALSE;

    //
    // Run the real test routine.
    // All resources allocated inside the test routine should be released when
    // the test routine returns. Many objects are KSharedPtr<>. They will be
    // destroyed when going out of the test routine scope.
    //

    status = TestRoutine(argc, args);

    //
    // BUGBUG
    // Put an artificial delay here to wait for memory pools to drain.
    //

    for (ULONG i = 0; i < 5; i++)
    {
        if (startingAllocCount == KAllocatorSupport::gs_AllocsRemaining)
        {
            break;
        }
        KNt::Sleep(1000);        
    }

    KDbgMirrorToDebugger = TRUE;    

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();    

    KTestPrintf("************************\n\n");

    KTestPrintf("Completing %S: Status = %x, Number Of allocations = %I64d\n\n",
        TestCaseName, status, KAllocatorSupport::gs_AllocsRemaining);

    KTestPrintf("************************\n\n");

    return status;
}

//
// This macro is the generic version of K_CONSTRUCT() for collection service messages.
// The user context is assumed to be a KAllocator pointer.
//

#define GENERIC_CONSTRUCT(_MessageClass, AllocationTag) \
    K_CONSTRUCT(_MessageClass) \
    { \
        KAllocator* allocator = (KAllocator*)In.UserContext; \
       _This = _new(AllocationTag, *allocator) _MessageClass(); \
       NTSTATUS status = (_This == nullptr) ? STATUS_INSUFFICIENT_RESOURCES : _This->Status(); \
       return status;   \
    }

class TestReq1 : public KObject<TestReq1>, public KShared<TestReq1>
{
    K_SERIALIZABLE(TestReq1);    

public:

    typedef KSharedPtr<TestReq1> SPtr;

    TestReq1() {}
    ~TestReq1() {}

    TestReq1(
        __in ULONGLONG Arg1,
        __in ULONGLONG Arg2
        )
    {
        _Arg1 = Arg1;
        _Arg2 = Arg2;
    }    

    ULONGLONG _Arg1;
    ULONGLONG _Arg2;

    static const GUID MessageTypeId;
};

// {AA952264-A72F-4FCB-B12B-E2DBF9E4DF94}
const GUID TestReq1::MessageTypeId = 
    { 0xaa952264, 0xa72f, 0x4fcb, { 0xb1, 0x2b, 0xe2, 0xdb, 0xf9, 0xe4, 0xdf, 0x94 } };

K_SERIALIZE_MESSAGE(TestReq1, TestReq1::MessageTypeId)
{
    K_EMIT_MESSAGE_GUID(TestReq1::MessageTypeId);

    Out << This._Arg1;
    Out << This._Arg2;    

    return Out;    
}

K_DESERIALIZE_MESSAGE(TestReq1, TestReq1::MessageTypeId)
{
    K_VERIFY_MESSAGE_GUID(TestReq1::MessageTypeId);

    In >> This._Arg1;
    In >> This._Arg2;

    return In;
}    

GENERIC_CONSTRUCT(TestReq1, KTL_TAG_TEST);

class TestReply1 : public KObject<TestReply1>, public KShared<TestReply1>
{
    K_SERIALIZABLE(TestReply1);

public:

    typedef KSharedPtr<TestReply1> SPtr;    

    TestReply1() : _Text(GetThisAllocator()) {}
    ~TestReply1() {}    

    //
    // This simple service entry adds two numbers together.
    //

    TestReply1(
        __in TestReq1& Request
        ) : _Text(GetThisAllocator())
    {
        _Sum = Request._Arg1 + Request._Arg2;
        TestReply1::ValueToString(Request._Arg1, Request._Arg2, _Text);
    }

    //
    // This method verifies the reply is correct for the request.
    //

    VOID
    Verify(
        __in TestReq1& Request
        )
    {
        KFatal(_Sum == Request._Arg1 + Request._Arg2);
        
        KWString expectedString(GetThisAllocator());
        ValueToString(Request._Arg1, Request._Arg2, expectedString);
        KFatal(expectedString.CompareTo(_Text) == 0);
    }

    static
    VOID 
    ValueToString(
        __in ULONGLONG Arg1,
        __in ULONGLONG Arg2,
        __out KWString& Text
        )
    {
        WCHAR buffer[] = L"0x12345678 + 0x12345678 = 0x12345678";
#if KTL_USER_MODE
        StringCchPrintfW(
#else
        RtlStringCchPrintfW(
#endif        
            buffer, ARRAYSIZE(buffer),
            L"0x%08x + 0x%08x = 0x%08x",
            Arg1, Arg2, Arg1 + Arg2
            );
        Text = buffer;
    }

    KWString _Text;
    ULONGLONG _Sum;

    static const GUID MessageTypeId;
};

// {7E72FDB4-A523-4483-9C4E-85BCAB282DF6}
const GUID TestReply1::MessageTypeId = 
    { 0x7e72fdb4, 0xa523, 0x4483, { 0x9c, 0x4e, 0x85, 0xbc, 0xab, 0x28, 0x2d, 0xf6 } };

K_SERIALIZE_MESSAGE(TestReply1, TestReply1::MessageTypeId)
{
    K_EMIT_MESSAGE_GUID(TestReply1::MessageTypeId);
    
    Out << This._Text;
    Out << This._Sum;    

    return Out;    
}

K_DESERIALIZE_MESSAGE(TestReply1, TestReply1::MessageTypeId)
{
    K_VERIFY_MESSAGE_GUID(TestReply1::MessageTypeId);

    In >> This._Text;
    In >> This._Sum;

    return In;
}    

GENERIC_CONSTRUCT(TestReply1, KTL_TAG_TEST);

class TestReq2 : public KObject<TestReq2>, public KShared<TestReq2>
{
    K_SERIALIZABLE(TestReq2);

public:

    typedef KSharedPtr<TestReq2> SPtr;    

    TestReq2() {}
    ~TestReq2() {}

    TestReq2(
        __in KGuid& Arg1,
        __in ULONGLONG Arg2
        )
    {
        _Arg1 = Arg1;
        _Arg2 = Arg2;
    }    

    KGuid _Arg1;
    ULONGLONG _Arg2;

    static const GUID MessageTypeId;
};

// {8427F1DB-6D7E-499B-ABFA-BA101DEDF906}
const GUID TestReq2::MessageTypeId = 
    { 0x8427f1db, 0x6d7e, 0x499b, { 0xab, 0xfa, 0xba, 0x10, 0x1d, 0xed, 0xf9, 0x6 } };   

K_SERIALIZE_MESSAGE(TestReq2, TestReq2::MessageTypeId)
{
    K_EMIT_MESSAGE_GUID(TestReq2::MessageTypeId);

    Out << This._Arg1;
    Out << This._Arg2;    

    return Out;    
}

K_DESERIALIZE_MESSAGE(TestReq2, TestReq2::MessageTypeId)
{
    K_VERIFY_MESSAGE_GUID(TestReq2::MessageTypeId);

    In >> This._Arg1;
    In >> This._Arg2;

    return In;
}      

GENERIC_CONSTRUCT(TestReq2, KTL_TAG_TEST);

class TestService : public KObject<TestService>, public KShared<TestService>
{
public:

    typedef KSharedPtr<TestService> SPtr;

    TestService() {}
    ~TestService() {}

    class RequestHandlerContext : public KAsyncContextBase
    {        
    public:

        RequestHandlerContext()
        {
            NTSTATUS status = KTimer::Create(_Timer, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TIMER);
            KFatal(NT_SUCCESS(status));
        }

        ~RequestHandlerContext()
        {
        }
        
        VOID
        StartRequest(
            __in KNetChannelRequestInstance::SPtr RequestInstance,        
            __in KAsyncContextBase* const Parent, 
            __in KAsyncContextBase::CompletionCallback Callback        
            )
        {
            _RequestInstance = Ktl::Move(RequestInstance);
            Start(Parent, Callback);
        }
        
    protected:

        VOID OnStart()
        {
            //
            // Start a timer to simulate the request processing time.
            //
            
            ULONG delayInMs = (ULONG)(KNt::GetPerformanceTime() % 3000);
            _Timer->StartTimer(
                delayInMs,
                this,   // ParentAsync
                KAsyncContextBase::CompletionCallback(this, &TestService::RequestHandlerContext::TimerCallback)
                ); 

            //
            // There is nothing else to do other than the timer callback.
            //
            Complete(STATUS_SUCCESS);            
        }

        VOID
        TimerCallback(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingOperation
            )
        {
            UNREFERENCED_PARAMETER(Parent);
            
            //NTSTATUS status = 
                CompletingOperation.Status();
            CompletingOperation.Reuse();

            OnTimerCallback();
        }

        virtual VOID OnTimerCallback() = 0;        
        
        KTimer::SPtr _Timer;
        KNetChannelRequestInstance::SPtr _RequestInstance;
    };

    class Req1HandlerContext : public RequestHandlerContext
    {
        
    public:
        
        Req1HandlerContext() {}
        ~Req1HandlerContext() {}
        
        VOID OnTimerCallback()
        {            
            TestReq1::SPtr request;
            NTSTATUS status = _RequestInstance->AcquireRequest(request);
            KFatal(NT_SUCCESS(status));

            if (KNt::GetPerformanceTime() % 20 != 0)
            {            
                TestReply1::SPtr reply = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) TestReply1(*request);
                _RequestInstance->CompleteRequest(reply);            
            }
            else
            {
                //
                // Once in a while, fake an error on the server side.
                //
                _RequestInstance->FailRequest(STATUS_ACCESS_DENIED);
            }
        }
    };
    
    VOID
    TestReq1Handler(
        __in KNetChannelRequestInstance::SPtr NewRequest    
        )
    {        
        KSharedPtr<Req1HandlerContext> context = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) 
            Req1HandlerContext();

        context->StartRequest(
            Ktl::Move(NewRequest),
            NewRequest.RawPtr(),    // ParentAsync
            nullptr                 // Callback
            );
    }

    static 
    NTSTATUS
    ComputeTestReq2Reply(
        __in TestReq2& Request
        )
    {
        ULONGLONG value = *((PULONGLONG)&Request._Arg1) ^ Request._Arg2;
        if (value % 2 == 0)
        {
            return STATUS_SUCCESS;
        }
        else
        {
            return STATUS_NOT_FOUND;
        }
    }    

    class Req2HandlerContext : public RequestHandlerContext
    {

    public:
        
        Req2HandlerContext() {}
        ~Req2HandlerContext() {}
        
        VOID OnTimerCallback()
        {
            TestReq2::SPtr request;
            NTSTATUS status = _RequestInstance->AcquireRequest(request);
            KFatal(NT_SUCCESS(status));
            
            //
            // This service returns an NTSTATUS.
            //
                        
            NTSTATUS returnStatus = ComputeTestReq2Reply(*request);
            _RequestInstance->CompleteRequest(returnStatus);       
        }
    };    

    VOID
    TestReq2Handler(
        __in KNetChannelRequestInstance::SPtr NewRequest
        )
    {
        KSharedPtr<Req2HandlerContext> context = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) 
            Req2HandlerContext();

        context->StartRequest(
            Ktl::Move(NewRequest),
            NewRequest.RawPtr(),    // ParentAsync
            nullptr                 // Callback
            );
    }            
};

struct ClientTestContext 
{
    KNetChannelClient::SPtr _Client;    
    KNetChannelClient::RequestContext::SPtr _NetChannelContext;

    ULONG _RequestType;
    
    TestReq1::SPtr _RequestType1;
    TestReq2::SPtr _RequestType2;

    LONG _ThisInstanceId;

    static LONG _NextInstanceId;
    static LONG _TotalNumberOfRequests;
    static LONG _NumberOfStartedRequests;    
    static LONG _NumberOfCompletedRequests;    
    static LONG _NumberOfTimeOut;
    static LONG _NumberOfCancel;    
    static LONG _NumberOfNoServerEntry;
    static LONG _NumberOfAccessDenied;    
    static KEvent* _CompletionEvent;    

    ClientTestContext()
    {
        _ThisInstanceId = InterlockedIncrement(&_NextInstanceId);
        
        _RequestType = ~0ui32;

        _RequestType1 = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) TestReq1();    
        KFatal(_RequestType1);
        
        _RequestType2 = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) TestReq2();                    
        KFatal(_RequestType2); 
    }

    VOID
    StartNewRequest()
    {
        NTSTATUS status = STATUS_SUCCESS;
        KAsyncContextBase::CompletionCallback callback(this, &ClientTestContext::CallServerCallback);
        
        LONG n = InterlockedIncrement(&_NumberOfStartedRequests);

        //
        // Deliberately start more requests than we need.
        // When _TotalNumberOfRequests requests complete, the test will shutdown the client object
        // while the additional requests may still be pending. 
        //
        if (n <= _TotalNumberOfRequests + 50)
        {
            //
            // Still need to fire more requests. Randomly pick one.
            //
            
            _RequestType = KNt::GetPerformanceTime() % 3;
            switch (_RequestType)
            {
                case 0:
                    _RequestType1->_Arg1 = (ULONGLONG)KNt::GetPerformanceTime();

                    //
                    // Bump up _Arg2 by a prime number in case GetPerformanceTime() returns the same value.
                    //
                    _RequestType1->_Arg2 = (ULONGLONG)KNt::GetPerformanceTime() + 17;   

                    status = _Client->CallServer<TestReq1, TestReply1>(
                        _RequestType1, 
                        callback, 
                        nullptr,                        // ParentAsync
                        _NetChannelContext.RawPtr(),    // ChildAsync
                        0,                              // ActId
                        1000                            // RequestTimeoutInMs
                        );
                    KFatal(NT_SUCCESS(status) || status == STATUS_FILE_CLOSED);

                    break;

                case 1:
                    _RequestType2->_Arg1.CreateNew();
                    _RequestType2->_Arg2 = (ULONGLONG)KNt::GetPerformanceTime();

                    status = _Client->CallServerWithReturnStatus<TestReq2>(
                        _RequestType2, 
                        callback,                         
                        nullptr,                        // ParentAsync
                        _NetChannelContext.RawPtr(),    // ChildAsync
                        0,                              // ActId
                        1000                            // RequestTimeoutInMs
                        );
                    KFatal(NT_SUCCESS(status) || status == STATUS_FILE_CLOSED);

                    break;

                case 2:

                    //
                    // This is not a supported entry on the server.
                    // It should fail with RPC_NT_ENTRY_NOT_FOUND.
                    // We also set the timeout to infinite. So it must either be correctly cancelled
                    // or failed by a RPC_NT_ENTRY_NOT_FOUND. Otherwise it will stop responding.
                    //

                    _RequestType1->_Arg1 = (ULONGLONG)KNt::GetPerformanceTime();
                    
                    //
                    // Bump up _Arg2 by a prime number in case GetPerformanceTime() returns the same value.
                    //
                    _RequestType1->_Arg2 = (ULONGLONG)KNt::GetPerformanceTime() + 17;   
                    
                    status = _Client->CallServerWithReturnStatus<TestReq1>(
                        _RequestType1, 
                        callback, 
                        nullptr,                        // ParentAsync
                        _NetChannelContext.RawPtr(),    // ChildAsync
                        0,                              // ActId
                        ~0ui32                          // RequestTimeoutInMs
                        );
                    KFatal(NT_SUCCESS(status) || status == STATUS_FILE_CLOSED);
                    
                    break;                    

                default:
                    KFatal(FALSE);
            }
        }        
    }

    VOID
    CallServerCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp
        )
    {
        UNREFERENCED_PARAMETER(Parent);

        KFatal(&CompletingSubOp == static_cast<KAsyncContextBase*>(_NetChannelContext.RawPtr()));

        //
        // First, extract the reply in the async context and verify its correctness.
        //
        
        NTSTATUS status = CompletingSubOp.Status();
        if (status != STATUS_SUCCESS)
        {
            switch (status)
            {
                case STATUS_IO_TIMEOUT:
                    InterlockedIncrement(&_NumberOfTimeOut); 
                    break;

                case STATUS_CANCELLED:
                    InterlockedIncrement(&_NumberOfCancel);                    
                    break;

                case RPC_NT_ENTRY_NOT_FOUND:
                    InterlockedIncrement(&_NumberOfNoServerEntry);                    
                    break;

                case STATUS_ACCESS_DENIED:
                    InterlockedIncrement(&_NumberOfAccessDenied);
                    break;

                default:
                    KFatal(FALSE);
            }            
        }

        TestReply1::SPtr replyType1;
        NTSTATUS completionStatus;
        
        switch (_RequestType)
        {
            case 0:
                status = _NetChannelContext->AcquireReply(replyType1);
                if (NT_SUCCESS(status))
                {
                    replyType1->Verify(*_RequestType1);
                }
                else
                {
                    KFatal(status == STATUS_NOT_FOUND);
                }
                break;
        
            case 1:
                status = _NetChannelContext->AcquireReply(completionStatus);
                if (NT_SUCCESS(status))
                {
                    KFatal(completionStatus == TestService::ComputeTestReq2Reply(*_RequestType2));
                }
                else
                {
                    KFatal(status == STATUS_NOT_FOUND);                    
                }
                
                break;

            case 2:
                KFatal(status == RPC_NT_ENTRY_NOT_FOUND || status == STATUS_CANCELLED);
                break;
        
            default:
                KFatal(FALSE);
        }
        
        //
        // Reuse the async context AFTER we extract the reply.
        //
        
        CompletingSubOp.Reuse();

        LONG n = InterlockedIncrement(&_NumberOfCompletedRequests);
        if (n % 50 == 0)
        {
            KTestPrintf("Completed %d requests\n", n);
        }
        
        if (n == _TotalNumberOfRequests)
        {
            _CompletionEvent->SetEvent();
            return;
        }

        if (n < _TotalNumberOfRequests)
        {
            StartNewRequest();
        }
    }    
};

LONG ClientTestContext::_NextInstanceId = 0;
LONG ClientTestContext::_TotalNumberOfRequests = 0;
LONG ClientTestContext::_NumberOfStartedRequests = 0;    
LONG ClientTestContext::_NumberOfCompletedRequests = 0;      
LONG ClientTestContext::_NumberOfTimeOut = 0;      
LONG ClientTestContext::_NumberOfCancel = 0;      
LONG ClientTestContext::_NumberOfNoServerEntry= 0;      
LONG ClientTestContext::_NumberOfAccessDenied = 0; 


KEvent* ClientTestContext::_CompletionEvent = nullptr;

NTSTATUS
KNetChannelTestInternal(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;
    Synchronizer syncObject;

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
	
    status = NetServiceLayerStartup(KUriView(L"rvd://(local):1"), KtlSystem::GlobalNonPagedAllocator());
    KFatal(NT_SUCCESS(status));    

    KFinally([](){ NetServiceLayerShutdown(); });    

    KNetChannelServer::SPtr server;
    status = KNetChannelServer::Create(KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_NET_CHANNEL, server);
    KFatal(NT_SUCCESS(status));

    KGuid serverEp;
    serverEp.CreateNew();

    KWString serverUri(KtlSystem::GlobalNonPagedAllocator(), L"rvd://(local)/server");

    Synchronizer serverDeactivation;
    status = server->Activate(serverUri, serverEp, serverDeactivation);
    KFatal(K_ASYNC_SUCCESS(status));    

    TestService::SPtr service = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) TestService();

    //
    // The service can handle two types of requests. Register them here.
    //

    KNetChannelReceiverType receiver;

    receiver.Bind(service.RawPtr(), &TestService::TestReq1Handler);
    status = server->RegisterReceiver<TestReq1, TestReply1>(receiver, &KtlSystem::GlobalNonPagedAllocator());
    KFatal(NT_SUCCESS(status));    

    receiver.Bind(service.RawPtr(), &TestService::TestReq2Handler);
    status = server->RegisterReceiver<TestReq2, KNetChannelNtStatusMessage>(receiver, &KtlSystem::GlobalNonPagedAllocator());
    KFatal(NT_SUCCESS(status));        

    //
    // Create a client object.
    //

    KNetChannelClient::SPtr client;
    status = KNetChannelClient::Create(KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_NET_CHANNEL, client);
    KFatal(NT_SUCCESS(status));

    KGuid clientEp;
    clientEp.CreateNew();

    KWString clientUri(KtlSystem::GlobalNonPagedAllocator(), L"rvd://(local)/client");

    Synchronizer clientDeactivation;
    status = client->Activate(clientUri, clientEp, serverUri, serverEp, clientDeactivation);
    KFatal(K_ASYNC_SUCCESS(status));

    //
    // Register a reply type. This tells the client object how to deserialize the reply messages.
    //

    status = client->RegisterServerReplyType<TestReply1>(&KtlSystem::GlobalNonPagedAllocator());
    KFatal(NT_SUCCESS(status));    

    //
    // Let the server nodes know about this client. This is required for the
    // current networking scheme before we have a namespace.
    //

    status = server->AddKnownClient(clientEp, clientUri);
    KFatal(NT_SUCCESS(status));

    //
    // Send one request to the service.
    //

    KNetChannelClient::RequestContext::SPtr requestContext;
    status = client->AllocateRequestContext(requestContext);
    KFatal(NT_SUCCESS(status));    

    TestReq1::SPtr req1 = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) TestReq1(1, 2);    
    
    client->CallServer<TestReq1, TestReply1>(req1, syncObject, nullptr, requestContext.RawPtr(), 0, (ULONG)(-1));
    status = syncObject.WaitForCompletion();
    KFatal(NT_SUCCESS(status) || status == STATUS_ACCESS_DENIED);    

    if (NT_SUCCESS(status))
    {
        TestReply1::SPtr reply1;
        status = requestContext->AcquireReply(reply1);
        KFatal(NT_SUCCESS(status));
        
        reply1->Verify(*req1);        
    }

    //
    // Run parallel tests.
    //

    KEvent testDoneEvent;

    ClientTestContext::_TotalNumberOfRequests = 500;
    ClientTestContext::_NumberOfStartedRequests = 0;    
    ClientTestContext::_NumberOfCompletedRequests = 0;       
    ClientTestContext::_NumberOfTimeOut = 0;      
    ClientTestContext::_NumberOfCancel = 0;    
    ClientTestContext::_NumberOfNoServerEntry = 0;            
    ClientTestContext::_NumberOfAccessDenied = 0;                
    
    ClientTestContext::_CompletionEvent = &testDoneEvent;

    KUniquePtr<ClientTestContext, ArrayDeleter<ClientTestContext>> contextArray;
    contextArray = _newArray<ClientTestContext>(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), 32);
    for (ULONG i = 0; i < 32; i++)
    {
        ClientTestContext& context = contextArray[i];
        context._Client = client;
        
        status = client->AllocateRequestContext(context._NetChannelContext);
        KFatal(NT_SUCCESS(status));

        context.StartNewRequest();
    }

    //
    // All done. Shut down the server and the client. Wait for them to complete.
    //

    testDoneEvent.WaitUntilSet();

    server->Deactivate();
    client->Deactivate();   

    status = serverDeactivation.WaitForCompletion();
    KFatal(NT_SUCCESS(status));        

    status = clientDeactivation.WaitForCompletion();
    KFatal(NT_SUCCESS(status)); 

    KTestPrintf("NumberOfCompletion = %d\n", ClientTestContext::_NumberOfCompletedRequests);
    
    KTestPrintf("NumberOfTimeOut = %d; NumberOfCancel = %d, NumberOfNoServerEntry = %d, NumberOfAccessDenied = %d\n",
        ClientTestContext::_NumberOfTimeOut,
        ClientTestContext::_NumberOfCancel,
        ClientTestContext::_NumberOfNoServerEntry,
        ClientTestContext::_NumberOfAccessDenied
        );

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
	
    return status;
}

NTSTATUS
KNetChannelTest(
    int argc, WCHAR* args[]
    )
{
    return TestWrapper(L"KNetChannelTest", KNetChannelTestInternal, argc, args);
}

#if CONSOLE_TEST
int
main(int argc, WCHAR* args[])
{
    NTSTATUS status = KNetChannelTest(argc, args);
    return status;
}
#endif
