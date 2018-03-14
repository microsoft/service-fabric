// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ServicePackageInstanceContext::ServicePackageInstanceContext()
    : servicePackageInstanceId_(),    
    applicationEnvironment_()
{
}

ServicePackageInstanceContext::ServicePackageInstanceContext(
    wstring const & servicePackageName,
    ServicePackageActivationContext activationContext,
	wstring const & servicePackagePublicActivationId,
    wstring const & applicationName,
    ApplicationIdentifier const & applicationId,    
    ApplicationEnvironmentContextSPtr const & applicationEnvironment)
    : servicePackageInstanceId_(ServicePackageIdentifier(applicationId, servicePackageName), activationContext, servicePackagePublicActivationId),
    applicationEnvironment_(applicationEnvironment),
    applicationName_(applicationName)
{
}

ServicePackageInstanceContext::ServicePackageInstanceContext(ServicePackageInstanceContext const & other)
    : servicePackageInstanceId_(other.servicePackageInstanceId_),    
    applicationEnvironment_(other.applicationEnvironment_),
    applicationName_(other.applicationName_)
{
}

ServicePackageInstanceContext::ServicePackageInstanceContext(ServicePackageInstanceContext && other)
    : servicePackageInstanceId_(move(other.servicePackageInstanceId_)),
    applicationEnvironment_(move(other.applicationEnvironment_)),
    applicationName_(move(other.applicationName_))
{
}

ServicePackageInstanceContext const & ServicePackageInstanceContext::operator = (ServicePackageInstanceContext const & other)
{
    if (this != &other)
    {
        this->servicePackageInstanceId_ = other.servicePackageInstanceId_;        
        this->applicationEnvironment_ = other.applicationEnvironment_;
        this->applicationName_ = other.applicationName_;
    }

    return *this;
}

ServicePackageInstanceContext const & ServicePackageInstanceContext::operator = (ServicePackageInstanceContext && other)
{
    if (this != &other)
    {
        this->servicePackageInstanceId_ = move(other.servicePackageInstanceId_);        
        this->applicationEnvironment_ = move(other.applicationEnvironment_);
        this->applicationName_ = move(other.applicationName_);
    }

    return *this;
}
