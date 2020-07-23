/*++

Copyright (c) Microsoft Corporation

Module Name:

    KSocketTests.cpp


--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KSocketTests.h"
#include <CommandLineParser.h>


#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


#define DEFAULT_PORT 5050


#if KTL_USER_MODE
extern volatile LONGLONG gs_AllocsRemaining;
#endif

KAllocator* g_Allocator = nullptr;


#define RETURN_ON_ERROR(Res) \
  if (!NT_SUCCESS(Res)) \
  {\
    return Res; \
  }


#define BUF_SIZE 0x10000

#define PACKET_SIGNATURE          0x12345678;
#define PACKET_OPERATION_MESSAGE  0x10
#define PACKET_OPERATION_QUIT     0x11

struct PACKET_HEADER
{
    ULONG _Signature;
    ULONG _Operation;
    ULONG _FollowingSize;
};

//////////////////////////////////////////////////////////////////////////
//
//  EchoServer
//
//  (1) Accepts a single connection
//  (2) Posts reads and echos back what was sent, plus additional proof-of-receipt
//  (3) When client closes the socket, the server should sense this and quit.
//  (4) TBD: Force the server to close first and see what happens to client
//
class EchoServerTest
{
public:
    ITransport::SPtr      _Transport;
    INetAddress::SPtr     _LocalAddress;
    ISocketListener::SPtr _Listener;
    ISocket::SPtr         _Socket;
    KAutoResetEvent       _LocalAddressEvent;
    KAutoResetEvent       _AcceptEvent;
    KAutoResetEvent       _QuitEvent;
    KAutoResetEvent       _ReadEvent;
    KAutoResetEvent       _WriteEvent;

    NTSTATUS
    CreateListener()
    {
        NTSTATUS Result;
        Result = _Transport->CreateListener(_LocalAddress, _Listener);
        return Result;
    }


    ////////////////////////////////////////////////////////////////////

    VOID
    AcceptCallback(
        __in ISocketAcceptOp& Accept
        )
    {
        // NTSTATUS Result = 
            Accept.Status();
        _Socket = Accept.GetSocket();
        _AcceptEvent.SetEvent();
    }

    static VOID
    _AcceptCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

        ISocketAcceptOp& AcceptOp = (ISocketAcceptOp&) Op;
        EchoServerTest* This = (EchoServerTest *) AcceptOp.GetUserData();
        This->AcceptCallback(AcceptOp);
    }

    NTSTATUS
    WaitAccept(
        USHORT Port
        )
    {
        UNREFERENCED_PARAMETER(Port);

        NTSTATUS Result;
        ISocketAcceptOp::SPtr Accept;

        Result = _Listener->CreateAcceptOp(Accept);
        RETURN_ON_ERROR(Result);

        Accept->SetUserData(this);

        Accept->StartAccept(_AcceptCallback);
        _AcceptEvent.WaitUntilSet();

        Result = Accept->Status();
        return Result;
    }

    //////////////////////////////////////////////////

    VOID
    ReadCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

        // ISocketReadOp& ReadOp = 
            (ISocketReadOp&) Op;
        _ReadEvent.SetEvent();
    }


    VOID
    WriteCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

        // ISocketWriteOp& WriteOp = 
            (ISocketWriteOp&) Op;
        _WriteEvent.SetEvent();
    }


    NTSTATUS ReadWriteSequence()
    {
        NTSTATUS Result;
        ISocketReadOp::SPtr ReadOp;
        UCHAR* pBuf = _newArray<UCHAR>(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), BUF_SIZE);
        KFinally([&](){ _deleteArray(pBuf); });

        for (;;)
        {
            // Post a read
            //
            Result = _Socket->CreateReadOp(ReadOp);
            RETURN_ON_ERROR(Result);

            RtlZeroMemory(pBuf, BUF_SIZE);
            KMemRef Mem;
            Mem._Address = pBuf;
            Mem._Size = BUF_SIZE;
            Mem._Param = BUF_SIZE;

            KAsyncContextBase::CompletionCallback CBack;
            CBack.Bind(this, &EchoServerTest::ReadCallback);
            Result = ReadOp->StartRead(1, &Mem, ISocketReadOp::ReadAny, CBack);

            RETURN_ON_ERROR(Result);

            _ReadEvent.WaitUntilSet();

            // Examine the message

            ULONG BytesRead = ReadOp->BytesTransferred();

            if (BytesRead == 0)
            {
                KTestPrintf("Client closed the socket\n");
                return STATUS_SUCCESS;
            }

            KTestPrintf("Bytes Read = %u\n", BytesRead);

            // Write a response
            //
            ISocketWriteOp::SPtr WriteOp;
            Result = _Socket->CreateWriteOp(WriteOp);
            RETURN_ON_ERROR(Result);

            KAsyncContextBase::CompletionCallback WCBack;
            WCBack.Bind(this, &EchoServerTest::WriteCallback);

            KMemRef MemRefs[3];

            ANSI_STRING Ansi;
            RtlInitString(&Ansi, "INCOMING MESSAGE WAS:");

            ANSI_STRING Ansi2;
            RtlInitString(&Ansi2, "THE-END");

            MemRefs[0]._Address = Ansi.Buffer;
            MemRefs[0]._Param = Ansi.Length;
            MemRefs[0]._Size  = Ansi.MaximumLength;

            MemRefs[1]._Address = Mem._Address;
            MemRefs[1]._Param = BytesRead;
            MemRefs[1]._Size  = BUF_SIZE;

            MemRefs[2]._Address = Ansi2.Buffer;
            MemRefs[2]._Size  = Ansi2.Length;
            MemRefs[2]._Param = Ansi2.MaximumLength;

            Result = WriteOp->StartWrite(3, &MemRefs[0], 1, WCBack);

            _WriteEvent.WaitUntilSet();

            Result = WriteOp->Status();
            RETURN_ON_ERROR(Result);

            ULONG BytesWritten = WriteOp->BytesTransferred();

            KTestPrintf("Bytes written = %u\n", BytesWritten);
        }
    }



    VOID
    AddressCallback(
        __in IBindAddressOp& BindOp
        )
    {
         // Check the status of the op
         //
         // This status should have been propagated into the INetAddress object
         //
         // NTSTATUS Result = 
             BindOp.Status();

         _LocalAddress = BindOp.GetAddress();
         _LocalAddressEvent.SetEvent();
    }


    static VOID
    _AddressCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

        IBindAddressOp& BindOp = (IBindAddressOp&) Op;
        EchoServerTest* This = (EchoServerTest *) BindOp.GetUserData();
        This->AddressCallback(BindOp);
    }


    NTSTATUS
    GetAddress(
        __in USHORT Port
        )
    {
        NTSTATUS Result;
        IBindAddressOp::SPtr Op1;

        Result = _Transport->CreateBindAddressOp(Op1);
        RETURN_ON_ERROR(Result);

        Op1->SetUserData(this);

        // Now bind the address

        Result = Op1->StartBind(L"..localmachine", Port, _AddressCallback);
        RETURN_ON_ERROR(Result);

        _LocalAddressEvent.WaitUntilSet();

        Result = _LocalAddress->Status();
        RETURN_ON_ERROR(Result);

        return STATUS_SUCCESS;

    }

    NTSTATUS
    Run(
        __in ITransport::SPtr& Trans,
        __in USHORT Port
        )
    {
        NTSTATUS Result;

        _Transport = Trans;
        Result = GetAddress(Port);
        RETURN_ON_ERROR(Result);

        Result = CreateListener();
        RETURN_ON_ERROR(Result);

        Result = WaitAccept(Port);
        RETURN_ON_ERROR(Result);

        Result = ReadWriteSequence();
        return Result;
    }
};



//
//  HttpClientTest
//
//  This is wrapped in a class using statics in order to delineate
//  the scope of the test & its helpers.
//
//  This test is the same for both user & kernel modes.
//  It does the following:
//      (a) Initialize a local address
//      (b) Initialize a remote address
//      (c) Connect as a client to port 80
//
//      LOOP
//        (d) Write bytes to the server in the form of HTTP GET
//        (e) Read bytes (HTTP Response)
//      END
//
//      (f) Close everything and exit
//

class HttpClientTest
{
public:
    ITransport::SPtr  _Transport;
    INetAddress::SPtr _LocalAddress;
    INetAddress::SPtr _RemoteAddress;
    ISocket::SPtr _TestSocket;
    KAutoResetEvent _LocalAddressEvent;
    KAutoResetEvent _RemoteAddressEvent;
    KAutoResetEvent _ConnectEvent;
    KAutoResetEvent _WriteEvent;
    KAutoResetEvent _ReadEvent;

    struct AddrInfo
    {
        HttpClientTest* _This;
        ULONG           _AddressIndex;
    };

    AddrInfo _AddrInfo[2];

    /////////////////////////////////////


    HttpClientTest() {}

    VOID
    AddressCallback(
        __in IBindAddressOp& BindOp,
        __in ULONG AddressIndex
        )
    {
         // Check the status of the op
         //
         // This status should have been propagated into the INetAddress object
         //
         // NTSTATUS Result = 
             BindOp.Status();

         if (AddressIndex == 1)
         {
             _LocalAddress = BindOp.GetAddress();
             _LocalAddressEvent.SetEvent();
         }
         else if (AddressIndex == 2)
         {
             _RemoteAddress = BindOp.GetAddress();
             _RemoteAddressEvent.SetEvent();
         }
         else
         {
            KFatal("Ugh!");
         }
    }


    static VOID
    _AddressCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

        IBindAddressOp& BindOp = (IBindAddressOp&) Op;
        AddrInfo* Ctx = (AddrInfo *) BindOp.GetUserData();
        Ctx->_This->AddressCallback(BindOp, Ctx->_AddressIndex);
    }


    NTSTATUS
    GetAddresses(LPCWSTR Server, USHORT Port)
    {
        // Allocate some addresses
        NTSTATUS Result;

        IBindAddressOp::SPtr Op1;
        IBindAddressOp::SPtr Op2;

        Result = _Transport->CreateBindAddressOp(Op1);
        RETURN_ON_ERROR(Result);

        Result = _Transport->CreateBindAddressOp(Op2);
        RETURN_ON_ERROR(Result);

        _AddrInfo[0]._This = this;
        _AddrInfo[1]._This = this;
        _AddrInfo[0]._AddressIndex = 1;
        _AddrInfo[1]._AddressIndex = 2;

        Op1->SetUserData(&this->_AddrInfo[0]);
        Op2->SetUserData(&this->_AddrInfo[1]);

        // Now bind the addresses

        Result = Op1->StartBind(L"..localmachine", 0, _AddressCallback);
        RETURN_ON_ERROR(Result);

        Result = Op2->StartBind(PWSTR(Server), Port,_AddressCallback);
        RETURN_ON_ERROR(Result);

        // Wait for the events and check the status

        _RemoteAddressEvent.WaitUntilSet();
        _LocalAddressEvent.WaitUntilSet();

        // Check the final status values of the two
        // address objects and make sure they are ok.

        Result = _LocalAddress->Status();
        RETURN_ON_ERROR(Result);

        Result = _RemoteAddress->Status();
        RETURN_ON_ERROR(Result);

        return STATUS_SUCCESS;
    }




    //////////////////////////////

    VOID
    ConnectCallback(
        __in ISocketConnectOp& ConnectOp
        )
    {
         // NTSTATUS Result = 
             ConnectOp.Status();

         // The socket should have the status
         // and not be a null object. Inspect in debugger at this location,
         // but additinal tests occur later in the code.

         _TestSocket = ConnectOp.GetSocket();
         _ConnectEvent.SetEvent();
    }


    static VOID
    _ConnectCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

        ISocketConnectOp& ConnectOp = (ISocketConnectOp&) Op;
        HttpClientTest* This = (HttpClientTest *) ConnectOp.GetUserData();
        This->ConnectCallback(ConnectOp);
    }


    //
    //
    // Error cases:
    //  IP not found (timeout)
    //  DNS name not found (where is this,address resolution?
    //  Connection refused (Bad port)
    //
    //
    NTSTATUS
    ConnectSocket()
    {
        NTSTATUS Result;
        ISocketConnectOp::SPtr ConnectOp;

        Result = _Transport->CreateConnectOp(ConnectOp);
        RETURN_ON_ERROR(Result);

        ConnectOp->SetUserData(this);

        //
        // Use the default wildcard local address.
        // Leave the routing decision to the OS.
        //
        _LocalAddress.Reset();
        
        Result = ConnectOp->StartConnect(_LocalAddress, _RemoteAddress, _ConnectCallback);
        RETURN_ON_ERROR(Result);

        _ConnectEvent.WaitUntilSet();

        // The status in the socket object should have been propagated
        // from the final status of the connect op.
        //
        Result = ConnectOp->Status();
        RETURN_ON_ERROR(Result);

        Result = _TestSocket->Status();
        RETURN_ON_ERROR(Result);

        return STATUS_SUCCESS;
    }

    ////////////////////////////////
    //
    // Error cases:
    //      (a) Treat partial or zero-byte write as a close-socket failure
    //      (b) Close the other end while this is in transit
    //


    VOID
    WriteCallback(
        __in ISocketWriteOp& WriteOp
        )
    {
        // NTSTATUS Result = 
            WriteOp.Status();
        // ULONG Transferred = 
            WriteOp.BytesTransferred();
        _WriteEvent.SetEvent();
    }

    static VOID
    _WriteCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

        ISocketWriteOp& WriteOp = (ISocketWriteOp&) Op;
        HttpClientTest* This = (HttpClientTest *) WriteOp.GetUserData();
        This->WriteCallback(WriteOp);
    }

    NTSTATUS
    WriteSocket()
    {
        NTSTATUS Result;
        ISocketWriteOp::SPtr WriteOp;

        Result = _TestSocket->CreateWriteOp(WriteOp);
        RETURN_ON_ERROR(Result);

        WriteOp->SetUserData(this);

        // Write a simple HTTP request
        //
        STRING Str;
        RtlInitString(&Str, "GET /index HTTP/1.1\r\nHost: www.google.com:80\r\n\r\n");
        KMemRef Buf;

        Buf._Address = Str.Buffer;
        Buf._Param = Str.Length;
        Buf._Size = Str.Length;

        Result = WriteOp->StartWrite(1, &Buf, 1, _WriteCallback);

        RETURN_ON_ERROR(Result);

        _WriteEvent.WaitUntilSet();

        // Check the write operation
        Result = WriteOp->Status();
        RETURN_ON_ERROR(Result);

        ULONG Transferred = WriteOp->BytesTransferred();
        KTestPrintf("Wrote %u bytes\n", Transferred);

        if (Transferred != Str.Length)
        {
            return STATUS_UNSUCCESSFUL;
        }
        return STATUS_SUCCESS;
    }

    ////////////////////////////////


    VOID
    ReadCallback(
        __in ISocketReadOp& ReadOp
        )
    {
         // NTSTATUS Result = 
             ReadOp.Status();
         // ULONG Transferred = 
             ReadOp.BytesTransferred();
         _ReadEvent.SetEvent();
    }

    static VOID
    _ReadCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        )
    {
        UNREFERENCED_PARAMETER(Parent);

        ISocketReadOp& ReadOp = (ISocketReadOp&) Op;
        HttpClientTest* This = (HttpClientTest *) ReadOp.GetUserData();
        This->ReadCallback(ReadOp);
    }

    NTSTATUS
    ReadSocket()
    {
        NTSTATUS Result;
        ISocketReadOp::SPtr ReadOp;

        UCHAR* Buf = _newArray<UCHAR>(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), 0x10000);
        if (!Buf)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KFinally([&](){ _deleteArray(Buf); });

        KMemRef MemRef;
        MemRef._Address = Buf;
        MemRef._Size = 0x10000;

        Result = _TestSocket->CreateReadOp(ReadOp);
        RETURN_ON_ERROR(Result);

        ReadOp->SetUserData(this);

        Result = ReadOp->StartRead(1, &MemRef, ISocketReadOp::ReadAny, _ReadCallback);
        RETURN_ON_ERROR(Result);

        _ReadEvent.WaitUntilSet();

        Result = ReadOp->Status();
        RETURN_ON_ERROR(Result);

        ULONG Received = ReadOp->BytesTransferred();
        KTestPrintf("Received %u bytes\n", Received);

        return STATUS_SUCCESS;
    }


    NTSTATUS Run(
        __in ITransport::SPtr& Transport,
        __in LPCWSTR Server,
        __in USHORT HttpPort
        )
    {
        _Transport = Transport;

        NTSTATUS Result;
        Result = GetAddresses(Server, HttpPort);
        RETURN_ON_ERROR(Result);

        Result = ConnectSocket();
        RETURN_ON_ERROR(Result);

        KFinally([&]() { _TestSocket->Close(); KTestPrintf("Closed the socket...\n"); });

        KTestPrintf("Doing a run of 50 Write/Read pairs on the same socket...\n");
        for (int i = 0; i < 50; i++)
        {
            Result = WriteSocket();
            RETURN_ON_ERROR(Result);

            Result = ReadSocket();
            RETURN_ON_ERROR(Result);
        }

        return STATUS_SUCCESS;
    }
};




//////////////////////////////////////////////////////////////////////////

NTSTATUS
HttpTestSequence(LPCWSTR Server, USHORT HttpPort)
{
    KTestPrintf("Starting basic HTTP client test sequence\n");
    ITransport::SPtr Trans;
    NTSTATUS Status;

#if KTL_USER_MODE

    Status = KUmTcpTransport::Initialize(*g_Allocator, Trans);
    RETURN_ON_ERROR(Status);

#else

    Status = KKmTcpTransport::Initialize(*g_Allocator, Trans);
    RETURN_ON_ERROR(Status);

#endif

    // Block to ensure a destructor call
    for (int i = 0; i < 10; i++)
    {
         HttpClientTest Test;
         Status = Test.Run(Trans, Server, HttpPort);
         if (!NT_SUCCESS(Status))
         {
            break;
         }
    }

    Trans->Shutdown();

    return Status;
}

NTSTATUS
RunEchoServer(USHORT Port)
{
    KTestPrintf("Starting basic HTTP client test sequence\n");
    ITransport::SPtr Trans;
    NTSTATUS Status;

#if KTL_USER_MODE

    Status = KUmTcpTransport::Initialize(*g_Allocator, Trans);
    RETURN_ON_ERROR(Status);

#else

    Status = KKmTcpTransport::Initialize(*g_Allocator, Trans);
    RETURN_ON_ERROR(Status);

#endif

    EchoServerTest Test;

    Status = Test.Run(Trans, Port);

    return Status;
}



NTSTATUS
KSocketTest(
    int argc, WCHAR* args[]
    )
{
    if (argc < 2)
    {
        KTestPrintf("Must use command line parameters:\n");
        KTestPrintf("  -httpclient    ServerName [Port]   // Port is optional if 80 is not to be used\n");
        KTestPrintf("  -echoserver    Port\n");
        return STATUS_UNSUCCESSFUL;
    }

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    NTSTATUS Result = STATUS_SUCCESS;
    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting KSocketTest test\n");
    KtlSystem::Initialize();
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    // Command line parser.
    //
    CmdLineParser Parser(*g_Allocator);
    Parameter *Param;
    BOOLEAN HttpClient = FALSE;
    BOOLEAN EchoServer = FALSE;
    USHORT  EchoPort = 0;
    USHORT  HttpPort = 80;
    LPCWSTR  Server = 0;

    if (Parser.Parse( argc, args))
    {
        if (!Parser.ParameterCount())
        {
            KTestPrintf("Missing parameters\n");
            return STATUS_UNSUCCESSFUL;
        }
        Param = Parser.GetParam(0);
        if ( _wcsicmp(Param->_Name, L"echoserver") == 0 )
        {
            EchoServer = TRUE;
            if (Param->_ValueCount)
            {
                EchoPort = (USHORT) _wtoi(Param->_Values[0]);
            }
            else
            {
                KTestPrintf("Missing port number\n");
                return STATUS_UNSUCCESSFUL;
            }
        }
        else if ( _wcsicmp(Param->_Name, L"httpclient") == 0 )
        {
            HttpClient = TRUE;
            Server = Param->_Values[0];

            if (Param->_ValueCount > 1)
            {
                HttpPort = (USHORT) _wtoi(Param->_Values[1]);
            }
        }
        else
        {
            KTestPrintf("Unknown command line param\n");
            return STATUS_UNSUCCESSFUL;
        }
    }


    if (HttpClient)
    {
        KTestPrintf("Running HTTP Client against %S, Port=%u\n", Server, HttpPort);
        Result = HttpTestSequence(Server, HttpPort);
    }
    else if (EchoServer)
    {
        KTestPrintf("Running EchoServer against port %u\n", EchoPort);
        Result = RunEchoServer(EchoPort);
    }

    Parser.Reset();

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

    return KSocketTest(argc, args);
}
#endif
