// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ComReplicatorSettingsResult
            : public IFabricReplicatorSettingsResult,
            protected Common::ComUnknownBase
        {
            DENY_COPY(ComReplicatorSettingsResult)

            COM_INTERFACE_LIST1(
            ComReplicatorSettingsResult,
            IID_IFabricReplicatorSettingsResult,
            IFabricReplicatorSettingsResult)

        public:
            ComReplicatorSettingsResult(ReplicatorSettingsUPtr const & value);
            virtual ~ComReplicatorSettingsResult();
            const FABRIC_REPLICATOR_SETTINGS * STDMETHODCALLTYPE get_ReplicatorSettings();

            static HRESULT STDMETHODCALLTYPE ReturnReplicatorSettingsResult(
                ReplicatorSettingsUPtr && settings,
                IFabricReplicatorSettingsResult ** value);

        private:
            Common::ScopedHeap heap_;
            Common::ReferencePointer<FABRIC_REPLICATOR_SETTINGS> settings_;
        };
    }
}
