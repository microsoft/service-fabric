// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {BECF12E2-8A58-4065-8DB9-D1BD7C26BB7F}
    static const GUID CLSID_ComGetDeployedServiceReplicaDetailResult = 
    { 0xbecf12e2, 0x8a58, 0x4065, { 0x8d, 0xb9, 0xd1, 0xbd, 0x7c, 0x26, 0xbb, 0x7f } };
    
    class ComGetDeployedServiceReplicaDetailResult :
        public IFabricGetDeployedServiceReplicaDetailResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetDeployedServiceReplicaDetailResult)

        COM_INTERFACE_LIST2(
            ComGetDeployedServiceReplicaDetailResult,
            IID_IFabricGetDeployedServiceReplicaDetailResult,
            IFabricGetDeployedServiceReplicaDetailResult,
            CLSID_ComGetDeployedServiceReplicaDetailResult,
            ComGetDeployedServiceReplicaDetailResult)

    public:
        explicit ComGetDeployedServiceReplicaDetailResult(ServiceModel::DeployedServiceReplicaDetailQueryResult && result);

        // 
        // IFabricGetDeployedServiceReplicaDetail methods
        // 
        const FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM *STDMETHODCALLTYPE get_ReplicaDetail(void);

    private:
        Common::ReferencePointer<FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM> result_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetDeployedServiceReplicaDetailResult> ComGetDeployedServiceReplicaDetailResultCPtr;
}
