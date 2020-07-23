// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetSystemServiceListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetSystemServiceListResult :
        public IFabricGetSystemServiceListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetSystemServiceListResult)

        COM_INTERFACE_LIST2(
            ComGetSystemServiceListResult,
            IID_IFabricGetSystemServiceListResult,
            IFabricGetSystemServiceListResult,
            CLSID_ComGetSystemServiceListResult,
            ComGetSystemServiceListResult)

    public:
        explicit ComGetSystemServiceListResult(std::vector<ServiceModel::ServiceQueryResult> && systemServiceList);

        // 
        // IFabricGetSystemServiceListResult methods
        // 
        const FABRIC_SERVICE_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_SystemServiceList( void);

    private:
        Common::ReferencePointer<FABRIC_SERVICE_QUERY_RESULT_LIST> systemServiceList_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetSystemServiceListResult> ComGetSystemServiceListResultCPtr;
}
