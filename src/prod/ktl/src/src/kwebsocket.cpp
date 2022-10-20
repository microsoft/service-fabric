#include <ktl.h>
#include <ktrace.h>

#if KTL_USER_MODE

class KWebSocket::ReceiveFragmentOperationImpl : public ReceiveFragmentOperation, public WebSocketReceiveOperation
{
    friend KWebSocket;
    K_FORCE_SHARED(ReceiveFragmentOperationImpl);

public:

    KDeclareSingleArgumentCreate(
        ReceiveFragmentOperation,       //  Output SPtr type
        ReceiveFragmentOperationImpl,   //  Implementation type
        ReceiveOperation,               //  Output arg name
        KTL_TAG_WEB_SOCKET,             //  default allocation tag
        KWebSocket&,                    //  Constructor arg type
        WebSocketContext                //  Constructor arg name
        );

    ReceiveFragmentOperationImpl(
        __in KWebSocket& Context
        );

//  ReceiveFragmentOperation Implementation
public:

    VOID
    StartReceiveFragment(
        __inout KBuffer& Buffer,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
        __in ULONG const Offset = 0,
        __in ULONG const Size = ULONG_MAX
        ) override;

    KBuffer::SPtr
    GetBuffer() override;

    ULONG 
    GetBytesReceived() override;

    MessageContentType
    GetContentType() override;

    BOOLEAN
    IsFinalFragment() override;

//  WebSocketReceiveOperation Implementation
public:

    NTSTATUS
    HandleReceive(
        __in_bcount(DataLengthInBytes) PVOID Data,
        __in ULONG DataLengthInBytes,
        __in MessageContentType DataContentType,
        __in BOOLEAN DataCompletesMessage,
        __out ULONG& DataConsumedInBytes,
        __out BOOLEAN& ReceiveMoreData
        ) override;

    VOID
    CompleteOperation(
        __in NTSTATUS Status
        ) override;

private:

    VOID OnStart() override;

    VOID OnReuse() override;

    BOOLEAN _IsFinalFragment;
};

class KWebSocket::ReceiveMessageOperationImpl : public ReceiveMessageOperation, public WebSocketReceiveOperation
{
    friend KWebSocket;
    K_FORCE_SHARED(ReceiveMessageOperationImpl);

public:

    KDeclareSingleArgumentCreate(
        ReceiveMessageOperation,        //  Output SPtr type
        ReceiveMessageOperationImpl,    //  Implementation type
        ReceiveOperation,               //  Output arg name
        KTL_TAG_WEB_SOCKET,             //  default allocation tag
        KWebSocket&,                    //  Constructor arg type
        WebSocketContext                //  Constructor arg name
        );

    ReceiveMessageOperationImpl(
        __in KWebSocket& Context
        );

//  ReceiveMessageOperation Implementation
public:

    VOID
    StartReceiveMessage(
        __inout KBuffer& Buffer,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
        __in ULONG const Offset = 0,
        __in ULONG const Size = ULONG_MAX
        ) override;

    KBuffer::SPtr
    GetBuffer() override;

    ULONG
    GetBytesReceived() override;

    MessageContentType
    GetContentType() override;

//  WebSocketReceiveOperation Implementation
public:

    NTSTATUS
    HandleReceive(
        __in_bcount(DataLengthInBytes) PVOID Data,
        __in ULONG DataLengthInBytes,
        __in MessageContentType DataContentType,
        __in BOOLEAN DataCompletesMessage,
        __out ULONG& DataConsumedInBytes,
        __out BOOLEAN& ReceiveMoreData
        ) override;

    VOID
    CompleteOperation(
        __in NTSTATUS Status
        ) override;

//  
private:

    VOID OnStart() override;

    VOID OnReuse() override;
};

NTSTATUS
KWebSocket::CreateReceiveFragmentOperation(
    __out ReceiveFragmentOperation::SPtr& ReceiveOperation
    ) 
{
    return ReceiveFragmentOperationImpl::Create(ReceiveOperation, *this, GetThisAllocator());
}


VOID KWebSocket::ReceiveFragmentOperationImpl::OnReuse()
{
    _Buffer = nullptr;
    _BytesReceived = 0;
    _Offset = 0;
    _BufferSizeInBytes = 0;
}

VOID KWebSocket::ReceiveMessageOperationImpl::OnReuse()
{
    _Buffer = nullptr;
    _BytesReceived = 0;
    _Offset = 0;
    _BufferSizeInBytes = 0;
}



