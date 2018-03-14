// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

    
using namespace std;
using namespace Common;
using namespace Api;
using namespace Reliability;

ComServiceEndpointsVersion::ComServiceEndpointsVersion()
    : impl_()
{
}

ComServiceEndpointsVersion::ComServiceEndpointsVersion(IServiceEndpointsVersionPtr const & impl)
    : impl_(impl)
{
}

HRESULT STDMETHODCALLTYPE ComServiceEndpointsVersion::Compare(
    __in /* [in] */ IFabricServiceEndpointsVersion * other,
    __out /* [out, retval] */ LONG * result)
{
    if (other == NULL || result == NULL) { return E_POINTER; }

    ComPointer<ComServiceEndpointsVersion> comPtr;
    auto hr = other->QueryInterface(CLSID_ComServiceEndpointsVersion, comPtr.VoidInitializationAddress());
    if (FAILED(hr)) { return hr; }

    *result = impl_->Compare(*(comPtr->get_Impl().get()));

    return S_OK;
}
