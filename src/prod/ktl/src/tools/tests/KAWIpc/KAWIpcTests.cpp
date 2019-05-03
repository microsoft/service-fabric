/*++

Copyright (c) Microsoft Corporation

Module Name:

    KAWIpcTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KAWIpcTests.h.
    2. Add an entry to the gs_KuTestCases table in KAWIpcTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#define UNIT_TEST

#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(PLATFORM_UNIX)
#include <vector>
#include <dirent.h>
#include <sys/eventfd.h>
#endif
#include <ktl.h>
#include <ktrace.h>
#include <KTpl.h>
#include <kawipc.h>

#include "KAWIpcTests.h"
#include <CommandLineParser.h>


#if KTL_USER_MODE
# define VERIFY_IS_TRUE(__condition) if (! (__condition)) {  printf("VERIFY_IS_TRUE(%s) failed at %s line %d\n", "_condition", __FILE__, __LINE__); KInvariant(FALSE); };
#else
# define VERIFY_IS_TRUE(__condition) if (! (__condition)) {  KDbgPrintf("VERIFY_IS_TRUE(%s) failed at %s line %d\n", "_condition", __FILE__, __LINE__); KInvariant(FALSE); };
#endif

#if KTL_USER_MODE
#include <ktlevents.um.h>
 extern volatile LONGLONG gs_AllocsRemaining;
#else
#include <ktlevents.km.h>
#endif

KAllocator* g_Allocator = nullptr;

#if defined(PLATFORM_UNIX)
ULONG GetNumberKAWIpcSharedMemorySections()
{
    ULONG count = 0;
    DIR           *d;
    struct dirent *dir;
    d = opendir("/dev/shm");
    if (d)
    {
        while ((dir = readdir(d)) != nullptr)
        {
            if (memcmp(dir->d_name, KAWIpcSharedMemory::_SharedMemoryNamePrefixAnsi, sizeof(KAWIpcSharedMemory::_SharedMemoryNamePrefixAnsi)) == 0)
            {
                count++;
            }
        }

        closedir(d);
    } else {
        VERIFY_IS_TRUE(FALSE);
    }
    
    return(count);
}
#endif

NTSTATUS
KAWIpcSharedMemoryTests()
{
    NTSTATUS status;

#if defined(PLATFORM_UNIX)
    //
    // For Linux, remove any stray shared memory sections that might
    // have been left over due to a crash.
    //
    if (TRUE)
    {
        ULONG count;
        status = SyncAwait(KAWIpcSharedMemory::CleanupFromPreviousCrashes(*g_Allocator, KTL_TAG_TEST));
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        count = GetNumberKAWIpcSharedMemorySections();
        VERIFY_IS_TRUE(count == 0);     
    }
#endif
    
    //
    // Test 1: Verify that shared memory section can be created,
    //         mapped, written to, and close.
    //
    {
        // Create server memory and verify size
        static const ULONG serverMemorySize = 16 * 4096;
        KAWIpcSharedMemory::SPtr serverMemory;

        status = KAWIpcSharedMemory::Create(*g_Allocator,
                                            KTL_TAG_TEST,
                                            serverMemorySize,
                                            serverMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(serverMemory->GetSize() == serverMemorySize);

        // Verify access to the memory
        PUCHAR p;
        p = (PUCHAR)serverMemory->GetBaseAddress();
        VERIFY_IS_TRUE(p != nullptr);

        for (ULONG i = 0; i < serverMemory->GetSize(); i++)
        {
            p[i] = 0;
        }
        
        serverMemory = nullptr;
        // TODO: Verify that the memory was unmapped and there is no
        //       access to it
    }


    //
    // Test 2: Verify that shared memory section can be created,
    //         mapped, written to, and mapped again to another address
    //         and verify that correct data is found
    //
    {
        // Create server memory and verify size
        static const ULONG serverMemorySize = 16 * 4096;
        KAWIpcSharedMemory::SPtr serverMemory;

        status = KAWIpcSharedMemory::Create(*g_Allocator,
                                            KTL_TAG_TEST,
                                            serverMemorySize,
                                            serverMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(serverMemory->GetSize() == serverMemorySize);

        // Verify access to the memory
        PUCHAR pServer;
        pServer = (PUCHAR)serverMemory->GetBaseAddress();
        VERIFY_IS_TRUE(pServer != nullptr);

        for (ULONG i = 0; i < serverMemory->GetSize(); i++)
        {
            pServer[i] = 3;
        }

        // Map memory into client side
        KAWIpcSharedMemory::SPtr clientMemory;
        status = KAWIpcSharedMemory::Create(*g_Allocator,
                                            KTL_TAG_TEST,
                                            serverMemory->GetId(),
                                            0,                       // StartingOffset
                                            serverMemorySize,
                                            0,                       // ClientBase
                                            clientMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(clientMemory->GetSize() == serverMemorySize);
        
        // Verify access to the memory
        PUCHAR pClient;
        pClient = (PUCHAR)clientMemory->GetBaseAddress();
        VERIFY_IS_TRUE(pClient != nullptr);

        for (ULONG i = 0; i < clientMemory->GetSize(); i++)
        {
            if (pClient[i] != pServer[i])
            {
                KTestPrintf("pClient[i] 0x%x != pServer[i] 0x%x at %d", pClient[i], pServer[i], i);
                VERIFY_IS_TRUE(FALSE);
            }
        }
        
        clientMemory = nullptr;
        // TODO: Verify that the memory was unmapped and there is no
        //       access to it
        
        serverMemory = nullptr;
        // TODO: Verify that the memory was unmapped and there is no
        //       access to it
    }
    
    //
    // Test 3: Create one large shared memory section and map it on the
    //         "server" side and write offsets into the shared memory
    //         section. Map parts of the large section into various
    //         "client" sections and then verify that the offsets and
    //         data written are correct. Then the "client" will update
    //         the offsets and data and then the "server" will verify
    //         those.
    //
    {
        // Create server memory and verify size
        static const ULONG serverMemorySectionSize = 16 * 4096;
        static const ULONG serverMemorySectionCount = 4;
        static const ULONG serverMemorySize = serverMemorySectionSize * serverMemorySectionCount;
        static const ULONG clientSectionOffset0 = 0;
        static const ULONG clientSectionOffset1 = serverMemorySectionSize;
        static const ULONG clientSectionOffset2 = 2 * serverMemorySectionSize;
        static const ULONG clientSectionOffset3 = 3 * serverMemorySectionSize;
        KAWIpcSharedMemory::SPtr serverMemory;

        status = KAWIpcSharedMemory::Create(*g_Allocator,
                                            KTL_TAG_TEST,
                                            serverMemorySize,
                                            serverMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(serverMemory->GetSize() == serverMemorySize);

        // Verify access to the memory
        PUCHAR pServer;
        pServer = (PUCHAR)serverMemory->GetBaseAddress();
        VERIFY_IS_TRUE(pServer != nullptr);

        // Populate the client2 shared memory with "pointers"
        static const ULONG xOffset = 472 + clientSectionOffset2;
        static const ULONG xSize = 99;      
        PUCHAR xServer = (PUCHAR)(serverMemory->OffsetToPtr(xOffset));
        for (ULONG i = 0; i < xSize; i++)
        {
            xServer[i] = (UCHAR)i;
        }

        // Map client2 and verify the data placed by server
        KAWIpcSharedMemory::SPtr clientMemory;
        status = KAWIpcSharedMemory::Create(*g_Allocator,
                                            KTL_TAG_TEST,
                                            serverMemory->GetId(),
                                            clientSectionOffset2,                       // StartingOffset
                                            serverMemorySectionSize,
                                            clientSectionOffset2,    // ClientBase
                                            clientMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(clientMemory->GetSize() == serverMemorySectionSize);
        
        // Verify access to the memory
        PUCHAR xClient;
        xClient = (PUCHAR)(clientMemory->OffsetToPtr(xOffset));
        for (ULONG i = 0; i < xSize; i++)
        {
            if (xClient[i] != xServer[i])
            {
                KTestPrintf("xClient[i] 0x%x != xServer[i] 0x%x at %d", xClient[i], xServer[i], i);
                VERIFY_IS_TRUE(FALSE);
            }
        }

        // TODO: More sections and offset/pointers

        
        clientMemory = nullptr;
        // TODO: Verify that the memory was unmapped and there is no
        //       access to it
        
        serverMemory = nullptr;
        // TODO: Verify that the memory was unmapped and there is no
        //       access to it

        
    }
#if defined(PLATFORM_UNIX)
    //
    // Test 4: Verify cleanup of stray shared memory sections
    //
    {
        // Create server memory and verify size
        static const ULONG serverMemorySize = 16 * 4096;
        KAWIpcSharedMemory::SPtr serverMemory, serverMemory2;

        status = KAWIpcSharedMemory::Create(*g_Allocator,
                                            KTL_TAG_TEST,
                                            serverMemorySize,
                                            serverMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(serverMemory->GetSize() == serverMemorySize);

        status = KAWIpcSharedMemory::Create(*g_Allocator,
                                            KTL_TAG_TEST,
                                            serverMemorySize,
                                            serverMemory2);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(serverMemory2->GetSize() == serverMemorySize);

        //
        // Change flag to indicate that the sections should not be
        // removed.
        //
        serverMemory->SetOwnSection(FALSE);
        serverMemory2->SetOwnSection(FALSE);
        serverMemory = nullptr;
        serverMemory2 = nullptr;

        //
        // Verify sections exist
        //
        ULONG count;
        count = GetNumberKAWIpcSharedMemorySections();
        VERIFY_IS_TRUE(count == 2);

        //
        // Cleanup and reverify
        //
        status = SyncAwait(KAWIpcSharedMemory::CleanupFromPreviousCrashes(*g_Allocator, KTL_TAG_TEST));
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        count = GetNumberKAWIpcSharedMemorySections();
        VERIFY_IS_TRUE(count == 0);     
    }
#endif  
    return(STATUS_SUCCESS);
}

class TestThread : public KAsyncContextBase, public KThreadPool::WorkItem
{
    K_FORCE_SHARED_WITH_INHERITANCE(TestThread);

    public:
        VOID StartThread(
            __in_opt PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt CompletionCallback CallbackPtr
        )
        {
            _Parameters = Parameters;
            Start(ParentAsyncContext, CallbackPtr);
        }
        
    protected:
        virtual VOID Execute()
        {
            Complete(STATUS_SUCCESS);
        }
                
        VOID
        OnStart(
            )
        {
            GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
        }

        PVOID GetParameters() { return(_Parameters); };
    private:
        PVOID _Parameters;
};

TestThread::TestThread()
{
}

TestThread::~TestThread()
{
}

class EventTestThread : public TestThread
{
    K_FORCE_SHARED(EventTestThread);
    
    public:
        struct EventTestThreadParameters
        {
            ULONG Counter;
            ULONG Wakeups;
#if ! defined(PLATFORM_UNIX)
            PWCHAR WaitName;
            PWCHAR KickName;
#else
            int WaitEventFd;
            int KickEventFd;
#endif
        };
        
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out EventTestThread::SPtr& Context
        )
        {
            EventTestThread::SPtr context;

            context = _new(AllocationTag, Allocator) EventTestThread();
            if (! context)
            {
                VERIFY_IS_TRUE(FALSE);
            }

            VERIFY_IS_TRUE(NT_SUCCESS(context->Status()));

            Context = Ktl::Move(context);
            return(STATUS_SUCCESS); 
            
        }

        VOID Execute()
        {
            NTSTATUS status;

            //
            // wait and kick are from the perspective of this thread
            //
            struct EventTestThreadParameters* params = (struct EventTestThreadParameters*)GetParameters();
            EVENTHANDLEFD wait;
            EVENTHANDLEFD kick;
            ULONG count = params->Counter;

#if ! defined(PLATFORM_UNIX)
            wait = OpenEvent(EVENT_ALL_ACCESS,
                             FALSE,
                             params->WaitName);
            VERIFY_IS_TRUE(wait != nullptr);

            kick = OpenEvent(EVENT_ALL_ACCESS,
                             FALSE,
                             params->KickName);
            VERIFY_IS_TRUE(kick != nullptr);
#else
            wait = params->WaitEventFd;
            kick = params->KickEventFd;
#endif
            KAWIpcEventWaiter::SPtr waiter;
            KAWIpcEventKicker::SPtr kicker;

            status = KAWIpcEventWaiter::Create(GetThisAllocator(),
                                               GetThisAllocationTag(),
                                               waiter);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            status = KAWIpcEventKicker::Create(GetThisAllocator(),
                                               GetThisAllocationTag(),
                                               kick,
                                               kicker);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            while (count > 0)
            {
                status = SyncAwait(waiter->WaitAsync(wait, nullptr));
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                KNt::Sleep(50);
                
                params->Wakeups++;
                
                kicker->Kick();
                count--;
            }
            
            Complete(STATUS_SUCCESS);
        }
};

EventTestThread::EventTestThread()
{
}

EventTestThread::~EventTestThread()
{
}

class Test5Client : public KAWIpcClient
{
    K_FORCE_SHARED(Test5Client);

    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out Test5Client::SPtr& Context
        );
                               
        
};
   
class Test5ClientServerConnection : public KAWIpcClientServerConnection
{
    K_FORCE_SHARED(Test5ClientServerConnection);

    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out Test5ClientServerConnection::SPtr& Context
        );
                               
        ktl::Awaitable<NTSTATUS> OpenAsync(
            __in KAWIpcClientPipe& ClientPipe,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        ktl::Awaitable<NTSTATUS> CloseAsync();

    private:
        ULONG _Counter;
};

Test5Client::Test5Client()
{
}

Test5Client::~Test5Client()
{
}

NTSTATUS Test5Client::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out Test5Client::SPtr& Context
    )
{
    Test5Client::SPtr context;
    
    context = _new(AllocationTag, Allocator) Test5Client();
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS); 
}
Test5ClientServerConnection::Test5ClientServerConnection()
{
}

Test5ClientServerConnection::~Test5ClientServerConnection()
{
}

NTSTATUS Test5ClientServerConnection::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out Test5ClientServerConnection::SPtr& Context
    )
{
    Test5ClientServerConnection::SPtr context;
    
    context = _new(AllocationTag, Allocator) Test5ClientServerConnection();
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS); 
}

ktl::Awaitable<NTSTATUS> Test5ClientServerConnection::OpenAsync(
    __in KAWIpcClientPipe& ClientPipe,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
)
{
    _Counter = 0;
    co_return co_await KAWIpcClientServerConnection::OpenAsync(ClientPipe, ParentAsync, GlobalContextOverride);
}

ktl::Awaitable<NTSTATUS> Test5ClientServerConnection::CloseAsync()
{
    // TODO: Sync with closing of Test5Client tasks
    co_return co_await KAWIpcClientServerConnection::CloseAsync();
}

#define IsAllowedErrorOnClose(status) ((NT_SUCCESS((status))) || \
                                 ((status) == STATUS_INVALID_HANDLE) || \
                                 ((status) == STATUS_PIPE_DISCONNECTED) || \
                                 ((status) == STATUS_PIPE_BROKEN) || \
                                 ((status) == STATUS_PIPE_EMPTY) || \
                                 ((status) == STATUS_CANCELLED) || \
                                 ((status) == K_STATUS_API_CLOSED))

volatile static LONG x = 0;
Task Test5Client(
    __in Test5Client& Client,
    __in ULONGLONG SocketAddress,
    __in ULONG BufferSize,
    __in ULONG MessageCount,
    __in BOOLEAN AllowErrors,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
    )
{
    NTSTATUS status;
    Test5Client::SPtr client;
    Test5ClientServerConnection::SPtr connection;
    KAWIpcClientPipe::SPtr pipe;
    ULONG bytesSent, bytesReceived;
    KBuffer::SPtr bufferOut, bufferIn;
    PUCHAR p;

    LONG xx = InterlockedIncrement(&x);
    
    KFinally([&](){  Acs.SetResult(STATUS_SUCCESS); });
    client = &Client;

    status = KBuffer::Create(BufferSize, bufferOut, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = KBuffer::Create(BufferSize, bufferIn, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = Test5ClientServerConnection::Create(*g_Allocator, KTL_TAG_TEST, connection);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    ULONG retries = 0;
    status = STATUS_IO_TIMEOUT;
    while ((status == STATUS_IO_TIMEOUT) && (retries < 15))
    {
        status = co_await client->ConnectPipe(SocketAddress,
                                              *connection);
        if (status != STATUS_IO_TIMEOUT)
        {
            VERIFY_IS_TRUE(AllowErrors ? ((NT_SUCCESS(status)) || (status == STATUS_NO_SUCH_FILE) ||
                                          (status == K_STATUS_API_CLOSED) || (status == STATUS_PIPE_DISCONNECTED))
                                       : NT_SUCCESS(status));
            if ((AllowErrors) && (! NT_SUCCESS(status)))
            {
                co_return;
            }
        } else {
            retries++;
            NTSTATUS status2 = co_await KTimer::StartTimerAsync(*g_Allocator,
                                             KTL_TAG_TEST,
                                             1000,
                                             nullptr);
            VERIFY_IS_TRUE(NT_SUCCESS(status2));
        }
    }
    VERIFY_IS_TRUE(NT_SUCCESS(status));
        
    pipe = connection->GetClientPipe();
    VERIFY_IS_TRUE(pipe);

    SOCKETHANDLEFD cc = pipe->GetSocketHandleFd();
    
    for (ULONG j = 0; j < MessageCount; j++)
    {
        p = (PUCHAR)bufferOut->GetBuffer();
        for (ULONG i = 0; i < BufferSize; i++)
        {
            p[i] = (UCHAR)xx;
        }
        
        ktl::Awaitable<NTSTATUS> a;
        a = pipe->SendDataAsync(FALSE, *bufferOut, bytesSent);
        status = co_await a;
        VERIFY_IS_TRUE(AllowErrors ? IsAllowedErrorOnClose(status) : NT_SUCCESS(status));
        if ((AllowErrors) && (! NT_SUCCESS(status)))
        {
            break;
        }
        VERIFY_IS_TRUE(bytesSent == BufferSize);
        
        status = co_await pipe->ReceiveDataAsync(FALSE, *bufferIn, bytesReceived);
        VERIFY_IS_TRUE(AllowErrors ? IsAllowedErrorOnClose(status) : NT_SUCCESS(status));
        if ((AllowErrors) && (! NT_SUCCESS(status)))
        {
            break;
        }
        VERIFY_IS_TRUE(bytesReceived == BufferSize);

        p = (PUCHAR)bufferIn->GetBuffer();
        UCHAR expected;
        for (ULONG i = 0; i < BufferSize; i++)
        {
            expected = (UCHAR)(xx+1);
            if (p[i] != expected)
            {
                printf("p[%d] == %d, expect %d\n", i, p[i], expected);
                VERIFY_IS_TRUE(FALSE);
            }
        }
    }

    status = co_await client->DisconnectPipe(*connection);
    KDbgCheckpointWDataInformational(0, "Test5Client Disconnected",
                                     STATUS_SUCCESS, (ULONGLONG)cc.Get(), (ULONGLONG)pipe->GetSocketHandleFd().Get(), (ULONGLONG)pipe.RawPtr(), 0);
    VERIFY_IS_TRUE(AllowErrors ? ((NT_SUCCESS(status)) || (status == STATUS_NOT_FOUND) || (status == K_STATUS_API_CLOSED))
                               : NT_SUCCESS(status));
}

Task Test2Client(
    __in ULONGLONG SocketAddress,
    __in ULONG BufferSize,
    __in ULONG BadClose,
    __in BOOLEAN& Completed
    )
{
    NTSTATUS status;
    KAWIpcClient::SPtr client;
    KAWIpcClientServerConnection::SPtr connection;
    KAWIpcClientPipe::SPtr pipe;
    ULONG bytesSent, bytesReceived;
    KBuffer::SPtr bufferOut, bufferIn;
    PUCHAR p;

    Completed = FALSE;
    
    status = KBuffer::Create(BufferSize, bufferOut, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = KBuffer::Create(BufferSize, bufferIn, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = KAWIpcClient::Create(*g_Allocator,
                                  KTL_TAG_TEST,
                                  client);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = co_await client->OpenAsync();
    VERIFY_IS_TRUE(NT_SUCCESS(status));


    status = client->CreateClientServerConnection(connection);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = co_await client->ConnectPipe(SocketAddress,
                                 *connection);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
        
    pipe = connection->GetClientPipe();
    
    p = (PUCHAR)bufferOut->GetBuffer();
    for (ULONG i = 0; i < BufferSize; i++)
    {
        p[i] = 8;
    }

    status = co_await pipe->SendDataAsync(FALSE, *bufferOut, bytesSent);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(bytesSent == BufferSize);

    status = co_await pipe->ReceiveDataAsync(FALSE, *bufferIn, bytesReceived);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    VERIFY_IS_TRUE(bytesReceived == BufferSize);

    p = (PUCHAR)bufferIn->GetBuffer();
    for (ULONG i = 0; i < BufferSize; i++)
    {
        if (p[i] != 9)
        {
            printf("p[%d] != 9 %p %d\n", i, p, p[i]);
            VERIFY_IS_TRUE(FALSE);
        }
    }   

    Completed = TRUE;
    
    if (BadClose == 2)
    {
        //
        // Simulate disconnected pipe
        //
        KAWIpcPipe::CloseSocketHandleFd(pipe->GetSocketHandleFd());
        pipe->SetSocketHandleFd((SOCKETHANDLEFD)0);
    }       
        
    status = co_await client->DisconnectPipe(*connection);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = co_await client->CloseAsync();
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    co_return;
}

Task Test3aClient(
    __in KAWIpcClient& Client,
    __in ULONGLONG SocketAddress
    )
{
    NTSTATUS status;
    KAWIpcClient::SPtr client = &Client;
    KAWIpcClientServerConnection::SPtr connection;
    KAWIpcClientPipe::SPtr pipe;
    ULONG bytesReceived;
    KBuffer::SPtr bufferIn;

    status = KBuffer::Create(512, bufferIn, *g_Allocator);
    VERIFY_IS_TRUE(NT_SUCCESS(status));    

    status = client->CreateClientServerConnection(connection);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = co_await client->ConnectPipe(SocketAddress,
                                 *connection);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
        
    pipe = connection->GetClientPipe();
    
    status = co_await pipe->ReceiveDataAsync(FALSE, *bufferIn, bytesReceived);
    VERIFY_IS_TRUE((status == STATUS_CANCELLED) || (STATUS_PIPE_BROKEN));

    co_return;
}

class EchoPlusOneServer;

class EchoPlusOneServerClientConnection : public KAWIpcServerClientConnection
{
    K_FORCE_SHARED(EchoPlusOneServerClientConnection);
    
    friend EchoPlusOneServer;

    public:
        ktl::Awaitable<NTSTATUS> OpenAsync(
            __in KAWIpcServerPipe& ServerPipe,
            __in_opt KAsyncContextBase* const ParentAsync = nullptr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

        ktl::Awaitable<NTSTATUS> CloseAsync();

        ULONG GetDummyFactoy() { return(_DummyFactor); };
        VOID SetDummyFactor(__in ULONG DummyFactor) { _DummyFactor = DummyFactor; };
        
    private:
        ULONG _DummyFactor;
    
};

class EchoPlusOneServer : public KAWIpcServer
{
    K_FORCE_SHARED(EchoPlusOneServer);

    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out EchoPlusOneServer::SPtr& Context
        );

        ktl::Awaitable<NTSTATUS> OnIncomingConnection(
            __in KAWIpcServerPipe& Pipe,
            __out KAWIpcServerClientConnection::SPtr& PerClientConnection
            );

    private:
        Task ConnectionHandler(
            __in EchoPlusOneServerClientConnection& PerClientConnection                                       
            );
        
        NTSTATUS CreateEchoPlusOneServerClientConnection(
            __out EchoPlusOneServerClientConnection::SPtr& Context
            );      
};

EchoPlusOneServer::EchoPlusOneServer()
{
}

EchoPlusOneServer::~EchoPlusOneServer()
{
}

NTSTATUS EchoPlusOneServer::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out EchoPlusOneServer::SPtr& Context
)
{
    EchoPlusOneServer::SPtr context;
    
    context = _new(AllocationTag, Allocator) EchoPlusOneServer();
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS); 
}

Task EchoPlusOneServer::ConnectionHandler(
    __in EchoPlusOneServerClientConnection& PerClientConnection                                       
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KAWIpcServer::SPtr server;
    EchoPlusOneServerClientConnection::SPtr perClientConnection = &PerClientConnection;
    KAWIpcServerPipe::SPtr pipe;
    KBuffer::SPtr buffer;
    ULONG bytesReceived, bytesSent;
    BOOLEAN first = TRUE;
    UCHAR id = 0;

    status = KBuffer::Create(512, buffer, GetThisAllocator());
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    pipe = perClientConnection->GetServerPipe();
    VERIFY_IS_TRUE(pipe);
    server = pipe->GetServer();
    VERIFY_IS_TRUE(server);

    ULONG count = 0;
    while (NT_SUCCESS(status))
    {
        status = co_await  pipe->ReceiveDataAsync(FALSE, *buffer, bytesReceived);;
        
        if (! NT_SUCCESS(status))
        {
            continue;
        }
        VERIFY_IS_TRUE(bytesReceived == 512);

        PUCHAR p = (PUCHAR)buffer->GetBuffer();
        if (first)
        {
            id = p[0];
            first = FALSE;
        }
        
        for (ULONG i = 0; i < bytesReceived; i++)
        {
            VERIFY_IS_TRUE(p[i] == id);
            p[i]++;
        }

        ktl::Awaitable<NTSTATUS> a;
        a = pipe->SendDataAsync(FALSE, *buffer, bytesSent);
        status = co_await a;
        
        if (! NT_SUCCESS(status))
        {
            continue;
        }
        VERIFY_IS_TRUE(bytesSent == bytesReceived);
        count++;
    }
    status = co_await server->ShutdownServerClientConnection(*perClientConnection);
}

ktl::Awaitable<NTSTATUS> EchoPlusOneServer::OnIncomingConnection(
    __in KAWIpcServerPipe& Pipe,
    __out KAWIpcServerClientConnection::SPtr& PerClientConnection
    )
{       
    KCoService$ApiEntry(TRUE);
    
    NTSTATUS status;
    EchoPlusOneServerClientConnection::SPtr connection;

    status = EchoPlusOneServer::CreateEchoPlusOneServerClientConnection(connection);
    if (! NT_SUCCESS(status))
    {
        co_return status;
    }
    
    status = co_await connection->OpenAsync(Pipe);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        co_return status;
    }
    
    ConnectionHandler(*connection);
    
    PerClientConnection = up_cast<KAWIpcServerClientConnection, EchoPlusOneServerClientConnection>(connection);
    co_return STATUS_SUCCESS;
}

EchoPlusOneServerClientConnection::EchoPlusOneServerClientConnection()
{
}

EchoPlusOneServerClientConnection::~EchoPlusOneServerClientConnection()
{
}


NTSTATUS EchoPlusOneServer::CreateEchoPlusOneServerClientConnection(
    __out EchoPlusOneServerClientConnection::SPtr& Context
)
{
    EchoPlusOneServerClientConnection::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) EchoPlusOneServerClientConnection();
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS); 
}

ktl::Awaitable<NTSTATUS> EchoPlusOneServerClientConnection::OpenAsync(
    __in KAWIpcServerPipe& ServerPipe,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
)
{
    _DummyFactor = 1;
    co_return co_await KAWIpcServerClientConnection::OpenAsync(ServerPipe, ParentAsync, GlobalContextOverride);
}

ktl::Awaitable<NTSTATUS> EchoPlusOneServerClientConnection::CloseAsync()
{
    _DummyFactor = 0;
    co_return co_await KAWIpcServerClientConnection::CloseAsync();
}

class FailToConnectServer : public KAWIpcServer
{
    K_FORCE_SHARED(FailToConnectServer);

    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocatorTag,
            __out FailToConnectServer::SPtr& Context
        );

        ktl::Awaitable<NTSTATUS> OnIncomingConnection(
            __in KAWIpcServerPipe& Pipe,
            __out KAWIpcServerClientConnection::SPtr& PerClientConnection
            );
};

FailToConnectServer::FailToConnectServer()
{
}

FailToConnectServer::~FailToConnectServer()
{
}

NTSTATUS FailToConnectServer::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out FailToConnectServer::SPtr& Context
)
{
    FailToConnectServer::SPtr context;
    
    context = _new(AllocationTag, Allocator) FailToConnectServer();
    if (! context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (! NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS); 
}

ktl::Awaitable<NTSTATUS> FailToConnectServer::OnIncomingConnection(
    __in KAWIpcServerPipe& Pipe,
    __out KAWIpcServerClientConnection::SPtr& PerClientConnection
    )
{       
    KCoService$ApiEntry(TRUE);
    
    PerClientConnection = nullptr;
    co_return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
KAWIpcPipeTests()
{
    NTSTATUS status;
    static const ULONGLONG SocketAddress = 8887;

    //
    // Test 1: Start the server and shut it down
    //
    {
        printf("KAWIpcPipeTests - Test 1\n");
        KAWIpcServer::SPtr server;

        status = KAWIpcServer::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      server);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(server->OpenAsync(SocketAddress));
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        printf("Sleeping 20 seconds....\n");
        KNt::Sleep(20 * 1000);

        status = SyncAwait(server->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 2: Start the server, connect one client, do some
    //         reads/writes and shut down client gracefully
    //
    {
        printf("KAWIpcPipeTests - Test 2\n");
        EchoPlusOneServer::SPtr server;
        BOOLEAN completed;

        status = EchoPlusOneServer::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      server);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(server->OpenAsync(SocketAddress));
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        Test2Client(SocketAddress, 512, 1, completed);
        
        printf("Sleeping 1 minute....\n");
        KNt::Sleep(1 * 60 * 1000);
        
        VERIFY_IS_TRUE(completed);

        status = SyncAwait(server->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 3: Start the server, connect one client, do some
    //         reads/writes and shut down client ungracefully
    //
    {
        printf("KAWIpcPipeTests - Test 3a\n");
        EchoPlusOneServer::SPtr server;
        BOOLEAN completed;

        status = EchoPlusOneServer::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      server);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(server->OpenAsync(SocketAddress));
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        Test2Client(SocketAddress, 512, 2, completed);
        
        printf("Sleeping 1 minute....\n");
        KNt::Sleep(1 * 60 * 1000);
        
        VERIFY_IS_TRUE(completed);

        status = SyncAwait(server->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    {
        printf("KAWIpcPipeTests - Test 3b\n");
        EchoPlusOneServer::SPtr server;
        BOOLEAN completed;

        status = EchoPlusOneServer::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      server);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(server->OpenAsync(SocketAddress));
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        Test2Client(SocketAddress, 512, 3, completed);
        
        printf("Sleeping 1 minute....\n");
        KNt::Sleep(1 * 60 * 1000);

        VERIFY_IS_TRUE(completed);

        status = SyncAwait(server->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 4a: Start the server and client, kick off a client read that
    //          blocks and then shutdown client and then server
    //
    {
        printf("KAWIpcPipeTests - Test 4a\n");
        EchoPlusOneServer::SPtr server;
        KAWIpcClient::SPtr client;

        status = EchoPlusOneServer::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      server);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(server->OpenAsync(SocketAddress));
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcClient::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      client);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(client->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        Test3aClient(*client, SocketAddress);
        
        printf("Sleeping 20 seconds....\n");
        KNt::Sleep(20 * 1000);

        status = SyncAwait(client->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     

        status = SyncAwait(server->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }
    {
        printf("KAWIpcPipeTests - Test 4b\n");
        EchoPlusOneServer::SPtr server;
        KAWIpcClient::SPtr client;

        status = EchoPlusOneServer::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      server);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(server->OpenAsync(SocketAddress));
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcClient::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      client);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(client->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        Test3aClient(*client, SocketAddress);
        
        printf("Sleeping 20 seconds....\n");
        KNt::Sleep(20 * 1000);

        status = SyncAwait(server->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = SyncAwait(client->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 5: Start the server and connect a lot of clients and have
    //         the clients do lots of IO and then shutdown gracefully
    //
    {
        printf("KAWIpcPipeTests - Test 5\n");
        EchoPlusOneServer::SPtr server;
        Test5Client::SPtr client;
        static const ULONG PipeCount = 16;
        ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs[PipeCount];
        
        status = EchoPlusOneServer::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      server);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(server->OpenAsync(SocketAddress));
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = Test5Client::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      client);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(client->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < PipeCount; i++) 
        {
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Test5Client(*client, SocketAddress, 512, 1024, FALSE, *acs[i]);
        }

        printf("Wait for clients to finish....\n");
        for (ULONG i = 0; i < PipeCount; i++)
        {
            status = SyncAwait(acs[i]->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }
        
        status = SyncAwait(client->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = SyncAwait(server->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }

    {
        printf("KAWIpcPipeTests - Test 5b\n");
        EchoPlusOneServer::SPtr server;
        Test5Client::SPtr client;
        static const ULONG PipeCount = 64;
        ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs[PipeCount];

        status = EchoPlusOneServer::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      server);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(server->OpenAsync(SocketAddress));
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = Test5Client::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      client);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(client->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < PipeCount; i++) 
        {
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Test5Client(*client, SocketAddress, 512, 4096, TRUE, *acs[i]);
        }

        // Don't wait for test to be done

        status = SyncAwait(client->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = SyncAwait(server->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        for (ULONG i = 0; i < PipeCount; i++)
        {
            status = SyncAwait(acs[i]->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }       
    }
    {
        printf("KAWIpcPipeTests - Test 5c\n");
        EchoPlusOneServer::SPtr server;
        Test5Client::SPtr client;
        static const ULONG PipeCount = 64;
        ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs[PipeCount];

        status = EchoPlusOneServer::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      server);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(server->OpenAsync(SocketAddress));
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = Test5Client::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      client);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(client->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
 

        for (ULONG i = 0; i < PipeCount; i++) 
        {
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Test5Client(*client, SocketAddress, 512, 4096, TRUE, *acs[i]);
        }

        // Don't wait for test to be done

        status = SyncAwait(server->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = SyncAwait(client->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        for (ULONG i = 0; i < PipeCount; i++)
        {
            status = SyncAwait(acs[i]->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }
    }

    //
    // Test 6: Start the server and connect a lot of clients and have
    //         the clients do lots of IO and then shutdown server
    //         ungracefully.
    //
    {
        printf("KAWIpcPipeTests - Test 6\n");
        EchoPlusOneServer::SPtr server;
        Test5Client::SPtr client;
        static const ULONG PipeCount = 64;
        ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs[PipeCount];

        status = EchoPlusOneServer::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      server);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(server->OpenAsync(SocketAddress));
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = Test5Client::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      client);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(client->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
 
        for (ULONG i = 0; i < PipeCount; i++) 
        {
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Test5Client(*client, SocketAddress, 512, 4096, TRUE, *acs[i]);
        }

        KNt::Sleep(15 * 1000);

        status = SyncAwait(server->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = SyncAwait(client->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));        
        for (ULONG i = 0; i < PipeCount; i++)
        {
            status = SyncAwait(acs[i]->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }
    }

    //
    // Test 7: Server fails to allow connection
    //
    {
        printf("KAWIpcPipeTests - Test 7\n");
        FailToConnectServer::SPtr server;
        Test5Client::SPtr client;
        static const ULONG PipeCount = 64;
        ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs[PipeCount];

        status = FailToConnectServer::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      server);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(server->OpenAsync(SocketAddress));
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = Test5Client::Create(*g_Allocator,
                                      KTL_TAG_TEST,
                                      client);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(client->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
 
        for (ULONG i = 0; i < PipeCount; i++) 
        {
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            Test5Client(*client, SocketAddress, 512, 4096, TRUE, *acs[i]);
        }

        KNt::Sleep(15 * 1000);

        status = SyncAwait(client->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        status = SyncAwait(server->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));        
        for (ULONG i = 0; i < PipeCount; i++)
        {
            status = SyncAwait(acs[i]->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }
    }

    // TODO: Many more test cases with broken pipes
    return(STATUS_SUCCESS);
}

NTSTATUS
KAWIpcEventTests()
{
    NTSTATUS status;
    //
    // Test 1: Create an event kick it 5 times and verify 5 wakeups
    //
    {
        struct EventTestThread::EventTestThreadParameters parameters;
        EventTestThread::SPtr eventTest;
        static const ULONG TestCount = 5;
        
        parameters.Counter = TestCount;
        
        //
        // Wait and Kick is from the perspective of the other thread
        //
        EVENTHANDLEFD wait;
        EVENTHANDLEFD kick;
        static const PWCHAR WaitNameText = L"AWIpcTestWaitEvent";
        static const PWCHAR KickNameText = L"AWIpcTestKickEvent";

        status = KAWIpcEventWaiter::CreateEventWaiter(KickNameText, kick);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = KAWIpcEventWaiter::CreateEventWaiter(WaitNameText, wait);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


#if !defined(PLATFORM_UNIX)
        parameters.WaitName = WaitNameText;
        parameters.KickName = KickNameText;
#else
        parameters.WaitEventFd = wait.Get();
        parameters.KickEventFd = kick.Get();
#endif

        KSynchronizer sync;
        status = EventTestThread::Create(*g_Allocator,
                                         KTL_TAG_TEST,
                                         eventTest);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        parameters.Wakeups = 0;
        eventTest->StartThread(&parameters,
                               nullptr,
                               sync);

        //
        // waiter and kicker are in the context of this thread
        //
        KAWIpcEventWaiter::SPtr waiter;
        KAWIpcEventKicker::SPtr kicker;
        status = KAWIpcEventWaiter::Create(*g_Allocator,
                                           KTL_TAG_TEST,
                                           waiter);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcEventKicker::Create(*g_Allocator,
                                           KTL_TAG_TEST,
                                           wait,      // Kick and wait are in the context of the other thread
                                           kicker);
        VERIFY_IS_TRUE(NT_SUCCESS(status));


        
        for (ULONG i = 0; i < TestCount; i++)
        {
            KSynchronizer sync2;
            
            status = sync.WaitForCompletion(1);
            VERIFY_IS_TRUE(status == STATUS_IO_TIMEOUT);

            status = kicker->Kick();
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            waiter->Reuse();
            waiter->StartWait(kick,      // Kick and wait are in the context of the other thread
                              nullptr,
                              sync2);
            status = sync2.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(parameters.Wakeups == TestCount);
        
        status = KAWIpcEventWaiter::CleanupEventWaiter(wait);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcEventWaiter::CleanupEventWaiter(kick);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Test 2: Start waiting for an event and cancel it and verify that
    //         it has indeed cancelled.
    //
    {
        EVENTHANDLEFD wait;
        KSynchronizer sync2;
            
        static const PWCHAR WaitNameText = L"AWIpcTestWaitEvent";
        
        status = KAWIpcEventWaiter::CreateEventWaiter(WaitNameText, wait);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(wait); });

        KAWIpcEventWaiter::SPtr waiter;
        status = KAWIpcEventWaiter::Create(*g_Allocator,
                                           KTL_TAG_TEST,
                                           waiter);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        waiter->StartWait(wait,
                          nullptr,
                          sync2);
        status = sync2.WaitForCompletion(500);
        VERIFY_IS_TRUE(status == STATUS_IO_TIMEOUT);

        waiter->Cancel();
        status = sync2.WaitForCompletion();
        VERIFY_IS_TRUE(status == STATUS_CANCELLED);         
    }
    
    return(STATUS_SUCCESS);
}

ULONG GetExpectedElementCount(
    __in ULONG Size
    )
{
    ULONG number64k, number4k;
    number64k = Size / KAWIpcPageAlignedHeap::SixtyFourKB;
    number4k = (Size - (number64k * KAWIpcPageAlignedHeap::SixtyFourKB)) / KAWIpcPageAlignedHeap::FourKB;

    return(number64k + number4k);
}

Task HeapAllocationTask(
    __in KAWIpcPageAlignedHeap& Heap,
    __in ULONG Size,
    __out KIoBuffer::SPtr& IoBuffer,
    __in KEvent& Event
    )
{
    NTSTATUS status;
    KAWIpcPageAlignedHeap::SPtr heap = &Heap;
    
    status = co_await heap->AllocateAsync(Size, IoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status));     
    Event.SetEvent();
}

Task HeapAllocationLoopTask(
    __in ULONG Id,
    __in KAWIpcPageAlignedHeap& Heap,
    __in BOOLEAN AllowErrors,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
 )
{
    KFinally([&](){  Acs.SetResult(STATUS_SUCCESS); });

    NTSTATUS status;
    KAWIpcPageAlignedHeap::SPtr heap = &Heap;
    static const ULONG randomIterations = 128;
    KIoBuffer::SPtr ioBuffer;
    ULONG totalBlocks;
    ULONG blocks;
    ULONG expectedElementCount;

    status = co_await KTimer::StartTimerAsync(*g_Allocator,
                                              KTL_TAG_TEST,
                                              1,
                                              nullptr); 
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    totalBlocks = (heap->GetMaximumAllocationPossible() / KAWIpcPageAlignedHeap::FourKB)-1;
    srand((ULONG)GetTickCount());

    for (ULONG i = 0; i < randomIterations; i++)
    {
        if ((i % 32) == 0)
        {
            printf("%d HeapAllocationLoopTask %d\n", Id, i);
        }
        
        blocks = ((rand() % totalBlocks) + 1);

        ULONG size = blocks * KAWIpcPageAlignedHeap::FourKB;
        status = co_await heap->AllocateAsync(size, ioBuffer);
        VERIFY_IS_TRUE((NT_SUCCESS(status)) ||
                       (AllowErrors && ((status == K_STATUS_SHUTDOWN_PENDING) || (status == K_STATUS_API_CLOSED)))); 

        if (! NT_SUCCESS(status))
        {
            break;
        }
        
        expectedElementCount = GetExpectedElementCount(size);           
        VERIFY_IS_TRUE(ioBuffer->QueryNumberOfIoBufferElements() == expectedElementCount);
        VERIFY_IS_TRUE(ioBuffer->QuerySize() == size);
        ioBuffer = nullptr;
        status = co_await KTimer::StartTimerAsync(*g_Allocator,
                                          KTL_TAG_TEST,
                                          1,
                                          nullptr); 
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
}

NTSTATUS
KAWIpcHeapTests()
{
    NTSTATUS status = STATUS_SUCCESS;
    static const ULONG sharedMemorySizeSmall = 1024 * 1024;  // 1MB
    KAWIpcSharedMemory::SPtr sharedMemorySmall;
    static const ULONG sharedMemorySizeLarge = 512 * 1024 * 1024;  // 512MB
    KAWIpcSharedMemory::SPtr sharedMemoryLarge;

#if defined(PLATFORM_UNIX)
    //
    // For Linux, remove any stray shared memory sections that might
    // have been left over due to a crash.
    //
    {
        ULONG count;
        status = SyncAwait(KAWIpcSharedMemory::CleanupFromPreviousCrashes(*g_Allocator, KTL_TAG_TEST));
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        count = GetNumberKAWIpcSharedMemorySections();
        VERIFY_IS_TRUE(count == 0);     
    }
#endif
    
    status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                        sharedMemorySizeSmall, sharedMemorySmall);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                        sharedMemorySizeLarge, sharedMemoryLarge);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Test 1: Start up a heap and then shut it down
    //
    {
        KAWIpcPageAlignedHeap::SPtr heap;

        status = KAWIpcPageAlignedHeap::Create(*g_Allocator, KTL_TAG_TEST, *sharedMemorySmall, heap);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(heap->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(heap->GetMaximumAllocationPossible() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);
        
        status = SyncAwait(heap->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 2: Start up a heap, do an allocation, free it and then shut
    //         it down.
    //
    {
        KAWIpcPageAlignedHeap::SPtr heap;
        KIoBuffer::SPtr ioBuffer;
        ULONG expectedElementCount;

        status = KAWIpcPageAlignedHeap::Create(*g_Allocator, KTL_TAG_TEST, *sharedMemorySmall, heap);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(heap->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(heap->GetMaximumAllocationPossible() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetNumberAllocations() == 0);

        //
        // Verify allocation of 0 bytes fails
        //
        status = SyncAwait(heap->AllocateAsync(0, ioBuffer));
        VERIFY_IS_TRUE(status == STATUS_INVALID_BUFFER_SIZE);        
        VERIFY_IS_TRUE(heap->GetNumberAllocations() == 0);

        //
        // Verify allocation of more than maximum fails
        //
        status = SyncAwait(heap->AllocateAsync(sharedMemorySizeSmall + KAWIpcPageAlignedHeap::FourKB, ioBuffer));
        VERIFY_IS_TRUE(status == STATUS_INVALID_BUFFER_SIZE);        
        VERIFY_IS_TRUE(heap->GetNumberAllocations() == 0);

        //
        // Verify allocation of non 4K block fails
        //
        status = SyncAwait(heap->AllocateAsync(KAWIpcPageAlignedHeap::FourKB + 237, ioBuffer));
        VERIFY_IS_TRUE(status == STATUS_INVALID_BUFFER_SIZE);        
        VERIFY_IS_TRUE(heap->GetNumberAllocations() == 0);
        
        //
        // Perform 4K allocation and free
        //
        status = SyncAwait(heap->AllocateAsync(KAWIpcPageAlignedHeap::FourKB, ioBuffer));
        VERIFY_IS_TRUE(NT_SUCCESS(status));     

        expectedElementCount = 1;
        VERIFY_IS_TRUE(ioBuffer->QueryNumberOfIoBufferElements() == expectedElementCount);
        VERIFY_IS_TRUE(ioBuffer->QuerySize() == KAWIpcPageAlignedHeap::FourKB);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == (sharedMemorySizeSmall - KAWIpcPageAlignedHeap::FourKB))
        ioBuffer = nullptr;
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetNumberAllocations() == 0);


        //
        // Perform 64K allocation and free
        //
        status = SyncAwait(heap->AllocateAsync(KAWIpcPageAlignedHeap::SixtyFourKB, ioBuffer));
        VERIFY_IS_TRUE(NT_SUCCESS(status));     

        expectedElementCount = 1;
        VERIFY_IS_TRUE(ioBuffer->QueryNumberOfIoBufferElements() == expectedElementCount);
        VERIFY_IS_TRUE(ioBuffer->QuerySize() == KAWIpcPageAlignedHeap::SixtyFourKB);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == (sharedMemorySizeSmall - KAWIpcPageAlignedHeap::SixtyFourKB))
        ioBuffer = nullptr;
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetNumberAllocations() == 0);

        
        //
        // Perform full allocation and free
        //
        status = SyncAwait(heap->AllocateAsync(sharedMemorySizeSmall, ioBuffer));
        VERIFY_IS_TRUE(NT_SUCCESS(status));     

        expectedElementCount = GetExpectedElementCount(sharedMemorySizeSmall);
        VERIFY_IS_TRUE(ioBuffer->QueryNumberOfIoBufferElements() == expectedElementCount);
        VERIFY_IS_TRUE(ioBuffer->QuerySize() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == 0);
        ioBuffer = nullptr;
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetNumberAllocations() == 0);

        //
        // Perform almost full allocation and free
        //
        ULONG almostFull = sharedMemorySizeSmall - KAWIpcPageAlignedHeap::FourKB;
        status = SyncAwait(heap->AllocateAsync(almostFull, ioBuffer));
        VERIFY_IS_TRUE(NT_SUCCESS(status));     

        expectedElementCount = GetExpectedElementCount(almostFull);
        VERIFY_IS_TRUE(ioBuffer->QueryNumberOfIoBufferElements() == expectedElementCount);
        VERIFY_IS_TRUE(ioBuffer->QuerySize() == almostFull);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == KAWIpcPageAlignedHeap::FourKB);
        ioBuffer = nullptr;
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetNumberAllocations() == 0);

        //
        // Perform a whole bunch of random allocations and verify them
        //
        static const ULONG randomIterations = 4096;
        ULONG totalBlocks = (sharedMemorySizeSmall / KAWIpcPageAlignedHeap::FourKB) - 1;
        ULONG blocks;
        
        srand((ULONG)GetTickCount());
        
        for (ULONG i = 0; i < randomIterations; i++)
        {
            blocks = ((rand() % totalBlocks) + 1);

            ULONG size = blocks * KAWIpcPageAlignedHeap::FourKB;
            status = SyncAwait(heap->AllocateAsync(size, ioBuffer));
            VERIFY_IS_TRUE(NT_SUCCESS(status));     

            expectedElementCount = GetExpectedElementCount(size);           
            VERIFY_IS_TRUE(ioBuffer->QueryNumberOfIoBufferElements() == expectedElementCount);
            VERIFY_IS_TRUE(ioBuffer->QuerySize() == size);
            VERIFY_IS_TRUE(heap->GetFreeAllocation() == (sharedMemorySizeSmall - size));
            ioBuffer = nullptr;
            VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);         
            VERIFY_IS_TRUE(heap->GetNumberAllocations() == 0);
        }
        
        status = SyncAwait(heap->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 3: Start up a heap, allocate all of the space and then
    //         allocate more. Verify that last allocation blocks until
    //         the exact amount of freed back. Shut down the heap.
    //
    {
        KAWIpcPageAlignedHeap::SPtr heap;
        KIoBuffer::SPtr ioBuffer;

        status = KAWIpcPageAlignedHeap::Create(*g_Allocator, KTL_TAG_TEST, *sharedMemorySmall, heap);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(heap->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(heap->GetMaximumAllocationPossible() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);


        //
        // Allocate all of memory
        //
        static const ULONG total64kBlocks = (sharedMemorySizeSmall / KAWIpcPageAlignedHeap::SixtyFourKB);
        KIoBuffer::SPtr ioBuffers[total64kBlocks];
        for (ULONG i = 0; i < total64kBlocks; i++)
        {
            status = SyncAwait(heap->AllocateAsync(KAWIpcPageAlignedHeap::SixtyFourKB, ioBuffers[i]));
            VERIFY_IS_TRUE(NT_SUCCESS(status));     

            VERIFY_IS_TRUE(ioBuffers[i]->QueryNumberOfIoBufferElements() == 1);
            VERIFY_IS_TRUE(ioBuffers[i]->QuerySize() == KAWIpcPageAlignedHeap::SixtyFourKB);
            VERIFY_IS_TRUE(heap->GetFreeAllocation() == (sharedMemorySizeSmall - ((i+1) * KAWIpcPageAlignedHeap::SixtyFourKB)));            
        }

        //
        // Now try to allocate more memory
        //
        KEvent event;
        BOOLEAN b;

        HeapAllocationTask(*heap, 4 * KAWIpcPageAlignedHeap::SixtyFourKB, ioBuffer, event);

        b = event.WaitUntilSet(1000);
        VERIFY_IS_TRUE(b == FALSE);

        ioBuffers[0] = nullptr;
        b = event.WaitUntilSet(1000);
        VERIFY_IS_TRUE(b == FALSE);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == KAWIpcPageAlignedHeap::SixtyFourKB);
        
        ioBuffers[1] = nullptr;
        b = event.WaitUntilSet(1000);
        VERIFY_IS_TRUE(b == FALSE);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == (2 * KAWIpcPageAlignedHeap::SixtyFourKB));
        
        ioBuffers[2] = nullptr;
        b = event.WaitUntilSet(1000);
        VERIFY_IS_TRUE(b == FALSE);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == (3 * KAWIpcPageAlignedHeap::SixtyFourKB));
        
        ioBuffers[3] = nullptr;
        b = event.WaitUntilSet(1000);
        VERIFY_IS_TRUE(b == TRUE);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == 0);

        for (ULONG i = 4; i < total64kBlocks; i++)
        {
            ioBuffers[i] = nullptr;
        }
        
        ioBuffer = nullptr;
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);

        
        status = SyncAwait(heap->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 4: Start up a heap, allocate in a way that will cause
    //         fragmentation and then allocate a block that will not
    //         fit easily due to fragmentation and verify that it is
    //         still allocated
    {
        KAWIpcPageAlignedHeap::SPtr heap;
        KIoBuffer::SPtr ioBuffer;

        status = KAWIpcPageAlignedHeap::Create(*g_Allocator, KTL_TAG_TEST, *sharedMemorySmall, heap);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(heap->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(heap->GetMaximumAllocationPossible() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);


        //
        // Allocate memory in a specific patter to generate
        // fragmentation
        //
        static const ULONG total64kBlocks = (sharedMemorySizeSmall /
                                            (KAWIpcPageAlignedHeap::SixtyFourKB + KAWIpcPageAlignedHeap::FourKB));
        static const ULONG total4kBlocks = ((sharedMemorySizeSmall - (total64kBlocks * KAWIpcPageAlignedHeap::SixtyFourKB)) /
                                            KAWIpcPageAlignedHeap::FourKB);
        static const ULONG extra4kBlocks = total4kBlocks - total64kBlocks;
        static const ULONG fragmentBlocks = (total4kBlocks - extra4kBlocks);
        static const ULONG fragmentSize = fragmentBlocks * KAWIpcPageAlignedHeap::FourKB;

        KIoBuffer::SPtr ioBuffers64k[total64kBlocks];
        KIoBuffer::SPtr ioBuffers4k[total4kBlocks];
        for (ULONG i = 0; i < total64kBlocks; i++)
        {
            status = SyncAwait(heap->AllocateAsync(KAWIpcPageAlignedHeap::SixtyFourKB, ioBuffers64k[i]));
            VERIFY_IS_TRUE(NT_SUCCESS(status));     

            VERIFY_IS_TRUE(ioBuffers64k[i]->QueryNumberOfIoBufferElements() == 1);
            VERIFY_IS_TRUE(ioBuffers64k[i]->QuerySize() == KAWIpcPageAlignedHeap::SixtyFourKB);
            VERIFY_IS_TRUE(heap->GetFreeAllocation() == (sharedMemorySizeSmall - 
                                                               ((i * (KAWIpcPageAlignedHeap::SixtyFourKB +KAWIpcPageAlignedHeap::FourKB)) +
                                                                KAWIpcPageAlignedHeap::SixtyFourKB)));
            
            status = SyncAwait(heap->AllocateAsync(KAWIpcPageAlignedHeap::FourKB, ioBuffers4k[i]));
            VERIFY_IS_TRUE(NT_SUCCESS(status));     

            VERIFY_IS_TRUE(ioBuffers4k[i]->QueryNumberOfIoBufferElements() == 1);
            VERIFY_IS_TRUE(ioBuffers4k[i]->QuerySize() == KAWIpcPageAlignedHeap::FourKB);
            VERIFY_IS_TRUE(heap->GetFreeAllocation() == (sharedMemorySizeSmall - 
                                                          ((i+1) * (KAWIpcPageAlignedHeap::SixtyFourKB + KAWIpcPageAlignedHeap::FourKB))));         
        }

        ULONG allocation = total64kBlocks * (KAWIpcPageAlignedHeap::SixtyFourKB + KAWIpcPageAlignedHeap::FourKB);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == (sharedMemorySizeSmall - allocation));
        
        for (ULONG i = total64kBlocks; i < (total64kBlocks + extra4kBlocks); i++)
        {
            status = SyncAwait(heap->AllocateAsync(KAWIpcPageAlignedHeap::FourKB, ioBuffers4k[i]));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            allocation += KAWIpcPageAlignedHeap::FourKB;

            VERIFY_IS_TRUE(ioBuffers4k[i]->QueryNumberOfIoBufferElements() == 1);
            VERIFY_IS_TRUE(ioBuffers4k[i]->QuerySize() == KAWIpcPageAlignedHeap::FourKB);           
            VERIFY_IS_TRUE(heap->GetFreeAllocation() == (sharedMemorySizeSmall - allocation));
        }

        for (ULONG i = 0; i < total4kBlocks; i++)
        {
            ioBuffers4k[i] = nullptr;
        }
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == (sharedMemorySizeSmall - (total64kBlocks * KAWIpcPageAlignedHeap::SixtyFourKB)));

        //
        // Perform fragmented allocation
        //
        status = SyncAwait(heap->AllocateAsync(fragmentSize, ioBuffer));
        VERIFY_IS_TRUE(NT_SUCCESS(status));     

        VERIFY_IS_TRUE(ioBuffer->QueryNumberOfIoBufferElements() == fragmentBlocks);     
        VERIFY_IS_TRUE(ioBuffer->QuerySize() == fragmentSize);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == (sharedMemorySizeSmall - 
                                                      (((total64kBlocks) * KAWIpcPageAlignedHeap::SixtyFourKB) + fragmentSize)));

        //
        // Finish freeing
        //
        for (ULONG i = 0; i < total64kBlocks; i++)
        {
            ioBuffers64k[i] = nullptr;
        }
        
        ioBuffer = nullptr;
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);

        
        status = SyncAwait(heap->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 5: Startup a heap and start worker tasks that do lots of
    //         allocation and deallocation in a multitthreaded way.
    //         Shutdown the heap after all requests finish.
    //
    {
        KAWIpcPageAlignedHeap::SPtr heap;
        static const ULONG tasks = 32; // 1024;
        ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs[tasks];

        status = KAWIpcPageAlignedHeap::Create(*g_Allocator, KTL_TAG_TEST, *sharedMemorySmall, heap);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(heap->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(heap->GetMaximumAllocationPossible() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);
        
        for (ULONG i = 0; i < tasks; i++)
        {
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            HeapAllocationLoopTask(i, *heap, FALSE, *acs[i]);
        }       
        
        for (ULONG i = 0; i < tasks; i++)
        {
            status = SyncAwait(acs[i]->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }
        
        status = SyncAwait(heap->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 6: Startup a heap and start worker tasks that do lots of
    //         allocation and deallocation in a multitthreaded way.
    //         Shutdown the heap while requests are in progress.
    //
    {
        KAWIpcPageAlignedHeap::SPtr heap;
        static const ULONG tasks = 1024;
        ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs[tasks];

        status = KAWIpcPageAlignedHeap::Create(*g_Allocator, KTL_TAG_TEST, *sharedMemorySmall, heap);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = SyncAwait(heap->OpenAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_IS_TRUE(heap->GetMaximumAllocationPossible() == sharedMemorySizeSmall);
        VERIFY_IS_TRUE(heap->GetFreeAllocation() == sharedMemorySizeSmall);
        
        for (ULONG i = 0; i < tasks; i++)
        {
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            HeapAllocationLoopTask(i, *heap, TRUE, *acs[i]);
        }

        KNt::Sleep(200);
        
        status = SyncAwait(heap->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        for (ULONG i = 0; i < tasks; i++)
        {
            status = SyncAwait(acs[i]->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }       
    }

    
    return(status);
}

VOID
KAWIpcLIFOCountTest(
    __in ULONG MessageSharedMemorySize,
    __in ULONG RingSharedMemorySize
)
{
    NTSTATUS status;
    KAWIpcSharedMemory::SPtr messageSharedMemory;
    KAWIpcSharedMemory::SPtr ringSharedMemory;
    KAWIpcLIFOList::SPtr lifoA, lifoB;
    KAWIpcRing::SPtr ringA, ringB;
    LONG ownerIdA = (LONG)(0x80000000 | 98);
    LONG ownerIdB = (LONG)(0x80000000 | 99);
    ULONG count64, count1024, count4096, count64k;
    KAWIpcMessage* message;
    BOOLEAN expectEmpty;
    BOOLEAN wasEmpty;
    ULONG ringCount;
    ULONG expected64Count = 0;
    ULONG expected1024Count = 0;
    ULONG expected4096Count = 0;
    ULONG expected64kCount = 0;
    ULONG expectedRingCount = expected64Count + expected1024Count + expected4096Count + expected64kCount;
    
    status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                        MessageSharedMemorySize, messageSharedMemory);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                        RingSharedMemorySize, ringSharedMemory);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KAWIpcLIFOList::CreateAndInitialize(*messageSharedMemory,
                                                 *g_Allocator,
                                                 KTL_TAG_TEST,
                                                 lifoA);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
	
	expected64Count = lifoA->GetCount64();
	expected1024Count = lifoA->GetCount1024();
	expected4096Count = lifoA->GetCount4096();
	expected64kCount = lifoA->GetCount64k();
	expectedRingCount = expected64Count + expected1024Count + expected4096Count + expected64kCount;
	
    status = KAWIpcRing::CreateAndInitialize(ownerIdA,
                                             *ringSharedMemory,
                                             *messageSharedMemory,
                                             *g_Allocator,
                                             KTL_TAG_TEST,
                                             ringA);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = KAWIpcLIFOList::Create(*messageSharedMemory,
                                    *g_Allocator,
                                    KTL_TAG_TEST,
                                    lifoB);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KAWIpcRing::Create(ownerIdB,
                                *ringSharedMemory,
                                *messageSharedMemory,
                                *g_Allocator,
                                KTL_TAG_TEST,
                                ringB);
    VERIFY_IS_TRUE(NT_SUCCESS(status));


	KInvariant(ringA->GetNumberEntries() > expectedRingCount);
	
    //
    // Pull off the LIFO and put on the ring
    //
    expectEmpty = TRUE;
    count64 = 0;
    do
    {
        message = lifoA->Remove(64);
        if (message != nullptr)
        {
            message->Data[0] = (UCHAR)count64;
            ringB->SetWaitingForEvent();
            status = ringB->Enqueue(message, wasEmpty);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(wasEmpty == expectEmpty);
            expectEmpty = FALSE;
            count64++;
        }
    } while (message != nullptr);
    VERIFY_IS_TRUE(count64 == expected64Count);

    count1024 = 0;
    do
    {
        message = lifoB->Remove(1024);
        if (message != nullptr)
        {
            message->Data[0] = (UCHAR)(count64 + count1024);
            ringB->SetWaitingForEvent();
            status = ringB->Enqueue(message, wasEmpty);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(wasEmpty == expectEmpty);
            count1024++;
        }
    } while (message != nullptr);
    VERIFY_IS_TRUE(count1024 == expected1024Count);

    count4096 = 0;
    do
    {
        message = lifoB->Remove(4096);
        if (message != nullptr)
        {
            message->Data[0] = (UCHAR)(count64 + count1024 + count4096);
            ringB->SetWaitingForEvent();
            status = ringB->Enqueue(message, wasEmpty);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(wasEmpty == expectEmpty);
            count4096++;
        }
    } while (message != nullptr);
    VERIFY_IS_TRUE(count4096 == expected4096Count);
	
    count64k = 0;
    do
    {
        message = lifoB->Remove(64 * 1024);
        if (message != nullptr)
        {
            message->Data[0] = (UCHAR)(count64 + count1024 + count4096 + count64k);
            ringB->SetWaitingForEvent();
            status = ringB->Enqueue(message, wasEmpty);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(wasEmpty == expectEmpty);
            count64k++;
        }
    } while (message != nullptr);
    VERIFY_IS_TRUE(count64k == expected64kCount);
	
    //
    // Pull off the ring
    //
    ringCount = 0;
    do
    {
        status = ringA->Dequeue(message);
        if (NT_SUCCESS(status))
        {

			VERIFY_IS_TRUE(message->Data[0] == (UCHAR)ringCount);
            if ((ringCount % 2) == 0)
            {
                lifoA->Add(*message);
            } else {
                lifoB->Add(*message);
            }
            ringCount++;            
        }
    } while (NT_SUCCESS(status));
    VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);
    VERIFY_IS_TRUE(ringCount == expectedRingCount);
}

Task
LIFORingProducerTask(
    __in ULONG Id,
    __in ULONG Count,
    __in KAWIpcSharedMemory& MessageSharedMemory,
    __in KAWIpcSharedMemory& RingSharedMemory,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
    )
{
    NTSTATUS status;
    KAWIpcSharedMemory::SPtr messageSharedMemory = &MessageSharedMemory;
    KAWIpcSharedMemory::SPtr ringSharedMemory = &RingSharedMemory;
    KAWIpcLIFOList::SPtr lifoB;
    KAWIpcRing::SPtr ringB;
    LONG ownerId = (0x80000000 | (LONG)Id);
    BOOLEAN wasEmpty;
    KAWIpcMessage* message;
    ULONG i;
    KFinally([&](){  Acs.SetResult(STATUS_SUCCESS); });

    status = co_await KTimer::StartTimerAsync(*g_Allocator,
                                              KTL_TAG_TEST,
                                              1,
                                              nullptr); 
    VERIFY_IS_TRUE(NT_SUCCESS(status));
 
    status = KAWIpcLIFOList::Create(*messageSharedMemory,
                                    *g_Allocator,
                                    KTL_TAG_TEST,
                                    lifoB);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KAWIpcRing::Create(ownerId,
                                *ringSharedMemory,
                                *messageSharedMemory,
                                *g_Allocator,
                                KTL_TAG_TEST,
                                ringB);
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 

    i = 0;
    message = nullptr;
    while (i < Count)
    {
        if (message == nullptr)
        {
            message = lifoB->Remove(64);
        }
        
        if (message != nullptr)
        {
            status = ringB->Enqueue(message, wasEmpty);
        } else {
            status = STATUS_UNSUCCESSFUL;
        }

        if (! NT_SUCCESS(status))
        {
            status = co_await KTimer::StartTimerAsync(*g_Allocator,
                                                      KTL_TAG_TEST,
                                                      10,
                                                      nullptr);
        } else {
            i++;
            message = nullptr;
        }
    }   
}

Task
LIFORingConsumerTask(
    __in ULONG Id,
    __in ULONG Count,
    __in KAWIpcSharedMemory& MessageSharedMemory,
    __in KAWIpcSharedMemory& RingSharedMemory,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
    )
{
    NTSTATUS status;
    KAWIpcSharedMemory::SPtr messageSharedMemory = &MessageSharedMemory;
    KAWIpcSharedMemory::SPtr ringSharedMemory = &RingSharedMemory;
    KAWIpcLIFOList::SPtr lifoB;
    KAWIpcRing::SPtr ringB;
    LONG ownerId = (0x80000000 | (LONG)Id);
    KAWIpcMessage* message;
    ULONG i;
    KFinally([&](){  Acs.SetResult(STATUS_SUCCESS); });
        
    status = co_await KTimer::StartTimerAsync(*g_Allocator,
                                              KTL_TAG_TEST,
                                              1,
                                              nullptr); 
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = KAWIpcLIFOList::Create(*messageSharedMemory,
                                    *g_Allocator,
                                    KTL_TAG_TEST,
                                    lifoB);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KAWIpcRing::Create(ownerId,
                                *ringSharedMemory,
                                *messageSharedMemory,
                                *g_Allocator,
                                KTL_TAG_TEST,
                                ringB);
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 

    i = 0;
    message = nullptr;
    while (i < Count)
    {
        status = ringB->Dequeue(message);
        if (NT_SUCCESS(status))
        {
            lifoB->Add(*message);
            i++;
        } else {
            VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);
            status = co_await KTimer::StartTimerAsync(*g_Allocator,
                                                      KTL_TAG_TEST,
                                                      10,
                                                      nullptr);
        }

    }
}

ULONG KInvariantLine;
BOOLEAN KInvariantBoolean;

KInvariantCalloutType PreviousKInvariantCallout;

BOOLEAN MyKInvariantCallout(
    __in LPCSTR Condition,
    __in LPCSTR File,
    __in ULONG Line
    )
{
    //
    // Remember the info passed
    //
    KInvariantLine = Line;

    //
    // Forward on to the previous callout and remember result
    //
    KInvariantBoolean = (*PreviousKInvariantCallout)(Condition, File, Line);

    //
    // Do not cause process to crash - we are testing after all.
    // KInvariant callouts in non-test code should almost NEVER return
    // FALSE.
    //
    return(FALSE);
}

VOID ResetKInvariantCallout()
{
    KInvariantLine = 0;
}

NTSTATUS
KAWIpcLIFORingTests()
{
    NTSTATUS status = STATUS_SUCCESS;

#if defined(PLATFORM_UNIX)
    //
    // For Linux, remove any stray shared memory sections that might
    // have been left over due to a crash.
    //
    {
        ULONG count;
        status = SyncAwait(KAWIpcSharedMemory::CleanupFromPreviousCrashes(*g_Allocator, KTL_TAG_TEST));
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        count = GetNumberKAWIpcSharedMemorySections();
        VERIFY_IS_TRUE(count == 0);
    }
#endif

    //
    // Test 1: Create LIFO with different shared memory sizes
    //         (1MB and 512MB) and verify count by allocating, placing in
    //         ring and then unwinding
    //
    {
        KAWIpcLIFOCountTest(256 * 1024, 16 * 0x1000);
    }

    //
    // Test 2: Insert more KAWIpcMessages than will fit in a ring and
    //         verify error code. Note that if the header for
    //         KAWIpcLIFOList changes then this test will need to be
    //         updated.
    //
    {
        KAWIpcSharedMemory::SPtr messageSharedMemory;
        KAWIpcSharedMemory::SPtr ringSharedMemory;
        KAWIpcMessage* message;
        KAWIpcLIFOList::SPtr lifoA;
        KAWIpcRing::SPtr ringA;
        LONG ownerId = (LONG)(0x80000000 | 101);
        BOOLEAN wasEmpty;
        ULONG count, ringCount;

        status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                            1024 * 1024, messageSharedMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                            4096, ringSharedMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcLIFOList::CreateAndInitialize(*messageSharedMemory,
                                                     *g_Allocator,
                                                     KTL_TAG_TEST,
                                                     lifoA);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcRing::CreateAndInitialize(ownerId,
                                                 *ringSharedMemory,
                                                 *messageSharedMemory,
                                                 *g_Allocator,
                                                 KTL_TAG_TEST,
                                                 ringA);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        
        count = 0;
        do
        {
            message = lifoA->Remove(64);
            VERIFY_IS_TRUE(message != nullptr);         
            count++;
            status = ringA->Enqueue(message, wasEmpty);
            if (count == (ringA->GetNumberEntries() + 1))
            {           
                VERIFY_IS_TRUE(status == STATUS_ARRAY_BOUNDS_EXCEEDED);
                lifoA->Add(*message);
                break;
            } else {
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

        } while (message != nullptr);

        ringCount = 0;
        do
        {
            status = ringA->Dequeue(message);
            if (NT_SUCCESS(status))
            {
                lifoA->Add(*message);
                ringCount++;            
            }
        } while (NT_SUCCESS(status));
        VERIFY_IS_TRUE(status == STATUS_NOT_FOUND);
        VERIFY_IS_TRUE(ringCount == ringA->GetNumberEntries());      
    }

    //
    // Test 3: Verify breaking lock abandoned when holdong Ring
    //
    {
        KAWIpcSharedMemory::SPtr messageSharedMemory;
        KAWIpcSharedMemory::SPtr ringSharedMemory;
        KAWIpcMessage* message;
        KAWIpcLIFOList::SPtr lifoA;
        KAWIpcRing::SPtr ringA, ringB;
        LONG ownerIdA = (LONG)(0x80000000 | 105);
        LONG ownerIdB = (LONG)(0x80000000 | 106);
        BOOLEAN wasEmpty;

        status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                            256 * 1024, messageSharedMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                            4 * 4096, ringSharedMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcLIFOList::CreateAndInitialize(*messageSharedMemory,
                                                     *g_Allocator,
                                                     KTL_TAG_TEST,
                                                     lifoA);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcRing::CreateAndInitialize(ownerIdA,
                                                 *ringSharedMemory,
                                                 *messageSharedMemory,
                                                 *g_Allocator,
                                                 KTL_TAG_TEST,
                                                 ringA);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcRing::Create(ownerIdB,
                                                 *ringSharedMemory,
                                                 *messageSharedMemory,
                                                 *g_Allocator,
                                                 KTL_TAG_TEST,
                                                 ringB);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        //
        // Produce a couple of things using ringA
        //
        for (ULONG i = 0; i < 5; i++)
        {
            message = lifoA->Remove(64);
            VERIFY_IS_TRUE(message != nullptr);         
            status = ringA->Enqueue(message, wasEmpty);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        //
        // Take the lock for ringA
        //
        ringA->Lock();

        //
        // Try to consume via ringB and verify that KInvariant called
        //
        {
            ResetKInvariantCallout();
            PreviousKInvariantCallout = SetKInvariantCallout(MyKInvariantCallout);
            KFinally([&](){  SetKInvariantCallout(PreviousKInvariantCallout); });

            status = ringB->Dequeue(message);
            VERIFY_IS_TRUE(KInvariantLine != 0);
        }
        
        //
        // Break lock for ringA and verify that activity continues
        //
        ringB->BreakOwner(ownerIdA);
        
        for (ULONG i = 0; i < 5; i++)
        {
            status = ringB->Dequeue(message);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            lifoA->Add(*message);
        }       
    }

    //
    // Test 4: Verify taking the lock twice fails
    //
    {
        KAWIpcSharedMemory::SPtr messageSharedMemory;
        KAWIpcSharedMemory::SPtr ringSharedMemory;
        KAWIpcLIFOList::SPtr lifoA;
        KAWIpcRing::SPtr ringA;
        LONG ownerIdA = (LONG)(0x80000000 | 107);
        LONG ownerIdB = (LONG)(0x80000000 | 108);

        status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                            256 * 1024, messageSharedMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                            4 * 4096, ringSharedMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcLIFOList::CreateAndInitialize(*messageSharedMemory,
                                                     *g_Allocator,
                                                     KTL_TAG_TEST,
                                                     lifoA);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcRing::CreateAndInitialize(ownerIdA,
                                                 *ringSharedMemory,
                                                 *messageSharedMemory,
                                                 *g_Allocator,
                                                 KTL_TAG_TEST,
                                                 ringA);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcRing::Create(ownerIdB,
                                                 *ringSharedMemory,
                                                 *messageSharedMemory,
                                                 *g_Allocator,
                                                 KTL_TAG_TEST,
                                                 ringA);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        {
            ResetKInvariantCallout();
            PreviousKInvariantCallout = SetKInvariantCallout(MyKInvariantCallout);
            KFinally([&](){  SetKInvariantCallout(PreviousKInvariantCallout); });

            status = ringA->Lock();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            status = ringA->Lock();
            VERIFY_IS_TRUE(KInvariantLine != 0);
            VERIFY_IS_TRUE(status == STATUS_INVALID_LOCK_SEQUENCE);

            ringA->Unlock();
        }
    }

    //
    // Test 5: Perform lots of operations in a multithreaded way
    //
    {
        KAWIpcSharedMemory::SPtr messageSharedMemory;
        KAWIpcSharedMemory::SPtr ringSharedMemory;
        KAWIpcLIFOList::SPtr lifoA;
        KAWIpcRing::SPtr ringA;
        LONG ownerId = (LONG)(0x80000000 | 102);

        status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                            1024 * 1024, messageSharedMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                            16 * 0x1000, ringSharedMemory);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcLIFOList::CreateAndInitialize(*messageSharedMemory,
                                                     *g_Allocator,
                                                     KTL_TAG_TEST,
                                                     lifoA);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = KAWIpcRing::CreateAndInitialize(ownerId,
                                                 *ringSharedMemory,
                                                 *messageSharedMemory,
                                                 *g_Allocator,
                                                 KTL_TAG_TEST,
                                                 ringA);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
                
        static const ULONG taskCount = 256;
        static const ULONG messageCount = 65536;
        ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP[taskCount];
        ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC[taskCount];
        
        for (ULONG i = 0; i < taskCount; i++)
        {
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            LIFORingProducerTask(i, messageCount, *messageSharedMemory, *ringSharedMemory, *acsP[i]);

            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            LIFORingConsumerTask(3 * i, messageCount, *messageSharedMemory, *ringSharedMemory, *acsC[i]);
        }

        for (ULONG i = 0; i < taskCount; i++)
        {
            status = SyncAwait(acsP[i]->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            status = SyncAwait(acsC[i]->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }
    }
    
    return(status);
}

VOID CreateLIFOAllocator(
    __in ULONG MessageSharedMemorySize,
    __in SHAREDMEMORYID MessageSharedMemoryId,
    __out KAWIpcSharedMemory::SPtr& MessageSharedMemoryR,
    __out KAWIpcMessageAllocator::SPtr& MallocR
    )
{
    NTSTATUS status;
    KAWIpcSharedMemory::SPtr messageSharedMemoryR;  
    KAWIpcLIFOList::SPtr lifoR;
    KAWIpcMessageAllocator::SPtr mallocR;
    
    status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                        MessageSharedMemoryId, 0, MessageSharedMemorySize, 0,
                                        messageSharedMemoryR);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = KAWIpcLIFOList::Create(*messageSharedMemoryR,
                                    *g_Allocator,
                                    KTL_TAG_TEST,
                                    lifoR);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = KAWIpcMessageAllocator::Create(*g_Allocator, KTL_TAG_TEST, mallocR);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = SyncAwait(mallocR->OpenAsync(*lifoR));
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    MessageSharedMemoryR = Ktl::Move(messageSharedMemoryR);
    MallocR = Ktl::Move(mallocR);
}

VOID CreateAndInitializeLIFOAllocator(
    __in ULONG MessageSharedMemorySize,
    __out SHAREDMEMORYID& MessageSharedMemoryId,
    __out KAWIpcSharedMemory::SPtr& MessageSharedMemoryR,
    __out KAWIpcMessageAllocator::SPtr& MallocR,
    __out ULONG& Count64,
    __out ULONG& Count1024,
    __out ULONG& Count4096,
    __out ULONG& Count64k
    )
{
    NTSTATUS status;
    KAWIpcSharedMemory::SPtr messageSharedMemoryR;  
    KAWIpcLIFOList::SPtr lifoR;
    KAWIpcMessageAllocator::SPtr mallocR;
    
    status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                        MessageSharedMemorySize, messageSharedMemoryR);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = KAWIpcLIFOList::CreateAndInitialize(*messageSharedMemoryR,
                                                 *g_Allocator,
                                                 KTL_TAG_TEST,
                                                 lifoR);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KAWIpcMessageAllocator::Create(*g_Allocator, KTL_TAG_TEST, mallocR);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    status = SyncAwait(mallocR->OpenAsync(*lifoR));
    VERIFY_IS_TRUE(NT_SUCCESS(status)); 
    
    MessageSharedMemoryId = messageSharedMemoryR->GetId();
    Count64 = lifoR->GetCount64();
    Count1024 = lifoR->GetCount1024();
    Count4096 = lifoR->GetCount4096();
    Count64k = lifoR->GetCount64k();
    MallocR = Ktl::Move(mallocR);
    MessageSharedMemoryR = Ktl::Move(messageSharedMemoryR);
}
VOID CreateCPRingPair(
    __in LONG OwnerIdC,
    __in LONG OwnerIdP,
    __in KAWIpcSharedMemory& MessageSharedMemoryC,
    __in KAWIpcSharedMemory& MessageSharedMemoryP,
    __in ULONG RingSharedMemorySize,
    __in PWCHAR EfdhText,
    __out ULONG& RingCount,
    __out KAWIpcRingConsumer::SPtr& ConsumerRing,
    __out KAWIpcRingProducer::SPtr& ProducerRing,
    __out SHAREDMEMORYID& RingSharedMemoryId,
    __out EVENTHANDLEFD& Efdh
    )
{
    NTSTATUS status;
    KAWIpcSharedMemory::SPtr messageSharedMemoryC = &MessageSharedMemoryC;
    KAWIpcSharedMemory::SPtr messageSharedMemoryP = &MessageSharedMemoryP;
    KAWIpcSharedMemory::SPtr ringSharedMemoryC;
    KAWIpcSharedMemory::SPtr ringSharedMemoryP;
    KAWIpcRing::SPtr ringC;
    KAWIpcRing::SPtr ringP;
    KAWIpcRingProducer::SPtr ringProducer;
    KAWIpcRingConsumer::SPtr ringConsumer;

    //
    // Create Ring objects
    //
    status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                        RingSharedMemorySize, ringSharedMemoryP);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    RingSharedMemoryId = ringSharedMemoryP->GetId();
    status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                        RingSharedMemoryId, 0, RingSharedMemorySize, 0,
                                        ringSharedMemoryC);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
        
    status = KAWIpcRing::CreateAndInitialize(OwnerIdP,
                                             *ringSharedMemoryP,
                                             *messageSharedMemoryP,
                                             *g_Allocator,
                                             KTL_TAG_TEST,
                                             ringP);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
        
    status = KAWIpcRing::Create(OwnerIdC,
                                *ringSharedMemoryC,
                                *messageSharedMemoryC,
                                *g_Allocator,
                                KTL_TAG_TEST,
                                ringC);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Allocate EventFDHandles
    //
    status = KAWIpcEventWaiter::CreateEventWaiter(EfdhText, Efdh);
    VERIFY_IS_TRUE(NT_SUCCESS(status));    

    //
    // Build RingConsumer and producer
    //
    status = KAWIpcRingProducer::Create(*g_Allocator,
                                        KTL_TAG_TEST,
                                        ringProducer);  
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = SyncAwait(ringProducer->OpenAsync(*ringP, Efdh));
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = KAWIpcRingConsumer::Create(*g_Allocator,
                                        KTL_TAG_TEST,
                                        ringConsumer);  
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = SyncAwait(ringConsumer->OpenAsync(*ringC, Efdh));
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    //
    // Return results
    //
    RingCount = ringP->GetNumberEntries();
    ConsumerRing = Ktl::Move(ringConsumer);
    ProducerRing = Ktl::Move(ringProducer);
}

VOID CreateCPPair(
    __in LONG OwnerIdC,
    __in LONG OwnerIdP,
    __in ULONG MessageSharedMemorySize,
    __in ULONG RingSharedMemorySize,
    __in PWCHAR EfdhText,
    __out ULONG& Count64,
    __out ULONG& Count1024,
    __out ULONG& Count4096,
    __out ULONG& Count64k,
    __out ULONG& RingCount,
    __out KAWIpcMessageAllocator::SPtr& MallocC,
    __out KAWIpcMessageAllocator::SPtr& MallocP,
    __out KAWIpcRingConsumer::SPtr& ConsumerRing,
    __out KAWIpcRingProducer::SPtr& ProducerRing,
    __out SHAREDMEMORYID& MessageSharedMemoryId,
    __out SHAREDMEMORYID& RingSharedMemoryId,
    __out EVENTHANDLEFD& Efdh
    )
{
    KAWIpcSharedMemory::SPtr messageSharedMemoryC;
    KAWIpcSharedMemory::SPtr messageSharedMemoryP;
    CreateAndInitializeLIFOAllocator(MessageSharedMemorySize, MessageSharedMemoryId, messageSharedMemoryP, MallocP, Count64, Count1024, Count4096, Count64k);
    CreateLIFOAllocator(MessageSharedMemorySize, MessageSharedMemoryId, messageSharedMemoryC, MallocC);
    CreateCPRingPair(OwnerIdC, OwnerIdP, *messageSharedMemoryC, *messageSharedMemoryP, RingSharedMemorySize,
                     EfdhText, RingCount, ConsumerRing, ProducerRing, RingSharedMemoryId,
                     Efdh); 
}

VOID CreateAdditionalP(
    __in LONG OwnerIdP,
    __in KAWIpcSharedMemory& MessageSharedMemoryP,
    __in SHAREDMEMORYID RingSharedMemoryId,
    __in ULONG RingSharedMemorySize,
    __in EVENTHANDLEFD Efdh,
    __out KAWIpcRingProducer::SPtr& ProducerRing
    )
{
    NTSTATUS status;
    KAWIpcSharedMemory::SPtr messageSharedMemoryP = &MessageSharedMemoryP;
    KAWIpcSharedMemory::SPtr ringSharedMemoryP;
    SHAREDMEMORYID ringSharedMemoryId;
    KAWIpcRing::SPtr ringP;
    KAWIpcRingProducer::SPtr ringProducer;
    
    //
    // Create Ring objects
    //
    status = KAWIpcSharedMemory::Create(*g_Allocator, KTL_TAG_TEST,
                                        RingSharedMemoryId, 0, RingSharedMemorySize, 0,
                                        ringSharedMemoryP);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    status = KAWIpcRing::Create(OwnerIdP,
                                *ringSharedMemoryP,
                                *messageSharedMemoryP,
                                *g_Allocator,
                                KTL_TAG_TEST,
                                ringP);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
        
    //
    // Build RingConsumer and producer
    //
    status = KAWIpcRingProducer::Create(*g_Allocator,
                                        KTL_TAG_TEST,
                                        ringProducer);  
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    status = SyncAwait(ringProducer->OpenAsync(*ringP, Efdh));
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    //
    // Return results
    //
    ProducerRing = Ktl::Move(ringProducer);
}

VOID CreateGuidText(
    __out KDynString& Name
    )
{

    NTSTATUS status = STATUS_SUCCESS;
    KStringView prefix(L"KAWIpcTest_");
    BOOLEAN b;
    KGuid guid;
    

    guid.CreateNew();
    b = Name.Concat(prefix);
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
    b = Name.FromGUID(guid, TRUE);
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    b = Name.SetNullTerminator();
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
}

class Decoupled
{
    public:
        Decoupled()
        {
            NTSTATUS status;
            _IsCompleted = FALSE;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, _Acs);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
        
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr _Acs;
    BOOLEAN _IsCompleted;
};

Task DecoupledAllocate(
    __in Decoupled& State,
    __in ULONG Size,
    __in KAWIpcMessageAllocator& Mallocr,
    __out KAWIpcMessage*& Message
  )
{
    NTSTATUS status = STATUS_SUCCESS;
    KAWIpcMessageAllocator::SPtr mallocr = &Mallocr;
    
    status = co_await mallocr->AllocateAsync(Size, Message);
    
    State._IsCompleted = TRUE;
    State._Acs->SetResult(status);
    co_return;
}

Task DecoupledRingFill(
    __in Decoupled& State,
    __in KAWIpcRingProducer& RingP,
    __in KAWIpcMessage* Message
  )
{
    NTSTATUS status = STATUS_SUCCESS;
    KAWIpcRingProducer::SPtr ringP = &RingP;

    status = co_await ringP->ProduceAsync(Message);            
    VERIFY_IS_TRUE(NT_SUCCESS(status));         
    
    State._IsCompleted = TRUE;
    State._Acs->SetResult(status);
    co_return;
}

Task CPRingProducerWithDelayTask(
    __in KAWIpcMessageAllocator& MallocP,
    __in KAWIpcRingProducer& RingP,
    __in ULONG Count,
    __in ULONG Delay,
    __in BOOLEAN ExitOnError,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs,
    __in ULONG id = 0
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs = &Acs;
    KAWIpcMessageAllocator::SPtr mallocP = &MallocP;
    KAWIpcRingProducer::SPtr ringP = &RingP;
    KAWIpcMessage* message;
    ULONG delay;
    KFinally([&](){  acs->SetResult(status); });

    //
    // Use timer to get on KTL thread
    //
    status = co_await KTimer::StartTimerAsync(*g_Allocator, KTL_TAG_TEST, 1, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    srand((ULONG)GetTickCount());
    
    for (ULONG i = 0; i < Count; i++)
    {
        if ((i % 5000) == 0)
        {
            printf("CPRingProducerWithDelayTask %d\n", i);
        }
        
        message = nullptr;
        status = co_await mallocP->AllocateAsync(64, message);
        VERIFY_IS_TRUE(NT_SUCCESS(status));         
        VERIFY_IS_TRUE(message != nullptr);

        status = co_await ringP->ProduceAsync(message);
        if (ExitOnError && (! NT_SUCCESS(status)))
        {
            mallocP->Free(message);
            co_return;
        }
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        if (Delay != 0)
        {
            if (Delay == (ULONG)-1)
            {
                delay = rand() % 10;
            } else {
                delay = Delay;
            }

            status = co_await KTimer::StartTimerAsync(*g_Allocator, KTL_TAG_TEST, delay, nullptr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        } 
    }
    co_return;
}

Task CPRingConsumerWithDelayTask(
    __in KAWIpcMessageAllocator& MallocC,
    __in KAWIpcRingConsumer& RingC,
    __in ULONG Count,
    __in ULONG Delay,
    __in BOOLEAN ExitOnError,
    __in BOOLEAN* DrainAndExit,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KAWIpcMessage* message;
    ULONG delay;
    KAWIpcMessageAllocator::SPtr mallocC = &MallocC;
    KAWIpcRingConsumer::SPtr ringC = &RingC;
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs = &Acs;
    KFinally([&](){  acs->SetResult(status); });
    
    //
    // Use timer to get on KTL thread
    //
    status = co_await KTimer::StartTimerAsync(*g_Allocator, KTL_TAG_TEST, 1, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    srand((ULONG)GetTickCount());
    
    for (ULONG i = 0; i < Count; i++)
    {
        if ((i % 5000) == 0)
        {
            printf("CPRingConsumerWithDelayTask %d\n", i);
        }

        if (*DrainAndExit && ringC->IsEmpty())
        {
            co_return;
        }
        
        message = nullptr;
        status = co_await ringC->ConsumeAsync(message);
        if (ExitOnError && (! NT_SUCCESS(status)))
        {
            co_return;
        }
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        mallocC->Free(message);
        
        if (Delay != 0)
        {
            if (Delay == (ULONG)-1)
            {
                delay = rand() % 10;
            } else {
                delay = Delay;
            }
            
            status = co_await KTimer::StartTimerAsync(*g_Allocator, KTL_TAG_TEST, delay, nullptr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        } 
    }   
    co_return;
}

Task CPRingProducerWithYieldTask(
    __in KAWIpcMessageAllocator& MallocP,
    __in KAWIpcRingProducer& RingP,
    __in ULONG Count,
    __in ULONG Y,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs = &Acs;
    KAWIpcMessageAllocator::SPtr mallocP = &MallocP;
    KAWIpcRingProducer::SPtr ringP = &RingP;
    KAWIpcMessage* message;
    ULONG count;
    KFinally([&](){  acs->SetResult(status); });
    
    //
    // Use timer to get on KTL thread
    //
    status = co_await KTimer::StartTimerAsync(*g_Allocator, KTL_TAG_TEST, 1, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    srand((ULONG)GetTickCount());

    count = 0;
    for (ULONG i = 0; i < Count; i++)
    {
        count++;
        message = nullptr;
        status = co_await mallocP->AllocateAsync(64, message);
        VERIFY_IS_TRUE(NT_SUCCESS(status));         
        VERIFY_IS_TRUE(message != nullptr);

        status = co_await ringP->ProduceAsync(message);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        if (Y == count)
        {
            KNt::Sleep(0);
            count = 0;
        }
    }   
    co_return;
}

Task CPRingConsumerWithYieldTask(
    __in KAWIpcMessageAllocator& MallocC,
    __in KAWIpcRingConsumer& RingC,
    __in ULONG Count,
    __in ULONG Y,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KAWIpcMessageAllocator::SPtr mallocC = &MallocC;
    KAWIpcRingConsumer::SPtr ringC = &RingC;
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs = &Acs;
    KAWIpcMessage* message;
    ULONG count;
    KFinally([&](){  acs->SetResult(status); });
    
    //
    // Use timer to get on KTL thread
    //
    status = co_await KTimer::StartTimerAsync(*g_Allocator, KTL_TAG_TEST, 1, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    
    srand((ULONG)GetTickCount());
    
    count = 0;
    for (ULONG i = 0; i < Count; i++)
    {
        count++;
        status = co_await ringC->ConsumeAsync(message);            
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        mallocC->Free(message);
        
        if (Y == count)
        {
            KNt::Sleep(0);
            count = 0;
        }
    }   
    co_return;
}

typedef struct
{
    ULONG ClientId;
    ULONG Value;
} ApiMessageData;

Task CPRingApiServer(
    __in ULONG ClientCount,
    __in KAWIpcRingConsumer& ApiConsumer,
    __in KAWIpcRingProducer::SPtr* ClientProducers,
    __in ULONG Delay,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
    )
{
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs = &Acs;
    NTSTATUS status = STATUS_SUCCESS;
    KFinally([&](){  acs->SetResult(status); });
    
    //
    // Use timer to get on KTL thread
    //
    status = co_await KTimer::StartTimerAsync(*g_Allocator, KTL_TAG_TEST, 1, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KAWIpcRingConsumer::SPtr apiConsumer = &ApiConsumer;
    KAWIpcMessage* message;
    ApiMessageData* messageData;
    ULONG clientId;
    ULONG delay;
    ULONG count = 0;

    do
    {
        if ((count % 5000) == 0)
        {
            printf("ApiServer %d\n", count);
        }
        
        message = nullptr;
        messageData = nullptr;
        
        status = co_await apiConsumer->ConsumeAsync(message);
        if (status == STATUS_CANCELLED)
        {
            status = STATUS_SUCCESS;
            break;
        }
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        messageData = (ApiMessageData*)&message->Data[0];

        clientId = messageData->ClientId;
        messageData->Value = (messageData->Value * 2) + 1;

        if (Delay != 0)
        {
            if (Delay == (ULONG)-1)
            {
                delay = rand() % 10;
            } else {
                delay = Delay;
            }
            status = co_await KTimer::StartTimerAsync(*g_Allocator, KTL_TAG_TEST, delay, nullptr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        } 
        
        status = co_await ClientProducers[clientId]->ProduceAsync(message);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        count++;
    } while (TRUE);

    co_return;
}

Task CPRingApiClient(
    __in ULONG EachClientCount,
    __in ULONG ClientId,
    __in KAWIpcRingProducer& RingProducer,
    __in KAWIpcRingConsumer& RingConsumer,
    __in KAWIpcMessageAllocator& MallocR,
    __in ULONG Delay,
    __in ktl::AwaitableCompletionSource<NTSTATUS>& Acs
    )
{
    ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acs = &Acs;
    NTSTATUS status = STATUS_SUCCESS;
    KFinally([&](){  acs->SetResult(status); });
    ULONG count = 0;
    
    //
    // Use timer to get on KTL thread
    //
    status = co_await KTimer::StartTimerAsync(*g_Allocator, KTL_TAG_TEST, 1, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status));

    KAWIpcRingProducer::SPtr ringProducer = &RingProducer;
    KAWIpcRingConsumer::SPtr ringConsumer = &RingConsumer;
    KAWIpcMessageAllocator::SPtr mallocR = &MallocR;
    KAWIpcMessage* messageOut;
    KAWIpcMessage* messageIn;
    ApiMessageData* messageData;
    ULONG delay;
    ULONG value;
    
    srand((ULONG)GetTickCount());

    for (ULONG i = 0; i < EachClientCount; i++)
    {
        messageOut = nullptr;
        messageIn = nullptr;
        
        status = co_await mallocR->AllocateAsync(64, messageOut);
        VERIFY_IS_TRUE(NT_SUCCESS(status));         
        VERIFY_IS_TRUE(messageOut != nullptr);

        value = (rand() % 0x3ffffff8);

        messageData = (ApiMessageData*)&messageOut->Data[0];
        messageData->ClientId = ClientId;
        messageData->Value = value;
        status = co_await ringProducer->ProduceAsync(messageOut);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        status = co_await ringConsumer->ConsumeAsync(messageIn);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        messageData = (ApiMessageData*)&messageIn->Data[0];
        VERIFY_IS_TRUE(messageIn == messageOut);
        VERIFY_IS_TRUE(messageData->ClientId == ClientId);
        VERIFY_IS_TRUE(messageData->Value == (value * 2) + 1);

        mallocR->Free(messageIn);
        
        if (Delay != 0)
        {
            if (Delay == (ULONG)-1)
            {
                delay = rand() % 10;
            } else {
                delay = Delay;
            }
            status = co_await KTimer::StartTimerAsync(*g_Allocator, KTL_TAG_TEST, delay, nullptr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
        count++;
    }
}

NTSTATUS
KAWIpcCPRingTests()
{
    NTSTATUS status = STATUS_SUCCESS;

#if defined(PLATFORM_UNIX)
    //
    // For Linux, remove any stray shared memory sections that might
    // have been left over due to a crash.
    //
    {
        ULONG count;
        status = SyncAwait(KAWIpcSharedMemory::CleanupFromPreviousCrashes(*g_Allocator, KTL_TAG_TEST));
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        count = GetNumberKAWIpcSharedMemorySections();
        VERIFY_IS_TRUE(count == 0);
    }
#endif

    //
    // Test 1: Create consumer and producer rings, produce a couple of
    //         things, consume a couple of things and verify
    //         everything works.
    //
    {
        static const LONG ownerIdC = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const ULONG messageSharedMemorySize = 192 * 1024;
        static const ULONG ringSharedMemorySize = 4096;
        static const ULONG counter = 5;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;
        
        KAWIpcMessageAllocator::SPtr mallocC;
        KAWIpcMessageAllocator::SPtr mallocP;
        KAWIpcRingConsumer::SPtr consumerRing;
        KAWIpcRingProducer::SPtr producerRing;
        KAWIpcMessage* message;
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efhd;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        CreateCPPair(ownerIdC, ownerIdP,
                     messageSharedMemorySize, ringSharedMemorySize,
                     (PWCHAR)efdhText,
                     count64, count1024, count4096, count64k, ringCount,
                     mallocC, mallocP,
                     consumerRing, producerRing,
                     messageSharedMemoryId, ringSharedMemoryId,
                     efhd);
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efhd); });

		KInvariant(count64 >= counter);

        printf("Test 1: %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);
        
        for (ULONG i = 0; i < counter; i++)
        {
            message = nullptr;
            status = SyncAwait(mallocP->AllocateAsync(64, message));
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            VERIFY_IS_TRUE(message != nullptr);

            message->Data[0] = (UCHAR)i;
            
            status = SyncAwait(producerRing->ProduceAsync(message));            
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }

        for (ULONG i = 0; i < counter; i++)
        {
            message = nullptr;
            status = SyncAwait(consumerRing->ConsumeAsync(message));            
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(message->Data[0] == (UCHAR)i);
            
            mallocC->Free(message);
        }

        //
        // Shutdown
        //
        status = SyncAwait(mallocC->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(mallocP->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = consumerRing->LockForClose();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(consumerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 2: Force producer to be throttled waiting for free messages
    //         to become available. Verify that the scenario works.
    //
    {
        static const LONG ownerIdC = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const ULONG messageSharedMemorySize = 256 * 1024;
        static const ULONG ringSharedMemorySize = 16 * 1024;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;
        
        KAWIpcMessageAllocator::SPtr mallocC;
        KAWIpcMessageAllocator::SPtr mallocP;
        KAWIpcRingConsumer::SPtr consumerRing;
        KAWIpcRingProducer::SPtr producerRing;
        KAWIpcMessage* message;
        KAWIpcMessage* messageF;
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efhd;     
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        CreateCPPair(ownerIdC, ownerIdP,
                     messageSharedMemorySize, ringSharedMemorySize,
                     efdhText,
                     count64, count1024, count4096, count64k, ringCount,
                     mallocC, mallocP,
                     consumerRing, producerRing,
                     messageSharedMemoryId, ringSharedMemoryId,
                     efhd);
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efhd); });

		KInvariant(ringCount > count64);
		
        printf("Test 2: %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);
        
        //
        // Allocate all messages from the lifo
        //
        for (ULONG i = 0; i < count64; i++)
        {
            message = nullptr;
            status = SyncAwait(mallocP->AllocateAsync(64, message));
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            VERIFY_IS_TRUE(message != nullptr);

            message->Data[0] = (UCHAR)i;
            
            status = SyncAwait(producerRing->ProduceAsync(message));            
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }


        //
        // Start a task to allocate another one which should block
        //
        Decoupled dx;        
        DecoupledAllocate(dx, 64, *mallocP, message);

        VERIFY_IS_TRUE(dx._IsCompleted == FALSE);
        KNt::Sleep(1024);
        VERIFY_IS_TRUE(dx._IsCompleted == FALSE);


        //
        // Release a message back and verify
        //
        {
            messageF = nullptr;
            status = SyncAwait(consumerRing->ConsumeAsync(messageF));            
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(messageF->Data[0] == 0);
            mallocC->Free(messageF);
        }
        

        KNt::Sleep(256 + 50);       
        VERIFY_IS_TRUE(dx._IsCompleted == TRUE);

        status = SyncAwait(dx._Acs->GetAwaitable());
        VERIFY_IS_TRUE(NT_SUCCESS(status));         
        
        
        message->Data[0] = (UCHAR)(count64);

        status = SyncAwait(producerRing->ProduceAsync(message));            
        VERIFY_IS_TRUE(NT_SUCCESS(status));         


        //
        // Drain messages
        //
        for (ULONG i = 0; i < count64; i++)
        {
            message = nullptr;
            status = SyncAwait(consumerRing->ConsumeAsync(message));            
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(message->Data[0] == (UCHAR)(i+1));
            
            mallocC->Free(message);
        }
        
        //
        // Shutdown
        //
        status = SyncAwait(mallocC->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(mallocP->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = consumerRing->LockForClose();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(consumerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 3: Force producer to be throttled waiting for free ring slots
    //         to become available. Verify that the scenario works.
    //
    {
        static const LONG ownerIdC = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;
        
        KAWIpcMessageAllocator::SPtr mallocC;
        KAWIpcMessageAllocator::SPtr mallocP;
        KAWIpcRingConsumer::SPtr consumerRing;
        KAWIpcRingProducer::SPtr producerRing;
        KAWIpcMessage* message;
        KAWIpcMessage* messageF;
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efhd;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        CreateCPPair(ownerIdC, ownerIdP,
                     messageSharedMemorySize, ringSharedMemorySize,
                     efdhText,
                     count64, count1024, count4096, count64k, ringCount,
                     mallocC, mallocP,
                     consumerRing, producerRing,
                     messageSharedMemoryId, ringSharedMemoryId,
                     efhd);
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efhd); });

		KInvariant(ringCount < count64);
		
        printf("Test 3: %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);
        
        //
        // Fill the ring
        //
        for (ULONG i = 0; i < ringCount; i++)
        {
            message = nullptr;
            status = SyncAwait(mallocP->AllocateAsync(64, message));
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            VERIFY_IS_TRUE(message != nullptr);

            message->Data[0] = (UCHAR)i;
            
            status = SyncAwait(producerRing->ProduceAsync(message));            
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }

        //
        // Start a task to produce another one which should block
        //
        message = nullptr;
        status = SyncAwait(mallocP->AllocateAsync(64, message));
        VERIFY_IS_TRUE(NT_SUCCESS(status));         
        VERIFY_IS_TRUE(message != nullptr);

        message->Data[0] = (UCHAR)ringCount;
        
        Decoupled dx;        
        DecoupledRingFill(dx, *producerRing, message);

        VERIFY_IS_TRUE(dx._IsCompleted == FALSE);
        KNt::Sleep(1024);
        VERIFY_IS_TRUE(dx._IsCompleted == FALSE);

        //
        // Release a message back and verify
        //
        {
            messageF = nullptr;
            status = SyncAwait(consumerRing->ConsumeAsync(messageF));            
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(messageF->Data[0] == 0);
            mallocC->Free(messageF);
        }
        
        KNt::Sleep(256 + 50);       
        VERIFY_IS_TRUE(dx._IsCompleted == TRUE);

        status = SyncAwait(dx._Acs->GetAwaitable());
        VERIFY_IS_TRUE(NT_SUCCESS(status));         
        
        //
        // Drain messages
        //
        for (ULONG i = 0; i < ringCount; i++)
        {
            message = nullptr;
            status = SyncAwait(consumerRing->ConsumeAsync(message));            
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(message->Data[0] == (UCHAR)(i+1));
            
            mallocC->Free(message);
        }               
        
        //
        // Shutdown
        //
        status = SyncAwait(mallocC->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(mallocP->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = consumerRing->LockForClose();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(consumerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    
    //
    // Test 4: Create consumer and producer rings, and produce much
    //         faster than consuming. LIFO > Ring
    //
    {
        static const ULONG totalCount = 32768;
        static const LONG ownerIdC = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 4 * 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;
        
        KAWIpcMessageAllocator::SPtr mallocC;
        KAWIpcMessageAllocator::SPtr mallocP;
        KAWIpcRingConsumer::SPtr consumerRing;
        KAWIpcRingProducer::SPtr producerRing;
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efhd;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        CreateCPPair(ownerIdC, ownerIdP,
                     messageSharedMemorySize, ringSharedMemorySize,
                     efdhText,
                     count64, count1024, count4096, count64k, ringCount,
                     mallocC, mallocP,
                     consumerRing, producerRing,
                     messageSharedMemoryId, ringSharedMemoryId,
                     efhd);
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efhd); });

        VERIFY_IS_TRUE(count64 > ringCount);
        VERIFY_IS_TRUE(totalCount > 2 * ringCount);
        
        printf("Test 4: Ring full %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);

        {
            printf("Test 4a: Producer is faster than consumer\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            BOOLEAN drainAndExit = FALSE;
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, 1, FALSE, &drainAndExit, *acsC);
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, 0, FALSE, *acsP);
            
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }

        {
            printf("Test 4b: Producer is faster than consumer\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            BOOLEAN drainAndExit = FALSE;
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, 0, FALSE, *acsP);
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, 3, FALSE, &drainAndExit, *acsC);
            
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }

        {
            printf("Test 4c: Producer yields but consumer does not\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            CPRingConsumerWithYieldTask(*mallocC, *consumerRing, totalCount, 32, *acsC);
            CPRingProducerWithYieldTask(*mallocP, *producerRing, totalCount, 0, *acsP);
            
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }
        
        //
        // Shutdown
        //
        status = SyncAwait(mallocC->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(mallocP->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = consumerRing->LockForClose();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(consumerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }


    //
    // Test 4Prime: Create consumer and producer rings, and produce much
    //         faster than consuming.  Ring > LIFO
    //
    {
        static const ULONG totalCount = 32768;
        static const LONG ownerIdC = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 16 * 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;
        
        KAWIpcMessageAllocator::SPtr mallocC;
        KAWIpcMessageAllocator::SPtr mallocP;
        KAWIpcRingConsumer::SPtr consumerRing;
        KAWIpcRingProducer::SPtr producerRing;
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efhd;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        CreateCPPair(ownerIdC, ownerIdP,
                     messageSharedMemorySize, ringSharedMemorySize,
                     efdhText,
                     count64, count1024, count4096, count64k, ringCount,
                     mallocC, mallocP,
                     consumerRing, producerRing,
                     messageSharedMemoryId, ringSharedMemoryId,
                     efhd);
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efhd); });

        VERIFY_IS_TRUE(count64 < ringCount);
        
        printf("Test 4P: LIFO empty %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);

        {
            printf("Test 4Pa: Producer is faster than consumer\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            BOOLEAN drainAndExit = FALSE;
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, 1, FALSE, &drainAndExit, *acsC);
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, 0, FALSE, *acsP);
            
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }

        {
            printf("Test 4Pb: Producer is faster than consumer\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            BOOLEAN drainAndExit = FALSE;
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, 0, FALSE, *acsP);
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, 3, FALSE, &drainAndExit, *acsC);
            
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }

        {
            printf("Test 4Pc: Producer yields but consumer does not\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            CPRingConsumerWithYieldTask(*mallocC, *consumerRing, totalCount, 32, *acsC);
            CPRingProducerWithYieldTask(*mallocP, *producerRing, totalCount, 0, *acsP);
            
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }
        
        //
        // Shutdown
        //
        status = SyncAwait(mallocC->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(mallocP->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = consumerRing->LockForClose();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(consumerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    
    //
    // Test 5: Create consumer and producer rings, and consume much
    //         faster than producing.
    //  
    {
        static const ULONG totalCount = 32768;
        static const LONG ownerIdC = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 4 * 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;
        
        KAWIpcMessageAllocator::SPtr mallocC;
        KAWIpcMessageAllocator::SPtr mallocP;
        KAWIpcRingConsumer::SPtr consumerRing;
        KAWIpcRingProducer::SPtr producerRing;
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efhd;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        CreateCPPair(ownerIdC, ownerIdP,
                     messageSharedMemorySize, ringSharedMemorySize,
                     efdhText,
                     count64, count1024, count4096, count64k, ringCount,
                     mallocC, mallocP,
                     consumerRing, producerRing,
                     messageSharedMemoryId, ringSharedMemoryId,
                     efhd);
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efhd); });

        VERIFY_IS_TRUE(count64 > ringCount);
        VERIFY_IS_TRUE(totalCount > 2 * ringCount);
        
        printf("Test 5: %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);
        
        {
            printf("Test 5a: Consumer faster than producer\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            BOOLEAN drainAndExit = FALSE;
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, 1, FALSE, *acsP);
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, 0, FALSE, &drainAndExit, *acsC);

            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
            

        {
            printf("Test 5b: Consumer faster than producer\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            BOOLEAN drainAndExit = FALSE;
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, 0, FALSE, &drainAndExit, *acsC);
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, 3, FALSE, *acsP);
            
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }

        {           
            printf("Test 5c: Producer yields but consumer does not\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            CPRingProducerWithYieldTask(*mallocP, *producerRing, totalCount, 32, *acsP);
            CPRingConsumerWithYieldTask(*mallocC, *consumerRing, totalCount, 0, *acsC);
            
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }           
                
        //
        // Shutdown
        //
        status = SyncAwait(mallocC->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(mallocP->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = consumerRing->LockForClose();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(consumerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }


    //
    // Test 5Prime: Create consumer and producer rings, and consume much
    //         faster than producing.
    //  
    {
        static const ULONG totalCount = 32768;
        static const LONG ownerIdC = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 16 * 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;
        
        KAWIpcMessageAllocator::SPtr mallocC;
        KAWIpcMessageAllocator::SPtr mallocP;
        KAWIpcRingConsumer::SPtr consumerRing;
        KAWIpcRingProducer::SPtr producerRing;
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efhd;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        CreateCPPair(ownerIdC, ownerIdP,
                     messageSharedMemorySize, ringSharedMemorySize,
                     efdhText,
                     count64, count1024, count4096, count64k, ringCount,
                     mallocC, mallocP,
                     consumerRing, producerRing,
                     messageSharedMemoryId, ringSharedMemoryId,
                     efhd);
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efhd); });

        VERIFY_IS_TRUE(count64 < ringCount);
        
        printf("Test 5P: %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);
        
        {
            printf("Test 5Pa: Consumer faster than producer\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            BOOLEAN drainAndExit = FALSE;
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, 1, FALSE, *acsP);
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, 0, FALSE, &drainAndExit, *acsC);

            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }
            

        {
            printf("Test 5Pb: Consumer faster than producer\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            BOOLEAN drainAndExit = FALSE;
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, 0, FALSE, &drainAndExit, *acsC);
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, 3, FALSE, *acsP);
            
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }

        {           
            printf("Test 5Pc: Producer yields but consumer does not\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            CPRingProducerWithYieldTask(*mallocP, *producerRing, totalCount, 32, *acsP);
            CPRingConsumerWithYieldTask(*mallocC, *consumerRing, totalCount, 0, *acsC);
            
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }           
                
        //
        // Shutdown
        //
        status = SyncAwait(mallocC->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(mallocP->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = consumerRing->LockForClose();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(consumerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }

    //
    // Test 6: Create consumer and producer rings, and consume and
    // produce with random delays
    //  
    {
        static const ULONG totalCount = 32768;
        static const LONG ownerIdC = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 4 * 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;
        
        KAWIpcMessageAllocator::SPtr mallocC;
        KAWIpcMessageAllocator::SPtr mallocP;
        KAWIpcRingConsumer::SPtr consumerRing;
        KAWIpcRingProducer::SPtr producerRing;
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efhd;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        CreateCPPair(ownerIdC, ownerIdP,
                     messageSharedMemorySize, ringSharedMemorySize,
                     efdhText,
                     count64, count1024, count4096, count64k, ringCount,
                     mallocC, mallocP,
                     consumerRing, producerRing,
                     messageSharedMemoryId, ringSharedMemoryId,
                     efhd);
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efhd); });

        VERIFY_IS_TRUE(count64 > ringCount);
        
        printf("Test 6: %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);
        {
            printf("Test 6a: Consumer and producer have random delays\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            BOOLEAN drainAndExit = FALSE;
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, (ULONG)-1, FALSE, *acsP);
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, (ULONG)-1, FALSE, &drainAndExit, *acsC);
            
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
        }

        {
            printf("Test 6b: Consumer and producer have no delays\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            CPRingConsumerWithYieldTask(*mallocC, *consumerRing, totalCount, (ULONG)0, *acsC);
            CPRingProducerWithYieldTask(*mallocP, *producerRing, totalCount, (ULONG)0, *acsP);

            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));                     
        }
                
        //
        // Shutdown
        //
        status = SyncAwait(mallocC->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(mallocP->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = consumerRing->LockForClose();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(consumerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));     
    }
    
    //
    // Test 7: Setup a consumer/producer pair to emulate a call/return
    //         pattern where the return does a fixed operation on the
    //         data. No delays
    //
    {
        static const ULONG eachClientCount = 8192;
        static const ULONG clientCount = 64;
        static const LONG ownerIdApi = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const LONG ownerIdClient = 0x80000000 | 3;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 4096;
        static const ULONG ringSharedMemorySizeClient = 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;

        KAWIpcSharedMemory::SPtr messageSharedMemoryApi;
        KAWIpcMessageAllocator::SPtr mallocApi;
        KAWIpcRingProducer::SPtr producerRing;
        KAWIpcRingConsumer::SPtr apiConsumer;                      // api server consumes
        KAWIpcRingProducer::SPtr apiProducers[clientCount];        // client produces
        KAWIpcRingConsumer::SPtr clientConsumers[clientCount];     // client consumers
        KAWIpcRingProducer::SPtr clientProducers[clientCount];     // api server produces
        KAWIpcMessageAllocator::SPtr clientMalloc[clientCount];    // client uses
        EVENTHANDLEFD efdhClient[clientCount];
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efdh;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        //
        // Create API Server objects including global free list and API
        // server consumer ring. Note we will ignore the producer ring
        //
        CreateAndInitializeLIFOAllocator(messageSharedMemorySize, messageSharedMemoryId, messageSharedMemoryApi,
                                         mallocApi, count64, count1024, count4096, count64k);
        
        CreateCPRingPair(ownerIdApi, ownerIdP,
                         *messageSharedMemoryApi, *messageSharedMemoryApi,
                         ringSharedMemorySize,
                         efdhText,
                         ringCount,
                         apiConsumer, producerRing,
                         ringSharedMemoryId,
                         efdh);        
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efdh); });

		KInvariant(count64 > ringCount);
		
        printf("Test 7: %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);

        //
        // Create separate message allocators and producer rings that
        // are used by the clients. Also create the CP pair used for
        // API Server to respond back to the clients
        //
        for (ULONG i = 0; i < clientCount; i++)
        {
            KAWIpcSharedMemory::SPtr messageSharedMemoryP;
            KAWIpcRingProducer::SPtr ringProducer;
            KDynString efdh2Text(*g_Allocator, 64);
            SHAREDMEMORYID ringClientSharedMemoryId;
            
            CreateGuidText(efdh2Text);

            CreateLIFOAllocator(messageSharedMemorySize, messageSharedMemoryId, messageSharedMemoryP, clientMalloc[i]);
            
            CreateAdditionalP(ownerIdClient+1,
                              *messageSharedMemoryP,
                              ringSharedMemoryId,
                              ringSharedMemorySize,
                              efdh,
                              apiProducers[i]);

            //
            // This creates a pair where the API server is the producer
            // and the client is the consumer
            //
            CreateCPRingPair(ownerIdClient+1, ownerIdApi,
                             *messageSharedMemoryP, *messageSharedMemoryApi, 
                             ringSharedMemorySizeClient,
                             efdh2Text,
                             ringCount,
                             clientConsumers[i], ringProducer,
                             ringClientSharedMemoryId,
                             efdhClient[i]);
            clientProducers[i] = Ktl::Move(ringProducer);
        }
        
        {
            ULONGLONG startTicks, endTicks;
            float opsPerMs;
            
            printf("Test 7a: ApiServer no waiting\n");
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP[clientCount];

            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            for (ULONG i = 0; i < clientCount; i++)
            {
                status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP[i]);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            startTicks = GetTickCount64();
            CPRingApiServer(clientCount, *apiConsumer, &clientProducers[0], 0, *acsC);
            for (ULONG i = 0; i < clientCount; i++)
            {
                CPRingApiClient(eachClientCount, i, *(apiProducers[i]), *(clientConsumers[i]), *(clientMalloc[i]), 0, *(acsP[i]));
            }

            //
            // Wait for api clients to finish
            //
            for (ULONG i = 0; i < clientCount; i++)
            {
                status = SyncAwait(acsP[i]->GetAwaitable());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }
            
            //
            // Shutdown api server
            //
            status = apiConsumer->LockForClose();               
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = SyncAwait(apiConsumer->CloseAsync());
            VERIFY_IS_TRUE(NT_SUCCESS(status));     
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            endTicks = GetTickCount64();

            float ops, ms;
            ops = (float)(eachClientCount * clientCount);
            ms = (float)(endTicks - startTicks);
            opsPerMs = ops / ms;
            printf("%f operations in %f ms %f ops/ms\n", ops, ms, opsPerMs);
            
            for (ULONG i = 0; i < clientCount; i++)
            {
                status = SyncAwait(clientProducers[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                
                status = SyncAwait(apiProducers[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = clientConsumers[i]->LockForClose();                
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                status = SyncAwait(clientConsumers[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = SyncAwait(clientMalloc[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                KAWIpcEventWaiter::CleanupEventWaiter(efdhClient[i]);
            }           
        }
        
        //
        // Shutdown
        //
        status = SyncAwait(mallocApi->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }


    //
    // Test 7b: Setup a consumer/producer pair to emulate a call/return
    //         pattern where the return does a fixed operation on the
    //         data. Consumer delays.
    //
    {
        static const ULONG eachClientCount = 1024;
        static const ULONG clientCount = 64;
        static const LONG ownerIdApi = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const LONG ownerIdClient = 0x80000000 | 3;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 4096;
        static const ULONG ringSharedMemorySizeClient = 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;

        KAWIpcSharedMemory::SPtr messageSharedMemoryApi;
        KAWIpcMessageAllocator::SPtr mallocApi;
        KAWIpcRingProducer::SPtr producerRing;
        KAWIpcRingConsumer::SPtr apiConsumer;                      // api server consumes
        KAWIpcRingProducer::SPtr apiProducers[clientCount];        // client produces
        KAWIpcRingConsumer::SPtr clientConsumers[clientCount];     // client consumers
        KAWIpcRingProducer::SPtr clientProducers[clientCount];     // api server produces
        KAWIpcMessageAllocator::SPtr clientMalloc[clientCount];    // client uses
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efdh;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        //
        // Create API Server objects including global free list and API
        // server consumer ring. Note we will ignore the producer ring
        //
        CreateAndInitializeLIFOAllocator(messageSharedMemorySize, messageSharedMemoryId, messageSharedMemoryApi,
                                         mallocApi, count64, count1024, count4096, count64k);
        
        CreateCPRingPair(ownerIdApi, ownerIdP,
                         *messageSharedMemoryApi, *messageSharedMemoryApi,
                         ringSharedMemorySize,
                         efdhText,
                         ringCount,
                         apiConsumer, producerRing,
                         ringSharedMemoryId,
                         efdh);        

        printf("Test 7b: %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);

        //
        // Create separate message allocators and producer rings that
        // are used by the clients. Also create the CP pair used for
        // API Server to respond back to the clients
        //
        for (ULONG i = 0; i < clientCount; i++)
        {
            KAWIpcSharedMemory::SPtr messageSharedMemoryP;
            KAWIpcRingProducer::SPtr ringProducer;
            KDynString efdh2Text(*g_Allocator, 64);
            SHAREDMEMORYID ringClientSharedMemoryId;
            EVENTHANDLEFD efdhClient;
            
            CreateGuidText(efdh2Text);

            CreateLIFOAllocator(messageSharedMemorySize, messageSharedMemoryId, messageSharedMemoryP, clientMalloc[i]);
            
            CreateAdditionalP(ownerIdClient+1,
                              *messageSharedMemoryP,
                              ringSharedMemoryId,
                              ringSharedMemorySize,
                              efdh,
                              apiProducers[i]);

            //
            // This creates a pair where the API server is the producer
            // and the client is the consumer
            //
            CreateCPRingPair(ownerIdClient+1, ownerIdApi,
                             *messageSharedMemoryP, *messageSharedMemoryApi, 
                             ringSharedMemorySizeClient,
                             efdh2Text,
                             ringCount,
                             clientConsumers[i], ringProducer,
                             ringClientSharedMemoryId,
                             efdhClient);
            clientProducers[i] = Ktl::Move(ringProducer);
        }
        
        {           
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP[clientCount];

            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            for (ULONG i = 0; i < clientCount; i++)
            {
                status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP[i]);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            CPRingApiServer(clientCount, *apiConsumer, &clientProducers[0], 3, *acsC);
            for (ULONG i = 0; i < clientCount; i++)
            {
                CPRingApiClient(eachClientCount, i, *(apiProducers[i]), *(clientConsumers[i]), *(clientMalloc[i]), 0, *(acsP[i]));
            }

            //
            // Wait for api clients to finish
            //
            for (ULONG i = 0; i < clientCount; i++)
            {
                status = SyncAwait(acsP[i]->GetAwaitable());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }
            
            //
            // Shutdown api server
            //
            status = apiConsumer->LockForClose();               
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = SyncAwait(apiConsumer->CloseAsync());
            VERIFY_IS_TRUE(NT_SUCCESS(status));     
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            
            for (ULONG i = 0; i < clientCount; i++)
            {
                status = SyncAwait(clientProducers[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                
                status = SyncAwait(apiProducers[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = clientConsumers[i]->LockForClose();                
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                status = SyncAwait(clientConsumers[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = SyncAwait(clientMalloc[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));             
            }           

        }
        
        //
        // Shutdown
        //
        status = SyncAwait(mallocApi->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Test 7c: Setup a consumer/producer pair to emulate a call/return
    //         pattern where the return does a fixed operation on the
    //         data. Producer delays.
    //
    {
        static const ULONG eachClientCount = 1024;
        static const ULONG clientCount = 64;
        static const LONG ownerIdApi = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const LONG ownerIdClient = 0x80000000 | 3;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 4096;
        static const ULONG ringSharedMemorySizeClient = 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;

        KAWIpcSharedMemory::SPtr messageSharedMemoryApi;
        KAWIpcMessageAllocator::SPtr mallocApi;
        KAWIpcRingProducer::SPtr producerRing;
        KAWIpcRingConsumer::SPtr apiConsumer;                      // api server consumes
        KAWIpcRingProducer::SPtr apiProducers[clientCount];        // client produces
        KAWIpcRingConsumer::SPtr clientConsumers[clientCount];     // client consumers
        KAWIpcRingProducer::SPtr clientProducers[clientCount];     // api server produces
        KAWIpcMessageAllocator::SPtr clientMalloc[clientCount];    // client uses
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efdh;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        //
        // Create API Server objects including global free list and API
        // server consumer ring. Note we will ignore the producer ring
        //
        CreateAndInitializeLIFOAllocator(messageSharedMemorySize, messageSharedMemoryId, messageSharedMemoryApi,
                                         mallocApi, count64, count1024, count4096, count64k);
        
        CreateCPRingPair(ownerIdApi, ownerIdP,
                         *messageSharedMemoryApi, *messageSharedMemoryApi,
                         ringSharedMemorySize,
                         efdhText,
                         ringCount,
                         apiConsumer, producerRing,
                         ringSharedMemoryId,
                         efdh);        

        printf("Test 7b: %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);

        //
        // Create separate message allocators and producer rings that
        // are used by the clients. Also create the CP pair used for
        // API Server to respond back to the clients
        //
        for (ULONG i = 0; i < clientCount; i++)
        {
            KAWIpcSharedMemory::SPtr messageSharedMemoryP;
            KAWIpcRingProducer::SPtr ringProducer;
            KDynString efdh2Text(*g_Allocator, 64);
            SHAREDMEMORYID ringClientSharedMemoryId;
            EVENTHANDLEFD efdhClient;
            
            CreateGuidText(efdh2Text);

            CreateLIFOAllocator(messageSharedMemorySize, messageSharedMemoryId, messageSharedMemoryP, clientMalloc[i]);
            
            CreateAdditionalP(ownerIdClient+1,
                              *messageSharedMemoryP,
                              ringSharedMemoryId,
                              ringSharedMemorySize,
                              efdh,
                              apiProducers[i]);

            //
            // This creates a pair where the API server is the producer
            // and the client is the consumer
            //
            CreateCPRingPair(ownerIdClient+1, ownerIdApi,
                             *messageSharedMemoryP, *messageSharedMemoryApi, 
                             ringSharedMemorySizeClient,
                             efdh2Text,
                             ringCount,
                             clientConsumers[i], ringProducer,
                             ringClientSharedMemoryId,
                             efdhClient);
            clientProducers[i] = Ktl::Move(ringProducer);
        }
        
        {           
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP[clientCount];

            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            for (ULONG i = 0; i < clientCount; i++)
            {
                status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP[i]);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            CPRingApiServer(clientCount, *apiConsumer, &clientProducers[0], 0, *acsC);
            for (ULONG i = 0; i < clientCount; i++)
            {
                CPRingApiClient(eachClientCount, i, *(apiProducers[i]), *(clientConsumers[i]), *(clientMalloc[i]), 3, *(acsP[i]));
            }

            //
            // Wait for api clients to finish
            //
            for (ULONG i = 0; i < clientCount; i++)
            {
                status = SyncAwait(acsP[i]->GetAwaitable());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }
            
            //
            // Shutdown api server
            //
            status = apiConsumer->LockForClose();               
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = SyncAwait(apiConsumer->CloseAsync());
            VERIFY_IS_TRUE(NT_SUCCESS(status));     
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE(NT_SUCCESS(status));         
            
            for (ULONG i = 0; i < clientCount; i++)
            {
                status = SyncAwait(clientProducers[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                
                status = SyncAwait(apiProducers[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = clientConsumers[i]->LockForClose();                
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                status = SyncAwait(clientConsumers[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = SyncAwait(clientMalloc[i]->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));             
            }           

        }
        
        //
        // Shutdown
        //
        status = SyncAwait(mallocApi->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    //
    // Test 8: Shutdown ring consumer while producer is running 
    //
    {
        static const ULONG totalCount = 32768;
        static const LONG ownerIdC = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 4 * 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;
        
        KAWIpcMessageAllocator::SPtr mallocC;
        KAWIpcMessageAllocator::SPtr mallocP;
        KAWIpcRingConsumer::SPtr consumerRing;
        KAWIpcRingProducer::SPtr producerRing;
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efhd;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        CreateCPPair(ownerIdC, ownerIdP,
                     messageSharedMemorySize, ringSharedMemorySize,
                     efdhText,
                     count64, count1024, count4096, count64k, ringCount,
                     mallocC, mallocP,
                     consumerRing, producerRing,
                     messageSharedMemoryId, ringSharedMemoryId,
                     efhd);
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efhd); });

        VERIFY_IS_TRUE(count64 > ringCount);
        VERIFY_IS_TRUE(totalCount > 2 * ringCount);
        
        printf("Test 8: Ring full %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);

        {
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            BOOLEAN drainAndExit = FALSE;
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, 0, TRUE, &drainAndExit, *acsC);
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, 0, TRUE, *acsP);

            KNt::Sleep(1);
            
            status = consumerRing->LockForClose();
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = SyncAwait(acsC->GetAwaitable());
            VERIFY_IS_TRUE((NT_SUCCESS(status)) ||
                           (status == STATUS_CANCELLED) ||
                           (status == STATUS_FILE_FORCED_CLOSED) || (status == K_STATUS_API_CLOSED));         
            status = SyncAwait(consumerRing->CloseAsync());
            VERIFY_IS_TRUE(NT_SUCCESS(status));     
            
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE((NT_SUCCESS(status)) ||
                           (status == STATUS_CANCELLED) ||
                           (status == STATUS_FILE_FORCED_CLOSED) || (status == K_STATUS_API_CLOSED));         
        }
        
        //
        // Shutdown
        //
        status = SyncAwait(mallocC->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(mallocP->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(producerRing->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
    
    //
    // Test 9: Shutdown ring producer while consumer is running 
    //
    {
        static const ULONG totalCount = 32768;
        static const LONG ownerIdC = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 4 * 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;
        
        KAWIpcMessageAllocator::SPtr mallocC;
        KAWIpcMessageAllocator::SPtr mallocP;
        KAWIpcRingConsumer::SPtr consumerRing;
        KAWIpcRingProducer::SPtr producerRing;
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efhd;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        CreateCPPair(ownerIdC, ownerIdP,
                     messageSharedMemorySize, ringSharedMemorySize,
                     efdhText,
                     count64, count1024, count4096, count64k, ringCount,
                     mallocC, mallocP,
                     consumerRing, producerRing,
                     messageSharedMemoryId, ringSharedMemoryId,
                     efhd);
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efhd); });

        VERIFY_IS_TRUE(count64 > ringCount);
        VERIFY_IS_TRUE(totalCount > 2 * ringCount);
        printf("Test 9: Ring full %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);

        {
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            BOOLEAN drainAndExit = FALSE;
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, 0, TRUE, &drainAndExit, *acsC);
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, 0, TRUE, *acsP);

            status = SyncAwait(producerRing->CloseAsync());
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE((NT_SUCCESS(status)) ||
                           (status == STATUS_CANCELLED) ||
                           (status == STATUS_FILE_FORCED_CLOSED) || (status == K_STATUS_API_CLOSED));         

            //
            // Expect ring to be empty
            //
            if (consumerRing->IsEmpty())
            {
                status = consumerRing->LockForClose();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                status = SyncAwait(consumerRing->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                
                status = SyncAwait(acsC->GetAwaitable());
                VERIFY_IS_TRUE((NT_SUCCESS(status)) ||
                               (status == STATUS_CANCELLED) ||
                               (status == STATUS_FILE_FORCED_CLOSED) || (status == K_STATUS_API_CLOSED));         
            } else {
                printf("Ring not empty -> unexpected\n");
                printf("Drain and exit\n");
                drainAndExit = TRUE;
                status = SyncAwait(acsC->GetAwaitable());
                VERIFY_IS_TRUE((NT_SUCCESS(status)) ||
                               (status == STATUS_CANCELLED) ||
                               (status == STATUS_FILE_FORCED_CLOSED) || (status == K_STATUS_API_CLOSED));
                
                status = consumerRing->LockForClose();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                status = SyncAwait(consumerRing->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }                                    
        }
        
        //
        // Shutdown
        //
        status = SyncAwait(mallocC->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(mallocP->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }

    //
    // Test 10: Shutdown ring producer while consumer is running 
    //
    {
        static const ULONG totalCount = 32768;
        static const LONG ownerIdC = 0x80000000 | 1;
        static const LONG ownerIdP = 0x80000000 | 2;
        static const ULONG messageSharedMemorySize = 1024 * 1024;
        static const ULONG ringSharedMemorySize = 4 * 4096;
        ULONG count64, count1024, count4096, count64k;
        ULONG ringCount;
        
        KAWIpcMessageAllocator::SPtr mallocC;
        KAWIpcMessageAllocator::SPtr mallocP;
        KAWIpcRingConsumer::SPtr consumerRing;
        KAWIpcRingProducer::SPtr producerRing;
        SHAREDMEMORYID messageSharedMemoryId, ringSharedMemoryId;
        EVENTHANDLEFD efhd;
        KDynString efdhText(*g_Allocator, 64);
        CreateGuidText(efdhText);

        CreateCPPair(ownerIdC, ownerIdP,
                     messageSharedMemorySize, ringSharedMemorySize,
                     efdhText,
                     count64, count1024, count4096, count64k, ringCount,
                     mallocC, mallocP,
                     consumerRing, producerRing,
                     messageSharedMemoryId, ringSharedMemoryId,
                     efhd);
        KFinally([&](){  KAWIpcEventWaiter::CleanupEventWaiter(efhd); });

        VERIFY_IS_TRUE(count64 > ringCount);
        VERIFY_IS_TRUE(totalCount > 2 * ringCount);
        printf("Test 9: Ring full %d count64 %d count1024 %d ring\n", count64, count1024, ringCount);

        {
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsC;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr acsP;
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsC);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = ktl::AwaitableCompletionSource<NTSTATUS>::Create(*g_Allocator, KTL_TAG_TEST, acsP);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            
            BOOLEAN drainAndExit = FALSE;
            CPRingConsumerWithDelayTask(*mallocC, *consumerRing, totalCount, 3, TRUE, &drainAndExit, *acsC);
            CPRingProducerWithDelayTask(*mallocP, *producerRing, totalCount, 0, TRUE, *acsP);

            KNt::Sleep(2000);
            
            status = SyncAwait(producerRing->CloseAsync());
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = SyncAwait(acsP->GetAwaitable());
            VERIFY_IS_TRUE((NT_SUCCESS(status)) ||
                           (status == STATUS_CANCELLED) ||
                           (status == STATUS_FILE_FORCED_CLOSED) || (status == K_STATUS_API_CLOSED));         

            //
            // Expect ring to be empty
            //
            if (consumerRing->IsEmpty())
            {
                printf("Ring empty -> unexpected\n");
                status = consumerRing->LockForClose();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                status = SyncAwait(consumerRing->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                
                status = SyncAwait(acsC->GetAwaitable());
                VERIFY_IS_TRUE((NT_SUCCESS(status)) ||
                               (status == STATUS_CANCELLED) ||
                               (status == STATUS_FILE_FORCED_CLOSED) || (status == K_STATUS_API_CLOSED));         
            } else {
                printf("Drain and exit\n");
                drainAndExit = TRUE;
                status = SyncAwait(acsC->GetAwaitable());
                VERIFY_IS_TRUE((NT_SUCCESS(status)) ||
                               (status == STATUS_CANCELLED) ||
                               (status == STATUS_FILE_FORCED_CLOSED) || (status == K_STATUS_API_CLOSED));
                
                status = consumerRing->LockForClose();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                status = SyncAwait(consumerRing->CloseAsync());
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }                                    
        }
        
        //
        // Shutdown
        //
        status = SyncAwait(mallocC->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        status = SyncAwait(mallocP->CloseAsync());
        VERIFY_IS_TRUE(NT_SUCCESS(status));

    }
    
    // TODO: Timing/performance
    
    return(STATUS_SUCCESS);
}

NTSTATUS
TestSequence()
{
    NTSTATUS Res = STATUS_SUCCESS;

    Res = KAWIpcHeapTests();
    if (!NT_SUCCESS(Res))
    {
        return(Res);
    }

    Res = KAWIpcLIFORingTests();
    if (!NT_SUCCESS(Res))
    {
        return(Res);
    }

    Res = KAWIpcPipeTests();
    if (!NT_SUCCESS(Res))
    {
        return(Res);
    }

    Res = KAWIpcSharedMemoryTests();
    if (!NT_SUCCESS(Res))
    {
        return(Res);
    }

    Res = KAWIpcEventTests();
    if (! NT_SUCCESS(Res))
    {
        return(Res);
    }

    Res = KAWIpcCPRingTests();
    if (!NT_SUCCESS(Res))
    {
        return(Res);
    }

    return Res;
}


NTSTATUS
KAWIpcTest(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS Result;
    KtlSystem* underlyingSystem;
    Result = KtlSystem::Initialize(&underlyingSystem);
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KtlSystem::Initializer() failure\n");
        return Result;
    }
    KFinally([&](){ KtlSystem::Shutdown(); });
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

	
    // Turn on strict allocation tracking
    underlyingSystem->SetStrictAllocationChecks(TRUE);

    
    EventRegisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
	
    
    // ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    Result = TestSequence();

    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Test failure\n");
    }

#if defined(PLATFORM_UNIX)
	KtlTraceUnregister();
#endif	
	
    EventUnregisterMicrosoft_Windows_KTL();
	
    return Result;
}


#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
main(int argc, WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
    std::vector<WCHAR*> args_vec(argc);
    WCHAR** args = (WCHAR**)args_vec.data();
    std::vector<std::wstring> wargs(argc);
    for (int iter = 0; iter < argc; iter++)
    {
        wargs[iter] = Utf8To16(cargs[iter]);
        args[iter] = (WCHAR*)(wargs[iter].data());
    }
#endif
    KAWIpcTest(argc, args);
return 0;
}
#endif



