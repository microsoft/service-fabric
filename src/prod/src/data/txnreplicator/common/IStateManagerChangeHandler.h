// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    interface IStateManagerChangeHandler
    {
        K_SHARED_INTERFACE(IStateManagerChangeHandler)

    public:
        virtual void OnRebuilt(
            __in ITransactionalReplicator & source,
            __in Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, KSharedPtr<IStateProvider2>>> & stateProviders) = 0;

        virtual void OnAdded(
            __in ITransactionalReplicator & source,
            __in ITransaction const & transaction,
            __in KUri const & stateProviderName,
            __in IStateProvider2 & stateProvider) = 0;

        virtual void OnRemoved(
            __in ITransactionalReplicator & source,
            __in ITransaction const & transaction,
            __in KUri const & stateProviderName,
            __in IStateProvider2 & stateProvider) = 0;
    };
}
