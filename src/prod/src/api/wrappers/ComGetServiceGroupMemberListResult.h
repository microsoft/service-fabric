// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetServiceGroupMemberListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetServiceGroupMemberListResult :
        public IFabricGetServiceGroupMemberListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetServiceGroupMemberListResult)

        COM_INTERFACE_LIST2(
            ComGetServiceGroupMemberListResult,
            IID_IFabricGetServiceGroupMemberListResult,
            IFabricGetServiceGroupMemberListResult,
            CLSID_ComGetServiceGroupMemberListResult,
            ComGetServiceGroupMemberListResult)

    public:
        explicit ComGetServiceGroupMemberListResult(std::vector<ServiceModel::ServiceGroupMemberQueryResult> && serviceGroupMemberList);

        // 
        // IFabricGetServiceGroupMemberListResult methods
        // 
        const FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_ServiceGroupMemberList(void);

    private:
        Common::ReferencePointer<FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_LIST> serviceGroupMemberList_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetServiceGroupMemberListResult> ComGetServiceGroupMemberListResultCPtr;
}
