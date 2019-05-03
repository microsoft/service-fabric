// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace TpccService;
using namespace TxnReplicator;
using namespace Data::Utilities;

void DistrictStoreUtil::CreateStore(
    __in const Data::StateManager::FactoryArguments& factoryArg,
    __in FABRIC_REPLICA_ID replicaId,
    __in KAllocator& allocator,
    __out TxnReplicator::IStateProvider2::SPtr& stateProvider)
{
    KUriView stateProviderName = *factoryArg.Name;
    KStringView typeName = *factoryArg.TypeName;
    CreateStoreUtil::CreateStore<
        DistrictKey::SPtr,
        DistrictValue::SPtr,
        DistrictKeyComparer,
        DistrictKeySerializer,
        DistrictValueByteSerializer>(
            replicaId,
            DistrictKey::Hash,
            stateProviderName,
            factoryArg.StateProviderId,
            allocator,
            stateProvider);
}
