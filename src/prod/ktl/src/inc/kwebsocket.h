/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kwebsocket.h

    Description:
      Kernel Tempate Library (KTL): WebSocket Package

      User-mode only

    History:
      tyadam          27-March-20114         Initial version.

--*/

#pragma once
#include <KAsyncService.h>

#if KTL_USER_MODE

//
//  Define narrow and wide versions of a string constant
//
#ifndef KDualStringDefine
#define KDualStringDefine(str, value) \
    CHAR const * const str##A = value; \
    WCHAR const * const str = L##value;
#endif

//
//  WebSocket-relevant headers to be used in Unknown headers section when looking
//  at and responding to a websocket client
//
//  See: http://tools.ietf.org/html/rfc6455#section-11.3
//
namespace WebSocketHeaders
{
    KDualStringDefine(Connection,   "Connection");
    KDualStringDefine(Upgrade,      "Upgrade");
    KDualStringDefine(Key,          "Sec-WebSocket-Key");
    KDualStringDefine(Protocol,     "Sec-WebSocket-Protocol");    //  Subprotocol
    KDualStringDefine(Version,      "Sec-WebSocket-Version");     //  Protocol version(s)
    KDualStringDefine(Extensions,   "Sec-WebSocket-Extensions");  //  Currently unused (protocol extension)
};

namespace WebSocketSchemes
{
    WCHAR const * const Insecure  =  L"ws";
    WCHAR const * const Secure    =  L"wss";
};

class KWebSocket : public KAsyncServiceBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWebSocket);

