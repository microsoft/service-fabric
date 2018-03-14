// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {7A63221F-7758-44FF-BF00-F26E08897FEA}
    static const GUID CLSID_ComKeyValueStoreReplicaSettingsResult = 
        {0x7A63221F,0x7758,0x44FF,{0xBF,0x00,0xF2,0x6E,0x08,0x89,0x7F,0xEA}};

    class ComKeyValueStoreReplicaSettingsResult :
        public IFabricKeyValueStoreReplicaSettingsResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComKeyValueStoreReplicaSettingsResult)

        BEGIN_COM_INTERFACE_LIST(ComKeyValueStoreReplicaSettingsResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricKeyValueStoreReplicaSettingsResult)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreReplicaSettingsResult, IFabricKeyValueStoreReplicaSettingsResult)
            COM_INTERFACE_ITEM(CLSID_ComKeyValueStoreReplicaSettingsResult, ComKeyValueStoreReplicaSettingsResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComKeyValueStoreReplicaSettingsResult(IKeyValueStoreReplicaSettingsResultPtr const & impl);
        virtual ~ComKeyValueStoreReplicaSettingsResult();

        const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS * STDMETHODCALLTYPE get_Settings();

    private:
        Common::ScopedHeap heap_;
        IKeyValueStoreReplicaSettingsResultPtr impl_;
    };
}
