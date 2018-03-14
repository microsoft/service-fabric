// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {A55F1F34-BA23-46AC-8CA9-C226B0A6EB94}
    static const GUID CLSID_ComGatewayInformationResult = 
        {0xA55F1F34,0xBA23,0x46AC,{0x8C,0xA9,0xC2,0x26,0xB0,0xA6,0xEB,0x94}};

    class ComGatewayInformationResult :
        public IFabricGatewayInformationResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComGatewayInformationResult)

        BEGIN_COM_INTERFACE_LIST(ComGatewayInformationResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricGatewayInformationResult)
            COM_INTERFACE_ITEM(IID_IFabricGatewayInformationResult, IFabricGatewayInformationResult)
            COM_INTERFACE_ITEM(CLSID_ComGatewayInformationResult, ComGatewayInformationResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComGatewayInformationResult(Naming::GatewayDescription const & impl);
        virtual ~ComGatewayInformationResult();

        const FABRIC_GATEWAY_INFORMATION * STDMETHODCALLTYPE get_GatewayInformation();

    private:
        Common::ScopedHeap heap_;
        Naming::GatewayDescription impl_;
    };
}