public:

    static const USHORT MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES = 125;

    //
    //  Queryable internal state of the WebSocket connection, e.g. OpenStarted/Open/Closed
    //
    //  Values:
    //      Error – An error has occurred, so the WebSocket is in an Error/Invalid state.  All
    //          operations will fail.
    //
    //      Initialized – The Open procedure is available to be started, but has not yet
    //          started.  Send and Receive operations are able to be started but are not
    //          being processed.
    //
    //      OpenStarted – The transport is open and the handshake is in process.  Send and
    //          Receive operations are able to be started but are not being processed.
    //
    //      Open – The WebSocket is fully open.  Send and Receive operations are able to be
    //          started and are being processed.
    //
    //      CloseInitiated – StartCloseWebSocket has been called, and a CLOSE frame has not
    //          been received.  Send operations will fail to start and in-flight Send
    //          operations will fail as they are processed.
    //
    //      CloseReceived – A CLOSE frame has been received, but StartCloseWebSocket has not
    //          been called.  Receive operations will fail to start and in-flight Receive
    //          operations will fail as they are processed.
    //
    //      CloseCompleting – Both a CLOSE frame has been received and StartCloseWebSocket
    //          has been called, but the close procedure is not yet complete.  Send and
    //          Receive operations will fail to start and in-flight Send and Receive
    //          operations will fail as they are processed.
    //
    //      Closed – The close procedure is complete.  The service close callback will fire
    //          upon transitioning to this state.  Send and Receive operations will fail to
    //          start and in-flight Send and Receive operations will fail as the are
    //          processed.
    //
    enum class ConnectionStatus : SHORT
    {
        Error            = -1,
        Initialized      =  1,
        OpenStarted      =  2,
        Open             =  3,
        CloseInitiated   =  4,
        CloseReceived    =  5,
        CloseCompleting  =  6,
        Closed           =  7,
    };

    //
    //  The type of data a particular message contains (text/binary)
    //
    //  Values:
    //      Invalid – Data is invalid and MUST NOT be interpreted
    //
    //      Text – Data is to be validated and interpreted as UTF-8 text
    //
    //      Binary – Data is arbitrary binary data to be interpreted by the application
    //
    enum class MessageContentType : SHORT
    {
        Invalid  = -1,
        Text     =  1,
        Binary   =  2,
    };

    //
    //  Timing constants available to be set by SetTimingConstant
    //
    //  Values:
    //      OpenTimeoutMs – The number of milliseconds to allow the open handshake to execute
    //          before the WebSocket will transition to an error state.  Accepted
    //          values for this option are KWebSocket::TimingConstantValue::Default,
    //          KWebSocket::TimingConstantValue::None, or a positive nonzero LONGLONG
    //          integer in the ULONG range.  Without being set, the default is 5000 (five seconds).
    //
    //      CloseTimeoutMs – The number of milliseconds to allow the Close procedure to execute
    //          before the WebSocket will transition to an error state.  Accepted
    //          values are KWebSocket::TimingConstantValue::Default,
    //          KWebSocket::TimingConstantValue::None, or a positive nonzero LONG
    //          integer in the ULONG range.  Without being set, the default is 5000 (five seconds).
    //
    //      PongKeepalivePeriodMs – The number of milliseconds to wait after the last
    //          transport send before issuing an unsolicited PONG frame.  Accepted values for this option
    //          are KWebSocket::TimingConstantValue::Default,
    //          KWebSocket::TimingConstantValue::None, or a positive nonzero LONGLONG
    //          integer in the ULONG range.  Without being set, the default is
    //          KWebSocket::TimingConstantValue::None (no pongs will be sent to keep
    //          the transport alive).
    //
    //      PingQuietChannelPeriodMs – The number of milliseconds to wait after the last
    //          transport receive before issuing a PING frame.  Accepted values for this
    //          option are KWebSocket::TimingConstantValue::Default,
    //          KWebSocket::TimingConstantValue::None, or a positive nonzero LONGLONG
    //          integer in the ULONG range.  Without being set, the default is
    //          KWebSocket::TimingConstantValue::None (no pings will be sent to verify
    //          that the remote endpoint is responsible).
    //
    //      PongTimeoutMs – The number of milliseconds to allow for a PONG or CLOSE frame to be
    //          received after sending a PING frame before the WebSocket will transition to an
    //          error state.  Accepted values for this option are
    //          KWebSocket::TimingConstantValue::Default,
    //          KWebSocket::TimingConstantValue::None, or a positive nonzero LONGLONG
    //          integer in the ULONG range.  Without being set, the default is 5000 (five seconds).
    //
    enum class TimingConstant : SHORT
    {
        OpenTimeoutMs               =  1,
        CloseTimeoutMs              =  2,
        PongKeepalivePeriodMs       =  3,
        PingQuietChannelPeriodMs    =  4,
        PongTimeoutMs               =  5
    };

    //
    //  Values to pass to SetTimingConstant.  Used as a typedef with some special values defined.
    //
    //  Values:
    //      Invalid – Value is invalid and MUST NOT be interpreted 
    //
    //      Default – Use the internally-specified default value for this parameter.
    //          Explicitly setting an option to this value is equivalent to not setting the
    //          option.
    //
    //      None – Do not use this paramteter (e.g. infinite timeout, or operation based on
    //          a period should never be executed)
    //
    enum class TimingConstantValue : LONGLONG
    {
        Invalid     = -2,
        Default     = -1,
        None        =  0,
    };

    //
    //  Standard status codes for WebSocket closure defined by RFC 6455.  See the RFC for
    //  descriptions.  Used as a typedef with some special values defined.
    //
    enum class CloseStatusCode : USHORT
    {
        //  Nonstandard
        InvalidStatusCode   =  0,

        //  Standard
        NormalClosure       =  1000,
        GoingAway           =  1001,
        ProtocolError       =  1002,
        InvalidDataFormat   =  1003,
        Reserved            =  1004,  //  Reserved for future use
        NoStatus            =  1005,  //  May NOT be set as a status when closing if CloseReason is specified
        AbnormalClosure     =  1006,  //  May NOT be set as a status when closing
        DataFormatMismatch  =  1007,
        GenericClosure      =  1008,
        MessageTooLarge     =  1009,
        MissingExtension    =  1010,
    };

    //
    //  Called when the WebSocket receives a CLOSE frame
    //
    typedef KDelegate<VOID(
            KWebSocket&
        )> WebSocketCloseReceivedCallback;

