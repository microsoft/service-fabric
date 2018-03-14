// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "FabricCommon.h"
#include "FabricRuntime.h"
#include "FabricClient.h"

#include "ServiceGroup.Public.h"
#include "ServiceGroup.StatefulServiceFactory.h"
#include "ServiceGroup.StatelessServiceFactory.h"
#include "ServiceGroup.Runtime.h"
#include "ServiceGroup.InternalPublic.h"

using namespace ServiceGroup;

HRESULT CreateServiceGroupFactoryBuilder(__in IFabricCodePackageActivationContext * activationContext, __out IFabricServiceGroupFactoryBuilder ** builder)
{
    if (NULL == builder)
    {
        return E_POINTER;
    }

    CServiceGroupFactoryBuilder* factoryBuilder = new (std::nothrow) CServiceGroupFactoryBuilder(activationContext);
    if (NULL == factoryBuilder)
    {
        return E_OUTOFMEMORY;
    }

    *builder = factoryBuilder;
    return S_OK;
}

