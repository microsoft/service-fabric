// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        class FactoryArguments
        {
        public:
            __declspec(property(get = get_Name)) KUri::CSPtr Name;
            KUri::CSPtr get_Name() const;

            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID get_StateProviderId() const;

            __declspec(property(get = get_TypeName)) KString::CSPtr TypeName;
            KString::CSPtr get_TypeName() const;

            __declspec(property(get = get_PartitionId)) KGuid PartitionId;
            KGuid get_PartitionId() const;

            __declspec(property(get = get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const;

            __declspec(property(get = get_InitializationParameters)) Data::Utilities::OperationData::CSPtr InitializationParameters;
            Data::Utilities::OperationData::CSPtr get_InitializationParameters() const;

        public:
            NOFAIL FactoryArguments(
                __in KUri const & name,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in KString const & typeName,
                __in KGuid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in_opt Data::Utilities::OperationData const * const initializationParameters = nullptr) noexcept;

        private:
            KUri::CSPtr name_;
            FABRIC_STATE_PROVIDER_ID stateProviderId_;
            KString::CSPtr typeName_;
            KGuid partitionId_;
            FABRIC_REPLICA_ID replicaId_;
            Data::Utilities::OperationData::CSPtr initializationParameters_;
        };
    }
}
