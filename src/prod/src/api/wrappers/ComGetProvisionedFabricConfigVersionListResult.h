// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5398CFC8-61E1-4E3D-B1BA-15F7A150F565}
    static const GUID CLSID_ComGetFabricConfigVersionListResult = 
    { 0x5398cfc8, 0x61e1, 0x4e3d, { 0xb1, 0xba, 0x15, 0xf7, 0xa1, 0x50, 0xf5, 0x65 } };
    
    class ComGetProvisionedFabricConfigVersionListResult :
        public IFabricGetProvisionedConfigVersionListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetProvisionedFabricConfigVersionListResult)

        COM_INTERFACE_LIST2(
            ComGetProvisionedFabricConfigVersionListResult,
            IID_IFabricGetProvisionedConfigVersionListResult,
            IFabricGetProvisionedConfigVersionListResult,
            CLSID_ComGetFabricConfigVersionListResult,
            ComGetProvisionedFabricConfigVersionListResult)

    public:
        explicit ComGetProvisionedFabricConfigVersionListResult(std::vector<ServiceModel::ProvisionedFabricConfigVersionQueryResultItem> && ClusterConfigVersionList);

        // 
        // IFabricGetProvisionedConfigVersionListResult methods
        // 
        const FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_ProvisionedConfigVersionList( void);

    private:
        Common::ReferencePointer<FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_LIST> configVersionList_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetProvisionedFabricConfigVersionListResult> ComGetFabricConfigVersionListResultCPtr;
}
