// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"
#include "FabricClient.h"

//
// Service Group Exports.
//
HRESULT CreateServiceGroupFactoryBuilder(__in IFabricCodePackageActivationContext * activationContext, __out IFabricServiceGroupFactoryBuilder ** builder);

// Queryable interface for concrete IFabricStatefulServiceFactory used internally
static const GUID IID_InternalIStatefulServiceGroupFactory =
    {0x0d237c9c,0xba7f,0x4b54,{0xb4,0x8b,0xc9,0x8f,0x6f,0xc1,0xb9,0x4d}};

// Queryable interface for concrete IFabricStatelessServiceFactory used internally
static const GUID IID_InternalStatelessServiceGroupFactory =
    {0xf1f3edbc,0x58fa,0x4cd9,{0x8d,0x9d,0x1f,0xb3,0x46,0xc5,0xbf,0x83}};

