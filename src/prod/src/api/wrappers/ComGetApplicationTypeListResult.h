// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetApplicationTypeListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetApplicationTypeListResult :
        public IFabricGetApplicationTypeListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetApplicationTypeListResult)

        COM_INTERFACE_LIST2(
            ComGetApplicationTypeListResult,
            IID_IFabricGetApplicationTypeListResult,
            IFabricGetApplicationTypeListResult,
            CLSID_ComGetApplicationTypeListResult,
            ComGetApplicationTypeListResult)

    public:
        explicit ComGetApplicationTypeListResult(std::vector<ServiceModel::ApplicationTypeQueryResult> && applicationTypeList);

        // 
        // IFabricGetApplicationTypeListResult methods
        // 
        const FABRIC_APPLICATION_TYPE_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_ApplicationTypeList( void);

    private:
        Common::ReferencePointer<FABRIC_APPLICATION_TYPE_QUERY_RESULT_LIST> applicationTypeList_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetApplicationTypeListResult> ComGetApplicationTypeListResultCPtr;
}
