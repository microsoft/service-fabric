// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ServiceGroup.ServiceBaseUnit.h"
#include "ServiceGroup.Constants.h"

using namespace ServiceGroup;

//
// Constructor/Destructor.
//
CBaseServiceUnit::CBaseServiceUnit(__in FABRIC_PARTITION_ID partitionId) : 
    partitionId_(Common::Guid(partitionId))
{
    opened_ = FALSE;
    outerServiceGroupReport_ = NULL;
}

CBaseServiceUnit::~CBaseServiceUnit(void) 
{
}

