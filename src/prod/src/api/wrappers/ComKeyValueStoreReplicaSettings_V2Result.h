// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {63BE1B43-1519-43F3-A1F5-5C2F3010009A}
    static const GUID CLSID_ComKeyValueStoreReplicaSettings_V2Result= 
        {0x63BE1B43,0x1519,0x43F3,{0xA1,0xF5,0x5C,0x2F,0x30,0x10,0x00,0x9A}};

    class ComKeyValueStoreReplicaSettings_V2Result :
        public IFabricKeyValueStoreReplicaSettings_V2Result,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComKeyValueStoreReplicaSettings_V2Result)

        BEGIN_COM_INTERFACE_LIST(ComKeyValueStoreReplicaSettings_V2Result)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricKeyValueStoreReplicaSettings_V2Result)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreReplicaSettings_V2Result, IFabricKeyValueStoreReplicaSettings_V2Result)
            COM_INTERFACE_ITEM(CLSID_ComKeyValueStoreReplicaSettings_V2Result, ComKeyValueStoreReplicaSettings_V2Result)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComKeyValueStoreReplicaSettings_V2Result(IKeyValueStoreReplicaSettings_V2ResultPtr const & impl);
        virtual ~ComKeyValueStoreReplicaSettings_V2Result();

        const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 * STDMETHODCALLTYPE get_Settings();

    private:
        Common::ScopedHeap heap_;
        IKeyValueStoreReplicaSettings_V2ResultPtr impl_;
    };
}
