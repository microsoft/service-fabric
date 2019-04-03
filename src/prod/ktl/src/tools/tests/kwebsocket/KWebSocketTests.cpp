/*++

Copyright (c) Microsoft Corporation

Module Name:

    KWebSocketTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KWebSocketTests.h.
    2. Add an entry to the gs_KuTestCases table in KWebSocketTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <ktl.h>
#include <ktrace.h>
#include "KWebSocketTests.h"
#include <CommandLineParser.h>


namespace KWebSocketTests
{
    class MockKWebSocket : public KWebSocket
    {
    public:

        static NTSTATUS
        Create(
            __out MockKWebSocket::SPtr& WebSocket,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_TEST
            )
        {
            NTSTATUS status;
            MockKWebSocket::SPtr webSocket;

            WebSocket = nullptr;

            KAsyncQueue<WebSocketReceiveOperation>::SPtr receiveRequestQueue;
            status = KAsyncQueue<WebSocketReceiveOperation>::Create(
                AllocationTag,
                Allocator,
                FIELD_OFFSET(WebSocketReceiveOperation, _Links),
                receiveRequestQueue
                );

            if (! NT_SUCCESS(status))
            {
                return status;
            }

            KAsyncQueue<WebSocketOperation>::SPtr sendRequestQueue;
            status = KAsyncQueue<WebSocketOperation>::Create(
                AllocationTag,
                Allocator,
                FIELD_OFFSET(WebSocketOperation, _Links),
                sendRequestQueue
                );

            if (! NT_SUCCESS(status))
            {
                return status;
            }

            webSocket = _new(AllocationTag, Allocator) MockKWebSocket(*receiveRequestQueue, *sendRequestQueue);
            if (! webSocket)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory(0, status, NULL, 0, 0);
                return status;
            }

            WebSocket = webSocket;
            return STATUS_SUCCESS;
        }

    VOID
    Abort() override
    {
    }

    NTSTATUS
    CreateReceiveFragmentOperation(
        __out ReceiveFragmentOperation::SPtr& ReceiveOperation
        ) override
    {
        UNREFERENCED_PARAMETER(ReceiveOperation);

        return STATUS_SUCCESS;
    }

    NTSTATUS
    CreateReceiveMessageOperation(
        __out ReceiveMessageOperation::SPtr& ReceiveOperation
        ) override
    {
        UNREFERENCED_PARAMETER(ReceiveOperation);

        return STATUS_SUCCESS;
    }

    NTSTATUS
    CreateSendFragmentOperation(
        __out SendFragmentOperation::SPtr& SendOperation
        ) override
    {
        UNREFERENCED_PARAMETER(SendOperation);

        return STATUS_SUCCESS;
    }

    NTSTATUS
    CreateSendMessageOperation(
        __out SendMessageOperation::SPtr& SendOperation
        ) override
    {
        UNREFERENCED_PARAMETER(SendOperation);

        return STATUS_SUCCESS;
    }

    private:

        MockKWebSocket(
            __in KAsyncQueue<WebSocketReceiveOperation>& receiveRequestQueue,
            __in KAsyncQueue<WebSocketOperation>& sendRequestQueue
            ) :
                KWebSocket(receiveRequestQueue, sendRequestQueue)
        {
        }
    };

    namespace TimingConstantOptionsTest
    {
        NTSTATUS
        Execute()
        {
            NTSTATUS status = STATUS_SUCCESS;

            KWebSocket::SPtr webSocket;
            status = KWebSocketTests::MockKWebSocket::Create(
                webSocket,
                KtlSystem::GlobalNonPagedAllocator());

            if (! NT_SUCCESS(status))
            {
                return status;
            }

            KWebSocket::TimingConstant constants[5] = 
            {
                KWebSocket::TimingConstant::CloseTimeoutMs,
                KWebSocket::TimingConstant::OpenTimeoutMs,
                KWebSocket::TimingConstant::PongKeepalivePeriodMs,
                KWebSocket::TimingConstant::PingQuietChannelPeriodMs,
                KWebSocket::TimingConstant::PongTimeoutMs
            }; 

            KWebSocket::TimingConstant constant;
    
            for (int i = 0; i < 5; i++)
            {
                constant = constants[i];

                //  Make sure that the constant is initialized to the same value as explicitly setting to default
                {
                    KWebSocket::TimingConstantValue val0, val1;

                    val0 = webSocket->GetTimingConstant(constant);

                    status = webSocket->SetTimingConstant(constant, KWebSocket::TimingConstantValue::Default);
                    if (! NT_SUCCESS(status))
                    {
                        return status;
                    }

                    val1 = webSocket->GetTimingConstant(constant);

                    if (val0 != val1)
                    {
                        return STATUS_UNSUCCESSFUL;
                    }
                }

                //  Test setting to an arbitrary legal value
                {
                    status = webSocket->SetTimingConstant(constant, static_cast<KWebSocket::TimingConstantValue>(5L));
                    if (! NT_SUCCESS(status))
                    {
                        return status;
                    }
                    if (webSocket->GetTimingConstant(constant) != static_cast<KWebSocket::TimingConstantValue>(5L))
                    {
                        return STATUS_UNSUCCESSFUL;
                    }
                }

                //  Test setting to None and reading back the value
                {
                    status = webSocket->SetTimingConstant(constant, KWebSocket::TimingConstantValue::None);
                    if (! NT_SUCCESS(status))
                    {
                        return status;
                    }
                    if (webSocket->GetTimingConstant(constant) != KWebSocket::TimingConstantValue::None)
                    {
                        return STATUS_UNSUCCESSFUL;
                    }
                }

                //  Test setting to Invalid and verifying that it fails, and does not change the option value
                {
                    KWebSocket::TimingConstantValue val;
                    val = webSocket->GetTimingConstant(constant);
                    if (val == KWebSocket::TimingConstantValue::Invalid)
                    {
                        return STATUS_UNSUCCESSFUL;
                    }

                    status = webSocket->SetTimingConstant(constant, KWebSocket::TimingConstantValue::Invalid);
                    if (NT_SUCCESS(status))
                    {
                        return STATUS_UNSUCCESSFUL;
                    }
                    if (webSocket->GetTimingConstant(constant) != val)
                    {
                        return STATUS_UNSUCCESSFUL;
                    }
                }
        
                //  Test setting to arbitrary illegal values and verifying that it fails, and does not change the option value
                {
                    KWebSocket::TimingConstantValue val;
                    val = webSocket->GetTimingConstant(constant);
                    if (val == KWebSocket::TimingConstantValue::Invalid)
                    {
                        return STATUS_UNSUCCESSFUL;
                    }

                    status = webSocket->SetTimingConstant(constant, static_cast<KWebSocket::TimingConstantValue>(-5L));
                    if (NT_SUCCESS(status))
                    {
                        return STATUS_UNSUCCESSFUL;
                    }
                    if (webSocket->GetTimingConstant(constant) != val)
                    {
                        return STATUS_UNSUCCESSFUL;
                    }

                    status = webSocket->SetTimingConstant(constant, static_cast<KWebSocket::TimingConstantValue>(LLONG_MAX));
                    if (NT_SUCCESS(status))
                    {
                        return STATUS_UNSUCCESSFUL;
                    }
                    if (webSocket->GetTimingConstant(constant) != val)
                    {
                        return STATUS_UNSUCCESSFUL;
                    }
                }
            }

            return STATUS_SUCCESS;
        }
    };

    namespace WebSocketOperationsTest
    {

        NTSTATUS Execute()
        {
            return STATUS_SUCCESS;
        }
    };

    NTSTATUS
    TestSequence()
    {
        NTSTATUS status;

        status = TimingConstantOptionsTest::Execute();
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        status = WebSocketOperationsTest::Execute();
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        return STATUS_SUCCESS;
    }
};


NTSTATUS
KWebSocketTest(
    int argc,
    WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status;
    KtlSystem* system;

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
	
    status = KtlSystem::Initialize(&system);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("KtlSystem::Initializer() failure\n");
        return status;
    }
    KFinally([&](){ KtlSystem::Shutdown(); });

    system->SetStrictAllocationChecks(TRUE);
    
    ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    EventRegisterMicrosoft_Windows_KTL();

    if (NT_SUCCESS(status))
    {
       status = KWebSocketTests::TestSequence();
    }

    ULONGLONG Leaks = KAllocatorSupport::gs_AllocsRemaining - StartingAllocs;
    if (Leaks)
    {
        KTestPrintf("Leaks = %d\n", Leaks);
        status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }

    EventUnregisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
	
    return status;
}


#if CONSOLE_TEST
int
wmain(
    int argc,
    WCHAR* args[]
    )
{
    if (argc > 0)
    {
        argc--;
        args++;
    }

    return KWebSocketTest(argc, args);
}
#endif


