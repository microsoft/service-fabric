// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ProvisionApplicationTypeDescriptionBase");

ProvisionApplicationTypeDescriptionBase::ProvisionApplicationTypeDescriptionBase() 
    : kind_(FABRIC_PROVISION_APPLICATION_TYPE_KIND_INVALID)
    , isAsync_(false)
{ 
}

ProvisionApplicationTypeDescriptionBase::ProvisionApplicationTypeDescriptionBase(
    FABRIC_PROVISION_APPLICATION_TYPE_KIND kind)
    : kind_(kind)
    , isAsync_(false)
{
}

ProvisionApplicationTypeDescriptionBase::~ProvisionApplicationTypeDescriptionBase()
{
}

ErrorCode ProvisionApplicationTypeDescriptionBase::TraceAndGetError(
    ErrorCodeValue::Enum errorValue,
    wstring && message) const
{
    Trace.WriteWarning(TraceComponent, "{0}: {1}", errorValue, message);
    return ErrorCode(errorValue, move(message));
}

ProvisionApplicationTypeDescriptionBaseSPtr ProvisionApplicationTypeDescriptionBase::CreateSPtr(FABRIC_PROVISION_APPLICATION_TYPE_KIND kind)
{
    switch (kind)
    {
    case FABRIC_PROVISION_APPLICATION_TYPE_KIND_IMAGE_STORE_PATH:
        return ProvisionApplicationTypeSerializationTypeActivator<FABRIC_PROVISION_APPLICATION_TYPE_KIND_IMAGE_STORE_PATH>::CreateSPtr();
    case FABRIC_PROVISION_APPLICATION_TYPE_KIND_EXTERNAL_STORE:
        return ProvisionApplicationTypeSerializationTypeActivator<FABRIC_PROVISION_APPLICATION_TYPE_KIND_EXTERNAL_STORE>::CreateSPtr();
   
    default:
        return make_shared<ProvisionApplicationTypeDescriptionBase>(FABRIC_PROVISION_APPLICATION_TYPE_KIND_INVALID);
    }
}

