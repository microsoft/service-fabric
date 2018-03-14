// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        interface IStateProvider2Factory
        {
            K_SHARED_INTERFACE(IStateProvider2Factory)

        public:
            /// <summary>
            /// Creates the state provider.
            /// </summary>
            virtual NTSTATUS Create(
                __in FactoryArguments const & factoryArguments,
                __out TxnReplicator::IStateProvider2::SPtr & stateProvider) noexcept = 0;
        };
    }
}
