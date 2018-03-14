// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::StateManager;

StateProviderInfoInternal::StateProviderInfoInternal() noexcept
    : KObject()
    , typeName_()
    , name_()
    , stateProviderId_()
    , stateProviderSPtr_()
{
    SetConstructorStatus(STATUS_SUCCESS);
}

StateProviderInfoInternal::StateProviderInfoInternal(
    __in KString const & typeName,
    __in KUri const & name,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in TxnReplicator::IStateProvider2 & stateProvider) noexcept
    : KObject()
    , typeName_(&typeName)
    , name_(&name)
    , stateProviderId_(stateProviderId)
    , stateProviderSPtr_(&stateProvider)
{
    SetConstructorStatus(STATUS_SUCCESS);
}

StateProviderInfoInternal::StateProviderInfoInternal(
    __in StateProviderInfoInternal const & other) noexcept
    : StateProviderInfoInternal(*other.TypeName, *other.Name, other.StateProviderId, *other.StateProviderSPtr)
{
}

StateProviderInfoInternal& StateProviderInfoInternal::operator=(__in StateProviderInfoInternal const & other) noexcept
{
    if (&other == this)
    {
        return *this;
    }

    typeName_ = other.TypeName;
    name_ = other.Name;
    stateProviderId_ = other.StateProviderId;
    stateProviderSPtr_ = other.StateProviderSPtr;

    return *this;
}

StateProviderInfoInternal::~StateProviderInfoInternal()
{
}
