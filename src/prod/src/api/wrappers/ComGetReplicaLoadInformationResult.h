// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {3de74185-d1a8-4973-b14c-7bee458f7f76}
    static const GUID CLSID_ComGetReplicaLoadInformationResult = 
    {0x3de74185,0xd1a8,0x4973,{0xb1, 0x4c, 0x7b, 0xee, 0x45, 0x8f, 0x7f, 0x76}};
    
    class ComGetReplicaLoadInformationResult :
        public IFabricGetReplicaLoadInformationResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetReplicaLoadInformationResult)

        COM_INTERFACE_LIST2(
            ComGetReplicaLoadInformationResult,
            IID_IFabricGetReplicaLoadInformationResult,
            IFabricGetReplicaLoadInformationResult,
            CLSID_ComGetReplicaLoadInformationResult,
            ComGetReplicaLoadInformationResult)

    public:
        explicit ComGetReplicaLoadInformationResult(ServiceModel::ReplicaLoadInformationQueryResult const & replicaLoadInformation);

        // 
        // IFabricGetReplicaLoadInformationResult methods
        // 
        const FABRIC_REPLICA_LOAD_INFORMATION *STDMETHODCALLTYPE get_ReplicaLoadInformation( void);

    private:
        Common::ReferencePointer<FABRIC_REPLICA_LOAD_INFORMATION> replicaLoadInformation_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetReplicaLoadInformationResult> ComGetReplicaLoadInformationResultCPtr;
}
