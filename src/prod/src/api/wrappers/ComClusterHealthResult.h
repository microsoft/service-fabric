// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {1742BB44-0B0A-4CB0-BC8F-9E5D49FBF6DA}
    static const GUID CLSID_ComClusterHealthResult = 
        { 0x1742bb44, 0xb0a, 0x4cb0, { 0xbc, 0x8f, 0x9e, 0x5d, 0x49, 0xfb, 0xf6, 0xda } };
    

    class ComClusterHealthResult :
        public IFabricClusterHealthResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComClusterHealthResult)

        BEGIN_COM_INTERFACE_LIST(ComClusterHealthResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricClusterHealthResult)
            COM_INTERFACE_ITEM(IID_IFabricClusterHealthResult, IFabricClusterHealthResult)
            COM_INTERFACE_ITEM(CLSID_ComClusterHealthResult, ComClusterHealthResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComClusterHealthResult(ServiceModel::ClusterHealth const & clusterHealth);
        virtual ~ComClusterHealthResult();

        // 
        // IFabricClusterHealthResult methods
        // 
        const FABRIC_CLUSTER_HEALTH * STDMETHODCALLTYPE get_ClusterHealth();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_CLUSTER_HEALTH> clusterHealth_;
    };
}
