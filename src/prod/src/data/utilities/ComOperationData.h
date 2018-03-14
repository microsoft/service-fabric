// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        //
        // Com based operation data to convert KTL based OperationData to com object IFabricOperationData
        // used during primary replication code path
        //
        class ComOperationData final
            : public KObject<ComOperationData>
            , public KShared<ComOperationData>
            , public IFabricOperationData
        {
            K_FORCE_SHARED(ComOperationData)

            K_BEGIN_COM_INTERFACE_LIST(ComOperationData)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationData)
                COM_INTERFACE_ITEM(IID_IFabricOperationData, IFabricOperationData)
            K_END_COM_INTERFACE_LIST()

        public:

            static NTSTATUS Create(
                __in OperationData const & innerOperationData,
                __in KAllocator & allocator,
                __out Common::ComPointer<IFabricOperationData> & object);

            STDMETHOD_(HRESULT, GetData)(
                __out ULONG *count,
                __out const FABRIC_OPERATION_DATA_BUFFER **buffers) override;

        private:

            ComOperationData(__in OperationData const & innerOperationData);

            OperationData::CSPtr innerOperationData_;
            KArray<FABRIC_OPERATION_DATA_BUFFER> buffersArray_;
        };
    }
}
