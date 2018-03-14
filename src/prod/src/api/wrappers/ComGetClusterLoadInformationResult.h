// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetClusterLoadInformationResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};
    
    class ComGetClusterLoadInformationResult :
        public IFabricGetClusterLoadInformationResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetClusterLoadInformationResult)

        COM_INTERFACE_LIST2(
            ComGetClusterLoadInformationResult,
            IID_IFabricGetClusterLoadInformationResult,
            IFabricGetClusterLoadInformationResult,
            CLSID_ComGetClusterLoadInformationResult,
            ComGetClusterLoadInformationResult)

    public:
        explicit ComGetClusterLoadInformationResult(ServiceModel::ClusterLoadInformationQueryResult && clusterLoadInformation);

        // 
        // IFabricGetNodeListResult methods
        // 
        const FABRIC_CLUSTER_LOAD_INFORMATION *STDMETHODCALLTYPE get_ClusterLoadInformation( void);

    private:
        Common::ReferencePointer<FABRIC_CLUSTER_LOAD_INFORMATION> clusterLoadInformation_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetClusterLoadInformationResult> ComGetClusterLoadInformationResultCPtr;
}
