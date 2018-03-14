// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

template <class TEntityId>
ProcessSequenceStreamJobItem<TEntityId>::ProcessSequenceStreamJobItem(
    __in HealthEntityCache<TEntityId> & healthEntityCache,
    SequenceStreamRequestContext && context)
    : IHealthJobItem(HealthEntityKind::FromHealthInformationKind(context.Kind))
    , context_(move(context))
    , healthEntityCache_(healthEntityCache)
{
}

template <class TEntityId>
ProcessSequenceStreamJobItem<TEntityId>::~ProcessSequenceStreamJobItem()
{
}

template <class TEntityId>
void ProcessSequenceStreamJobItem<TEntityId>::ProcessInternal(IHealthJobItemSPtr const & thisSPtr)
{
    healthEntityCache_.StartProcessSequenceStream(thisSPtr, context_);
}

template <class TEntityId>
void ProcessSequenceStreamJobItem<TEntityId>::OnComplete(Common::ErrorCode const & error)
{
    // Notify caller of result
    auto copyError = error;
    healthEntityCache_.HealthManagerReplicaObj.QueueSequenceStreamForCompletion(move(context_), move(copyError));
}

template <class TEntityId>
void ProcessSequenceStreamJobItem<TEntityId>::FinishAsyncWork()
{
    healthEntityCache_.EndProcessSequenceStream(*this, this->PendingAsyncWork);
}

// Template specializations
TEMPLATE_SPECIALIZATION_ENTITY_ID(Management::HealthManager::ProcessSequenceStreamJobItem)
