/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:

    kmessageport.h

Abstract:

    This file defines the message port and message socket classes for messaging.

Author:

    Norbert P. Kusters (norbertk) 27-Jan-2012

Environment:

    Kernel mode and User mode

Notes:

--*/

class KMessage : public KObject<KMessage>, public KShared<KMessage> {

    K_FORCE_SHARED_WITH_INHERITANCE(KMessage);

    public:

        //
        // A list of the different types of KMessage objects with possibly different on wire representations.
        //

        enum MessageEncodingType : ULONG {
            eStandard = 1
        };

        //
        // The different types of buffers.
        //

        enum BufferType : ULONG {
            eNormal = 0,
            ePageAligned = 1
        };

        static
        NTSTATUS
        Create(
            __in MessageEncodingType EncodingType,
            __out KMessage::SPtr& Message,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_MESSAGE_PORT
            );

        virtual
        MessageEncodingType
        QueryMessageEncodingType(
            ) = 0;

        //
        // Routines for building up the Message
        //

        virtual
        VOID
        Clear(
            ) = 0;

        virtual
        KBuffer*
        AddNormalBuffer(
            ) = 0;

        virtual
        NTSTATUS
        AddPageAlignedBuffer(
            __in VOID* Buffer,
            __in ULONG Size
            ) = 0;

        //
        // Routines to query/modify message buffers.
        //

        virtual
        ULONG
        QueryNumberOfBuffers(
            ) = 0;

        virtual
        BufferType
        QueryBufferType(
            __in ULONG Index
            ) = 0;

        virtual
        KBuffer*
        GetNormalBuffer(
            __in ULONG Index
            ) = 0;

        virtual
        VOID*
        GetPageAlignedBuffer(
            __in ULONG Index,
            __out ULONG& Size
            ) = 0;

        virtual
        VOID
        SetPageAlignedBuffer(
            __in ULONG Index,
            __in VOID* Buffer,
            __in ULONG Size
            ) = 0;

        virtual
        NTSTATUS
        RemoveBuffer(
            __in ULONG Index
            ) = 0;

        //
        // Routines to manipulate the properties of this message.
        //

        virtual
        NTSTATUS
        InitializeProperties(
            ) = 0;

        virtual
        NTSTATUS
        PrepareBuffers(
            ) = 0;

};

class KMessageEndpointAddress : public KObject<KMessageEndpointAddress>, public KShared<KMessageEndpointAddress> {

    K_FORCE_SHARED_WITH_INHERITANCE(KMessageEndpointAddress);

    public:

        enum AddressType : ULONG {
            eTcp = 1,
            eSmbDirect = 2
        };

        static
        NTSTATUS
        Create(
            __in_z const WCHAR* Uri,
            __out KMessageEndpointAddress::SPtr& MessageEndpointAddress,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_MESSAGE_PORT
            );

        virtual
        BOOLEAN
        IsType(
            __in AddressType Type
            ) = 0;

        virtual
        NTSTATUS
        Serialize(
            __out KBuffer::SPtr& Buffer,
            __in ULONG AllocationTag = KTL_TAG_MESSAGE_PORT
            ) = 0;

        virtual
        NTSTATUS
        Deserialize(
            __in_bcount(Size) VOID* Buffer,
            __in ULONG Size
            ) = 0;

        virtual
        const WCHAR*
        GetString(
            ) = 0;

};

class KMessageSocket : public KObject<KMessageSocket>, public KShared<KMessageSocket> {

    K_FORCE_SHARED_WITH_INHERITANCE(KMessageSocket);

    public:

        class SendContext : public KAsyncContextBase {
            K_FORCE_SHARED_WITH_INHERITANCE(SendContext);
        };

        virtual
        NTSTATUS
        AllocateSend(
            __out SendContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_MESSAGE_PORT
            ) = 0;

        virtual
        VOID
        Send(
            __in SendContext& Async,
            __in KMessage& Message,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL
            ) = 0;

        virtual
        KMessageEndpointAddress*
        GetLocalMessageEndpointAddress(
            ) = 0;