NTSTATUS
KWebSocket::CreateReceiveMessageOperation(
    __out ReceiveMessageOperation::SPtr& ReceiveOperation
    ) 
{
    return ReceiveMessageOperationImpl::Create(ReceiveOperation, *this, GetThisAllocator());
}



KWebSocket::KWebSocket(
    __in KAsyncQueue<WebSocketReceiveOperation>& ReceiveOperationQueue,
    __in KAsyncQueue<WebSocketOperation>& SendOperationQueue
    ) 
    :
        _ReceiveRequestQueue(&ReceiveOperationQueue),
        _SendRequestQueue(&SendOperationQueue),

        _ConnectionStatus(ConnectionStatus::Initialized),
        _LocalCloseStatusCode(CloseStatusCode::InvalidStatusCode),
        _RemoteCloseStatusCode(CloseStatusCode::InvalidStatusCode),

        _IsCloseStarted(FALSE),
        _IsReceiveRequestQueueDeactivated(FALSE)
{
    SetDefaultTimingConstantValues();
    NTSTATUS status;

    status = KSharedBufferStringA::Create(_LocalCloseReason, GetThisAllocator());
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = KSharedBufferStringA::Create(_RemoteCloseReason, GetThisAllocator());
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
}

KWebSocket::~KWebSocket() {}

VOID
KWebSocket::SelectSubprotocol(
    __in KStringViewA const & SupportedSubprotocolsString,
    __in KStringViewA const & RequestedSubProtocolsString,
    __out KStringViewA& SelectedSubprotocol
    )
{
    KStringViewA empty("");
    SelectedSubprotocol = empty;

    if (SupportedSubprotocolsString.Compare(empty) == 0
        || RequestedSubProtocolsString.Compare(empty) == 0)
    {
        return;
    }

    KStringViewTokenizerA supportedTokenizer;
    KStringViewTokenizerA requestedTokenizer;

    KStringViewA supportedSubprotocol;
    KStringViewA requestedSubprotocol;

    supportedTokenizer.Initialize(SupportedSubprotocolsString, ',');
    while (supportedTokenizer.NextToken(supportedSubprotocol))
    {
        supportedSubprotocol.StripWs(TRUE, TRUE);

        requestedTokenizer.Initialize(RequestedSubProtocolsString, ',');
        while (requestedTokenizer.NextToken(requestedSubprotocol))
        {
            requestedSubprotocol.StripWs(TRUE, TRUE);

            if (supportedSubprotocol.Compare(requestedSubprotocol) == 0)
            {
                SelectedSubprotocol = supportedSubprotocol;
                return;
            }
        }
    }
}


KWebSocket::TimingConstantValue
KWebSocket::GetTimingConstantDefaultValue(
    __in TimingConstant Constant
    )
{
    switch (Constant)
    {
    case TimingConstant::OpenTimeoutMs:
        return static_cast<TimingConstantValue>(5000L);

    case TimingConstant::CloseTimeoutMs:
        return static_cast<TimingConstantValue>(5000L);

    case TimingConstant::PongKeepalivePeriodMs:
        return TimingConstantValue::None;

    case TimingConstant::PingQuietChannelPeriodMs:
        return TimingConstantValue::None;

    case TimingConstant::PongTimeoutMs:
        return static_cast<TimingConstantValue>(5000L);

    default:
        return TimingConstantValue::Invalid;
    }
}

VOID
KWebSocket::SetDefaultTimingConstantValues()
{
    SetTimingConstant(TimingConstant::OpenTimeoutMs, TimingConstantValue::Default);
    SetTimingConstant(TimingConstant::CloseTimeoutMs, TimingConstantValue::Default);
    SetTimingConstant(TimingConstant::PongKeepalivePeriodMs, TimingConstantValue::Default);
    SetTimingConstant(TimingConstant::PingQuietChannelPeriodMs, TimingConstantValue::Default);
    SetTimingConstant(TimingConstant::PongTimeoutMs, TimingConstantValue::Default);
}

