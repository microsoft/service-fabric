// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {1A412DD1-6FC6-49A8-B50C-9ED842F52231}
    static const GUID CLSID_ComGetServiceGroupMemberTypeListResult =
    { 0x1a412dd1, 0x6fc6, 0x49a8, { 0xb5, 0xc, 0x9e, 0xd8, 0x42, 0xf5, 0x22, 0x31 } };
    
    class ComGetServiceGroupMemberTypeListResult :
        public IFabricGetServiceGroupMemberTypeListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetServiceGroupMemberTypeListResult)

        COM_INTERFACE_LIST2(
            ComGetServiceGroupMemberTypeListResult,
            IID_IFabricGetServiceGroupMemberTypeListResult,
            IFabricGetServiceGroupMemberTypeListResult,
            CLSID_ComGetServiceGroupMemberTypeListResult,
            ComGetServiceGroupMemberTypeListResult)

    public:
        explicit ComGetServiceGroupMemberTypeListResult(std::vector<ServiceModel::ServiceGroupMemberTypeQueryResult> && ServiceGroupMemberTypeList);

        // 
        // IFabricGetServiceGroupMemberTypeListResult methods
        // 
        const FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_ServiceGroupMemberTypeList( void);

    private:
        Common::ReferencePointer<FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_LIST> ServiceGroupMemberTypeList_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetServiceGroupMemberTypeListResult> ComGetServiceGroupMemberTypeListResultCPtr;
}
