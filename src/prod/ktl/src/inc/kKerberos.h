/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kKerberos.h

    Description:
      Kernel Tempate Library (KTL): KKerberos

    History:
      leikong         Nov-2012.........Adding implementation
--*/

#pragma once

//
// KKerbTransport: a wrapper of SSPI(Negotiate/Kerberos)
//
class KKerbTransport : public KSspiTransport
{
public:
    typedef KSharedPtr<KKerbTransport> SPtr;

    static NTSTATUS Initialize(
        _Out_ ITransport::SPtr& transport,
        _In_ KAllocator& alloc,
        ITransport::SPtr const & innerTransport,
        KSspiCredential::SPtr const & credential,
        AuthorizationContext::SPtr const & clientAuthContext);

    NTSTATUS CreateSspiSocket(
        ISocket::SPtr && innerSocket,
        KSspiContext::UPtr && sspiContext,
        __out ISocket::SPtr & sspiSocket);

    // SSL has its own framing/record protocol, so it doesn't need extra framing.
    // This method will simply return the input innerSocket.
    NTSTATUS CreateFramingSocket(
        ISocket::SPtr && innerSocket,
        __out ISocket::SPtr & framingSocket);

    NTSTATUS CreateSspiClientContext(
        _In_opt_ PWSTR targetName,
        _Out_ KSspiContext::UPtr & sspiContext);

    NTSTATUS CreateSspiServerContext(_Out_ KSspiContext::UPtr & sspiContext);

private:
    KKerbTransport(
        ITransport::SPtr const & innerTransport,
        KSspiCredential::SPtr const & credential,
        AuthorizationContext::SPtr const & clientAuthContext);
};

//
// KKerbSocket
//
class KKerbSocket : public KSspiSocket
{
    friend class KKerbTransport;
    friend class KKerbSocketReadOp;
    friend class KKerbSocketWriteOp;

public:
    typedef KSharedPtr<KKerbSocket> SPtr;

    struct DataMessagePreamble
    {
        unsigned short _padding;
        unsigned short _token;
    };

    ~KKerbSocket();

    NTSTATUS CreateReadOp(_Out_ ISocketReadOp::SPtr& newOp);
    NTSTATUS CreateWriteOp(_Out_ ISocketWriteOp::SPtr& newOp);

private:
    KKerbSocket(
        KSspiTransport::SPtr && sspiTransport,
        ISocket::SPtr && innerSocket,
        KSspiContext::UPtr && sspiContext);

    NTSTATUS SetupBuffers();

private:
    SecPkgContext_Sizes _sizes;
};

//
// KKerbContext
//
class KKerbContext : public KSspiContext
{
    friend class KKerbTransport;

public:
    ~KKerbContext();

private:
    KKerbContext(
        _In_ KAllocator& alloc,
        _In_ PCredHandle credHandle,
        _In_ PWSTR targetName,
        bool accept,
        ULONG requirement,
        ULONG receiveBufferSize);

    ULONG TargetDataRep();

    PSecBufferDesc InputSecBufferDesc();

    void SetInputSecBufferDesc(PVOID inputAddress, ULONG inputLength);

private:
    SecBuffer _inputSecBuffer[1];
    SecBufferDesc _inputSecBufferDesc;
};

class KKerbSocketReadOp : public KSspiSocketReadOp
{
    friend class KKerbSocket;

    KKerbSocketReadOp(KKerbSocket::SPtr && kerbSocket);

    void DecryptMessage();
};

class KKerbSocketWriteOp : public KSspiSocketWriteOp
{
    friend class KKerbSocket;

    KKerbSocketWriteOp(KKerbSocket::SPtr && kerbSocket);

    NTSTATUS EncryptMessage();
};