NTSTATUS
KWebSocket::SetTimingConstant(
    __in TimingConstant Constant,
    __in TimingConstantValue Value
    )
{
    K_LOCK_BLOCK(_StartOpenLock)
    {
        if (GetConnectionStatus() != ConnectionStatus::Initialized)
        {
            return K_STATUS_API_CLOSED;
        }

        if (Value == TimingConstantValue::Invalid 
            || (Value != TimingConstantValue::None 
                && Value != TimingConstantValue::Default
                && (static_cast<LONGLONG>(Value) <= 0 || static_cast<LONGLONG>(Value) > ULONG_MAX)
                )
            )
        {
            return STATUS_INVALID_PARAMETER_2;
        }

        if (Value == TimingConstantValue::Default)
        {
            Value = GetTimingConstantDefaultValue(Constant);
        }

        switch (Constant)
        {
        case TimingConstant::OpenTimeoutMs:
            _OpenTimeoutMs = Value;
            break;

        case TimingConstant::CloseTimeoutMs:
            _CloseTimeoutMs = Value;
            break;

        case TimingConstant::PongKeepalivePeriodMs:
            _PongKeepalivePeriodMs = Value;
            break;

        case TimingConstant::PingQuietChannelPeriodMs:
            _PingQuietChannelPeriodMs = Value;
            break;

        case TimingConstant::PongTimeoutMs:
            _PongTimeoutMs = Value;
            break;

        default:
            return STATUS_INVALID_PARAMETER_1;
        }
    }

    return STATUS_SUCCESS;
}

KWebSocket::TimingConstantValue
KWebSocket::GetTimingConstant(
    __in TimingConstant Constant
    )
{
    switch (Constant)
    {
        case TimingConstant::OpenTimeoutMs:
            return _OpenTimeoutMs;

        case TimingConstant::CloseTimeoutMs:
            return _CloseTimeoutMs;

        case TimingConstant::PongKeepalivePeriodMs:
            return _PongKeepalivePeriodMs;

        case TimingConstant::PingQuietChannelPeriodMs:
            return _PingQuietChannelPeriodMs;

        case TimingConstant::PongTimeoutMs:
            return _PongTimeoutMs;

        default:
            return TimingConstantValue::Invalid;
    }
}

KWebSocket::ConnectionStatus
KWebSocket::GetConnectionStatus()
{
    return _ConnectionStatus;
}

KWebSocket::CloseStatusCode 
KWebSocket::GetLocalWebSocketCloseStatusCode()
{
    return _LocalCloseStatusCode;       
}

KWebSocket::CloseStatusCode 
KWebSocket::GetRemoteWebSocketCloseStatusCode()
{
    return _RemoteCloseStatusCode;       
}

KSharedBufferStringA::SPtr
KWebSocket::GetLocalCloseReason()
{
    return _LocalCloseReason;
}

KSharedBufferStringA::SPtr
KWebSocket::GetRemoteCloseReason()
{
    return _RemoteCloseReason;
}

KWebSocket::WebSocketOperation::WebSocketOperation(
    __in UserOperationType OperationType
    )
    :
        _OperationType(OperationType)
{
}

KWebSocket::MessageContentType
KWebSocket::WebSocketReceiveOperation::GetDataContentType()
{
    return _MessageContentType;
}

KWebSocket::WebSocketReceiveOperation::WebSocketReceiveOperation(
    __in KWebSocket& Context,
    __in UserOperationType OperationType
    )
    :
        WebSocketOperation(OperationType),

        _Buffer(nullptr),
        _Offset(0),
        _BufferSizeInBytes(0),
        _MessageContentType(MessageContentType::Invalid),

        _BytesReceived(0),

        _Context(&Context)
{
    KAssert(_OperationType == UserOperationType::MessageReceive || _OperationType == UserOperationType::FragmentReceive);
}



KWebSocket::WebSocketSendFragmentOperation::WebSocketSendFragmentOperation(
    __in KWebSocket& Context,
    __in UserOperationType OperationType
    )
    :
        WebSocketOperation(OperationType),

        _Buffer(nullptr),
        _Offset(0),
        _BufferSizeInBytes(0),
        _MessageContentType(MessageContentType::Invalid),

        _IsFinalFragment(FALSE),

        _Context(&Context)
{
    KAssert(_OperationType == UserOperationType::MessageSend || _OperationType == UserOperationType::FragmentSend);
}

KWebSocket::WebSocketSendCloseOperation::WebSocketSendCloseOperation()
    :
        WebSocketOperation(UserOperationType::CloseSend),

        _CloseStatus(CloseStatusCode::InvalidStatusCode),
        _CloseReason(nullptr)
{
}


KWebSocket::ReceiveFragmentOperation::ReceiveFragmentOperation()
{
}

KWebSocket::ReceiveFragmentOperation::~ReceiveFragmentOperation()
{
}


KWebSocket::ReceiveMessageOperation::ReceiveMessageOperation()
{
}

KWebSocket::ReceiveMessageOperation::~ReceiveMessageOperation()
{
}


KWebSocket::SendFragmentOperation::SendFragmentOperation()
{
}

