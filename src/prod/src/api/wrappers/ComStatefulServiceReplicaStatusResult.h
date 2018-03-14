// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {d03af7c8-7d61-4fb4-9f38-ffa6035dd824}
    static const GUID CLSID_ComStatefulServiceReplicaStatusResult = 
    {0xd03af7c8,0x7d61,0x4fb4,{0x9f,0x38,0xff,0xa6,0x03,0x5d,0xd8,0x24}};

    class ComStatefulServiceReplicaStatusResult
        : public IFabricStatefulServiceReplicaStatusResult
        , private Common::ComUnknownBase
    {
        DENY_COPY(ComStatefulServiceReplicaStatusResult)

        BEGIN_COM_INTERFACE_LIST(ComStatefulServiceReplicaStatusResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServiceReplicaStatusResult)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceReplicaStatusResult, IFabricStatefulServiceReplicaStatusResult)
            COM_INTERFACE_ITEM(CLSID_ComStatefulServiceReplicaStatusResult, ComStatefulServiceReplicaStatusResult)
        END_COM_INTERFACE_LIST()

    public:
        ComStatefulServiceReplicaStatusResult(IStatefulServiceReplicaStatusResultPtr const & impl);
        virtual ~ComStatefulServiceReplicaStatusResult();

        IStatefulServiceReplicaStatusResultPtr const & get_Impl() const { return impl_; }

        //
        // IFabricStatefulServiceReplicaStatusResult
        //

        const FABRIC_REPLICA_STATUS_QUERY_RESULT * STDMETHODCALLTYPE get_Result();

    private:
        Common::ScopedHeap heap_;
        IStatefulServiceReplicaStatusResultPtr impl_;
    };
}
