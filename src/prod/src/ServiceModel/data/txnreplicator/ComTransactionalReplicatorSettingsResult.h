// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class ComTransactionalReplicatorSettingsResult
        : public IFabricTransactionalReplicatorSettingsResult,
        protected Common::ComUnknownBase
    {
        DENY_COPY(ComTransactionalReplicatorSettingsResult)

            COM_INTERFACE_LIST1(
                ComTransactionalReplicatorSettingsResult,
                IID_IFabricTransactionalReplicatorSettingsResult,
                IFabricTransactionalReplicatorSettingsResult)

    public:
        ComTransactionalReplicatorSettingsResult(TransactionalReplicatorSettingsUPtr const & value);
        virtual ~ComTransactionalReplicatorSettingsResult();

        const TRANSACTIONAL_REPLICATOR_SETTINGS * STDMETHODCALLTYPE get_TransactionalReplicatorSettings();

        static HRESULT STDMETHODCALLTYPE ReturnTransactionalReplicatorSettingsResult(
            __in TransactionalReplicatorSettingsUPtr && settings,
            __out IFabricTransactionalReplicatorSettingsResult ** value);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<TRANSACTIONAL_REPLICATOR_SETTINGS> settings_;
    };
}