KWebSocket::SendFragmentOperation::~SendFragmentOperation()
{
}



KWebSocket::SendMessageOperation::SendMessageOperation()
{
}

KWebSocket::SendMessageOperation::~SendMessageOperation()
{
}


ULONG
KWebSocket::WebSocketReceiveOperation::GetBytesReceived()
{
    return _BytesReceived;
}

NTSTATUS
KWebSocket::StartCloseWebSocket(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncServiceBase::CloseCompletionCallback CallbackPtr,
    __in_opt CloseStatusCode const CloseStatusCode,
    __in_opt KSharedBufferStringA::SPtr CloseReason
    )
{
    //
    //  Allow a single call to StartClose
    //
    if (KInterlockedSetFlag(_IsCloseStarted))
    {
        _LocalCloseStatusCode = CloseStatusCode;
        _LocalCloseReason = CloseReason ? CloseReason : _RemoteCloseReason;

        return StartClose(ParentAsyncContext, CallbackPtr);
    }
    
    KDbgCheckpointWData(
        0,
        "StartCloseWebSocket failed due to already being called.",
        STATUS_SHARING_VIOLATION,
        ULONG_PTR(this),
        static_cast<SHORT>(_ConnectionStatus),
        static_cast<USHORT>(CloseStatusCode),
        0
        );
    return STATUS_SHARING_VIOLATION;
}




KBuffer::SPtr
KWebSocket::ReceiveFragmentOperationImpl::GetBuffer()
{
    return _Buffer;
}

