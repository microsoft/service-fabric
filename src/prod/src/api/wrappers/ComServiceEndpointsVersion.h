// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    //6aafd615-b378-4ae9-9865-2efcd88e8f90
    static const GUID CLSID_ComServiceEndpointsVersion =
    {0x6aafd615, 0xb378, 0x4ae9, {0x98, 0x65, 0x2e, 0xfc, 0xd8, 0x8e, 0x8f, 0x90}};

    class ComServiceEndpointsVersion
        : public IFabricServiceEndpointsVersion,
          private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComServiceEndpointsVersion);

        BEGIN_COM_INTERFACE_LIST(ComServiceEndpointsVersion)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricServiceEndpointsVersion)
            COM_INTERFACE_ITEM(IID_IFabricServiceEndpointsVersion,IFabricServiceEndpointsVersion)
            COM_INTERFACE_ITEM(CLSID_ComServiceEndpointsVersion, ComServiceEndpointsVersion)
        END_COM_INTERFACE_LIST()
        
    public:

        ComServiceEndpointsVersion();

        explicit ComServiceEndpointsVersion(IServiceEndpointsVersionPtr const & impl);

        IServiceEndpointsVersionPtr const & get_Impl() const { return impl_; }

        HRESULT STDMETHODCALLTYPE Compare(
            __in /* [in] */ IFabricServiceEndpointsVersion * other,
            __out /* [out, retval] */ LONG * result);

    private:

        IServiceEndpointsVersionPtr impl_;
    };
}
