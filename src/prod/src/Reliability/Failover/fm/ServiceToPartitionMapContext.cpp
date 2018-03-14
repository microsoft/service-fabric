// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

ServiceToPartitionMapContext::ServiceToPartitionMapContext(wstring const & serviceName, uint64 serviceInstance)
    : serviceName_(serviceName),
      serviceInstance_(serviceInstance),
      failoverUnitIds_(),
      BackgroundThreadContext(L"ServiceToPartitionMapContext." + serviceName)
{
}

BackgroundThreadContextUPtr ServiceToPartitionMapContext::CreateNewContext() const
{
    return make_unique<ServiceToPartitionMapContext>(serviceName_, serviceInstance_);
}

void ServiceToPartitionMapContext::Merge(BackgroundThreadContext const & context)
{
    ServiceToPartitionMapContext const& other = dynamic_cast<ServiceToPartitionMapContext const&>(context);

    failoverUnitIds_.insert(other.failoverUnitIds_.begin(), other.failoverUnitIds_.end());
}

void ServiceToPartitionMapContext::Process(FailoverManager const& fm, FailoverUnit const & failoverUnit)
{
    UNREFERENCED_PARAMETER(fm);

    if (!failoverUnit.IsOrphaned &&
        failoverUnit.ServiceInfoObj->Name == serviceName_)
    {
        failoverUnitIds_.insert(failoverUnit.Id);
    }
}

bool ServiceToPartitionMapContext::ReadyToComplete()
{
    return true;
}

void ServiceToPartitionMapContext::Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted)
{
    UNREFERENCED_PARAMETER(isEnumerationAborted);

    fm.WriteInfo(
        Constants::ServiceSource, serviceName_,
        "ServiceToPartitionMap: Name={0}, Instance={1}, IsContextCompleted={2}",
        serviceName_, serviceInstance_, isContextCompleted);

    if (isContextCompleted)
    {
        fm.ServiceCacheObj.SetFailoverUnitIdsAsync(serviceName_, serviceInstance_, move(failoverUnitIds_));
    }
}
