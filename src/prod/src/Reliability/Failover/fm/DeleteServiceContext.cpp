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

DeleteServiceContext::DeleteServiceContext(wstring const & serviceName, uint64 serviceInstance)
    : serviceName_(serviceName),
      serviceInstance_(serviceInstance),
      failoverUnitCount_(0),
      failoverUnitId_(Guid::Empty()),
      BackgroundThreadContext(L"DeleteServiceContext." + serviceName)
{
}

BackgroundThreadContextUPtr DeleteServiceContext::CreateNewContext() const
{
    return make_unique<DeleteServiceContext>(serviceName_, serviceInstance_);
}

void DeleteServiceContext::Merge(BackgroundThreadContext const & context)
{
    DeleteServiceContext const & other = dynamic_cast<DeleteServiceContext const &>(context);
    if (failoverUnitCount_ == 0)
    {
        failoverUnitId_ = other.failoverUnitId_;
    }
    failoverUnitCount_ +=  other.failoverUnitCount_;
}

void DeleteServiceContext::Process(FailoverManager const& fm, FailoverUnit const & failoverUnit)
{
    UNREFERENCED_PARAMETER(fm);

    if (failoverUnit.ServiceInfoObj->Name == serviceName_)
    {
        if (failoverUnitCount_ == 0)
        {
            failoverUnitId_ = failoverUnit.Id;
        }
        failoverUnitCount_++;
    }
}

bool DeleteServiceContext::ReadyToComplete()
{
    return (failoverUnitCount_ == 0);
}

void DeleteServiceContext::Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted)
{
    UNREFERENCED_PARAMETER(isEnumerationAborted);

    fm.WriteInfo(Constants::ServiceSource, serviceName_,
        "DeleteService {0}: Instance={1}, FTCount={2}, Id={3}, IsContextCompleted={4}",
        serviceName_, serviceInstance_, failoverUnitCount_, failoverUnitId_, isContextCompleted);

    if (isContextCompleted)
    {
        fm.ServiceCacheObj.MarkServiceAsDeletedAsync(serviceName_, serviceInstance_);
    }
}
