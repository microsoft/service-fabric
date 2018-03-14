// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // 3db17d12-b32c-46ac-b26c-afd76b9e9d48 
    static const GUID CLSID_ComPartitionHealthResult = 
        {0x3db17d12, 0xb32c, 0x46ac, {0xb2, 0x6c, 0xaf, 0xd7, 0x6b, 0x9e, 0x9d, 0x48}};

    class ComPartitionHealthResult :
        public IFabricPartitionHealthResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComPartitionHealthResult)

        BEGIN_COM_INTERFACE_LIST(ComPartitionHealthResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricPartitionHealthResult)
            COM_INTERFACE_ITEM(IID_IFabricPartitionHealthResult, IFabricPartitionHealthResult)
            COM_INTERFACE_ITEM(CLSID_ComPartitionHealthResult, ComPartitionHealthResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComPartitionHealthResult(ServiceModel::PartitionHealth const & partitionHealth);
        virtual ~ComPartitionHealthResult();

        // 
        // IFabricPartitionHealthResult methods
        // 
        const FABRIC_PARTITION_HEALTH * STDMETHODCALLTYPE get_PartitionHealth();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_PARTITION_HEALTH> partitionHealth_;
    };
}