protected:

    enum class FrameType : USHORT
    {
        Invalid                     =  0,
        TextContinuationFragment    =  1,
        BinaryContinuationFragment  =  2,
        TextFinalFragment           =  3,
        BinaryFinalFragment         =  4,
        Close                       =  5,
        Ping                        =  6,
        Pong                        =  7
    };

    enum class UserOperationType : USHORT
    {
        Invalid             =  0,
        FragmentSend        =  1,
        MessageSend         =  2,
        FragmentReceive     =  3,
        MessageReceive      =  4,
        CloseSend           =  5
    };

    static inline MessageContentType
    FrameTypeToMessageContentType(
        __in FrameType Type
        )
    {
        switch (Type)
        {
        case FrameType::TextContinuationFragment:
        case FrameType::TextFinalFragment:
            return MessageContentType::Text;

        case FrameType::BinaryContinuationFragment:
        case FrameType::BinaryFinalFragment:
            return MessageContentType::Binary;

        default:
            return MessageContentType::Invalid;
        }
    }

    static inline BOOLEAN
    IsControlFrame(
        __in FrameType Type
        )
    {
        return Type == FrameType::Close
            || Type == FrameType::Ping
            || Type == FrameType::Pong;
    }

    static inline BOOLEAN
    IsFinalFragment(
        __in FrameType Type
        )
    {
        return Type == FrameType::TextFinalFragment
            || Type == FrameType::BinaryFinalFragment;
    }

    //
    //  Common base class for user operations
    //
    class WebSocketOperation
    {

    public:

        //  Complete the operation.  Implementation should just call KAsyncContextBase::Complete()
        virtual VOID
        CompleteOperation(
            __in NTSTATUS Status
            ) = 0;

        UserOperationType
        GetOperationType()
        {
            return _OperationType;
        }

    protected:

        WebSocketOperation(
            __in UserOperationType OperationType
            );

    public:

        KListEntry _Links;

    protected:

        UserOperationType _OperationType;
    };

    //
    //  Base class for internal representation of user receive operations (receiving user data)
    //
    class WebSocketReceiveOperation : public WebSocketOperation
    {
    public:

        virtual NTSTATUS
        HandleReceive(
            __in_bcount(DataLengthInBytes) PVOID Data,
            __in ULONG DataLengthInBytes,
            __in MessageContentType DataContentType,
            __in BOOLEAN DataCompletesMessage,
            __out ULONG& DataConsumedInBytes,
            __out BOOLEAN& ReceiveMoreData
            ) = 0;

        //  After the transport receive is completed, this will return the length of the user data in the buffer
        ULONG
        GetBytesReceived();

        //
        //  Get the content type of this operation's data
        //
        MessageContentType
        GetDataContentType();

        virtual VOID
        CompleteOperation(
            __in NTSTATUS Status
            ) override = 0;

    protected:

        WebSocketReceiveOperation(
            __in KWebSocket& Context,
            __in UserOperationType OperationType
            );

        ~WebSocketReceiveOperation()
        {
        }

    protected:

        KBuffer::SPtr _Buffer;
        ULONG _Offset;
        ULONG _BufferSizeInBytes;
        MessageContentType _MessageContentType;

        ULONG _BytesReceived;

        KWebSocket::SPtr _Context;
    };

    //
    //  Base class for internal representation of user send operations (User data is encapsulated and sent as one or more
    //  instances of WebSocketSendFragmentOperation)
    //
    class WebSocketSendFragmentOperation : public WebSocketOperation
    {

    public:

        PBYTE
        GetData()
        {
            return static_cast<PBYTE>(_Buffer->GetBuffer()) + _Offset;
        }

        ULONG
        GetDataLength()
        {
            return _BufferSizeInBytes;
        }

        BOOLEAN
        IsFinalFragment()
        {
            return _IsFinalFragment;
        }

        MessageContentType
        GetContentType()
        {
            return _MessageContentType;
        }

        virtual VOID
        CompleteOperation(
            __in NTSTATUS Status
            ) override = 0;

    protected:

        WebSocketSendFragmentOperation(
            __in KWebSocket& Context,
            __in UserOperationType OperationType
            );

        ~WebSocketSendFragmentOperation()
        {
        }

    protected:

        KBuffer::SPtr _Buffer;
        ULONG _Offset;
        ULONG _BufferSizeInBytes;
        MessageContentType _MessageContentType;

        BOOLEAN _IsFinalFragment;

        KWebSocket::SPtr _Context;
    };

    //
    //  Base class for internal representation of user send-close operations
    //
    class WebSocketSendCloseOperation : public WebSocketOperation
    {
    
    public:

        PBYTE
        GetCloseReason()
        {
            return static_cast<PBYTE>((PVOID) *_CloseReason);
        }

        ULONG
        GetCloseReasonLength()
        {
            return _CloseReason->LengthInBytes();
        }

        virtual VOID
        CompleteOperation(
            __in NTSTATUS Status
            ) override = 0;

    protected:

        WebSocketSendCloseOperation();

        ~WebSocketSendCloseOperation()
        {
        }

    protected:

        CloseStatusCode _CloseStatus;
        KSharedBufferStringA::SPtr _CloseReason;
    };

