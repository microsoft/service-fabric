/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kKerberos.cpp

    Description:
      Kernel Tempate Library (KTL): KKerberos

      Describes a Kerberos wrapper for network security purposes.

    History:
      leikong         Nov-2012.........Adding implementation
--*/

#include <ktl.h>
#include <ktrace.h>

#if KTL_USER_MODE
 #include <ws2tcpip.h>
 #include <ws2ipdef.h>
 #include <iphlpapi.h>
#else
 #include <netioapi.h>
 #include <windef.h>
 #include <wbasek.h>
 #include <winerror.h>
 #include <spseal.h>
#endif

#include <schannel.h>

#pragma region KerbFramingSocket

class KerbFramingSocket : public ISocket
{
    friend class KKerbTransport;
    friend class KerbFramingSocketReadOp;
    friend class KerbFramingSocketWriteOp;

public:
    typedef KSharedPtr<KerbFramingSocket> SPtr;

    static const ULONG MaxFrameSize = 0xffff; // 64KB

    ~KerbFramingSocket() {}

    VOID Close();

    NTSTATUS CreateReadOp(_Out_ ISocketReadOp::SPtr& NewOp);
    NTSTATUS CreateWriteOp(_Out_ ISocketWriteOp::SPtr& NewOp);

    ULONG GetMaxMessageSize();

    NTSTATUS GetLocalAddress(_Out_ INetAddress::SPtr& Addr);
    NTSTATUS GetRemoteAddress(_Out_ INetAddress::SPtr& Addr);

    ULONG PendingOps();

    VOID SetUserData(_In_ PVOID Ptr);
    PVOID GetUserData();

    BOOLEAN Ok();

    PVOID GetSocketId();

    BOOLEAN Lock();
    VOID Unlock();

private:
    K_DENY_COPY(KerbFramingSocket);

    KerbFramingSocket(ISocket::SPtr && innerSocket);

    ISocket::SPtr const _innerSocket;
};

class KerbFramingSocketReadOp : public ISocketReadOp
{
    friend class KerbFramingSocket;

public:
    ~KerbFramingSocketReadOp(){}

    NTSTATUS StartRead(
        ULONG bufferCount,
        _In_ KMemRef* buffers,
        ULONG flags,
        _In_opt_ KAsyncContextBase::CompletionCallback callback,
        _In_opt_ KAsyncContextBase* const parentContext = nullptr);

    KSharedPtr<ISocket> GetSocket();

    ULONG BytesToTransfer();
    ULONG BytesTransferred();

    VOID SetUserData(_In_ PVOID ptr);
    PVOID GetUserData();

#if KTL_IO_TIMESTAMP
    KIoTimestamps const * IoTimestamps();
#endif

protected:
    void OnStart();

private:
    KerbFramingSocketReadOp(KerbFramingSocket::SPtr && framingSocket);

    void StartInnerReadOp(_In_ KMemRef & buffer);
    void InnerReadCallback(
        _In_opt_ KAsyncContextBase* const parent,
        _In_ KAsyncContextBase& completingOperation);

private:
    KerbFramingSocket::SPtr _framingSocket;
    ISocketReadOp::SPtr _innerSocketReadOp;
    CompletionCallback _innerReadCallback;
    PVOID _userData;
    KMemRef _outputBuffer;
    ULONG _frameSize;
    bool _frameSizeRead;
};

KerbFramingSocketReadOp::KerbFramingSocketReadOp(KerbFramingSocket::SPtr && framingSocket)
    : _framingSocket(Ktl::Move(framingSocket))
{
    NTSTATUS status = _framingSocket->_innerSocket->CreateReadOp(_innerSocketReadOp);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _innerReadCallback.Bind(this, &KerbFramingSocketReadOp::InnerReadCallback);
}

#if KTL_IO_TIMESTAMP

KIoTimestamps const * KerbFramingSocketReadOp::IoTimestamps()
{
    return _innerSocketReadOp->IoTimestamps();
}

#endif

_Use_decl_annotations_
NTSTATUS KerbFramingSocketReadOp::StartRead(
    ULONG bufferCount,
    KMemRef* buffers,
    ULONG flags,
    KAsyncContextBase::CompletionCallback callback,
    KAsyncContextBase* const parentContext)
{
    bufferCount;
    KAssert(bufferCount == 1); // KKerbSocket always reads into exactly one buffer
    KAssert(buffers != nullptr);
    flags; // flags is ignored because we always read one complete frame

    _outputBuffer = buffers[0];

    Start(parentContext, callback);
    return STATUS_PENDING;
}