        virtual
        KMessageEndpointAddress*
        GetRemoteMessageEndpointAddress(
            ) = 0;

        virtual
        VOID
        Close(
            ) = 0;

};

class KMessagePort : public KObject<KMessagePort>, public KShared<KMessagePort> {

    K_FORCE_SHARED_WITH_INHERITANCE(KMessagePort);

    public:

        class SetMessageReceiveCallbackContext : public KAsyncContextBase {
            K_FORCE_SHARED_WITH_INHERITANCE(SetMessageReceiveCallbackContext);
        };

        class CloseContext : public KAsyncContextBase {
            K_FORCE_SHARED_WITH_INHERITANCE(CloseContext);
        };

        //
        // 'MessageReceiveAllocationCallback' is called first when a message is received if there are any page aligned buffers
        // that are part of the message.
        //
        // The message object has all of the buffers of the message defined but where all of the page aligned buffers are set to
        // NULL.
        //
        // This call may return a failure if it cannot fill in the memory pointers for all of the page aligned buffers.
        //
        // This function may set the 'ContextFromAllocate' parameter in order to help pair up when
        // 'MessageReceiveCompletedCallback' is called.
        //
        // If this function returns success, then this guarantees that the 'MessageReceiveCompletedCallback' will be made.
        //

        typedef
        KDelegate<
        NTSTATUS (
            __in KMessageSocket& MessageSocket,
            __inout KMessage& Message,
            __out_opt VOID*& ContextFromAllocate
            )> MessageReceiveAllocationCallback;

        //
        // 'MessageReceiveCompletedCallback' is called once the message is completely populated from the incoming data
        // over the network.
        //
        // This call is guaranteed to happen if a 'MessageReceiveAllocationCallback' was made.
        //
        // Any failure in transmission from the sender is reflected in the 'Status'.
        //

        typedef
        KDelegate<
        VOID (
            __in NTSTATUS Status,
            __in KMessageSocket& MessageSocket,
            __inout KMessage& Message,
            __in_opt VOID* ContextFromAllocate
            )> MessageReceiveCompletedCallback;

        //
        // 'Create' will create a local port for the given physical URI.  Only the authority part of this URI is used, for
        // creating the local endpoint.  In the case of TCP, this amounts to just the local IP address and port number.  If
        // 'EnableInboundConnections' is set to true then this indicates that the port will accept connections.
        //

        static
        NTSTATUS
        Create(
            __in_z const WCHAR* LocalUri,
            __in BOOLEAN EnableInboundConnections,
            __out KMessagePort::SPtr& MessagePort,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_MESSAGE_PORT
            );

        virtual
        NTSTATUS
        AllocateSetMessageReceiveCallbacks(
            __out SetMessageReceiveCallbackContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_MESSAGE_PORT
            ) = 0;

        //
        // 'SetMessageReceiveCallback' instantiate the use of the new callbacks for receive and drains out the use of any old
        // callbacks.  This async does not complete until the old callbacks are out of service.
        //

        virtual
        NTSTATUS
        SetMessageReceiveCallbacks(
            __in SetMessageReceiveCallbackContext& Async,
            __in MessageReceiveAllocationCallback ReceiveAllocationCallback,
            __in MessageReceiveCompletedCallback ReceiveCompletedCallback,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL
            ) = 0;

        //
        // 'CreateMessageSocket' will just give the pointer to the logical connection with the given URI.  Nothing really
        // happens here until 'Send'.
        //

        virtual
        NTSTATUS
        CreateMessageSocket(
            __in KMessageEndpointAddress& MessageEndpointAddress,
            __out KMessageSocket::SPtr& MessageSocket
            ) = 0;

        virtual
        NTSTATUS
        AllocateClose(
            __out CloseContext::SPtr& Async,
            __in ULONG AllocationTag = KTL_TAG_MESSAGE_PORT
            ) = 0;

        //
        // 'Close' disables this message port and completes once there can no longer be any use of any registered receive
        // callbacks.
        //

        virtual
        VOID
        Close(
            __in CloseContext& Async,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync = NULL
            ) = 0;

};
