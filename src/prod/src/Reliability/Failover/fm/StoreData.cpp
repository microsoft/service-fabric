// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

StoreData::StoreData()
    : operationLSN_(0),
      persistenceState_(PersistenceState::NoChange)
{
} 

StoreData::StoreData(StoreData const& other)
    : operationLSN_(other.operationLSN_),
      persistenceState_(other.persistenceState_)
{
} 

StoreData::StoreData(int64 operationLSN) 
    : operationLSN_(operationLSN),
      persistenceState_(PersistenceState::NoChange)
{
} 

StoreData::StoreData(PersistenceState::Enum persistenceState)
    : operationLSN_(0),
      persistenceState_(persistenceState)
{
}

StoreData::StoreData(int64 operationLSN, PersistenceState::Enum persistenceState)
    : operationLSN_(operationLSN),
      persistenceState_(persistenceState)
{
}

void StoreData::PostRead(::FABRIC_SEQUENCE_NUMBER operationLSN)
{
    operationLSN_ = operationLSN;
}

void StoreData::PostCommit(::FABRIC_SEQUENCE_NUMBER operationLSN)
{
    operationLSN_ = operationLSN;
    PersistenceState = PersistenceState::NoChange;
}
