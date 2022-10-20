/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    Description:
      Kernel Tempate Library (KTL): SSL socket

      Implements a SSL wrapper of socket classes

    History:
      leikong            Oct-2011.........Adding implementation
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

#pragma region KSslSocket

KSslSocket::KSslSocket(
    KSspiTransport::SPtr && sspiTransport,
    ISocket::SPtr && innerSocket,
    KSspiContext::UPtr && sspiContext)
    : KSspiSocket(Ktl::Move(sspiTransport), Ktl::Move(innerSocket), Ktl::Move(sspiContext))
{
}

KSslSocket::~KSslSocket()
{
}

_Use_decl_annotations_
NTSTATUS KSslSocket::CreateReadOp(ISocketReadOp::SPtr& newOp)
{
    newOp = _new (KTL_TAG_SSPI, GetThisAllocator()) KSslSocketReadOp(this);
    if (!newOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return newOp->Status();
}

_Use_decl_annotations_
NTSTATUS KSslSocket::CreateWriteOp(ISocketWriteOp::SPtr& newOp)
{
    newOp = _new (KTL_TAG_SSPI, GetThisAllocator()) KSslSocketWriteOp(this);
    if (!newOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return newOp->Status();
}

NTSTATUS KSslSocket::OnNegotiationSucceeded()
{
    CopyExtraDataReceivedDuringNegotiation();

    return STATUS_SUCCESS;
}

NTSTATUS KSslSocket::SetupBuffers()
{
    NTSTATUS status = _sspiContext->QueryContextAttributes(SECPKG_ATTR_STREAM_SIZES, &_streamSizes);
    if (status == SEC_E_OK)
    {
        KDbgPrintf(
            "SSPI stream sizes: cbMaximumMessage = %u, cbHeader = %u, cbTrailer = %u, cbBlockSize = %u, cBuffers = %u",
            _streamSizes.cbMaximumMessage,
            _streamSizes.cbHeader,
            _streamSizes.cbTrailer,
            _streamSizes.cbBlockSize,
            _streamSizes.cBuffers);
    }
    else
    {
        KDbgPrintf("Failed to get SSPI stream sizes: %x", status);
        return status;
    }

    ULONG sslOverhead = _streamSizes.cbHeader + _streamSizes.cbTrailer;
    _messageBufferSize =  _streamSizes.cbMaximumMessage + sslOverhead;

    if (_innerSocket->GetMaxMessageSize() <= sslOverhead)
    {
        return STATUS_NOT_SUPPORTED;
    }

    _maxMessageSize = _streamSizes.cbMaximumMessage;
    ULONG innerSocketLimit = _innerSocket->GetMaxMessageSize() - sslOverhead;
    if (_maxMessageSize > innerSocketLimit)
    {
        _maxMessageSize = innerSocketLimit;
    }

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

void KSslSocket::CopyExtraDataReceivedDuringNegotiation()
{
    if (_sspiContext->ExtraData()._Param > 0)
    {
        KMemCpySafe(
            _readMemRef._Address,
            _readMemRef._Size,
            _sspiContext->ExtraData()._Address,
            _sspiContext->ExtraData()._Param);
        _readMemRef._Param = _sspiContext->ExtraData()._Param;

        KDbgPrintf(
            "socket %llx: copied data received during negotiation: %u",
            _traceId, _sspiContext->ExtraData()._Param);
    }
}

#pragma endregion

#pragma region KSslTransport

KSslTransport::KSslTransport(
    ITransport::SPtr const & innerTransport,
    KSspiCredential::SPtr const & clientCredential,
    KSspiCredential::SPtr const & serverCredential,
    AuthorizationContext::SPtr const & authorizationContext)
    : KSspiTransport(
        innerTransport,
        clientCredential,
        serverCredential,
        authorizationContext,
        authorizationContext)
{
    SetClientContextRequirement(ISC_REQ_MANUAL_CRED_VALIDATION);
    SetServerContextRequirement(ASC_REQ_MUTUAL_AUTH);
    RtlInitUnicodeString(&_sspiPackageName, SCHANNEL_NAME);
}

_Use_decl_annotations_
NTSTATUS KSslTransport::Initialize(
    ITransport::SPtr& transport,
    KAllocator& alloc,
    ITransport::SPtr const & innerTransport,
    KSspiCredential::SPtr const & clientCredential,
    KSspiCredential::SPtr const & serverCredential,
    AuthorizationContext::SPtr const & authorizationContext)
{
    transport = _new (KTL_TAG_SSPI, alloc) KSslTransport(
        Ktl::Move(innerTransport),
        clientCredential,
        serverCredential,
        authorizationContext);

    if(!transport)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return transport->Status();
}

_Use_decl_annotations_
NTSTATUS KSslTransport::CreateSspiClientContext(
    PWSTR targetName,
    KSspiContext::UPtr & sspiContext)
{
    targetName; // targetName is ignored because we do manual certificate verification

    sspiContext = _new (KTL_TAG_SSPI, GetThisAllocator()) KSslContext(
        GetThisAllocator(),
        ClientCredential()->Handle(),
        nullptr,
        false,
        ClientContextRequirement(),
        SspiMaxTokenSize());
    if (!sspiContext)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return sspiContext->Status();
}

_Use_decl_annotations_
NTSTATUS KSslTransport::CreateSspiServerContext(KSspiContext::UPtr & sspiContext)
{
    sspiContext = _new (KTL_TAG_SSPI, GetThisAllocator()) KSslContext(
        GetThisAllocator(),
        ServerCredential()->Handle(),
        nullptr,
        true,
        ServerContextRequirement(),
        SspiMaxTokenSize());
    if (!sspiContext)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return sspiContext->Status();
}

NTSTATUS KSslTransport::CreateSspiSocket(
        ISocket::SPtr && innerSocket,
        KSspiContext::UPtr && sspiContext,
        __out ISocket::SPtr & sspiSocket)
{
    sspiSocket = _new (KTL_TAG_SSPI, GetThisAllocator()) KSslSocket(
        this,
        Ktl::Move(innerSocket),
        Ktl::Move(sspiContext));
    if (!sspiSocket)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return sspiSocket->Status();
}

NTSTATUS KSslTransport::CreateFramingSocket(
    ISocket::SPtr && innerSocket,
    __out ISocket::SPtr & framingSocket)
{
    framingSocket = Ktl::Move(innerSocket);
    return STATUS_SUCCESS;
}

#pragma endregion

#pragma region KSslContext

_Use_decl_annotations_
KSslContext::KSslContext(
    KAllocator& alloc,
    PCredHandle credHandle,
    PWSTR targetName,
    bool inbound,
    ULONG requirement,
    ULONG receiveBufferSize)
    : KSspiContext(alloc, credHandle, targetName, inbound, requirement, receiveBufferSize)
    , _extraData(0, nullptr)
{
    RtlZeroMemory(&_inputSecBufferDesc, sizeof(_inputSecBufferDesc));
}

KSslContext::~KSslContext()
{
}

PSecBufferDesc KSslContext::InputSecBufferDesc()
{
    return &_inputSecBufferDesc;
}

void KSslContext::SetInputSecBufferDesc(PVOID inputAddress, ULONG inputLength)
{
    _inputSecBuffer[0].BufferType = SECBUFFER_TOKEN;
    _inputSecBuffer[0].cbBuffer = inputLength;
    _inputSecBuffer[0].pvBuffer = inputAddress;

    _inputSecBuffer[1].BufferType = SECBUFFER_EMPTY;
    _inputSecBuffer[1].cbBuffer = 0;
    _inputSecBuffer[1].pvBuffer = nullptr;

    _inputSecBufferDesc.ulVersion = SECBUFFER_VERSION;
    _inputSecBufferDesc.cBuffers = 2;
    _inputSecBufferDesc.pBuffers = _inputSecBuffer;
}

ULONG KSslContext::RemainingInputLength()
{
    return _inputSecBuffer[1].cbBuffer;
}

ULONG KSslContext::TargetDataRep()
{
    return 0;
}

void KSslContext::SaveExtraInputIfAny()
{
    KAssert(_inputSecBufferDesc.cBuffers == 2);
    const int extraBufferIndex = 1; // There are only two input buffers and the first one is SECBUFFER_TOKEN

    if ((_inputSecBuffer[extraBufferIndex].BufferType == SECBUFFER_EXTRA)
        && (_inputSecBuffer[extraBufferIndex].cbBuffer != 0))
    {
        _extraData._Size = _extraData._Param = _inputSecBuffer[extraBufferIndex].cbBuffer;
        _extraData._Address = static_cast<PUCHAR>(_inputSecBuffer[0].pvBuffer) + _inputSecBuffer[0].cbBuffer - _extraData._Size;
    }
}

KMemRef KSslContext::ExtraData() const
{
    return _extraData;
}

#pragma endregion

#pragma region KSslSocketReadOp

KSslSocketReadOp::KSslSocketReadOp(KSslSocket::SPtr && sslSocket) : KSspiSocketReadOp(sslSocket.RawPtr())
{
}

void KSslSocketReadOp::DecryptMessage()
{
    SecBuffer buffers[4] = {};

    buffers[0].BufferType = SECBUFFER_DATA;
    buffers[0].pvBuffer = _sspiSocket->_readMemRef._Address;
    buffers[0].cbBuffer = _sspiSocket->_readMemRef._Param;

    buffers[1].BufferType = SECBUFFER_EMPTY;
    buffers[2].BufferType = SECBUFFER_EMPTY;
    buffers[3].BufferType = SECBUFFER_EMPTY;

    SecBufferDesc bufferDesc;
    bufferDesc.ulVersion = SECBUFFER_VERSION;
    bufferDesc.cBuffers = 4;
    bufferDesc.pBuffers = buffers;

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

    if (status == SEC_E_INCOMPLETE_MESSAGE)
    {
        StartInnerReadOp();
        return;
    }

    if (status == SEC_E_OK)
    {
        ULONG copied = CopyCleartext(buffers[1].pvBuffer, buffers[1].cbBuffer);
        if (copied < buffers[1].cbBuffer)
        {
            //KDbgPrintf("socket %llx: save extra cleartext: %u", _traceId, buffers[1].cbBuffer - copied);
            _sspiSocket->SaveExtraCleartext(
                (PUCHAR)buffers[1].pvBuffer + copied,
                buffers[1].cbBuffer - copied);
        }

        MoveExtraCiphertextToStart(buffers);

        if (ShouldComplete())
        {
            UnlockAndComplete(status);
            return;
        }

        //KDbgPrintf("socket %llx: need to read more", _traceId);
        StartInnerReadOp();
        return;
    }

    KDbgPrintf("socket %llx: DecryptMessage failed: %x", _traceId, status);
    UnlockAndComplete(status);
}

void KSslSocketReadOp::MoveExtraCiphertextToStart(SecBuffer const * buffers)
{
    int index = 3;
    while((buffers[index].BufferType != SECBUFFER_EXTRA) && (index-- != 0));

    if (index != -1)
    {
        RtlMoveMemory(
            _sspiSocket->_readMemRef._Address,
            (PUCHAR)_sspiSocket->_readMemRef._Address + _sspiSocket->_readMemRef._Param - buffers[index].cbBuffer,
            buffers[index].cbBuffer);
        _sspiSocket->_readMemRef._Param =  buffers[index].cbBuffer;
    }
    else
    {
        _sspiSocket->_readMemRef._Param = 0; // no extra cipher text left
    }
}

#pragma endregion

#pragma region KSslSocketWriteOp

KSslSocketWriteOp::KSslSocketWriteOp(KSslSocket::SPtr && sslSocket) : KSspiSocketWriteOp(sslSocket.RawPtr())
{
}

NTSTATUS KSslSocketWriteOp::EncryptMessage()
{
    SecPkgContext_StreamSizes const & streamSizes = static_cast<KSslSocket*>(_sspiSocket.RawPtr())->_streamSizes;

    _sspiSocket->_writeMemRef._Param = streamSizes.cbHeader;
    ULONG cleartextIndexLimit = _sspiSocket->_writeMemRef._Size - streamSizes.cbTrailer;
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

    _cleartextToTransfer = _sspiSocket->_writeMemRef._Param - streamSizes.cbHeader;

    // count trailer size into the total length of occupied encryption buffer length
    _sspiSocket->_writeMemRef._Param += streamSizes.cbTrailer;

    SecBuffer buffers[3];
    buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
    PUCHAR bufferCursor = (PUCHAR)_sspiSocket->_writeMemRef._Address;
    buffers[0].pvBuffer = bufferCursor;
    buffers[0].cbBuffer = streamSizes.cbHeader;
    bufferCursor += buffers[0].cbBuffer;

    buffers[1].BufferType = SECBUFFER_DATA;
    buffers[1].pvBuffer = bufferCursor;
    buffers[1].cbBuffer = _cleartextToTransfer;
    bufferCursor += buffers[1].cbBuffer;

    buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
    buffers[2].pvBuffer = bufferCursor;
    buffers[2].cbBuffer = streamSizes.cbTrailer;

    SecBufferDesc bufferDesc;
    bufferDesc.ulVersion = SECBUFFER_VERSION;
    bufferDesc.cBuffers = 3;
    bufferDesc.pBuffers = buffers;

#if KTL_USER_MODE
    NTSTATUS status = ::EncryptMessage(_sspiSocket->SspiContext().Handle(), 0, &bufferDesc, 0);
#else
    NTSTATUS status = ::SealMessage(_sspiSocket->SspiContext().Handle(), 0, &bufferDesc, 0);
#endif

    if (status == SEC_E_OK)
    {
        _sspiSocket->_writeMemRef._Param = buffers[0].cbBuffer + buffers[1].cbBuffer + buffers[2].cbBuffer;
    }

    return status;
}

#pragma endregion