void KerbFramingSocketReadOp::OnStart()
{
    KMemRef frameSizeBuffer;
    frameSizeBuffer._Address = &_frameSize;
    frameSizeBuffer._Size = frameSizeBuffer._Param = sizeof(_frameSize);
    _frameSizeRead = false;

    StartInnerReadOp(frameSizeBuffer);
}

_Use_decl_annotations_
void KerbFramingSocketReadOp::StartInnerReadOp(KMemRef & buffer)
{
    _innerSocketReadOp->Reuse();
    NTSTATUS status = _innerSocketReadOp->StartRead(
        1,
        &buffer,
        ISocketReadOp::ReadAll,
        _innerReadCallback,
        this);

    if (!NT_SUCCESS(status))
    {
        Complete(status);
    }
}

_Use_decl_annotations_
void KerbFramingSocketReadOp::InnerReadCallback(
    KAsyncContextBase* const parent,
    KAsyncContextBase& completingOperation)
{
    KFatal(static_cast<KAsyncContextBase*>(this) == parent);
    KFatal(static_cast<KAsyncContextBase*>(_innerSocketReadOp.RawPtr()) == &completingOperation);

    NTSTATUS status = _innerSocketReadOp->Status();
    if ((status != STATUS_SUCCESS) || _frameSizeRead)
    {
        Complete(status);
        return;
    }

    _frameSizeRead = true;
    _frameSize = RtlUlongByteSwap(_frameSize);
    //KDbgPrintf(
    //    "KerbFraming: socket %llx: read frame size: %u\n",
    //    _framingSocket->_innerSocket->GetSocketId(), _frameSize);

    if (_frameSize > KerbFramingSocket::MaxFrameSize)
    {
        KDbgPrintf(
            "socket %llx: KerbFramingSocketReadOp: incoming frame size %u exceeds limit %u",
            _frameSize,
            KerbFramingSocket::MaxFrameSize);

        Complete(STATUS_UNSUCCESSFUL);
        return;
    }

    if (_frameSize > _outputBuffer._Size)
    {
        KDbgPrintf(
            "socket %llx: KerbFramingSocketReadOp: incoming frame size %u exceeds output buffer size %u",
            _frameSize,
            _outputBuffer._Size);

        Complete(STATUS_INVALID_PARAMETER);
        return;
    }

    _outputBuffer._Size = _outputBuffer._Param = _frameSize;
    StartInnerReadOp(_outputBuffer);
}

ULONG KerbFramingSocketReadOp::BytesToTransfer()
{
    return _outputBuffer._Size;
}

ULONG KerbFramingSocketReadOp::BytesTransferred()
{
    return (Status() == STATUS_SUCCESS)? _frameSize : 0;
}

ISocket::SPtr KerbFramingSocketReadOp::GetSocket()
{
    return _framingSocket.RawPtr();
}

_Use_decl_annotations_
void KerbFramingSocketReadOp::SetUserData(PVOID Ptr)
{
    _userData = Ptr;
}

PVOID KerbFramingSocketReadOp::GetUserData()
{
    return _userData;
}

class KerbFramingSocketWriteOp : public ISocketWriteOp
{
    friend class KerbFramingSocket;

public:
    NTSTATUS StartWrite(
        ULONG bufferCount,
        _In_ KMemRef* buffers,
        ULONG priority,
        _In_opt_ KAsyncContextBase::CompletionCallback callback = 0,
        _In_opt_ KAsyncContextBase* const parentContext = nullptr);

    ULONG BytesToTransfer();
    ULONG BytesTransferred();

    ISocket::SPtr GetSocket();

    void SetUserData(_In_ PVOID Ptr);
    PVOID GetUserData();

#if KTL_IO_TIMESTAMP
    KIoTimestamps const * IoTimestamps() override;
#endif

protected:
    void OnStart();
    void OnReuse();

private:
    KerbFramingSocketWriteOp(KerbFramingSocket::SPtr && framingSocket);

