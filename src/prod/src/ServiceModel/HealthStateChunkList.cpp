// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_BASE_DYNAMIC_SIZE_ESTIMATION(HealthStateChunkList)

StringLiteral const TraceSource("HealthStateChunkList");

HealthStateChunkList::HealthStateChunkList()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , totalCount_(0)
{
}

HealthStateChunkList::HealthStateChunkList(ULONG totalCount)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , totalCount_(totalCount)
{
}

HealthStateChunkList::HealthStateChunkList(HealthStateChunkList && other)
    : Common::IFabricJsonSerializable(move(other))
    , Serialization::FabricSerializable(move(other))
    , totalCount_(move(other.totalCount_))
{
}

HealthStateChunkList & HealthStateChunkList::operator =(HealthStateChunkList && other)
{
    if (this != &other)
    {
        totalCount_ = move(other.totalCount_);
    }

    Common::IFabricJsonSerializable::operator=(move(other));
    Serialization::FabricSerializable::operator=(move(other));
    return *this;
}

HealthStateChunkList::~HealthStateChunkList()
{
}
