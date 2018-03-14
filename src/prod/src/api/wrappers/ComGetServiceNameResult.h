// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {04A724EB-3647-4B63-A865-E0B57752818E}
    static const GUID CLSID_ComGetServiceNameResult = 
    { 0x4a724eb, 0x3647, 0x4b63, { 0xa8, 0x65, 0xe0, 0xb5, 0x77, 0x52, 0x81, 0x8e } };

    class ComGetServiceNameResult :
        public IFabricGetServiceNameResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetServiceNameResult)

        COM_INTERFACE_LIST2(
            ComGetServiceNameResult,
            IID_IFabricGetServiceNameResult,
            IFabricGetServiceNameResult,
            CLSID_ComGetServiceNameResult,
            ComGetServiceNameResult)

    public:
        explicit ComGetServiceNameResult(ServiceModel::ServiceNameQueryResult && serviceName);
        virtual ~ComGetServiceNameResult();

        const FABRIC_SERVICE_NAME_QUERY_RESULT * STDMETHODCALLTYPE get_ServiceName(void);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_SERVICE_NAME_QUERY_RESULT> serviceName_;
    };
}