    void InnerWriteCallback(
        _In_ KAsyncContextBase* const parent,
        _In_ KAsyncContextBase& completingOperation);

private:
    KerbFramingSocket::SPtr _framingSocket;
    ULONG _frameSize;
    ULONG _frameSizeByteSwapped;
    KMemRef _inputBuffers[2];
    ULONG _priority;
    PVOID _userData;

    ISocketWriteOp::SPtr _innerSocketWriteOp;
    CompletionCallback _innerWriteCallback;
};

KerbFramingSocketWriteOp::KerbFramingSocketWriteOp(KerbFramingSocket::SPtr && framingSocket) : _framingSocket(Ktl::Move(framingSocket))
{
    NTSTATUS status = _framingSocket->_innerSocket->CreateWriteOp(_innerSocketWriteOp);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _innerWriteCallback.Bind(this, &KerbFramingSocketWriteOp::InnerWriteCallback);
}

#if KTL_IO_TIMESTAMP

KIoTimestamps const * KerbFramingSocketWriteOp::IoTimestamps()
{
    return _innerSocketWriteOp->IoTimestamps();
}

#endif

_Use_decl_annotations_
NTSTATUS KerbFramingSocketWriteOp::StartWrite(
    ULONG bufferCount,
    KMemRef* buffers,
    ULONG priority,
    KAsyncContextBase::CompletionCallback callback,
    KAsyncContextBase* const parentContext)
{
    bufferCount;
    KAssert(bufferCount == 1); // Kerberos always send one buffer
    KAssert(buffers != nullptr);

    if (buffers[0]._Size > KerbFramingSocket::MaxFrameSize)
    {
        KDbgPrintf(
            "socket %llx: KerbFramingSocket::StartWrite: input buffer size %u exceeds limit %u",
            buffers[0]._Size,
            KerbFramingSocket::MaxFrameSize);

        return STATUS_INVALID_PARAMETER;
    }

    _frameSize = buffers[0]._Param;
    _frameSizeByteSwapped = RtlUlongByteSwap(_frameSize);
    _inputBuffers[0]._Address = &_frameSizeByteSwapped;
    _inputBuffers[0]._Size = _inputBuffers[0]._Param = sizeof(_frameSizeByteSwapped);
    _inputBuffers[1] = buffers[0];

    _priority = priority;

    Start(parentContext, callback);
    return STATUS_PENDING;
}

void KerbFramingSocketWriteOp::OnStart()
{
    //KDbgPrintf(
    //    "KerbFraming: socket %llx: writing frame of size: %u\n",
    //    _framingSocket->_innerSocket->GetSocketId(), _frameSize);

    NTSTATUS status = _innerSocketWriteOp->StartWrite(
        sizeof(_inputBuffers)/sizeof(KMemRef),
        _inputBuffers,
        _priority,
        _innerWriteCallback,
        this);

    if (!NT_SUCCESS(status))
    {
        Complete(status);
    }
}

void KerbFramingSocketWriteOp::OnReuse()
{
    _innerSocketWriteOp->Reuse();
}

_Use_decl_annotations_
void KerbFramingSocketWriteOp::InnerWriteCallback(
    KAsyncContextBase* const parent,
    KAsyncContextBase& completingOperation)
{
    KFatal(static_cast<KAsyncContextBase*>(this) == parent);
    KFatal(static_cast<KAsyncContextBase*>(_innerSocketWriteOp.RawPtr()) == &completingOperation);
    Complete(_innerSocketWriteOp->Status());
}

ULONG KerbFramingSocketWriteOp::BytesToTransfer()
{
    return _frameSize;
}

ULONG KerbFramingSocketWriteOp::BytesTransferred()
{
    return (Status() == STATUS_SUCCESS)? _frameSize : 0;
}

ISocket::SPtr KerbFramingSocketWriteOp::GetSocket()
{
    return _framingSocket.RawPtr();
}

_Use_decl_annotations_
void KerbFramingSocketWriteOp::SetUserData(PVOID Ptr)
{
    _userData = Ptr;
}

PVOID KerbFramingSocketWriteOp::GetUserData()
{
    return _userData;
}

KerbFramingSocket::KerbFramingSocket(ISocket::SPtr && innerSocket) : _innerSocket(Ktl::Move(innerSocket))
{
}

VOID KerbFramingSocket::Close()
{
    _innerSocket->Close();
}

