// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::StateManager;

NTSTATUS ActiveStateProviderEnumerator::Create(
    __in IEnumerator<KeyValuePair<KUri::CSPtr, Metadata::SPtr>> & enumerator,
    __in StateProviderFilterMode::FilterMode mode,
    __in KAllocator & allocator,
    __out SPtr & result)
{
    result = _new(ACTIVES_STATEPROVIDER_ENUMERATOR, allocator) ActiveStateProviderEnumerator(enumerator, mode);

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

KeyValuePair<KUri::CSPtr, Metadata::SPtr> ActiveStateProviderEnumerator::Current()
{
    KeyValuePair<KUri::CSPtr, Metadata::SPtr> current(enumerator_->Current().Key, enumerator_->Current().Value);
    return current;
}

bool ActiveStateProviderEnumerator::MoveNext()
{
    while (enumerator_->MoveNext())
    {
        if (mode_ == StateProviderFilterMode::FilterMode::All)
        {
            return true;
        }

        KeyValuePair<KUri::CSPtr, Metadata::SPtr> entry(enumerator_->Current());
        if ((entry.Value)->TransientCreate == (mode_ == StateProviderFilterMode::FilterMode::Transient))
        {
            return true;
        }
    }

    return false;
}

ActiveStateProviderEnumerator::ActiveStateProviderEnumerator(
    __in IEnumerator<KeyValuePair<KUri::CSPtr, Metadata::SPtr>> & enumerator,
    __in StateProviderFilterMode::FilterMode mode)
    : enumerator_(&enumerator)
    , mode_(mode)
{
}

ActiveStateProviderEnumerator::~ActiveStateProviderEnumerator()
{
}
