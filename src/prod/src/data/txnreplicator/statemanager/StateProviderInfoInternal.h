// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        class StateProviderInfoInternal : public KObject<StateProviderInfoInternal>
        {
        public:
            __declspec(property(get = get_TypeName)) KString::CSPtr TypeName;
            KString::CSPtr StateProviderInfoInternal::get_TypeName() const { return typeName_; }

            __declspec(property(get = get_Name)) KUri::CSPtr Name;
            KUri::CSPtr StateProviderInfoInternal::get_Name() const { return name_; }

            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID StateProviderInfoInternal::get_StateProviderId() const { return stateProviderId_; }

            __declspec(property(get = get_StateProviderSPtr)) TxnReplicator::IStateProvider2::SPtr StateProviderSPtr;
            TxnReplicator::IStateProvider2::SPtr StateProviderInfoInternal::get_StateProviderSPtr() const { return stateProviderSPtr_; }

        public:
            NOFAIL StateProviderInfoInternal() noexcept;

            NOFAIL StateProviderInfoInternal(
                __in KString const & typeName,
                __in KUri const & name,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in TxnReplicator::IStateProvider2 & stateProvider) noexcept;

            // Copy constructor
            NOFAIL StateProviderInfoInternal(StateProviderInfoInternal const & other) noexcept;

            // Copy assignment operator
            StateProviderInfoInternal& operator=(__in StateProviderInfoInternal const & other) noexcept;

            // Destructor
            ~StateProviderInfoInternal();

        private:
            KString::CSPtr typeName_;
            KUri::CSPtr name_;
            FABRIC_STATE_PROVIDER_ID stateProviderId_;
            TxnReplicator::IStateProvider2::SPtr stateProviderSPtr_;
        };
    }
}
