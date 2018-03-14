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

EntityJobItem::EntityJobItem(
    std::shared_ptr<HealthEntity> const & entity)
    : IHealthJobItem(entity->EntityKind)
    , entity_(entity)
{
}

EntityJobItem::~EntityJobItem()
{
}
