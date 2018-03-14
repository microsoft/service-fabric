// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {7aa617ae-0070-4d69-83aa-2b5522371bce}
    static const GUID CLSID_ComGetPartitionLoadInformationResult =
    {0x7aa617ae,0x0070,0x4d69,{0x83, 0xaa, 0x2b, 0x55, 0x22, 0x37, 0x1b, 0xce}};
    
    class ComGetPartitionLoadInformationResult :
        public IFabricGetPartitionLoadInformationResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetPartitionLoadInformationResult)

        COM_INTERFACE_LIST2(
            ComGetPartitionLoadInformationResult,
            IID_IFabricGetPartitionLoadInformationResult,
            IFabricGetPartitionLoadInformationResult,
            CLSID_ComGetPartitionLoadInformationResult,
            ComGetPartitionLoadInformationResult)

    public:
        explicit ComGetPartitionLoadInformationResult(ServiceModel::PartitionLoadInformationQueryResult const & partitionLoadInformation);

        // 
        // IFabricGetPartitionLoadInformationResult methods
        // 
        const FABRIC_PARTITION_LOAD_INFORMATION *STDMETHODCALLTYPE get_PartitionLoadInformation( void);

    private:
        Common::ReferencePointer<FABRIC_PARTITION_LOAD_INFORMATION> partitionLoadInformation_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetPartitionLoadInformationResult> ComGetPartitionLoadInformationResultCPtr;
}
