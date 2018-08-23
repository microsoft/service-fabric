/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    KNetworkEndpoint.h

    Description:
      Kernel Tempate Library (KTL): KNetworkEndpoint

      Defines generic network address abstraction for use by other networking components.

    History:
      raymcc         15-May-2012        New design to use KRTT

--*/

#pragma once



//
//  KINetworkEndpoint
//
//  Generic representation of a network endpoint.  All network endpoints must implement this
//  interface.
//
class KNetworkEndpoint : public KRTT, public KObject<KNetworkEndpoint>, public KShared<KNetworkEndpoint>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KNetworkEndpoint);
    K_RUNTIME_TYPED(KNetworkEndpoint);

public:

    // GetUriRepresentation
    //
    // Implementations must be able to provide a read-only URI-based representation.
    //
    virtual KUri::SPtr
    GetUri() = 0;

    // Implementation must be able to compare two addresses for being 'the same'.
    //
    virtual BOOLEAN
    IsEqual(
        __in KNetworkEndpoint::SPtr Comparand
        ) = 0;
};



class KHttpNetworkEndpoint : public KNetworkEndpoint
{
    K_FORCE_SHARED_WITH_INHERITANCE(KHttpNetworkEndpoint);
    K_RUNTIME_TYPED(KHttpNetworkEndpoint);

public:

    static NTSTATUS
    Create(
        __in  KUriView&   HttpUri,
        __in  KAllocator& Allocator,
        __out KNetworkEndpoint::SPtr& NewEp
        );


    KUri::SPtr
    GetUri() override
    {
        return _Url;
    }

    BOOLEAN
    IsEqual(
        __in KNetworkEndpoint::SPtr Comparand
        )
    {
        if (!Comparand)
        {
            return FALSE;
        }
        if (!is_type<KHttpNetworkEndpoint>(Comparand))
        {
            return FALSE;
        }
        KHttpNetworkEndpoint::SPtr Other = down_cast<KHttpNetworkEndpoint, KNetworkEndpoint>(Comparand);
        return _Url->Compare(*Other->_Url);
    }

private:

    KUri::SPtr _Url;
};


class KRvdTcpNetworkEndpoint : public KNetworkEndpoint
{
    K_FORCE_SHARED_WITH_INHERITANCE(KRvdTcpNetworkEndpoint);
    K_RUNTIME_TYPED(KRvdTcpNetworkEndpoint);

public:
    //  Create
    //
    //  Parameters:
    //      ProposedUri         The full URI including the scheme & port number
    //
    static NTSTATUS
    Create(
        __in  KUriView&   ProposedUri,
        __in  KAllocator& Allocator,
        __out KNetworkEndpoint::SPtr& NewEp
        );

    KUri::SPtr
    GetUri() override
    {
        return _Uri;
    }

    USHORT
    GetPort()
    {
        return (USHORT) _Uri->GetPort();
    }

    BOOLEAN
    IsEqual(
        __in KNetworkEndpoint::SPtr Comparand
        )
    {
        if (!Comparand)
        {
            return FALSE;
        }
        if (!is_type<KRvdTcpNetworkEndpoint>(Comparand))
        {
            return FALSE;
        }
        KRvdTcpNetworkEndpoint::SPtr Other = down_cast<KRvdTcpNetworkEndpoint, KNetworkEndpoint>(Comparand);
        return _Uri->Compare(*Other->_Uri);
    }

private:

    KUri::SPtr _Uri;
};
