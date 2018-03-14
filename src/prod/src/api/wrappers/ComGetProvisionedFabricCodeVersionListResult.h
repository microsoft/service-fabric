// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {DE8906DE-4A03-48FA-9A03-F92D19CFF9A7}
    static const GUID CLSID_ComGetFabricCodeVersionListResult = 
    { 0xde8906de, 0x4a03, 0x48fa, { 0x9a, 0x3, 0xf9, 0x2d, 0x19, 0xcf, 0xf9, 0xa7 } };
    
    class ComGetProvisionedFabricCodeVersionListResult :
        public IFabricGetProvisionedCodeVersionListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetProvisionedFabricCodeVersionListResult)

        COM_INTERFACE_LIST2(
            ComGetProvisionedFabricCodeVersionListResult,
            IID_IFabricGetProvisionedCodeVersionListResult,
            IFabricGetProvisionedCodeVersionListResult,
            CLSID_ComGetFabricCodeVersionListResult,
            ComGetProvisionedFabricCodeVersionListResult)

    public:
        explicit ComGetProvisionedFabricCodeVersionListResult(std::vector<ServiceModel::ProvisionedFabricCodeVersionQueryResultItem> && ClusterCodeVersionList);

        // 
        // IFabricGetProvisionedCodeVersionListResult methods
        // 
        const FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_ProvisionedCodeVersionList( void);

    private:
        Common::ReferencePointer<FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_LIST> codeVersionList_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetProvisionedFabricCodeVersionListResult> ComGetFabricCodeVersionListResultCPtr;
}