VOID
KWebSocket::ReceiveFragmentOperationImpl::StartReceiveFragment(
    __inout KBuffer& Buffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
    __in ULONG const Offset,
    __in ULONG const Size
    )
{
    _Buffer = KBuffer::SPtr(&Buffer);
    _Offset = Offset;
    _BufferSizeInBytes = Size;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KWebSocket::ReceiveFragmentOperationImpl::OnStart()
{
    NTSTATUS status;

    status = _Context->_ReceiveRequestQueue->Enqueue(*this);
    if (! NT_SUCCESS(status))
    {
        Complete(K_STATUS_API_CLOSED);
    }
}

ULONG
KWebSocket::ReceiveFragmentOperationImpl::GetBytesReceived()
{
    return _BytesReceived;
}

KWebSocket::MessageContentType
KWebSocket::ReceiveFragmentOperationImpl::GetContentType()
{
    return _MessageContentType;
}

BOOLEAN
KWebSocket::ReceiveFragmentOperationImpl::IsFinalFragment()
{
    return _IsFinalFragment;
}

NTSTATUS
KWebSocket::ReceiveFragmentOperationImpl::HandleReceive(
    __in_bcount(DataLengthInBytes) PVOID Data,
    __in ULONG DataLengthInBytes,
    __in MessageContentType DataContentType,
    __in BOOLEAN DataCompletesMessage,
    __out ULONG& DataConsumedInBytes,
    __out BOOLEAN& ReceiveMoreData
    )
{
    NTSTATUS status;

    _MessageContentType = DataContentType;
    if (DataContentType == MessageContentType::Invalid)
    {
        DataConsumedInBytes = 0;
        ReceiveMoreData = FALSE;
        status = STATUS_BAD_DATA;

        _IsFinalFragment = FALSE;
        _BytesReceived = 0;

        KTraceFailedAsyncRequest(status, this, static_cast<ULONGLONG>(DataContentType), 0);
        Complete(status);
        return status;
    }

    DataConsumedInBytes = __min(_BufferSizeInBytes - _BytesReceived, DataLengthInBytes);
    ReceiveMoreData = FALSE;
    status = STATUS_SUCCESS;

    KMemCpySafe(
        static_cast<PUCHAR>(_Buffer->GetBuffer()) + _Offset + _BytesReceived, 
        _Buffer->QuerySize() - (_Offset + _BytesReceived), 
        Data, 
        DataConsumedInBytes);

    _IsFinalFragment = (DataConsumedInBytes == DataLengthInBytes) && DataCompletesMessage;
    _BytesReceived += DataConsumedInBytes;

    Complete(status);
    return status;
}


KDefineSingleArgumentCreate(KWebSocket::ReceiveFragmentOperation, KWebSocket::ReceiveFragmentOperationImpl, ReceiveOperation, KWebSocket&, WebSocketContext);

KWebSocket::ReceiveFragmentOperationImpl::ReceiveFragmentOperationImpl(
    __in KWebSocket& Context
    )
    :
        WebSocketReceiveOperation(Context, UserOperationType::FragmentReceive)
{
}

KWebSocket::ReceiveFragmentOperationImpl::~ReceiveFragmentOperationImpl()
{
}

VOID
KWebSocket::ReceiveFragmentOperationImpl::CompleteOperation(
    __in NTSTATUS Status
    )
{
    Complete(Status);
}




KBuffer::SPtr
KWebSocket::ReceiveMessageOperationImpl::GetBuffer()
{
    return _Buffer;
}

VOID
KWebSocket::ReceiveMessageOperationImpl::StartReceiveMessage(
    __inout KBuffer& Buffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
    __in ULONG const Offset,
    __in ULONG const Size
    )
{
    _Buffer = KSharedPtr<KBuffer>(&Buffer);
    _Offset = Offset;
    _BufferSizeInBytes = Size;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KWebSocket::ReceiveMessageOperationImpl::OnStart()
{
    NTSTATUS status;

    status = _Context->_ReceiveRequestQueue->Enqueue(*this);
    if (! NT_SUCCESS(status))
    {
        Complete(K_STATUS_API_CLOSED);
    }
}

ULONG
KWebSocket::ReceiveMessageOperationImpl::GetBytesReceived()
{
    return _BytesReceived;
}

KWebSocket::MessageContentType
KWebSocket::ReceiveMessageOperationImpl::GetContentType()
{
    return _MessageContentType;
}

NTSTATUS
KWebSocket::ReceiveMessageOperationImpl::HandleReceive(
    __in_bcount(DataLengthInBytes) PVOID Data,
    __in ULONG DataLengthInBytes,
    __in MessageContentType DataContentType,
    __in BOOLEAN DataCompletesMessage,
    __out ULONG& DataConsumedInBytes,
    __out BOOLEAN& ReceiveMoreData
    )
{
    NTSTATUS status;

    if (_BytesReceived + DataLengthInBytes > _BufferSizeInBytes) //  The buffer is not large enough
    {
        DataConsumedInBytes = 0;
        ReceiveMoreData = FALSE;
        status = STATUS_BUFFER_OVERFLOW;

        KTraceFailedAsyncRequest(status, this, _BytesReceived + DataLengthInBytes, _BufferSizeInBytes);
        Complete(status);
        return status;
    }
    else if (DataContentType == MessageContentType::Invalid)
    {
        DataConsumedInBytes = 0;
        ReceiveMoreData = FALSE;
        status = STATUS_BAD_DATA;

        KTraceFailedAsyncRequest(status, this, static_cast<ULONGLONG>(DataContentType), 0);
        Complete(status);
        return status;
    }
    else
    {
        if (_BytesReceived == 0) //  This is the first receive, so set the content type
        {
            _MessageContentType = DataContentType;
        }
        else if (DataContentType != _MessageContentType) //  content type mismatch with previously received data
        {
            DataConsumedInBytes = 0;
            ReceiveMoreData = FALSE;
            status = STATUS_BAD_DATA;

            KTraceFailedAsyncRequest(status, this, static_cast<ULONGLONG>(DataContentType), static_cast<ULONGLONG>(_MessageContentType));
            Complete(status);
            return status;
        }

        DataConsumedInBytes = DataLengthInBytes;
        status = STATUS_SUCCESS;

        KMemCpySafe(
            static_cast<PUCHAR>(_Buffer->GetBuffer()) + _Offset + _BytesReceived, 
            _Buffer->QuerySize() - (_Offset + _BytesReceived), 
            Data, 
            DataConsumedInBytes);

        _BytesReceived += DataConsumedInBytes;

        if (DataCompletesMessage)
        {
            ReceiveMoreData = FALSE;
            Complete(status);
            return status;
        }
        else
        {
            ReceiveMoreData = TRUE;
            return status;
        }
    }
}


KDefineSingleArgumentCreate(KWebSocket::ReceiveMessageOperation, KWebSocket::ReceiveMessageOperationImpl, ReceiveOperation, KWebSocket&, WebSocketContext);

KWebSocket::ReceiveMessageOperationImpl::ReceiveMessageOperationImpl(
    __in KWebSocket& Context
    )
    :
        WebSocketReceiveOperation(Context, UserOperationType::MessageReceive)
{
}

KWebSocket::ReceiveMessageOperationImpl::~ReceiveMessageOperationImpl()
{
}

VOID
KWebSocket::ReceiveMessageOperationImpl::CompleteOperation(
    __in NTSTATUS Status
    )
{
    Complete(Status);
}


#endif