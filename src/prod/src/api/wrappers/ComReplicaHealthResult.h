// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // 94b04971-9a80-4c91-a40d-a9c171ab9810 
    static const GUID CLSID_ComReplicaHealthResult = 
        {0x94b04971, 0x9a80, 0x4c91, {0xa4, 0x0d, 0xa9, 0xc1, 0x71, 0xab, 0x98, 0x10}};

    class ComReplicaHealthResult :
        public IFabricReplicaHealthResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComReplicaHealthResult)

        BEGIN_COM_INTERFACE_LIST(ComReplicaHealthResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricReplicaHealthResult)
            COM_INTERFACE_ITEM(IID_IFabricReplicaHealthResult, IFabricReplicaHealthResult)
            COM_INTERFACE_ITEM(CLSID_ComReplicaHealthResult, ComReplicaHealthResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComReplicaHealthResult(ServiceModel::ReplicaHealth const & replicaHealth);
        virtual ~ComReplicaHealthResult();

        // 
        // IFabricReplicaHealthResult methods
        // 
        const FABRIC_REPLICA_HEALTH * STDMETHODCALLTYPE get_ReplicaHealth();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_REPLICA_HEALTH> replicaHealth_;
    };
}