_Use_decl_annotations_
NTSTATUS KerbFramingSocket::CreateReadOp(ISocketReadOp::SPtr& newOp)
{
    newOp = _new(KTL_TAG_SSPI, GetThisAllocator()) KerbFramingSocketReadOp(this);
    if (!newOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return newOp->Status();
}

_Use_decl_annotations_
NTSTATUS KerbFramingSocket::CreateWriteOp(ISocketWriteOp::SPtr& newOp)
{
    newOp = _new(KTL_TAG_SSPI, GetThisAllocator()) KerbFramingSocketWriteOp(this);
    if (!newOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return newOp->Status();
}

ULONG KerbFramingSocket::GetMaxMessageSize()
{
    return (KerbFramingSocket::MaxFrameSize <= _innerSocket->GetMaxMessageSize()) ?
        KerbFramingSocket::MaxFrameSize : _innerSocket->GetMaxMessageSize();
}

_Use_decl_annotations_
NTSTATUS KerbFramingSocket::GetLocalAddress(INetAddress::SPtr & addr)
{
    return _innerSocket->GetLocalAddress(addr);
}

_Use_decl_annotations_
NTSTATUS KerbFramingSocket::GetRemoteAddress(INetAddress::SPtr & addr)
{
    return _innerSocket->GetRemoteAddress(addr);
}

ULONG KerbFramingSocket::PendingOps()
{
    return _innerSocket->PendingOps();
}

_Use_decl_annotations_
VOID KerbFramingSocket::SetUserData(PVOID ptr)
{
    _innerSocket->SetUserData(ptr);
}

PVOID KerbFramingSocket::GetUserData()
{
    return _innerSocket->GetUserData();
}

BOOLEAN KerbFramingSocket::Ok()
{
    return _innerSocket->Ok();
}

PVOID KerbFramingSocket::GetSocketId()
{
    return _innerSocket->GetSocketId();
}

BOOLEAN KerbFramingSocket::Lock()
{
    return _innerSocket->Lock();
}

VOID KerbFramingSocket::Unlock()
{
    _innerSocket->Unlock();
}

#pragma endregion

#pragma region KKerbSocketReadOp

KKerbSocketReadOp::KKerbSocketReadOp(KKerbSocket::SPtr && kerbSocket) : KSspiSocketReadOp(kerbSocket.RawPtr())
{
}

void KKerbSocketReadOp::DecryptMessage()
{
    KKerbSocket::DataMessagePreamble & preamble = *((KKerbSocket::DataMessagePreamble*)(_sspiSocket->_readMemRef._Address));
    preamble._padding = RtlUshortByteSwap(preamble._padding);
    preamble._token = RtlUshortByteSwap(preamble._token);

    ULONG totalSize = _sspiSocket->_readMemRef._Param;
    if ((sizeof(preamble) >= totalSize) ||
        (preamble._padding >= totalSize) ||
        (preamble._token >= totalSize) ||
        (((sizeof(preamble) + preamble._padding) + preamble._token) >= totalSize))
    {
        KDbgPrintf(
            "socket %llx: DecryptMessage failed: invalid preamble: preamble=%u, padding=%u, token=%u, total=%u",
            _traceId, sizeof(preamble), preamble._padding, preamble._token, totalSize);

        UnlockAndComplete(STATUS_INVALID_PARAMETER);
        return;
    }

    SecBuffer secBuffer[3];
    secBuffer[0].BufferType = SECBUFFER_DATA;
    secBuffer[0].cbBuffer = _sspiSocket->_readMemRef._Param - preamble._padding - preamble._token - sizeof(preamble);
    PUCHAR bufferCursor = (PUCHAR)_sspiSocket->_readMemRef._Address + sizeof(preamble);
    secBuffer[0].pvBuffer = bufferCursor;
    bufferCursor += secBuffer[0].cbBuffer;

    secBuffer[1].BufferType = SECBUFFER_PADDING;
    secBuffer[1].cbBuffer = preamble._padding;
    secBuffer[1].pvBuffer = bufferCursor;
    bufferCursor += secBuffer[1].cbBuffer;

    secBuffer[2].BufferType = SECBUFFER_TOKEN;
    secBuffer[2].cbBuffer = preamble._token;
    secBuffer[2].pvBuffer = bufferCursor;

    SecBufferDesc bufferDesc;
    bufferDesc.ulVersion = SECBUFFER_VERSION;
    bufferDesc.cBuffers = 3;
    bufferDesc.pBuffers = secBuffer;

#if KTL_USER_MODE
    NTSTATUS status = ::DecryptMessage(
        _sspiSocket->SspiContext().Handle(),
        &bufferDesc,
        0,
        nullptr);
#else
    NTSTATUS status = ::UnsealMessage(
        _sspiSocket->SspiContext().Handle(),
        &bufferDesc,
        0,
        nullptr);
#endif

    if (status != SEC_E_OK)
    {
        KDbgPrintf("socket %llx: DecryptMessage failed: %x", _traceId, status);
        UnlockAndComplete(status);
        return;
    }

    _sspiSocket->_readMemRef._Param = 0; //Delete processed ciphertext

    ULONG copied = CopyCleartext(secBuffer[0].pvBuffer, secBuffer[0].cbBuffer);
    if (copied < secBuffer[0].cbBuffer)
    {
        //KDbgPrintf("socket %llx: save extra cleartext: %u", _traceId, buffers[1].cbBuffer - copied);
        _sspiSocket->SaveExtraCleartext(
            (PUCHAR)secBuffer[0].pvBuffer + copied,
            secBuffer[0].cbBuffer - copied);
    }

    if (ShouldComplete())
    {
        UnlockAndComplete(status);
        return;
    }

    StartInnerReadOp();
}

#pragma endregion

#pragma region KKerbSocketWriteOp

KKerbSocketWriteOp::KKerbSocketWriteOp(KKerbSocket::SPtr && kerbSocket) : KSspiSocketWriteOp(kerbSocket.RawPtr())
{
}

NTSTATUS KKerbSocketWriteOp::EncryptMessage()
{
    SecPkgContext_Sizes const & sizes = static_cast<KKerbSocket*>(_sspiSocket.RawPtr())->_sizes;
    KKerbSocket::DataMessagePreamble& preamble = *((KKerbSocket::DataMessagePreamble*)(_sspiSocket->_writeMemRef._Address));

    _sspiSocket->_writeMemRef._Param = sizeof(preamble);
    ULONG cleartextIndexLimit = 
        _sspiSocket->_writeMemRef._Size -
        sizeof(preamble) -
        sizes.cbBlockSize - 
        sizes.cbSecurityTrailer;

    for (ULONG i = 0; i < _inputBuffers.Count(); ++ i)
    {
        if ((_sspiSocket->_writeMemRef._Param + _inputBuffers[i]._Param) <= cleartextIndexLimit)
        {
            KMemCpySafe(
                (PUCHAR)_sspiSocket->_writeMemRef._Address + _sspiSocket->_writeMemRef._Param,
                _sspiSocket->_writeMemRef._Size - _sspiSocket->_writeMemRef._Param,
                _inputBuffers[i]._Address,
                _inputBuffers[i]._Param);
            _sspiSocket->_writeMemRef._Param += _inputBuffers[i]._Param;
        }
        else
        {
            KDbgPrintf("socket %llx: input buffer total length exceeds limit %u", _traceId, _sspiSocket->_writeMemRef._Size);
            return STATUS_INVALID_PARAMETER_2;
        }
    }

    _cleartextToTransfer = _sspiSocket->_writeMemRef._Param - sizeof(preamble);

    SecBuffer secBuffer[3];
    secBuffer[0].BufferType = SECBUFFER_DATA;
    secBuffer[0].cbBuffer = _cleartextToTransfer;
    PUCHAR bufferCursor = (PUCHAR)_sspiSocket->_writeMemRef._Address + sizeof(preamble);
    secBuffer[0].pvBuffer = bufferCursor;
    bufferCursor += secBuffer[0].cbBuffer;
    PUCHAR paddingStart = bufferCursor;

    secBuffer[1].BufferType = SECBUFFER_PADDING;
    secBuffer[1].cbBuffer = sizes.cbBlockSize;
    secBuffer[1].pvBuffer = bufferCursor;
    bufferCursor += secBuffer[1].cbBuffer;

    secBuffer[2].BufferType = SECBUFFER_TOKEN;
    secBuffer[2].cbBuffer = sizes.cbSecurityTrailer;
    secBuffer[2].pvBuffer = bufferCursor;

    SecBufferDesc secBufferDesc;
    secBufferDesc.cBuffers = 3;
    secBufferDesc.pBuffers = secBuffer;
    secBufferDesc.ulVersion = SECBUFFER_VERSION;

#if KTL_USER_MODE
    NTSTATUS status = ::EncryptMessage(_sspiSocket->SspiContext().Handle(), 0, &secBufferDesc, 0);
#else
    NTSTATUS status = ::SealMessage(_sspiSocket->SspiContext().Handle(), 0, &secBufferDesc, 0);
#endif

    if (status != SEC_E_OK)
    {
        return status;
    }

    KAssert(secBuffer[1].BufferType == SECBUFFER_PADDING);
    KAssert(secBuffer[1].cbBuffer <= 0xffff);
    KAssert(secBuffer[2].BufferType == SECBUFFER_TOKEN);
    KAssert(secBuffer[2].cbBuffer <= 0xffff);

    preamble._padding = (unsigned short)secBuffer[1].cbBuffer;
    preamble._token = (unsigned short)secBuffer[2].cbBuffer;

    // Remove possible padding gap
    RtlMoveMemory(
        paddingStart + preamble._padding,
        paddingStart + sizes.cbBlockSize,
        preamble._token);

    _sspiSocket->_writeMemRef._Param = sizeof(preamble) + _cleartextToTransfer + preamble._padding + preamble._token;

    preamble._padding = RtlUshortByteSwap(preamble._padding);
    preamble._token = RtlUshortByteSwap(preamble._token);

    return status;
}

#pragma endregion

#pragma region KKerbSocket

KKerbSocket::KKerbSocket(
    KSspiTransport::SPtr && sspiTransport,
    ISocket::SPtr && innerSocket,
    KSspiContext::UPtr && sspiContext)
    : KSspiSocket(Ktl::Move(sspiTransport), Ktl::Move(innerSocket), Ktl::Move(sspiContext))
{
}

KKerbSocket::~KKerbSocket()
{
}

_Use_decl_annotations_
NTSTATUS KKerbSocket::CreateReadOp(ISocketReadOp::SPtr& newOp)
{
    newOp = _new (KTL_TAG_SSPI, GetThisAllocator()) KKerbSocketReadOp(this);
    if (!newOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return newOp->Status();
}

_Use_decl_annotations_
NTSTATUS KKerbSocket::CreateWriteOp(ISocketWriteOp::SPtr& newOp)
{
    newOp = _new (KTL_TAG_SSPI, GetThisAllocator()) KKerbSocketWriteOp(this);
    if (!newOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return newOp->Status();
}

NTSTATUS KKerbSocket::SetupBuffers()
{
    NTSTATUS status = _sspiContext->QueryContextAttributes(SECPKG_ATTR_SIZES, &_sizes);
    if (status == SEC_E_OK)
    {
        KDbgPrintf(
            "SSPI sizes: cbMaxToken = %u, cbMaxSignature = %u, cbBlockSize = %u, cbSecurityTrailer = %u",
            _sizes.cbMaxToken, _sizes.cbMaxSignature, _sizes.cbBlockSize, _sizes.cbSecurityTrailer);
    }
    else
    {
        KDbgPrintf("Failed to get SSPI sizes: %x", status);
        return status;
    }

    ULONG kerbOverhead =  _sizes.cbBlockSize + _sizes.cbSecurityTrailer + sizeof(KKerbSocket::DataMessagePreamble);
    if (_innerSocket->GetMaxMessageSize() <= kerbOverhead)
    {
        return STATUS_NOT_SUPPORTED;
    }

    _maxMessageSize = _innerSocket->GetMaxMessageSize() - kerbOverhead;
    _messageBufferSize = _innerSocket->GetMaxMessageSize(); // <= KerbFramingSocket::MaxFrameSize

    status = InitializeBuffer(_readMemRef);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = InitializeBuffer(_writeMemRef);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = InitializeBuffer(_savedCleartextMemRef);
    _savedCleartextBuffer = (PUCHAR)_savedCleartextMemRef._Address;
    return status;
}

#pragma endregion

#pragma region KKerbTransport

KKerbTransport::KKerbTransport(
    ITransport::SPtr const & innerTransport,
    KSspiCredential::SPtr const & credential,
    AuthorizationContext::SPtr const & clientAuthContext)
    : KSspiTransport(
        innerTransport,
        credential,
        credential,
        clientAuthContext,
        nullptr)
{
    RtlInitUnicodeString(&_sspiPackageName, MICROSOFT_KERBEROS_NAME);
    SetClientContextRequirement(ISC_REQ_MUTUAL_AUTH | ISC_REQ_INTEGRITY | ISC_REQ_CONFIDENTIALITY);
    SetServerContextRequirement(ASC_REQ_MUTUAL_AUTH | ASC_REQ_INTEGRITY | ASC_REQ_CONFIDENTIALITY);
}

_Use_decl_annotations_
NTSTATUS KKerbTransport::Initialize(
    ITransport::SPtr& transport,
    KAllocator& alloc,
    ITransport::SPtr const & innerTransport,
    KSspiCredential::SPtr const & credential,
    AuthorizationContext::SPtr const & clientAuthContext)
{
    transport = _new (KTL_TAG_SSPI, alloc) KKerbTransport(
        Ktl::Move(innerTransport),
        credential,
        clientAuthContext);

    if(!transport)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return transport->Status();
}

_Use_decl_annotations_
NTSTATUS KKerbTransport::CreateSspiClientContext(
    PWSTR targetName,
    KSspiContext::UPtr & sspiContext)
{
    sspiContext = _new (KTL_TAG_SSPI, GetThisAllocator()) KKerbContext(
        GetThisAllocator(),
        ClientCredential()->Handle(),
        targetName,
        false,
        ClientContextRequirement(),
        SspiMaxTokenSize());

    if (!sspiContext || (sspiContext->Status() != STATUS_SUCCESS))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return sspiContext->Status();
}

_Use_decl_annotations_
NTSTATUS KKerbTransport::CreateSspiServerContext(KSspiContext::UPtr & sspiContext)
{
    sspiContext = _new (KTL_TAG_SSPI, GetThisAllocator()) KKerbContext(
        GetThisAllocator(),
        ServerCredential()->Handle(),
        nullptr,
        true,
        ServerContextRequirement(),
        SspiMaxTokenSize());
    if (!sspiContext || (sspiContext->Status() != STATUS_SUCCESS))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return sspiContext->Status();
}

NTSTATUS KKerbTransport::CreateSspiSocket(
        ISocket::SPtr && innerSocket,
        KSspiContext::UPtr && sspiContext,
        __out ISocket::SPtr & sspiSocket)
{
    sspiSocket = _new (KTL_TAG_SSPI, GetThisAllocator()) KKerbSocket(
        this,
        Ktl::Move(innerSocket),
        Ktl::Move(sspiContext));
    if (!sspiSocket)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return sspiSocket->Status();
}

NTSTATUS KKerbTransport::CreateFramingSocket(
    ISocket::SPtr && innerSocket,
    __out ISocket::SPtr & framingSocket)
{
    framingSocket = _new (KTL_TAG_SSPI, GetThisAllocator()) KerbFramingSocket(Ktl::Move(innerSocket));
    if (!framingSocket)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return framingSocket->Status();
}

#pragma endregion

#pragma region KKerbContext

_Use_decl_annotations_
KKerbContext::KKerbContext(
    KAllocator& alloc,
    PCredHandle credHandle,
    PWSTR targetName,
    bool inbound,
    ULONG requirement,
    ULONG receiveBufferSize)
    : KSspiContext(alloc, credHandle, targetName, inbound, requirement, receiveBufferSize)
{
    RtlZeroMemory(&_inputSecBufferDesc, sizeof(_inputSecBufferDesc));
}

KKerbContext::~KKerbContext()
{
}

PSecBufferDesc KKerbContext::InputSecBufferDesc()
{
    return &_inputSecBufferDesc;
}

void KKerbContext::SetInputSecBufferDesc(PVOID inputAddress, ULONG inputLength)
{
    _inputSecBuffer[0].BufferType = SECBUFFER_TOKEN;
    _inputSecBuffer[0].cbBuffer = inputLength;
    _inputSecBuffer[0].pvBuffer = inputAddress;

    _inputSecBufferDesc.ulVersion = SECBUFFER_VERSION;
    _inputSecBufferDesc.cBuffers = 1;
    _inputSecBufferDesc.pBuffers = _inputSecBuffer;
}

ULONG KKerbContext::TargetDataRep()
{
    return SECURITY_NETWORK_DREP;
}

#pragma endregion
