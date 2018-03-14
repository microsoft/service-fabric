// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Utilities;
using namespace Data::StateManager;

FactoryArguments::FactoryArguments(
    __in KUri const & name,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in KString const & typeName,
    __in KGuid const & partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in_opt Data::Utilities::OperationData const * const initializationParameters) noexcept
    : name_(&name)
    , stateProviderId_(stateProviderId)
    , typeName_(&typeName)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , initializationParameters_(initializationParameters)
{
}

KUri::CSPtr FactoryArguments::get_Name() const { return name_; }

FABRIC_STATE_PROVIDER_ID FactoryArguments::get_StateProviderId() const { return stateProviderId_; }

KString::CSPtr FactoryArguments::get_TypeName() const { return typeName_; }

KGuid FactoryArguments::get_PartitionId() const { return partitionId_; }

FABRIC_REPLICA_ID FactoryArguments::get_ReplicaId() const { return replicaId_; }

OperationData::CSPtr FactoryArguments::get_InitializationParameters() const { return initializationParameters_; }
