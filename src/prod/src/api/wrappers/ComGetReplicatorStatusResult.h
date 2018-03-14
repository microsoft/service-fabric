// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {30E10C61-A710-4F99-A623-BB1403265186}
    static const GUID CLSID_ComGetReplicatorStatusResult =
    {0x30e10c61,0xa710,0x4f99,{0xa6,0x23,0xbb,0x14,0x03,0x26,0x51,0x86}};
    
    class ComGetReplicatorStatusResult :
        public IFabricGetReplicatorStatusResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetReplicatorStatusResult)

        COM_INTERFACE_LIST2(
            ComGetReplicatorStatusResult,
            IID_IFabricGetReplicatorStatusResult,
            IFabricGetReplicatorStatusResult,
            CLSID_ComGetReplicatorStatusResult,
            ComGetReplicatorStatusResult)

    public:
        explicit ComGetReplicatorStatusResult(ServiceModel::ReplicatorStatusQueryResultSPtr && replicatorStatus);

        // 
        // IFabricGetReplicatorStatusResult methods
        // 
        const FABRIC_REPLICATOR_STATUS_QUERY_RESULT *STDMETHODCALLTYPE get_ReplicatorStatus( void);

    private:
        Common::ReferencePointer<FABRIC_REPLICATOR_STATUS_QUERY_RESULT> result_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetReplicatorStatusResult> ComGetReplicatorStatusResultCPtr;
}
