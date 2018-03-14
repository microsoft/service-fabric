// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;

GlobalStore::GlobalStore()
: closed_(false)
{
}

void GlobalStore::Close()
{
    AcquireExclusiveLock grab(lock_);

    providerTable_.clear();
    providers_.clear();
    closed_ = true;
}

void GlobalStore::Register(GlobalStoreProviderSPtr const & provider)
{
    AcquireExclusiveLock grab(lock_);

    if (closed_)
    {
        return;
    }

    for (SerializableWithActivationTypes::Enum type : provider->Types)
    {
        ASSERT_IF(providerTable_.find(type) != providerTable_.end(), "Global store provider type {0} already registered", type);
        providerTable_[type] = provider;
    }

    providers_.push_back(provider);
}

GlobalStoreProviderSPtr GlobalStore::GetProvider(SerializableWithActivationTypes::Enum typeId)
{
    AcquireReadLock grab(lock_);

    auto it = providerTable_.find(typeId);
    if (it == providerTable_.end())
    {
        return nullptr;
    }

    return it->second;
}

SerializableWithActivationList GlobalStore::AddOutputState(NodeInstance const* target)
{
    vector<GlobalStoreProviderSPtr> providers;
    {
        AcquireReadLock grab(lock_);
        providers = providers_;
    }

    SerializableWithActivationList output;
    for (GlobalStoreProviderSPtr const & provider : providers)
    {
        provider->AddOutputState(output, target);
    }

    return output;
}

void GlobalStore::ProcessInputState(SerializableWithActivationList & input, NodeInstance const & from)
{
    SerializableWithActivationTypes::Enum typeId = SerializableWithActivationTypes::Invalid;
    GlobalStoreProviderSPtr provider;

    for (SerializableWithActivationUPtr const & state : input.Objects)
    {
        if (state)
        {
            if (state->TypeId != typeId)
            {
                typeId = state->TypeId;
                provider = GetProvider(typeId);
            }

            if (provider)
            {
                provider->ProcessInputState(*state, from);
            }
        }
    }
}
