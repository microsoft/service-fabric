// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetServiceTypeListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetServiceTypeListResult :
        public IFabricGetServiceTypeListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetServiceTypeListResult)

        COM_INTERFACE_LIST2(
            ComGetServiceTypeListResult,
            IID_IFabricGetServiceTypeListResult,
            IFabricGetServiceTypeListResult,
            CLSID_ComGetServiceTypeListResult,
            ComGetServiceTypeListResult)

    public:
        explicit ComGetServiceTypeListResult(std::vector<ServiceModel::ServiceTypeQueryResult> && serviceTypeList);

        // 
        // IFabricGetServiceTypeListResult methods
        // 
        const FABRIC_SERVICE_TYPE_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_ServiceTypeList( void);

    private:
        Common::ReferencePointer<FABRIC_SERVICE_TYPE_QUERY_RESULT_LIST> serviceTypeList_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetServiceTypeListResult> ComGetServiceTypeListResultCPtr;
}