public:

    //
    //  Set a timing constant value.  Not all options may be supported by a given derivation.
    //  Returns an error status if an invalid  constant or value is specified, or if an unsupported constant is specified.
    //
    //  Implementations should return K_STATUS_OPTION_UNSUPPORTED if the TimingConstant is not supported for the 
    //  derivation, STATUS_INVALID_PARAMETER_1 for an invalid Constant, and STATUS_INVALID_PARAMETER_2 for an invalid Value.
    //
    //  Parameters:
    //      Constant – The constant to set.
    //
    //      Value – The value to apply to this constant.  Allowed values are specific to each
    //          constant.
    //
    virtual NTSTATUS
    SetTimingConstant(
        __in TimingConstant Constant,
        __in TimingConstantValue Value
        );

    //
    //  Query the current value of a timing constant.
    //
    //  Parameters:
    //      Constant – The constant to query.
    //
    TimingConstantValue
    GetTimingConstant(
        __in TimingConstant Constant
        );

    //
    //  Gracefully close the WebSocket (soft-close) asynchronously.
    //
    //  Parameters:
    //      CloseStatusCode – An optional 2-byte unsigned integer signifying the reason
    //          for closure.  Defined according to the rules in section 7.4 of RFC 6455.  MUST
    //          be specified and not equal to NoStatus if CloseReason is specified.
    //
    //      CloseReason – An optional UTF-8 string explaining the reason for closure.  MAY NOT
    //          be specified if CloseStatusCode is not specified or equal to NoStatus.
    //
    NTSTATUS
    StartCloseWebSocket(
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncServiceBase::CloseCompletionCallback CallbackPtr,
        __in_opt CloseStatusCode const CloseStatusCode = CloseStatusCode::NormalClosure,
        __in_opt KSharedBufferStringA::SPtr CloseReason = nullptr
        );

    //
    //  Trigger an ungraceful close of the WebSocket (hard-close) synchronously by terminating
    //  the underlying transport session.
    //
    //  Note: the service will not be closed by this procedure, and all calls to Abort() should
    //  be accompanied by a call to StartCloseWebSocket().
    //
    virtual VOID
    Abort() = 0;

    //
    //  An operation encapsulating the reading of a single fragment from the
    //  WebSocket. Intended to be used for streaming or for reading messages where the size
    //  is unknown ahead of time by the receiver.
    //
    class ReceiveFragmentOperation : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(ReceiveFragmentOperation);

    public:

        //
        //  Start the fragment receive operation.
        //
        //  Parameters:
        //      Buffer – Buffer to hold the fragment data.  Note: future overload taking
        //          a KIoBufferElement may support high-performance page-aligned IO.
        //
        //      Offset – Offset into the supplied KBuffer to copy the received data
        //
        //      Size – Size in bytes of the buffer region to write to.  Value used will
        //          be the minimum between the value supplied (if any) and the size of
        //          the buffer region bounded by [Offset, Buffer.QuerySize()).
        //
        virtual VOID
        StartReceiveFragment(
            __inout KBuffer& Buffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
            __in ULONG const Offset = 0,
            __in ULONG const Size = ULONG_MAX
            ) = 0;

        //
        //  Return a shared pointer to the receive buffer.
        //
        virtual KBuffer::SPtr
        GetBuffer() = 0;

        //
        //  Get the number of bytes received into the buffer.  ONLY to
        //  be called in Completed state.
        //
        virtual ULONG
        GetBytesReceived() = 0;

        //
        //  Get the message content type.  ONLY to be called in Completed state.
        //
        virtual MessageContentType
        GetContentType() = 0;

        //
        //  Get whether this fragment was marked final.  ONLY to be called in Completed state.
        //
        virtual BOOLEAN
        IsFinalFragment() = 0;

    private:

        //
        //  Disallow canceling WebSocket operations.  Users of a WebSocket should instead abort or close the WebSocket.
        //  Casting the operation to a KAsyncContextBase and calling Cancel() is unsupported and the results are undefined.
        //
        BOOLEAN
        Cancel();
    };

    //
    //  Create a ReceiveFragmentOperation
    //
    virtual NTSTATUS
    CreateReceiveFragmentOperation(
        __out ReceiveFragmentOperation::SPtr& ReceiveOperation
        );


    //
    //  An operation encapsulating the receiving of a logical message (1 to N fragments) from
    //  the WebSocket.  Multiple fragments will be transparently coalesced as needed.  
    //
    class ReceiveMessageOperation : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(ReceiveMessageOperation);

    public:

        //
        //  Start the message receive operation.
        //
        //  Parameters:
        //      Buffer – Buffer to hold the message data  Note: future overload taking
        //          a KIoBuffer may support high-performance page-aligned IO.
        //
        //      Offset – Offset into the supplied KBuffer to copy the received data
        //
        //      Size – Size in bytes of the buffer region to write to.  Value used will
        //          be the minimum between the value supplied (if any) and the size of
        //          the buffer region bounded by [Offset, Buffer.QuerySize()). If the buffer
        //          region specified is too small to contain the message, this operation will
        //          fail and the WebSocket will transition to an error state.
        //
        virtual VOID
        StartReceiveMessage(
            __inout KBuffer& Buffer,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
            __in ULONG const Offset = 0,
            __in ULONG const Size = ULONG_MAX
            ) = 0;

        //
        //  Return a shared pointer to the receive buffer.
        //
        virtual KBuffer::SPtr
        GetBuffer() = 0;

        //
        //  Get the number of bytes received into the buffer.  ONLY to
        //  be called in Completed state.
        //
        virtual ULONG
        GetBytesReceived() = 0;

        //
        //  Get the message content type.  ONLY to be called in Completed state.
        //
        virtual MessageContentType
        GetContentType() = 0;

    private:

        //
        //  Disallow canceling WebSocket operations.  Users of a WebSocket should instead abort or close the WebSocket.
        //  Casting the operation to a KAsyncContextBase and calling Cancel() is unsupported and the results are undefined.
        //
        BOOLEAN
        Cancel();
    };

    //
    //  Create a ReceiveMessageOperation
    //
    virtual NTSTATUS
    CreateReceiveMessageOperation(
        __out ReceiveMessageOperation::SPtr& ReceiveOperation
        );


    //
    //  An operation encapsulating the sending of a single fragment over the WebSocket.
    //  Intended to be used for streaming or for fragmenting large messages.
    //
    class SendFragmentOperation : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(SendFragmentOperation);

    public:

        //
        //  Start the fragment send operation.
        //
        //  Parameters:
        //      Data – Buffer containing the fragment data to send.  Note: future overload taking a
        //          KIoBufferElement may support high-performance page-aligned IO.
        //
        //      IsFinalFragment – TRUE if this is the final fragment of a message, FALSE
        //          otherwise. 
        //
        //      FragmentDataFormat – The data format that this message fragment is to be 
        //          interpreted as.
        //
        //      Offset – Offset into the supplied KBuffer to write from
        //
        //      Size – size in bytes of the buffer region to send.  Value used will be the
        //          minimum between the value supplied (if any) and the size of the buffer
        //          region bounded by [Offset, Buffer.QuerySize()).
        //
        virtual VOID
        StartSendFragment(
            __in KBuffer& Data,
            __in BOOLEAN IsFinalFragment,
            __in MessageContentType FragmentDataFormat,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
            __in ULONG const Offset = 0,
            __in ULONG const Size = ULONG_MAX
            ) = 0;

    private:

        //
        //  Disallow canceling WebSocket operations.  Users of a WebSocket should instead abort or close the WebSocket.
        //
        using KAsyncContextBase::Cancel;
    };

    //
    //  Create a SendFragmentOperation
    //
    virtual NTSTATUS
    CreateSendFragmentOperation(
        __out SendFragmentOperation::SPtr& SendOperation
        ) = 0;


    //
    //  An operation encapsulating the sending of a logical message over the WebSocket.
    //  Arbitrary fragmentation may take place due to e.g. message size.
    // 
    class SendMessageOperation : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(SendMessageOperation);

    public:

        //
        //  Start the message send operation.
        //
        //  Parameters:
        //      Message – Buffer containing the message data to send.  Note: future overload taking a
        //          KIoBuffer may support high-performance page-aligned IO.
        //
        //      MessageDataFormat – The data format that this message is to be interpreted as.
        //
        //      Offset – Offset into the supplied KBuffer to write from
        //
        //      Size – size in bytes of the buffer region to send.  Value used will be the
        //          minimum between the value supplied (if any) and the size of the buffer
        //          region bounded by [Offset, Buffer.QuerySize()).
        // 
        virtual VOID
        StartSendMessage(
            __in KBuffer& Message,
            __in MessageContentType MessageDataFormat,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
            __in ULONG const Offset = 0,
            __in ULONG const Size = ULONG_MAX
            ) = 0;

    private:

        //
        //  Disallow canceling WebSocket operations.  Users of a WebSocket should instead abort or close the WebSocket.
        //
        using KAsyncContextBase::Cancel;
    };
    
    //
    //  Create a SendMessageOperation
    //
    virtual NTSTATUS
    CreateSendMessageOperation(
        __out SendMessageOperation::SPtr& SendOperation
        ) = 0;


    //
    //  Return the current connection status of this WebSocket.  This MUST agree with the
    //  value returned by KAsyncServiceBase::GetOpenStatus()
    //
    ConnectionStatus
    GetConnectionStatus();

    //
    //  Return the Active Subprotocol, if one has been negotiated.
    //
    virtual KStringViewA
    GetActiveSubProtocol()
	{
		return KStringViewA("");
	}

    //
    //  Return the closure status code passed to StartCloseWebSocket.  ONLY to be called while the WebSocket is in the CloseInitiated,
    //  CloseCompleting, or Closed states.  May be a standard status code defined in the CloseStatusCode enum, or an app-defined value
    //
    CloseStatusCode
    GetLocalWebSocketCloseStatusCode();

    //
    //  Return the closure status code sent by the remote WebSocket.  ONLY to be called while the WebSocket is in the CloseReceived,
    //  CloseCompleting, or Closed states.  May be a standard status code defined in the CloseStatusCode enum, or an app-defined value
    //
    CloseStatusCode
    GetRemoteWebSocketCloseStatusCode();

    //
    //  Return the closure reason string passed to StartCloseWebSocket.  ONLY to be called while the WebSocket is in the CloseInitiated,
    //  CloseCompleting, or Closed states.  Underlying string is UTF-8, which is not supported by the KStringView API.
    //
    KSharedBufferStringA::SPtr
    GetLocalCloseReason();

    //
    //  Return the closure reason string sent by the remote WebSocket.  ONLY to be called while the WebSocket is in the CloseReceived,
    //  CloseCompleting, or Closed states.  Underlying string is UTF-8, which is not supported by the KStringView API.
    //
    KSharedBufferStringA::SPtr
    GetRemoteCloseReason();

