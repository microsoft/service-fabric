// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Hosting;
using namespace ServiceModel;
using namespace Infrastructure;

ServiceTypeMapEntity::ServiceTypeMapEntity(
    Federation::NodeInstance const & nodeInstance,
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    TimeSpanConfigEntry const & retryInterval) :
    EntityRetryComponent<ServiceTypeInfo>(make_shared<Infrastructure::LinearRetryPolicy>(retryInterval)),
    count_(0),
    id_(registration->ServiceTypeId),
    idStr_(wformatString(registration->ServiceTypeId)),
    nodeInstance_(nodeInstance)
{
}

void ServiceTypeMapEntity::OnRegistrationAdded(
    std::wstring const &,
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    __out bool & isSendRequired)
{
    ASSERT_IF(id_ != registration->ServiceTypeId, "ServiceTypeId must match {0}", id_);

    // Initialize out parameter
    isSendRequired = false;

    count_++;

    if (version_ != registration->VersionedServiceTypeId.servicePackageVersionInstance || count_ == 1)
    {
        isSendRequired = true;
        MarkRetryRequired();

        version_ = registration->VersionedServiceTypeId.servicePackageVersionInstance;
        codePackageName_ = registration->CodePackageName;
    }

    Trace();
}

void ServiceTypeMapEntity::OnRegistrationRemoved(
    std::wstring const &,
    __out bool & isDeleteRequired)
{
    isDeleteRequired = false;

    count_--;
    if (count_ == 0)
    {
        isDeleteRequired = true;
    }

    Trace();
}

void ServiceTypeMapEntity::OnReply(ServiceTypeInfo const & info)
{
    if (info.VersionedServiceTypeId.servicePackageVersionInstance.InstanceId != version_.InstanceId)
    {
        return;
    }

    MarkRetryNotRequired();
}

ServiceTypeInfo ServiceTypeMapEntity::GenerateInternal() const
{
    VersionedServiceTypeIdentifier versionedServiceTypeId(id_, version_);
    return ServiceTypeInfo (versionedServiceTypeId, codePackageName_);
}

void ServiceTypeMapEntity::Trace() const
{
    RAEventSource::Events->ServiceType(idStr_, nodeInstance_, version_, codePackageName_, count_, IsRetryPending);
}
