/*++

Copyright (c) Microsoft Corporation

Module Name:

    KNetworkTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KNetworkTests.h.
    2. Add an entry to the gs_KuTestCases table in KNetworkTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KNetworkTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif

KAllocator* g_Allocator = nullptr;

BOOLEAN g_Loopback = FALSE;
BOOLEAN g_ServerOnly = FALSE;
BOOLEAN g_ClientOnly = FALSE;
BOOLEAN g_RunServer = TRUE;

wchar_t* g_LocalComputer = nullptr;
wchar_t* g_RemoteComputer = nullptr;
USHORT   g_ListenerPort = 0;


// {3AFEECFE-007E-4275-903E-97B7943AC7AC}
static const GUID CLIENT_GUID =
{ 0x3afeecfe, 0x7e, 0x4275, { 0x90, 0x3e, 0x97, 0xb7, 0x94, 0x3a, 0xc7, 0xac } };

//////////////////////////////////////////////////////////////////////////////////////////
//
// GetUnicast

#if KTL_USER_MODE
 #include <ws2tcpip.h>
 #include <ws2ipdef.h>
 #include <iphlpapi.h>
#else
 #include <netioapi.h>
#endif

void GetLocalAddresses()
{
    unsigned int i;

    NTSTATUS Result = 0;

    PMIB_UNICASTIPADDRESS_TABLE pipTable = NULL;

    Result = GetUnicastIpAddressTable(AF_INET, &pipTable);

    if (Result != STATUS_SUCCESS)
    {
        return;
    }

    for (i = 0; i < pipTable->NumEntries; i++)
    {
        MIB_IF_ROW2 InfoRow;
        RtlZeroMemory(&InfoRow, sizeof(InfoRow));
        InfoRow.InterfaceIndex = pipTable->Table[i].InterfaceIndex;
        Result = GetIfEntry2(&InfoRow);
        if (!NT_SUCCESS(Result))
        {
            return;
        }

        //
        // Skip loopback adapters
        //
        if (InfoRow.Type != IF_TYPE_ETHERNET_CSMACD && (InfoRow.Type != IF_TYPE_IEEE80211))
        {
            continue;
        }

        //
        // Eliminate network adapters not in UP state.
        // These adapters may have stale IP addresses from their last up.
        // Binding to them will fail.
        //
        if (InfoRow.OperStatus != IfOperStatusUp)
        {
            continue;
        }

        if (pipTable->Table[i].Address.si_family != AF_INET)
        {
            continue; // Filter out IPv6 for now
        }

        // Filter out various types of virtualized IP addresses.
        //
        if (memcmp((void*) L"Local Area Connection", (void *) InfoRow.Alias, 20) != 0 &&
            wcscmp(L"Wireless Network Connection", InfoRow.Alias) != 0)
        {
            continue;

        }

        PSOCKADDR tmp = (PSOCKADDR) &pipTable->Table[i].Address.Ipv4;
        SOCKADDR x;
        x = *tmp;
    }

    if (pipTable != NULL)
    {
        FreeMibTable(pipTable);
        pipTable = NULL;
    }
}


//////////////////////////////////////////////////////////////////////////////////////////

VOID
DumpBindingInfo(KNetBinding::SPtr Binding)
{
    KTestPrintf("---Begin BindingInfo---\n");
    KTestPrintf("   KNetworkEndpointsL\n");
    KStringView SenderUri = *Binding->GetRemoteNetworkEp()->GetUri();
    KStringView ReceiverUri = *Binding->GetLocalNetworkEp()->GetUri();
    KTestPrintf("      Sender   Net Ep = %S\n", LPCWSTR(SenderUri));
    KTestPrintf("      Receiver Net Ep = %S\n", LPCWSTR(ReceiverUri));

    // Dump the GUIDs too
    KLocalString<128> LocalGuidStr;
    KLocalString<128> RemoteGuidStr;

    GUID LocalGuid, RemoteGuid;

    Binding->GetLocalEp(LocalGuid);
    Binding->GetRemoteEp(RemoteGuid);
    LocalGuidStr.FromGUID(LocalGuid);
    RemoteGuidStr.FromGUID(RemoteGuid);
    LocalGuidStr.SetNullTerminator();
    RemoteGuidStr.SetNullTerminator();

    KTestPrintf("      Sender   GUID = %S\n", LPWSTR(RemoteGuidStr));
    KTestPrintf("      Receiver GUID = %S\n", LPWSTR(LocalGuidStr));
    KTestPrintf("---End Binding Info---\n\n");
}


//////////////////////////////////////////////////////////////////////////////////////////
//
//  Test Object


static const ULONG MAX_STR = 256;

static const GUID GUID_TestObject =
{ 0x001ad969, 0x6206, 0x4811, { 0xa5, 0xb2, 0xc0, 0xc7, 0xb8, 0xcd, 0x8e, 0x31 } };

struct TestObject
{
    TestObject() { v1 = 0; v2 = 0; RtlZeroMemory(&StrVal[0], MAX_STR); ShutdownServer = FALSE; }

    TestObject(ULONG x, ULONG y, wchar_t* Src)
    {
        v1 = x;
        v2 = y;
        SetStr(Src);
        ShutdownServer = FALSE;
    }

    ULONG get_v1() { return v1; }
    ULONG get_v2() { return v2; }


    VOID SetStr(
        __in wchar_t* Src
        )
    {
        wcscpy_s(StrVal, MAX_STR, Src);
    }

    wchar_t* GetStr()
    {
        return StrVal;
    }

    ULONG v1;
    ULONG v2;
    BOOLEAN ShutdownServer;
    wchar_t StrVal[MAX_STR];

    K_SERIALIZABLE(TestObject);
};

K_CONSTRUCT(TestObject)
{
    UNREFERENCED_PARAMETER(In);

    _This = _new(KTL_TAG_TEST, *g_Allocator) TestObject();
    return STATUS_SUCCESS;
}

K_SERIALIZE_POD_MESSAGE(TestObject, GUID_TestObject);

K_DESERIALIZE_POD_MESSAGE(TestObject, GUID_TestObject);






//////////////////////////////////////////////////////////////////////////////////////////
//
//  Test Object2



static const GUID GUID_TestObject2 =
{ 0x002d969, 0x6206, 0x4811, { 0xa5, 0xb2, 0xc0, 0xc7, 0xb8, 0xcd, 0x8e, 0x32 } };

struct TestObject2
{
    TestObject2() { SendReply = TRUE; ShutdownServer = FALSE; v1 = 0; v2 = 0; RtlZeroMemory(&StrVal[0], MAX_STR); RtlZeroMemory(&ReturnAddress[0], MAX_STR); }

    TestObject2(ULONG x, ULONG y, wchar_t* Src, wchar_t* ReturnAddress)
    {
        v1 = x;
        v2 = y;
        SetStr(Src);
        SetReturnAddress(ReturnAddress);
        SendReply = TRUE;
        ShutdownServer = FALSE;
    }

    ULONG get_v1() { return v1; }
    ULONG get_v2() { return v2; }


    VOID SetStr(
        __in wchar_t* Src
        )
    {
        wcscpy_s(StrVal, MAX_STR, Src);
    }

    VOID SetReturnAddress(
        __in wchar_t* Src
        )
    {
        wcscpy_s(ReturnAddress, MAX_STR, Src);
    }


    wchar_t* GetStr()
    {
        return StrVal;
    }

    wchar_t* GetReturnAddress()
    {
        return ReturnAddress;
    }

    ULONG v1;
    ULONG v2;
    BOOLEAN SendReply;
    BOOLEAN ShutdownServer;

    wchar_t StrVal[MAX_STR];
    wchar_t ReturnAddress[MAX_STR];

    K_SERIALIZABLE(TestObject2);
};

K_CONSTRUCT(TestObject2)
{
    UNREFERENCED_PARAMETER(In);

    _This = _new(KTL_TAG_TEST, *g_Allocator) TestObject2();
    return STATUS_SUCCESS;
}

K_SERIALIZE_POD_MESSAGE(TestObject2, GUID_TestObject2);

K_DESERIALIZE_POD_MESSAGE(TestObject2, GUID_TestObject2);



// TestObjec3
//
// This object canno be read by the server.  It is sent only as a special test case
// to ensure that the server can recover from an unknown object.
//

static const GUID GUID_TestObject3 =
{ 0xEEEEd969, 0x6206, 0x4811, { 0xa5, 0xb2, 0xc0, 0xc7, 0xb8, 0xcd, 0x8e, 0x31 } };

struct TestObject3
{
    TestObject3() { v1 = 0; v2 = 0; RtlZeroMemory(&StrVal[0], MAX_STR); ShutdownServer = FALSE; }

    TestObject3(ULONG x, ULONG y, wchar_t* Src)
    {
        v1 = x;
        v2 = y;
        SetStr(Src);
        ShutdownServer = FALSE;
    }

    ULONG get_v1() { return v1; }
    ULONG get_v2() { return v2; }


    VOID SetStr(
        __in wchar_t* Src
        )
    {
        wcscpy_s(StrVal, 8192, Src);
    }

    wchar_t* GetStr()
    {
        return StrVal;
    }

    ULONG v1;
    ULONG v2;
    BOOLEAN ShutdownServer;
    wchar_t StrVal[8192];

    K_SERIALIZABLE(TestObject3);
};

K_CONSTRUCT(TestObject3)
{
    UNREFERENCED_PARAMETER(In);

    _This = _new(KTL_TAG_TEST, *g_Allocator) TestObject3();
    return STATUS_SUCCESS;
}

K_SERIALIZE_POD_MESSAGE(TestObject3, GUID_TestObject3);

K_DESERIALIZE_POD_MESSAGE(TestObject3, GUID_TestObject3);



//
////////////////////////////////////////////////////////////////////////////////

class KOldServerTest
{
public:
    KNetwork::SPtr  _ServerNet;

    VOID
    ServerWriteCb(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

         KTestPrintf("ServerWriteCb callback: ");
        KDatagramWriteOp& WriteOp = (KDatagramWriteOp&) Op;
        NTSTATUS Res = WriteOp.Status();
        KTestPrintf("Status = %d 0x%X\n", Res, Res);
    }

    VOID
    ServerReceiver(
            __in const GUID& To,
            __in const GUID& From,
            __in const ULONG Flags,
            __in const PVOID ContextValue,
            __in KNetBinding::SPtr& Binding,
            __in KInDatagram::SPtr& Dgram
            )
    {
        UNREFERENCED_PARAMETER(To);
        UNREFERENCED_PARAMETER(From);
        UNREFERENCED_PARAMETER(Flags);
        UNREFERENCED_PARAMETER(ContextValue);

        KTestPrintf("\n\n** OBJECT ARRIVAL to SERVER\n");

        KWString StrFrom(KtlSystem::GlobalNonPagedAllocator());
        KWString StrTo(KtlSystem::GlobalNonPagedAllocator());
        Binding->GetRemoteEpUri(StrFrom);
        Binding->GetLocalEpUri(StrTo);

        KTestPrintf("  Object sent from {%S} to {%S}\n", PWSTR(StrFrom), PWSTR(StrTo));



        TestObject* Ptr = 0;
        NTSTATUS Result = Dgram->AcquireObject(Ptr);
        if (NT_SUCCESS(Result))
        {
            KTestPrintf("  Object v1=%d v2=%d str=%S\n", Ptr->get_v1(), Ptr->get_v2(), Ptr->GetStr());
        }
        else
        {
            KTestPrintf(">>>>>>>>> FAILED to get object\n");
            return;
        }

        KFinally( [&]()
        {
            if (Ptr->ShutdownServer)
            {
                g_RunServer = FALSE;
            }
            _delete(Ptr);
        });

        // Dump the info

        TestObject Test(Ptr->get_v1(), Ptr->get_v1()+10000, L"SERVER-ORIGINATED-REPLY");

        GUID RemoteGuid;
        GUID LocalGuid;
        Binding->GetRemoteEp(RemoteGuid);
        Binding->GetLocalEp(LocalGuid);

        // Reuse the incoming binding half the time or get a new one
        // half the time.

        static Alternator = 0;
        KNetBinding::SPtr ReplyBinding;

        if (Alternator)
        {
            ReplyBinding = Binding;
            Alternator = 0;
        }
        else
        {
            _ServerNet->GetBinding(RemoteGuid, LocalGuid, 0, ReplyBinding);
            Alternator++;
        }

        DumpBindingInfo(ReplyBinding);

        // Now set up a write
        //
        KDatagramWriteOp::SPtr Write;
        Result = ReplyBinding->CreateDatagramWriteOp(
            Write,
            KTL_TAG_TEST
            );

        if (!NT_SUCCESS(Result))
        {
            KTestPrintf("Failure!\n");
            g_RunServer = FALSE;
            return;
        }

        // Now send a test object
        //

        Result = Write->StartWrite(
            Test,
            0,
            KAsyncContextBase::CompletionCallback(this, &KOldServerTest::ServerWriteCb),
            NULL,
            KtlNetPriorityNormal,
            KActivityId(101),
            500000
            );


        if (Result != STATUS_PENDING)
        {
            g_RunServer = FALSE;
            KTestPrintf("Unable to start a write!\n");
            return;
        }
    }


    //
    //
    //
    NTSTATUS
    SetupServer(KUriView ServerUri)
    {
        NTSTATUS Result;

        // Create a network instance
        //
        Result = KNetwork::Create(
            0,
            *g_Allocator,
            _ServerNet
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Register a root local endpoint
        //
        KGuid RootEp;
        RootEp.CreateNew();

        KLocalString<256> UriStr;
        UriStr.CopyFrom(KStringView(ServerUri));
        UriStr.SetNullTerminator();

        KWString LocalRoot(KtlSystem::GlobalNonPagedAllocator(), UriStr);

        Result = _ServerNet->RegisterEp(LocalRoot, KNetwork::eLocalEp|KNetwork::eRemoteEp, RootEp);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Now, we register an EP and sit around and wait for messages.
        //
        Result = _ServerNet->RegisterReceiver<TestObject>(
            RootEp,
            GUID_TestObject,
            0,
            0,
            KNetwork::ReceiverType(this, &KOldServerTest::ServerReceiver)
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        return STATUS_SUCCESS;
    }

    VOID
    Cleanup()
    {
        KTestPrintf("Server Cleanup...\n");
        if (_ServerNet)
        {
            _ServerNet->Shutdown();
        }

        KNt::Sleep(100);
    }

    NTSTATUS
    Run(KUriView ServerUri)
    {
        NTSTATUS Result;
        Result = SetupServer(ServerUri);

        while (g_RunServer)
        {
            KNt::Sleep(10000);
        }
        Cleanup();
        return Result;
    }
};




//
////////////////////////////////////////////////////////////////////////////////

class KNewServerTest
{
public:
    KNetwork::SPtr  _ServerNet;
    KNetworkEndpoint::SPtr _ThisEp;

    VOID
    ServerWriteCb(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

         KTestPrintf("ServerWriteCb callback: ");
        KDatagramWriteOp& WriteOp = (KDatagramWriteOp&) Op;
        NTSTATUS Res = WriteOp.Status();
        KTestPrintf("Status = %d 0x%X\n", Res, Res);
    }

    VOID
    Test2Receiver(
            __in KNetBinding::SPtr& Binding,
            __in KInDatagram::SPtr& Dgram
            )
    {
        KTestPrintf("\n\n** Test2Receiver! OBJECT ARRIVAL to SERVER\n");

        KWString StrFrom(KtlSystem::GlobalNonPagedAllocator()), StrTo(KtlSystem::GlobalNonPagedAllocator());
        Binding->GetRemoteEpUri(StrFrom);
        Binding->GetLocalEpUri(StrTo);

        KTestPrintf("  TestObject2 sent from {%S} to {%S}\n", PWSTR(StrFrom), PWSTR(StrTo));



        TestObject2* Ptr = 0;
        NTSTATUS Result = Dgram->AcquireObject(Ptr);
        if (NT_SUCCESS(Result))
        {
            KTestPrintf("  Object v1=%d v2=%d str=%S\n", Ptr->get_v1(), Ptr->get_v2(), Ptr->GetStr());
        }
        else
        {
            KTestPrintf(">>>>>>>>> FAILED to get object\n");
            return;
        }

        KFinally( [&]()
        {
            if (Ptr->ShutdownServer)
            {
                g_RunServer = FALSE;
            }
            _delete(Ptr);
        }
        );

        // Maybe send a reply

        if (Ptr->SendReply == FALSE)
        {
            return;
        }

        TestObject2 Test(Ptr->get_v1(), Ptr->get_v1()+10000, L"SERVER-ORIGINATED-REPLY-TYPE-2", L"<no reply to>");

        // Reuse the incoming binding half the time or get a new one
        // half the time.

        static Alternator = 0;
        KNetBinding::SPtr ReplyBinding;

        if (Alternator)
        {
            ReplyBinding = Binding;
            Alternator = 0;
        }
        else
        {
            // Get a new binding by extracting the return address and using
            // the local binding to this endpoint.

            ReplyBinding = Binding;
            //_ServerNet->GetBinding(RemoteGuid, LocalGuid, 0, ReplyBinding);
            Alternator++;
        }

        DumpBindingInfo(ReplyBinding);

        // Now set up a write
        //
        KDatagramWriteOp::SPtr Write;
        Result = ReplyBinding->CreateDatagramWriteOp(
            Write,
            KTL_TAG_TEST
            );

        if (!NT_SUCCESS(Result))
        {
            KTestPrintf("Failure!\n");
            g_RunServer = FALSE;
            return;
        }

        // Now send a test object
        //

        Result = Write->StartWrite(
            Test,
            0,
            KAsyncContextBase::CompletionCallback(this, &KNewServerTest::ServerWriteCb),
            NULL,
            KtlNetPriorityNormal,
            KActivityId(101),
            500000
            );


        if (Result != STATUS_PENDING)
        {
            g_RunServer = FALSE;
            KTestPrintf("Unable to start a write!\n");
            return;
        }
    }


    VOID
    TestReceiver(
            __in KNetBinding::SPtr& Binding,
            __in KInDatagram::SPtr& Dgram
            )
    {
        KTestPrintf("\n\n** TestReceiver! OBJECT ARRIVAL to SERVER\n");

        KWString StrFrom(KtlSystem::GlobalNonPagedAllocator()), StrTo(KtlSystem::GlobalNonPagedAllocator());
        Binding->GetRemoteEpUri(StrFrom);
        Binding->GetLocalEpUri(StrTo);

        KTestPrintf("  TestObject sent from {%S} to {%S}\n", PWSTR(StrFrom), PWSTR(StrTo));



        TestObject* Ptr = 0;
        NTSTATUS Result = Dgram->AcquireObject(Ptr);
        if (NT_SUCCESS(Result))
        {
            KTestPrintf("  Object v1=%d v2=%d str=%S\n", Ptr->get_v1(), Ptr->get_v2(), Ptr->GetStr());
        }
        else
        {
            KTestPrintf(">>>>>>>>> FAILED to get object\n");
            return;
        }

        KFinally( [&]()
        {
            if (Ptr->ShutdownServer)
            {
                g_RunServer = FALSE;
            }
            _delete(Ptr);
        }
        );


        TestObject Test(Ptr->get_v1(), Ptr->get_v1()+10000, L"SERVER-ORIGINATED-REPLY-TYPE-1");

        // Reuse the incoming binding half the time or get a new one
        // half the time.

        static Alternator = 0;
        KNetBinding::SPtr ReplyBinding;

        if (Alternator)
        {
            ReplyBinding = Binding;
            Alternator = 0;
        }
        else
        {
            // Get a new binding by extracting the return address and using
            // the local binding to this endpoint.

            ReplyBinding = Binding;
            //_ServerNet->GetBinding(RemoteGuid, LocalGuid, 0, ReplyBinding);
            Alternator++;
        }

        DumpBindingInfo(ReplyBinding);

        // Now set up a write
        //
        KDatagramWriteOp::SPtr Write;
        Result = ReplyBinding->CreateDatagramWriteOp(
            Write,
            KTL_TAG_TEST
            );

        if (!NT_SUCCESS(Result))
        {
            KTestPrintf("Failure!\n");
            g_RunServer = FALSE;
            return;
        }

        // Now send a test object
        //

        Result = Write->StartWrite(
            Test,
            0,
            KAsyncContextBase::CompletionCallback(this, &KNewServerTest::ServerWriteCb),
            NULL,
            KtlNetPriorityNormal,
            KActivityId(101),
            500000
            );


        if (Result != STATUS_PENDING)
        {
            g_RunServer = FALSE;
            KTestPrintf("Unable to start a write!\n");
            return;
        }
    }


    //
    //
    //
    NTSTATUS
    SetupServer(KUriView ServerUri)
    {
        NTSTATUS Result;

        // Create a network instance
        //
        Result = KNetwork::Create(
            0,
            *g_Allocator,
            _ServerNet
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        // Register a 2nd gen receiver using the new technique.
        //
        // Resulting endpoint URI is the ServerUri plus this suffix.
        //
        KUriView uriView(L"Test");
        Result = _ServerNet->RegisterReceiver<TestObject2>(
            uriView,
            GUID_TestObject2,
            0,
            KNetwork::KNetReceiverType(this, &KNewServerTest::Test2Receiver),
            _ThisEp
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        // Register a 2nd gen receiver using the new technique.
        //
        // Resulting URI is same as previous call above.
        //
        KDynString Tmp(*g_Allocator);
        Tmp.CopyFrom(ServerUri);
        Tmp.Concat(KStringView(L"/Test"));
        KUriView TestView = Tmp;

        KNetworkEndpoint::SPtr Alternate;
        Result = _ServerNet->RegisterReceiver<TestObject>(
            TestView,
            GUID_TestObject,
            0,
            KNetwork::KNetReceiverType(this, &KNewServerTest::TestReceiver),
            Alternate
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        if (!Alternate->IsEqual(_ThisEp))
        {
            return STATUS_INTERNAL_ERROR;
        }

        return STATUS_SUCCESS;
    }

    NTSTATUS
    Cleanup()
    {
        KTestPrintf("Server Cleanup...\n");

        NTSTATUS Result;
        Result = _ServerNet->UnregisterReceiver(GUID_TestObject2, _ThisEp);
        if (!NT_SUCCESS(Result))
        {
            return STATUS_INTERNAL_ERROR;
        }

        Result = _ServerNet->UnregisterReceiver(GUID_TestObject, _ThisEp);
        if (!NT_SUCCESS(Result))
        {
            return STATUS_INTERNAL_ERROR;
        }

        if (_ServerNet)
        {
            _ServerNet->Shutdown();
        }

        KNt::Sleep(100);
        return STATUS_SUCCESS;
    }

    NTSTATUS
    Run(KUriView ServerUri)
    {
        NTSTATUS Result;
        Result = SetupServer(ServerUri);

        while (g_RunServer)
        {
            KNt::Sleep(10000);
        }
        Cleanup();
        return Result;
    }
};


//
////////////////////////////////////////////////////////////////////////////////

class KOldClientTest
{
public:
    KNetwork::SPtr _ClientNet;
    KAutoResetEvent _InitialSend;
    KAutoResetEvent _ServerResponse;
    GUID            _ListenerEp;
    LONG            _ResponsesToProcess;

    VOID
    ClientReceiver(
            __in const GUID& To,
            __in const GUID& From,
            __in const ULONG Flags,
            __in const PVOID ContextValue,
            __in KNetBinding::SPtr& Binding,
            __in KInDatagram::SPtr& Dgram
            )
    {
        UNREFERENCED_PARAMETER(To);
        UNREFERENCED_PARAMETER(From);
        UNREFERENCED_PARAMETER(Flags);
        UNREFERENCED_PARAMETER(ContextValue);

        KTestPrintf("** OBJECT ARRIVAL to CLIENT\n");

        KWString StrFrom(KtlSystem::GlobalNonPagedAllocator()), StrTo(KtlSystem::GlobalNonPagedAllocator());
        Binding->GetRemoteEpUri(StrFrom);
        Binding->GetLocalEpUri(StrTo);

        KTestPrintf("  Object arrival from {%S} to local ep {%S}\n", PWSTR(StrFrom), PWSTR(StrTo));

        TestObject* Ptr;

        NTSTATUS Result = Dgram->AcquireObject(Ptr);
        if (NT_SUCCESS(Result))
        {
            KTestPrintf("  Object v1=%d v2=%d str=%S\n", Ptr->get_v1(), Ptr->get_v2(), Ptr->GetStr());
            _delete(Ptr);
        }
        else
        {
            KTestPrintf("FAILED to get object\n");
        }

        InterlockedDecrement(&_ResponsesToProcess);
        _ServerResponse.SetEvent();
    }



    //
    //
    //
    VOID
    ClientWriteCb(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

         KTestPrintf("ClientWriteCb: ");
        KDatagramWriteOp& WriteOp = (KDatagramWriteOp&) Op;
        NTSTATUS Res = WriteOp.Status();
        KTestPrintf("Status = %d 0x%X\n", Res, Res);
        if (NT_SUCCESS(Res))
        {
        }
        else
        {
            InterlockedDecrement(&_ResponsesToProcess);
            KTestPrintf("************** FAILED TO SEND\n");
        }
        _InitialSend.SetEvent();
    }

    //
    //
    //
    NTSTATUS RunClient(KUriView LocalClient, KUriView RemoteServer, ULONG ClientCheckVal)
    {
        NTSTATUS Result;
        KNetwork::SPtr Network;
        _ResponsesToProcess = 0;

        // Create a network instance
        //
        Result = KNetwork::Create(
            0,
            *g_Allocator,
            _ClientNet
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Register a root local endpoint
        //
        KGuid RootEp;
        RootEp.CreateNew();

        KLocalString<256> Tmp;
        Tmp.CopyFrom(LocalClient);
        Tmp.SetNullTerminator();

        KWString LocalRoot(KtlSystem::GlobalNonPagedAllocator(), Tmp);

        Result = _ClientNet->RegisterEp(LocalRoot, KNetwork::eLocalEp|KNetwork::eRemoteEp, RootEp);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        KWString DummyTest(KtlSystem::GlobalNonPagedAllocator(), L"rvd://1.2.3.4/a/b");
        KGuid DummyGuid;
        DummyGuid.CreateNew();
        Result = _ClientNet->RegisterEp(DummyTest, KNetwork::eRemoteEp, DummyGuid);


        // Register a destination endpoint for sending stuff.
        //
        KGuid DestEp;
        DestEp.CreateNew();

        Tmp.CopyFrom(RemoteServer);
        Tmp.SetNullTerminator();
        KWString RemoteRoot(KtlSystem::GlobalNonPagedAllocator(), Tmp);

        Result = _ClientNet->RegisterEp(RemoteRoot, KNetwork::eRemoteEp, DestEp);

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Register a nonexistent EP that we can try to send to, but the server
        // doesn't really have that URI. This is to check that the server
        // can silently read those messages and drop them without closing
        // the socket.

        KGuid BadEp;
        BadEp.CreateNew();

        Tmp.CopyFrom(RemoteServer);
        Tmp.Concat(L"/BadEp");
        Tmp.SetNullTerminator();
        KWString BadRemoteRoot(KtlSystem::GlobalNonPagedAllocator(), Tmp);

        Result = _ClientNet->RegisterEp(BadRemoteRoot, KNetwork::eRemoteEp, BadEp);

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        // Take address of deserializer


        // Now, we can send stuff.  But to receive we need a receiver.
        //
        Result = _ClientNet->RegisterReceiver<TestObject>(
            DestEp,
            GUID_TestObject,
            0,
            0,
            KNetwork::ReceiverType(this, &KOldClientTest::ClientReceiver)
            );


        // Get the binding
        //
        KNetBinding::SPtr Binding;

        Result = _ClientNet->GetBinding(
            DestEp,
            RootEp,
            0,
            Binding
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Get the 'bad' binding

        KNetBinding::SPtr BadBinding;

        Result = _ClientNet->GetBinding(
            BadEp,
            RootEp,
            0,
            BadBinding
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        // Send one object at a time and wait for response
        // continuing
        //
        // Loop through a few objects
        //
        for (ULONG Index = 1; Index < 5; Index++)
        {
            if (Index == 3)
            {
                // If here, we have a special case.
                // Send an object to a non-existent URI on the server.

                TestObject3 Test(ClientCheckVal, ClientCheckVal+100, L"CLIENT-SENT-INDECIPHERABLE-MESSAGE");

                // Now set up a write
                //
                KDatagramWriteOp::SPtr Write;
                Result = BadBinding->CreateDatagramWriteOp(
                    Write,
                    KTL_TAG_TEST
                    );

                if (!NT_SUCCESS(Result))
                {
                    return Result;
                }

                Result = Write->StartWrite(
                    Test,
                    0,
                    KAsyncContextBase::CompletionCallback(this, &KOldClientTest::ClientWriteCb),
                    NULL,
                    KtlNetPriorityNormal,
                    KActivityId(101),
                    500000
                    );


                if (Result != STATUS_PENDING)
                {
                    return Result;
                }

                InterlockedIncrement(&_ResponsesToProcess);
                _InitialSend.WaitUntilSet();
                KTestPrintf("Special Error Object Iteration [%u] completed\n", Index);
                KNt::Sleep(5000);

                continue;
            }



            // If here, it is a normal message being sent.
            //
            TestObject Test(ClientCheckVal, ClientCheckVal+100, L"CLIENT-SENT");

            // Now set up a write
            //
            KDatagramWriteOp::SPtr Write;
            Result = Binding->CreateDatagramWriteOp(
                Write,
                KTL_TAG_TEST
                );

            if (!NT_SUCCESS(Result))
            {
                return Result;
            }

            // Now send a test object
            //

            Result = Write->StartWrite(
                Test,
                0,
                KAsyncContextBase::CompletionCallback(this, &KOldClientTest::ClientWriteCb),
                NULL,
                KtlNetPriorityNormal,
                KActivityId(101),
                500000
                );


            if (Result != STATUS_PENDING)
            {
                return Result;
            }

            InterlockedIncrement(&_ResponsesToProcess);


            // Wait for write to complete befor letting binding go out of scope
            //
            _InitialSend.WaitUntilSet();

            KTestPrintf("Iteration [%u] completed\n", Index);

            KNt::Sleep(5000);
        }


        // Now we wait for a response messages.
        //
        _ServerResponse.WaitUntilSet();


        // Don't let binding go out of scope until idle

        while(__KNetwork_PendingOperations > 0)
        {
            KNt::Sleep(50);
            KTestPrintf("Retry loop while waiting for networks to shut down\n");
        }

        KNt::Sleep(5000);

        return STATUS_SUCCESS;
    }


    VOID
    Cleanup()
    {
        KTestPrintf("Cleanup...\n");
        if (_ClientNet)
        {
            _ClientNet->Shutdown();
        }

        KNt::Sleep(100);
    }

    NTSTATUS
    Run(KUriView LocalClient, KUriView RemoteServer, ULONG CheckVal)
    {
        NTSTATUS Result;
        Result = RunClient(LocalClient, RemoteServer, CheckVal);
        Cleanup();
        return Result;
    }
};

//////////////////////
//
// New style client

//
////////////////////////////////////////////////////////////////////////////////

// Client coded only to the new style and TestObject2
//

class KNewClientTest
{
public:
    KNetwork::SPtr  _ClientNet;
    KAutoResetEvent _InitialSend;
    KAutoResetEvent _ServerResponse;
    GUID            _ListenerEp;
    LONG            _ResponsesToProcess;

    KNetworkEndpoint::SPtr _ThisEp;
    KNetworkEndpoint::SPtr _RemoteEp;

    VOID
    ClientReceiver(
            __in KNetBinding::SPtr& Binding,
            __in KInDatagram::SPtr& Dgram
            )
    {
        KTestPrintf("** OBJECT ARRIVAL to CLIENT\n");

        KWString StrFrom(KtlSystem::GlobalNonPagedAllocator()), StrTo(KtlSystem::GlobalNonPagedAllocator());
        Binding->GetRemoteEpUri(StrFrom);
        Binding->GetLocalEpUri(StrTo);

        KTestPrintf("  Object arrival from {%S} to local ep {%S}\n", PWSTR(StrFrom), PWSTR(StrTo));

        TestObject2* Ptr;

        NTSTATUS Result = Dgram->AcquireObject(Ptr);
        if (NT_SUCCESS(Result))
        {
            KTestPrintf("  Object v1=%d v2=%d str=%S ret=%S\n", Ptr->get_v1(), Ptr->get_v2(), Ptr->GetStr(), Ptr->GetReturnAddress());
            _delete(Ptr);
        }
        else
        {
            KTestPrintf("FAILED to get object\n");
        }

        InterlockedDecrement(&_ResponsesToProcess);
        _ServerResponse.SetEvent();
    }



    //
    //
    //
    VOID
    ClientWriteCb(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

         KTestPrintf("ClientWriteCb: ");
        KDatagramWriteOp& WriteOp = (KDatagramWriteOp&) Op;
        NTSTATUS Res = WriteOp.Status();
        KTestPrintf("Status = %d 0x%X\n", Res, Res);
        if (NT_SUCCESS(Res))
        {
        }
        else
        {
            InterlockedDecrement(&_ResponsesToProcess);
            KTestPrintf("************** FAILED TO SEND\n");
        }
        _InitialSend.SetEvent();
    }

    //
    //
    //
    NTSTATUS RunClient(KUriView LocalClient, KUriView RemoteServer, ULONG ClientCheckVal)
    {
        NTSTATUS Result;
        KNetwork::SPtr Network;
        _ResponsesToProcess = 0;

        // Create a network instance
        //
        Result = KNetwork::Create(
            0,
            *g_Allocator,
            _ClientNet
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        // Register a 2nd gen receiver using the new technique.
        //
        KUriView uriView(L"newclient");
        Result = _ClientNet->RegisterReceiver<TestObject2>(
            uriView,
            GUID_TestObject2,
            0,
            KNetwork::KNetReceiverType(this, &KNewClientTest::ClientReceiver),
            _ThisEp
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Register another one and then unregister it to prove unregister works.
        //
        KNetworkEndpoint::SPtr Doomed;
        uriView = L"newclientx/doomed";
        Result = _ClientNet->RegisterReceiver<TestObject2>(
            uriView,
            GUID_TestObject,
            0,
            KNetwork::KNetReceiverType(this, &KNewClientTest::ClientReceiver),
            Doomed
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        Result = _ClientNet->UnregisterReceiver(GUID_TestObject, Doomed);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Make the remote endpoint directly.
        KUriView proposedUri(RemoteServer);
        Result = KRvdTcpNetworkEndpoint::Create(proposedUri, *g_Allocator, _RemoteEp);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        // Get the binding.

        KNetBinding::SPtr Binding;
        Result = _ClientNet->GetBinding(_ThisEp, _RemoteEp, Binding);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Send one object at a time and wait for response
        // continuing
        //
        // Loop through a few objects
        //
        for (ULONG Index = 1; Index < 5; Index++)
        {
            TestObject2 Test(ClientCheckVal, ClientCheckVal+100, L"CLIENT-SENT-TYPE-2", L"(NewClient)");

            // Now set up a write
            //
            KDatagramWriteOp::SPtr Write;
            Result = Binding->CreateDatagramWriteOp(
                Write,
                KTL_TAG_TEST
                );

            if (!NT_SUCCESS(Result))
            {
                return Result;
            }

            // Now send a test object
            //

            Result = Write->StartWrite(
                Test,
                0,
                KAsyncContextBase::CompletionCallback(this, &KNewClientTest::ClientWriteCb),
                NULL,
                KtlNetPriorityNormal,
                KActivityId(101),
                500000
                );


            if (Result != STATUS_PENDING)
            {
                return Result;
            }

            InterlockedIncrement(&_ResponsesToProcess);


            // Wait for write to complete befor letting binding go out of scope
            //
            _InitialSend.WaitUntilSet();

            KTestPrintf("Iteration [%u] completed\n", Index);

            KNt::Sleep(5000);
        }


        // Now we wait for a response messages.
        //
        _ServerResponse.WaitUntilSet();


        // Don't let binding go out of scope until idle

        while(__KNetwork_PendingOperations > 0)
        {
            KNt::Sleep(50);
            KTestPrintf("Retry loop while waiting for networks to shut down\n");
        }

        KTestPrintf("Exiting test...\n");

        KNt::Sleep(5000);

        return STATUS_SUCCESS;
    }


    VOID
    Cleanup()
    {
        KTestPrintf("Cleanup...\n");
        if (_ClientNet)
        {
            _ClientNet->Shutdown();
        }

        KNt::Sleep(100);
    }

    NTSTATUS
    Run(KUriView LocalClient, KUriView RemoteServer, ULONG CheckVal)
    {
        NTSTATUS Result;
        Result = RunClient(LocalClient, RemoteServer, CheckVal);
        Cleanup();
        return Result;
    }
};


////////////////////////////////////////////////////////////////////////////////

class OldVNetTest
{
public:
    KNetwork::SPtr  _ServerNet;

    VOID
    ServerWriteCb(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

         KTestPrintf("ServerWriteCb callback: ");
        KDatagramWriteOp& WriteOp = (KDatagramWriteOp&) Op;
        NTSTATUS Res = WriteOp.Status();
        KTestPrintf("Status = %d 0x%X\n", Res, Res);
    }

    VOID
    ServerReceiver(
            __in const GUID& To,
            __in const GUID& From,
            __in const ULONG Flags,
            __in const PVOID ContextValue,
            __in KNetBinding::SPtr& Binding,
            __in KInDatagram::SPtr& Dgram
            )
    {
        UNREFERENCED_PARAMETER(To);
        UNREFERENCED_PARAMETER(From);
        UNREFERENCED_PARAMETER(Flags);
        UNREFERENCED_PARAMETER(ContextValue);

        KTestPrintf("\n\n** OBJECT ARRIVAL to SERVER\n");

        KWString StrFrom(KtlSystem::GlobalNonPagedAllocator()), StrTo(KtlSystem::GlobalNonPagedAllocator());
        Binding->GetRemoteEpUri(StrFrom);
        Binding->GetLocalEpUri(StrTo);

        KTestPrintf("  Object sent from {%S} to {%S}\n", PWSTR(StrFrom), PWSTR(StrTo));



        TestObject* Ptr = 0;
        NTSTATUS Result = Dgram->AcquireObject(Ptr);
        if (NT_SUCCESS(Result))
        {
            KTestPrintf("  Object v1=%d v2=%d str=%S\n", Ptr->get_v1(), Ptr->get_v2(), Ptr->GetStr());
        }
        else
        {
            KTestPrintf(">>>>>>>>> FAILED to get object\n");
            return;
        }

        KFinally( [&]()
        {
            if (Ptr->ShutdownServer)
            {
                g_RunServer = FALSE;
            }
            _delete(Ptr);
        });

        // Dump the info

        TestObject Test(Ptr->get_v1(), Ptr->get_v1()+10000, L"SERVER-ORIGINATED-REPLY");

        GUID RemoteGuid;
        GUID LocalGuid;
        Binding->GetRemoteEp(RemoteGuid);
        Binding->GetLocalEp(LocalGuid);

        // Reuse the incoming binding half the time or get a new one
        // half the time.

        static Alternator = 0;
        KNetBinding::SPtr ReplyBinding;

        if (Alternator)
        {
            ReplyBinding = Binding;
            Alternator = 0;
        }
        else
        {
            _ServerNet->GetBinding(RemoteGuid, LocalGuid, 0, ReplyBinding);
            Alternator++;
        }

        DumpBindingInfo(ReplyBinding);

        // Now set up a write
        //
        KDatagramWriteOp::SPtr Write;
        Result = ReplyBinding->CreateDatagramWriteOp(
            Write,
            KTL_TAG_TEST
            );

        if (!NT_SUCCESS(Result))
        {
            KTestPrintf("Failure!\n");
            g_RunServer = FALSE;
            return;
        }

        // Now send a test object
        //

        Result = Write->StartWrite(
            Test,
            0,
            KAsyncContextBase::CompletionCallback(this, &OldVNetTest::ServerWriteCb),
            NULL,
            KtlNetPriorityNormal,
            KActivityId(101),
            500000
            );


        if (Result != STATUS_PENDING)
        {
            g_RunServer = FALSE;
            KTestPrintf("Unable to start a write!\n");
            return;
        }
    }


    //
    //
    //
    NTSTATUS
    SetupServer()
    {
        NTSTATUS Result;

        // Create a network instance
        //
        Result = KNetwork::Create(
            0,
            *g_Allocator,
            _ServerNet
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Register a root local endpoint
        //
        KGuid RootEp;
        RootEp.CreateNew();

        KLocalString<256> UriStr;
        UriStr.CopyFrom(KStringView(L"rvd://(local):1/serverNode/collection/{e1d203f8-5bb4-456b-8c5e-145486ec8094}/replicator/{a56a54e3-643a-01cd-8c5e-145486ec8094}"));
        UriStr.SetNullTerminator();

        KWString LocalRoot(KtlSystem::GlobalNonPagedAllocator(), UriStr);

        Result = _ServerNet->RegisterEp(LocalRoot, KNetwork::eLocalEp|KNetwork::eRemoteEp, RootEp);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Now, we register an EP and sit around and wait for messages.
        //
        Result = _ServerNet->RegisterReceiver<TestObject>(
            RootEp,
            GUID_TestObject,
            0,
            0,
            KNetwork::ReceiverType(this, &OldVNetTest::ServerReceiver)
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Register the client so that we can receive.
        //

        KGuid CliEp;
        CliEp.CreateNew();

        UriStr.CopyFrom(KStringView(L"rvd://(local):1/node1/collection/{e1d203f8-5bb4-456b-8c5e-145486ec8094}/replicator/{a56a54e3-643a-01cd-8c5e-145486ec8094}"));
        UriStr.SetNullTerminator();

        KWString Cli(KtlSystem::GlobalNonPagedAllocator(), UriStr);

        Result = _ServerNet->RegisterEp(Cli, KNetwork::eRemoteEp, CliEp);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        return STATUS_SUCCESS;
    }


    KNetwork::SPtr _ClientNet;
    KAutoResetEvent _InitialSend;
    KAutoResetEvent _ServerResponse;
    GUID            _ListenerEp;
    LONG            _ResponsesToProcess;

    VOID
    ClientReceiver(
            __in const GUID& To,
            __in const GUID& From,
            __in const ULONG Flags,
            __in const PVOID ContextValue,
            __in KNetBinding::SPtr& Binding,
            __in KInDatagram::SPtr& Dgram
            )
    {
        UNREFERENCED_PARAMETER(To);
        UNREFERENCED_PARAMETER(From);
        UNREFERENCED_PARAMETER(Flags);
        UNREFERENCED_PARAMETER(ContextValue);

        KTestPrintf("** OBJECT ARRIVAL to CLIENT\n");

        KWString StrFrom(KtlSystem::GlobalNonPagedAllocator()), StrTo(KtlSystem::GlobalNonPagedAllocator());
        Binding->GetRemoteEpUri(StrFrom);
        Binding->GetLocalEpUri(StrTo);

        KTestPrintf("  Object arrival from {%S} to local ep {%S}\n", PWSTR(StrFrom), PWSTR(StrTo));

        TestObject* Ptr;

        NTSTATUS Result = Dgram->AcquireObject(Ptr);
        if (NT_SUCCESS(Result))
        {
            KTestPrintf("  Object v1=%d v2=%d str=%S\n", Ptr->get_v1(), Ptr->get_v2(), Ptr->GetStr());
            _delete(Ptr);
        }
        else
        {
            KTestPrintf("FAILED to get object\n");
        }

        InterlockedDecrement(&_ResponsesToProcess);
        _ServerResponse.SetEvent();
    }



    //
    //
    //
    VOID
    ClientWriteCb(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

         KTestPrintf("ClientWriteCb: ");
        KDatagramWriteOp& WriteOp = (KDatagramWriteOp&) Op;
        NTSTATUS Res = WriteOp.Status();
        KTestPrintf("Status = %d 0x%X\n", Res, Res);
        if (NT_SUCCESS(Res))
        {
        }
        else
        {
            InterlockedDecrement(&_ResponsesToProcess);
            KTestPrintf("************** FAILED TO SEND\n");
        }
        _InitialSend.SetEvent();
    }

    //
    //
    //
    NTSTATUS RunClient(ULONG ClientCheckVal)
    {
        NTSTATUS Result;
        KNetwork::SPtr Network;
        _ResponsesToProcess = 0;

        // Create a network instance
        //
        Result = KNetwork::Create(
            0,
            *g_Allocator,
            _ClientNet
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Register a root local endpoint
        //
        KGuid RootEp;
        RootEp.CreateNew();

        KLocalString<256> Tmp;
        Tmp.CopyFrom(KStringView(L"rvd://(local):1/node1/collection/{e1d203f8-5bb4-456b-8c5e-145486ec8094}/replicator/{a56a54e3-643a-01cd-8c5e-145486ec8094}"));
        Tmp.SetNullTerminator();

        KWString LocalRoot(KtlSystem::GlobalNonPagedAllocator(), Tmp);

        Result = _ClientNet->RegisterEp(LocalRoot, KNetwork::eLocalEp|KNetwork::eRemoteEp, RootEp);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Register a destination endpoint for sending stuff.
        //
        KGuid DestEp;
        DestEp.CreateNew();

        Tmp.CopyFrom(KStringView(L"rvd://(local):1/serverNode/collection/{e1d203f8-5bb4-456b-8c5e-145486ec8094}/replicator/{a56a54e3-643a-01cd-8c5e-145486ec8094}"));
        Tmp.SetNullTerminator();
        KWString RemoteRoot(KtlSystem::GlobalNonPagedAllocator(), Tmp);

        Result = _ClientNet->RegisterEp(RemoteRoot, KNetwork::eRemoteEp, DestEp);

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        // Take address of deserializer


        // Now, we can send stuff.  But to receive we need a receiver.
        //
        Result = _ClientNet->RegisterReceiver<TestObject>(
            DestEp,
            GUID_TestObject,
            0,
            0,
            KNetwork::ReceiverType(this, &OldVNetTest::ClientReceiver)
            );


        // Get the binding
        //
        KNetBinding::SPtr Binding;

        Result = _ClientNet->GetBinding(
            DestEp,
            RootEp,
            0,
            Binding
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        // Send one object at a time and wait for response
        // continuing
        //
        // Loop through a few objects
        //
        for (ULONG Index = 1; Index < 5; Index++)
        {
            // If here, it is a normal message being sent.
            //
            TestObject Test(ClientCheckVal, ClientCheckVal+100, L"CLIENT-SENT");

            // Now set up a write
            //
            KDatagramWriteOp::SPtr Write;
            Result = Binding->CreateDatagramWriteOp(
                Write,
                KTL_TAG_TEST
                );

            if (!NT_SUCCESS(Result))
            {
                return Result;
            }

            // Now send a test object
            //

            Result = Write->StartWrite(
                Test,
                0,
                KAsyncContextBase::CompletionCallback(this, &OldVNetTest::ClientWriteCb),
                NULL,
                KtlNetPriorityNormal,
                KActivityId(101),
                500000
                );


            if (Result != STATUS_PENDING)
            {
                return Result;
            }

            InterlockedIncrement(&_ResponsesToProcess);


            // Wait for write to complete befor letting binding go out of scope
            //
            _InitialSend.WaitUntilSet();

            KTestPrintf("Iteration [%u] completed\n", Index);

            KNt::Sleep(5000);
        }


        // Now we wait for a response messages.
        //
        _ServerResponse.WaitUntilSet();


        // Don't let binding go out of scope until idle

        while(__KNetwork_PendingOperations > 0)
        {
            KNt::Sleep(50);
            KTestPrintf("Retry loop while waiting for networks to shut down\n");
        }

        KNt::Sleep(5000);

        return STATUS_SUCCESS;
    }


    VOID
    Cleanup()
    {
        KTestPrintf("Client Cleanup...\n");
        if (_ClientNet)
        {
            _ClientNet->Shutdown();
        }

        KNt::Sleep(100);

        KTestPrintf("Server Cleanup...\n");
        if (_ServerNet)
        {
            _ServerNet->Shutdown();
        }

        KNt::Sleep(100);

    }


    NTSTATUS
    Run()
    {
        NTSTATUS Result;
        Result = SetupServer();

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        Result = RunClient(333);

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        Cleanup();

        KTestPrintf("Done with cleanup\n");

        return Result;
    }

};

/////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

class LoopbackTest
{
public:
    KNetwork::SPtr  _ServerNet;
    KNetworkEndpoint::SPtr _ServerEp1;
    KNetworkEndpoint::SPtr _ServerEp2;

    VOID
    ServerWriteCb(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

         KTestPrintf("ServerWriteCb callback: ");
        KDatagramWriteOp& WriteOp = (KDatagramWriteOp&) Op;
        NTSTATUS Res = WriteOp.Status();
        KTestPrintf("Status = %d 0x%X\n", Res, Res);
    }


    VOID
    TestReceiver(
            __in KNetBinding::SPtr& Binding,
            __in KInDatagram::SPtr& Dgram
            )
    {
        KTestPrintf("\n\n** TestReceiver! OBJECT ARRIVAL to SERVER\n");

        KWString StrFrom(KtlSystem::GlobalNonPagedAllocator()), StrTo(KtlSystem::GlobalNonPagedAllocator());
        Binding->GetRemoteEpUri(StrFrom);
        Binding->GetLocalEpUri(StrTo);

        KTestPrintf("  TestObject sent from {%S} to {%S}\n", PWSTR(StrFrom), PWSTR(StrTo));



        TestObject* Ptr = 0;
        NTSTATUS Result = Dgram->AcquireObject(Ptr);
        if (NT_SUCCESS(Result))
        {
            KTestPrintf("  Object v1=%d v2=%d str=%S\n", Ptr->get_v1(), Ptr->get_v2(), Ptr->GetStr());
        }
        else
        {
            KTestPrintf(">>>>>>>>> FAILED to get object\n");
            return;
        }

        KFinally( [&]()
        {
            if (Ptr->ShutdownServer)
            {
                g_RunServer = FALSE;
            }
            _delete(Ptr);
        }
        );


        TestObject Test(Ptr->get_v1(), Ptr->get_v1()+10000, L"SERVER-ORIGINATED-REPLY-TYPE-1");

        // Reuse the incoming binding half the time or get a new one
        // half the time.

        static Alternator = 0;
        KNetBinding::SPtr ReplyBinding;

        if (Alternator)
        {
            ReplyBinding = Binding;
            Alternator = 0;
        }
        else
        {
            // Get a new binding by extracting the return address and using
            // the local binding to this endpoint.

            ReplyBinding = Binding;
            //_ServerNet->GetBinding(RemoteGuid, LocalGuid, 0, ReplyBinding);
            Alternator++;
        }

        DumpBindingInfo(ReplyBinding);

        // Now set up a write
        //
        KDatagramWriteOp::SPtr Write;
        Result = ReplyBinding->CreateDatagramWriteOp(
            Write,
            KTL_TAG_TEST
            );

        if (!NT_SUCCESS(Result))
        {
            KTestPrintf("Failure!\n");
            g_RunServer = FALSE;
            return;
        }

        // Now send a test object
        //

        Result = Write->StartWrite(
            Test,
            0,
            KAsyncContextBase::CompletionCallback(this, &LoopbackTest::ServerWriteCb),
            NULL,
            KtlNetPriorityNormal,
            KActivityId(101),
            500000
            );


        if (Result != STATUS_PENDING)
        {
            g_RunServer = FALSE;
            KTestPrintf("Unable to start a write!\n");
            return;
        }
    }


    //
    //
    //
    NTSTATUS
    SetupServer(KUriView ServerUri)
    {
        NTSTATUS Result;

        // Create a network instance
        //
        Result = KNetwork::Create(
            0,
            *g_Allocator,
            _ServerNet
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Build the relative URI we need

        KLocalString<256> Tmp;
        Tmp.CopyFrom(ServerUri.Get(KUriView::ePath));
        Tmp.Concat(L"/TestObject");

        // Register a 2nd gen receiver using the new technique.
        //
        KUriView uriView(Tmp);
        Result = _ServerNet->RegisterReceiver<TestObject>(
            uriView,
            GUID_TestObject,
            0,
            KNetwork::KNetReceiverType(this, &LoopbackTest::TestReceiver),
            _ServerEp1
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        return STATUS_SUCCESS;
    }


    KNetwork::SPtr  _ClientNet;
    KAutoResetEvent _InitialSend;
    KAutoResetEvent _ServerResponse;
    GUID            _ListenerEp;
    LONG            _ResponsesToProcess;

    KNetworkEndpoint::SPtr _ClientEp;
    KNetworkEndpoint::SPtr _RemoteEp;

    VOID
    ClientReceiver(
            __in KNetBinding::SPtr& Binding,
            __in KInDatagram::SPtr& Dgram
            )
    {
        KTestPrintf("** OBJECT ARRIVAL to CLIENT\n");

        KWString StrFrom(KtlSystem::GlobalNonPagedAllocator()), StrTo(KtlSystem::GlobalNonPagedAllocator());
        Binding->GetRemoteEpUri(StrFrom);
        Binding->GetLocalEpUri(StrTo);

        KTestPrintf("  Object arrival from {%S} to local ep {%S}\n", PWSTR(StrFrom), PWSTR(StrTo));

        TestObject* Ptr;

        NTSTATUS Result = Dgram->AcquireObject(Ptr);
        if (NT_SUCCESS(Result))
        {
            KTestPrintf("  Object v1=%d v2=%d str=%S\n", Ptr->get_v1(), Ptr->get_v2(), Ptr->GetStr());
            _delete(Ptr);
        }
        else
        {
            KTestPrintf("FAILED to get object\n");
        }

        InterlockedDecrement(&_ResponsesToProcess);
        _ServerResponse.SetEvent();
    }



    //
    //
    //
    VOID
    ClientWriteCb(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

         KTestPrintf("ClientWriteCb: ");
        KDatagramWriteOp& WriteOp = (KDatagramWriteOp&) Op;
        NTSTATUS Res = WriteOp.Status();
        KTestPrintf("Status = %d 0x%X\n", Res, Res);
        if (NT_SUCCESS(Res))
        {
        }
        else
        {
            InterlockedDecrement(&_ResponsesToProcess);
            KTestPrintf("************** FAILED TO SEND\n");
        }
        _InitialSend.SetEvent();
    }

    //
    //
    //
    NTSTATUS RunClient(KUriView& ClientBaseUri, KUriView& ServerDestination)
    {
        UNREFERENCED_PARAMETER(ClientBaseUri);

        NTSTATUS Result;
        KNetwork::SPtr Network;
        _ResponsesToProcess = 0;

        // Create a network instance
        //
        Result = KNetwork::Create(
            0,
            *g_Allocator,
            _ClientNet
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        // Register a 2nd gen receiver using the new technique.
            //
        KUriView uriView(L"clientRoot/newclient");
        Result = _ClientNet->RegisterReceiver<TestObject2>(
            uriView,
            GUID_TestObject,
            0,
            KNetwork::KNetReceiverType(this, &LoopbackTest::ClientReceiver),
            _ClientEp
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }



        // Make the remote endpoint directly.
        KUriView proposedUri(ServerDestination);
        Result = KRvdTcpNetworkEndpoint::Create(proposedUri, *g_Allocator, _RemoteEp);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }


        // Get the binding.

        KNetBinding::SPtr Binding;
        Result = _ClientNet->GetBinding(_ClientEp, _RemoteEp, Binding);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Send one object at a time and wait for response
        // continuing
        //
        // Loop through a few objects
        //
        for (ULONG Index = 1; Index < 5; Index++)
        {
            TestObject Test(1, 10, L"CLIENT-SENT-TYPE-1b");

            // AMW
            if (Index == 4)
            {
                Test.ShutdownServer = TRUE;
            }
            //
            
            // Now set up a write
            //
            KDatagramWriteOp::SPtr Write;
            Result = Binding->CreateDatagramWriteOp(
                Write,
                KTL_TAG_TEST
                );

            if (!NT_SUCCESS(Result))
            {
                return Result;
            }

            // Now send a test object
            //

            Result = Write->StartWrite(
                Test,
                0,
                KAsyncContextBase::CompletionCallback(this, &LoopbackTest::ClientWriteCb),
                NULL,
                KtlNetPriorityNormal,
                KActivityId(101),
                500000
                );


            if (Result != STATUS_PENDING)
            {
                return Result;
            }

            InterlockedIncrement(&_ResponsesToProcess);


            // Wait for write to complete befor letting binding go out of scope
            //

            _InitialSend.WaitUntilSet();

            KTestPrintf("Iteration [%u] completed\n", Index);

           // KNt::Sleep(1000);
        }


        // Now we wait for a response messages.
        //
        _ServerResponse.WaitUntilSet();

        while (_ResponsesToProcess)
        {
            KNt::Sleep(4250);
            KTestPrintf("Processing responses; [%d] remaining\n", _ResponsesToProcess);
        }

        // Don't let binding go out of scope until idle

        while(__KNetwork_PendingOperations > 0)
        {
            KNt::Sleep(4000);
            KTestPrintf("Retry loop while waiting for networks to shut down\n");
        }

        while (g_RunServer)
        {
            KNt::Sleep(5000);
            KTestPrintf("Waiting for kill server request\n");
        }

        KTestPrintf("Exiting test...\n");

        KNt::Sleep(5000);

        return STATUS_SUCCESS;
    }


    VOID
    Cleanup()
    {
        KTestPrintf("Cleanup...\n");
        if (_ClientNet)
        {
            _ClientNet->Shutdown();
        }

        KNt::Sleep(100);

        KTestPrintf("Server Cleanup...\n");
        if (_ServerNet)
        {
            _ServerNet->Shutdown();
        }

        KNt::Sleep(100);
    }




    NTSTATUS
    Run(KUriView BaseUri)
    {
        KLocalString<256> ServerUri;

        ServerUri.CopyFrom(KStringView(BaseUri));
        ServerUri.Concat(KStringView(L"/serverRoot"));

        NTSTATUS Result;
        Result = SetupServer(ServerUri);

        // The server will have registered a URI like the following string
        ServerUri.Concat(L"/TestObject");


        KLocalString<256> ClientUri;

        KStringView base(BaseUri);
        ClientUri.CopyFrom(base);
        KStringView toConcat(L"/clientRoot");
        ClientUri.Concat(toConcat);

        KUriView clientBase(ClientUri);
        KUriView serverDestination(ServerUri);
        Result = RunClient(clientBase, serverDestination);

        Cleanup();
        return Result;
    }

};




////////////////////////////////////////////////////////////////////////////////

// Client coded only to the new style and TestObject2
//

class KillServerTest
{
public:
    KNetwork::SPtr  _ClientNet;
    KAutoResetEvent _InitialSend;
    GUID            _ListenerEp;

    KNetworkEndpoint::SPtr _ThisEp;
    KNetworkEndpoint::SPtr _RemoteEp;

    //
    //
    //
    VOID
    ClientWriteCb(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

         KTestPrintf("ClientWriteCb: ");
        KDatagramWriteOp& WriteOp = (KDatagramWriteOp&) Op;
        NTSTATUS Res = WriteOp.Status();
        KTestPrintf("Status = %d 0x%X\n", Res, Res);
        if (NT_SUCCESS(Res))
        {
        }
        else
        {
            KTestPrintf("************** FAILED TO SEND\n");
        }
        _InitialSend.SetEvent();
    }

    //
    //
    //
    NTSTATUS RunClient(KUriView LocalClient, KUriView RemoteServer)
    {
        NTSTATUS Result;
        KNetwork::SPtr Network;

        // Create a network instance
        //
        Result = KNetwork::Create(
            0,
            *g_Allocator,
            _ClientNet
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        KUriView proposedUri(RemoteServer);
        Result = KRvdTcpNetworkEndpoint::Create(proposedUri, *g_Allocator, _RemoteEp);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        KUriView relativeUri(L"fakeclientEp");
        Result = _ClientNet->MakeLocalEndpoint(relativeUri, _ThisEp);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Get the binding.

        KNetBinding::SPtr Binding;
        Result = _ClientNet->GetBinding(_ThisEp, _RemoteEp, Binding);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        TestObject2 Test(0, 0, L"CLIENT-SENT-TYPE-2-KILL-SERVER", L"(FakeClient)");
        Test.ShutdownServer = TRUE;

        KDatagramWriteOp::SPtr Write;
        Result = Binding->CreateDatagramWriteOp(
            Write,
            KTL_TAG_TEST
            );

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Now send a test object
        //

        Result = Write->StartWrite(
            Test,
            0,
            KAsyncContextBase::CompletionCallback(this, &KillServerTest::ClientWriteCb),
            NULL,
            KtlNetPriorityNormal,
            KActivityId(101),
            500000
            );


        if (Result != STATUS_PENDING)
        {
            return Result;
        }

        // Wait for write to complete befor letting binding go out of scope
        //
        _InitialSend.WaitUntilSet();


        KTestPrintf("Exiting test...\n");

        return STATUS_SUCCESS;
    }


    VOID
    Cleanup()
    {
        KTestPrintf("Cleanup...\n");
        if (_ClientNet)
        {
            _ClientNet->Shutdown();
        }

        KNt::Sleep(100);
    }

    NTSTATUS
    Run(KUriView LocalClient, KUriView RemoteServer)
    {
        NTSTATUS Result;
        Result = RunClient(LocalClient, RemoteServer);
        Cleanup();
        return Result;
    }
};






////////////// infra ////////////////////////////

KEvent* _StartupEvent;
NTSTATUS* _ResultPtr;

VOID StartupCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);

    KNetServiceLayerStartup& Startup = (KNetServiceLayerStartup&) Op;
    *_ResultPtr = Startup.Status();
    _StartupEvent->SetEvent();

    KTestPrintf("Network started with status %u 0x%x\n", *_ResultPtr, *_ResultPtr);

}

NTSTATUS
NetServiceLayerStartup(KUriView ListenerUri)
{
    KNetServiceLayer::Shutdown();

    NTSTATUS Result;
    KEvent StartupEvent;
    _StartupEvent = &StartupEvent;
    _ResultPtr = &Result;

    KNetServiceLayerStartup::SPtr Startup;
    Result = KNetServiceLayer::CreateStartup(*g_Allocator, Startup);

    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    KNetworkEndpoint::SPtr RootEp;

    Result = KRvdTcpNetworkEndpoint::Create(ListenerUri, *g_Allocator, RootEp);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }


    Result = Startup->StartNetwork(RootEp, StartupCallback);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    StartupEvent.WaitUntilSet();
    KTestPrintf("Network has started...\n");

    return Result;
}



VOID
NetServiceLayerShutdown()
{
    KNetServiceLayer::Shutdown();
}

NTSTATUS
RunOldClient(
    __in KUriView LocalClient,
    __in KUriView ServerUri,
    __in ULONG ClientCheckVal
    )
{
    NTSTATUS Result;

    if (!ServerUri.IsValid())
    {
        KTestPrintf("Invalid URI\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (ServerUri.Get(KUriView::eHost).Length() == 0)
    {
        KTestPrintf("Missing host\n");
         return STATUS_UNSUCCESSFUL;
    }

    if (ServerUri.GetPort() == 0)
    {
        KTestPrintf("Missing port\n");
         return STATUS_UNSUCCESSFUL;
    }

    Result = NetServiceLayerStartup(LocalClient);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    KOldClientTest Test;
    Result = Test.Run(LocalClient, ServerUri, ClientCheckVal);

    NetServiceLayerShutdown();

    return Result;
}


NTSTATUS
RunNewClient(
    __in KUriView LocalClient,
    __in KUriView ServerUri,
    __in ULONG ClientCheckVal
    )
{
    NTSTATUS Result;

    if (!ServerUri.IsValid())
    {
        KTestPrintf("Invalid URI\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (ServerUri.Get(KUriView::eHost).Length() == 0)
    {
        KTestPrintf("Missing host\n");
         return STATUS_UNSUCCESSFUL;
    }

    if (ServerUri.GetPort() == 0)
    {
        KTestPrintf("Missing port\n");
         return STATUS_UNSUCCESSFUL;
    }

    Result = NetServiceLayerStartup(LocalClient);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    KNewClientTest Test;
    Result = Test.Run(LocalClient, ServerUri, ClientCheckVal);

    NetServiceLayerShutdown();

    return Result;
}

NTSTATUS
RunKillServer(
    __in KUriView LocalClient,
    __in KUriView ServerUri
    )
{
    NTSTATUS Result;

    if (!ServerUri.IsValid())
    {
        KTestPrintf("Invalid URI\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (ServerUri.Get(KUriView::eHost).Length() == 0)
    {
        KTestPrintf("Missing host\n");
         return STATUS_UNSUCCESSFUL;
    }

    if (ServerUri.GetPort() == 0)
    {
        KTestPrintf("Missing port\n");
         return STATUS_UNSUCCESSFUL;
    }

    Result = NetServiceLayerStartup(LocalClient);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    KillServerTest Test;
    Result = Test.Run(LocalClient, ServerUri);

    NetServiceLayerShutdown();

    return Result;
}



NTSTATUS
RunOldServer(KUriView ServerUri)
{
    NTSTATUS Result;

    if (!ServerUri.IsValid())
    {
        KTestPrintf("Invalid URI\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (ServerUri.Get(KUriView::eHost).Length() == 0)
    {
        KTestPrintf("Missing host\n");
         return STATUS_UNSUCCESSFUL;
    }

    if (ServerUri.GetPort() == 0)
    {
        KTestPrintf("Missing port\n");
         return STATUS_UNSUCCESSFUL;
    }

    Result = NetServiceLayerStartup(ServerUri);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    KOldServerTest Test;

    Result = Test.Run(ServerUri);

    NetServiceLayerShutdown();

    return Result;
}

NTSTATUS
RunNewServer(KUriView ServerUri)
{
    NTSTATUS Result;

    if (!ServerUri.IsValid())
    {
        KTestPrintf("Invalid URI\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (ServerUri.Get(KUriView::eHost).Length() == 0)
    {
        KTestPrintf("Missing host\n");
         return STATUS_UNSUCCESSFUL;
    }

    if (ServerUri.GetPort() == 0)
    {
        KTestPrintf("Missing port\n");
         return STATUS_UNSUCCESSFUL;
    }

    Result = NetServiceLayerStartup(ServerUri);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    KNewServerTest Test;

    Result = Test.Run(ServerUri);

    NetServiceLayerShutdown();

    return Result;
}

NTSTATUS
RunLoopbackTest(KUriView& Uri)
{
    NTSTATUS Result = NetServiceLayerStartup(Uri);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    LoopbackTest Test;
    Result = Test.Run(Uri);

    NetServiceLayerShutdown();

    return Result;
}

NTSTATUS
RunVNetOld()
{
    NTSTATUS Result = NetServiceLayerStartup(L"rvd://(local):1");
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    OldVNetTest Test;
    Result = Test.Run();

    NetServiceLayerShutdown();

    return Result;
}

VOID
Test()
{
        KNetworkEndpoint::SPtr Ep;
        KUriView proposedUri(L"rvd://..localmachine:1/blah/{d6af3b96-e4ed-4163-99fb-f8a7f97c5365}");
        NTSTATUS Result = KRvdTcpNetworkEndpoint::Create(proposedUri, *g_Allocator, Ep);
        if (!NT_SUCCESS(Result))
        {
            return;
        }

        GUID x;

        Ep->GetUri()->Get(KUriView::eRaw).RightString(38).ToGUID(x);


}


NTSTATUS
KNetworkTest(
    int argc, WCHAR* args[]
    )
{
    if (argc < 1)
    {
        KTestPrintf("Usage:\n");
        KTestPrintf("-oldclient  LocalClientURI RemoteServerURI ClientCheckValue  // Uses the GUID-based endpoint model; both URIs must have machine name and port; ClientCheckValue is an integer\n");
        KTestPrintf("-oldserver  ServerListenerUri                                // Uses the GUID based endpoint model; server URI must contain port\n");
        KTestPrintf("-newclient  LocalClientURI RemoteServerURI ClientCheckValue  // KNetworkEndpoint clietn; both URIs must have machine name and port; ClientCheckValue is an integer\n");
        KTestPrintf("-newserver  ServerListenerUri                                // KNetworkEndpoint server; server URI must contain port\n");
        KTestPrintf("-vnetold    Runs the old style V-Net test                    // Deprecated\n");
        KTestPrintf("-killserver LocalClientURI RemoteServerUri                   // Send a message to tell the remote server to die\n");
        KTestPrintf("\n");
        KTestPrintf("-loopback   BaseUri      // Does an in process network loopback test. For Uri use one of the following authority/host patterns:\n");
        KTestPrintf("                         // rvd://server:port            \n");
        KTestPrintf("                         // rvd://myserver:5005          // Sample with the actual local machine name and port 5005\n");
        KTestPrintf("                         // rvd://.:5007                 // Dot is pseudonym for local machine\n");
        KTestPrintf("                         // rvd://..localmachine:5005    // ..localmachine is an alternate way of expressing the same as a singel dot\n");
        KTestPrintf("                         // rvd://(local):1              // For in-memory transport, use (local) and a port value of 1. \n");
        KTestPrintf("\n");
        KTestPrintf("Note: tests must be run individually as separate runs.\n");

        return STATUS_UNSUCCESSFUL;
    }

    EventRegisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    NTSTATUS Result;
    KtlSystem* underlyingSystem;
    Result = KtlSystem::Initialize(&underlyingSystem);
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KtlSystem::Initializer() failure\n");
        return Result;
    }
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    // Turn on strict allocation tracking
    underlyingSystem->SetStrictAllocationChecks(TRUE);

    
    ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    Test();

    //

    {
        CmdLineParser Parser(*g_Allocator);
        Parameter *Param;

        KDbgMirrorToDebugger = TRUE;

        if (Parser.Parse( argc, args))
        {
            if (!Parser.ParameterCount())
            {
                KTestPrintf("Missing parameters\n");
                return STATUS_UNSUCCESSFUL;
            }
            Param = Parser.GetParam(0);
            if (wcscmp(Param->_Name, L"loopback") == 0)
            {
                KUriView uriView(Param->_Values[0]);
                Result = RunLoopbackTest(uriView);
            }
            else if (wcscmp(Param->_Name, L"oldclient") == 0)
            {
                ULONG ClientCheckVal = 0;
                KStringView Tmp = Param->_Values[2];
                Tmp.ToULONG(ClientCheckVal);
                Result = RunOldClient(KUriView(Param->_Values[0]), KUriView(Param->_Values[1]), ClientCheckVal);
            }
            else if (wcscmp(Param->_Name, L"newclient") == 0)
            {
                ULONG ClientCheckVal = 0;
                KStringView Tmp = Param->_Values[2];
                Tmp.ToULONG(ClientCheckVal);
                Result = RunNewClient(KUriView(Param->_Values[0]), KUriView(Param->_Values[1]), ClientCheckVal);
            }
            else if (wcscmp(Param->_Name, L"killserver") == 0)
            {
                Result = RunKillServer(KUriView(Param->_Values[0]), KUriView(Param->_Values[1]));
            }
            else if (wcscmp(Param->_Name, L"oldserver") == 0)
            {
                Result = RunOldServer(KUriView(Param->_Values[0]));
            }
            else if (wcscmp(Param->_Name, L"newserver") == 0)
            {
                Result = RunNewServer(KUriView(Param->_Values[0]));
            }
            else if (wcscmp(Param->_Name, L"vnetold") == 0)
            {
                Result = RunVNetOld();
            }
            else
            {
                Result = STATUS_INVALID_PARAMETER;
                KTestPrintf("No test chosen. Stopping...\n");
            }
        }
    }

    // Test for leaks

    ULONGLONG Leaks = KAllocatorSupport::gs_AllocsRemaining - StartingAllocs;
    if (Leaks)
    {
        KTestPrintf("Leaks = %u\n", Leaks);
        Result = STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  

    return Result;
}



#if CONSOLE_TEST
int
wmain(int argc, WCHAR* args[])
{
    if (argc > 0)
    {
        argc--;
        args++;
    }

    return KNetworkTest(argc, args);
}
#endif