protected:

    KWebSocket(
        __in KAsyncQueue<WebSocketReceiveOperation>& ReceiveRequestQueue,
        __in KAsyncQueue<WebSocketOperation>& SendRequestQueue
        );

    static VOID
    SelectSubprotocol(
        __in KStringViewA const & SupportedSubprotocolsString,
        __in KStringViewA const & RequestSubProtocolsString,
        __out KStringViewA& SelectedSubprotocol
        );

    VOID
    SetDefaultTimingConstantValues();

    static TimingConstantValue
    GetTimingConstantDefaultValue(
        __in TimingConstant TimingConstant
        );

    inline BOOLEAN
    IsOpenCompleted()
    {
        ConnectionStatus state = GetConnectionStatus();
        return state != ConnectionStatus::Initialized
            && state != ConnectionStatus::OpenStarted;
    }

    inline BOOLEAN
    IsCloseStarted()
    {
        ConnectionStatus state = GetConnectionStatus();
        return state == ConnectionStatus::CloseInitiated
            || state == ConnectionStatus::CloseCompleting;
    }

    inline BOOLEAN
    IsStateTerminal()
    {
        ConnectionStatus state = GetConnectionStatus();
        return state == ConnectionStatus::Error
            || state == ConnectionStatus::Closed;
    }

protected:

    class ReceiveFragmentOperationImpl;
    class ReceiveMessageOperationImpl;

protected:

    CloseStatusCode  _LocalCloseStatusCode;
    CloseStatusCode  _RemoteCloseStatusCode;
    KSharedBufferStringA::SPtr  _LocalCloseReason;
    KSharedBufferStringA::SPtr _RemoteCloseReason;

    ConnectionStatus  _ConnectionStatus;

    KSpinLock _StartOpenLock;

    KAsyncQueue<WebSocketReceiveOperation>::SPtr _ReceiveRequestQueue;
    KAsyncQueue<WebSocketOperation>::SPtr _SendRequestQueue;
    BOOLEAN _IsReceiveRequestQueueDeactivated;

private:

    //  Timing constants
    TimingConstantValue  _OpenTimeoutMs;
    TimingConstantValue  _CloseTimeoutMs;
    TimingConstantValue  _PongKeepalivePeriodMs;
    TimingConstantValue  _PingQuietChannelPeriodMs;
    TimingConstantValue  _PongTimeoutMs;

    volatile ULONG _IsCloseStarted;
};

#endif
