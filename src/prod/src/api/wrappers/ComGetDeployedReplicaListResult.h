// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetDeployedReplicaListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetDeployedReplicaListResult :
        public IFabricGetDeployedReplicaListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetDeployedReplicaListResult)

        COM_INTERFACE_LIST2(
            ComGetDeployedReplicaListResult,
            IID_IFabricGetDeployedReplicaListResult,
            IFabricGetDeployedReplicaListResult,
            CLSID_ComGetDeployedReplicaListResult,
            ComGetDeployedReplicaListResult)

    public:
        explicit ComGetDeployedReplicaListResult(std::vector<ServiceModel::DeployedServiceReplicaQueryResult> && deployedReplicaList);

        // 
        // IFabricGetDeployedReplicaListResult methods
        // 
        const FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_DeployedReplicaList( void);

    private:
        Common::ReferencePointer<FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_LIST> deployedReplicaList_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetDeployedReplicaListResult> ComGetDeployedReplicaListResultCPtr;
}
