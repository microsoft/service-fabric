// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // c6e7b3f7-fc67-4501-9a80-ddeada44e9b3 
    static const GUID CLSID_ComGetClusterHealthChunkResult = 
        {0xc6e7b3f7,0xfc67,0x4501,{0x9a,0x80,0xdd,0xea,0xda,0x44,0xe9,0xb3}};

    class ComGetClusterHealthChunkResult :
        public IFabricGetClusterHealthChunkResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComGetClusterHealthChunkResult)

        BEGIN_COM_INTERFACE_LIST(ComGetClusterHealthChunkResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricGetClusterHealthChunkResult)
            COM_INTERFACE_ITEM(IID_IFabricGetClusterHealthChunkResult, IFabricGetClusterHealthChunkResult)
            COM_INTERFACE_ITEM(CLSID_ComGetClusterHealthChunkResult, ComGetClusterHealthChunkResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComGetClusterHealthChunkResult(ServiceModel::ClusterHealthChunk const & clusterHealthChunk);
        virtual ~ComGetClusterHealthChunkResult();

        // 
        // IFabricGetClusterHealthChunkResult methods
        // 
        const FABRIC_CLUSTER_HEALTH_CHUNK * STDMETHODCALLTYPE get_ClusterHealthChunk();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_CLUSTER_HEALTH_CHUNK> clusterHealthChunk_;
    };
}
