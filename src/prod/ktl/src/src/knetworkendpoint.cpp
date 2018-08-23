

/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    knetworkendpoint.cpp

    Description:
      Kernel Tempate Library (KTL): KNetworkEndpoint

    History:
      raymcc          15-May-2012         Initial version.

    Notes:


--*/

#include <ktl.h>

KNetworkEndpoint::KNetworkEndpoint()
{
}

KNetworkEndpoint::~KNetworkEndpoint()
{
}

KHttpNetworkEndpoint::KHttpNetworkEndpoint()
{
}

KHttpNetworkEndpoint::~KHttpNetworkEndpoint()
{
}



NTSTATUS
KHttpNetworkEndpoint::Create(
    __in  KUriView&   HttpUri,
    __in  KAllocator& Allocator,
    __out KNetworkEndpoint::SPtr& NewEp
    )
{
    if (HttpUri.IsEmpty() || !HttpUri.IsValid())
    {
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    // Verify that the scheme is HTTP/HTTPS and that the
    // authority/host is present.
    //
    KStringView Scheme = HttpUri.Get(KUri::eScheme);
    if (Scheme.CompareNoCase(KStringView(L"http")) != 0
        && Scheme.CompareNoCase(KStringView(L"https")) != 0)
    {
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    KStringView Auth = HttpUri.Get(KUri::eAuthority);
    if (Auth.Length() == 0)
    {
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    KUri::SPtr NewUrl;
    NTSTATUS Res = KUri::Create(HttpUri, Allocator, NewUrl);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KHttpNetworkEndpoint::SPtr Ep = _new(KTL_TAG_NETWORK_ENDPOINT, Allocator) KHttpNetworkEndpoint();
    if (!Ep)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Ep->_Url = NewUrl;
    NewEp = up_cast<KNetworkEndpoint, KHttpNetworkEndpoint>(Ep);

    return STATUS_SUCCESS;
}



KRvdTcpNetworkEndpoint::KRvdTcpNetworkEndpoint()
{
}

KRvdTcpNetworkEndpoint::~KRvdTcpNetworkEndpoint()
{
}

NTSTATUS
KRvdTcpNetworkEndpoint::Create(
    __in  KUriView&   ProposedUri,
    __in  KAllocator& Allocator,
    __out KNetworkEndpoint::SPtr& NewEp
    )
{
    NewEp = 0;

    // The scheme should eventually be rvd.tcp once we have RDMA in place
    //
    if (!ProposedUri.IsValid() ||
         ProposedUri.Get(KUriView::eScheme).Compare(KStringView(L"rvd")) != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    KUri::SPtr TmpUri;
    NTSTATUS Res = KUri::Create(ProposedUri, Allocator, TmpUri);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KRvdTcpNetworkEndpoint::SPtr TmpEp = _new(KTL_TAG_NETWORK_ENDPOINT, Allocator) KRvdTcpNetworkEndpoint();
    if (!TmpEp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TmpEp->_Uri = TmpUri;
    NewEp = (KNetworkEndpoint::SPtr&) TmpEp;

    return STATUS_SUCCESS;
}

