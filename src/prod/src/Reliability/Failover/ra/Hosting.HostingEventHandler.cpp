// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace ServiceModel;
using namespace Hosting;
using namespace Hosting2;

namespace
{
    StringLiteral const RANotReady("not ready");
    StringLiteral const RANotActivated("not activated");
    StringLiteral const RAUpgrading("upgrading");
    StringLiteral const ServiceTypeRegistrationTag("ServiceTypeRegistration");
}

template<typename TInput>
void CreateAndStartWork(
    std::wstring const & activityId,
    std::function<bool(Infrastructure::ReadOnlyLockedFailoverUnitPtr &)> filter,
    TInput && input,
    MultipleEntityWork::FactoryFunctionPtr factory,
    ReconfigurationAgent & ra)
{
    Diagnostics::FilterEntityMapPerformanceData performanceData(ra.ClockSPtr);
    auto fts = FilterUnderReadLockOverMap(ra.LocalFailoverUnitMapObj, JobItemCheck::None, filter, performanceData);
    
    auto work = make_shared<MultipleFailoverUnitWorkWithInput<TInput>>(
        activityId, 
        [performanceData] (MultipleEntityWork & innerWork, ReconfigurationAgent & innerRA)
        {
            RAEventSource::Events->HostingFinishEvent(innerRA.NodeInstanceIdStr, innerWork.ActivityId, performanceData);
        },
        move(input));

    ra.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(work, fts, factory);
}

HostingEventHandler::HostingEventHandler(ReconfigurationAgent & ra) : ra_(ra)
{
}

void HostingEventHandler::ProcessServiceTypeRegistered(ServiceTypeRegistrationSPtr const & registration)
{
    auto activityId = CreateActivityId();

    RAEventSource::Events->HostingServiceTypeRegistered(ra_.NodeInstanceIdStr, HostingEventName::ServiceTypeRegistered, registration->VersionedServiceTypeId, activityId);
    if (!CanProcessEvent(activityId, true))
    {
        return;
    }

    ServiceTypeRegisteredInput input(registration);
    auto handler = [](HandlerParameters & handlerParameters, JobItemContextBase & context)
    {
        return handlerParameters.RA.ServiceTypeRegisteredProcessor(handlerParameters, context);
    };

    auto factory = [handler](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
    {
        ServiceTypeRegisteredJobItem::Parameters jobItemParameters(
            entry,
            work,
            handler,
            JobItemCheck::DefaultAndOpen,
            *JobItemDescription::ServiceTypeRegistered,
            move(JobItemContextBase()));

        return make_shared<ServiceTypeRegisteredJobItem>(move(jobItemParameters));
    };

    auto const & activationContext = registration->ActivationContext;
    if (activationContext.IsExclusive)
    {
        auto entry = ra_.LocalFailoverUnitMapObj.GetEntry(FailoverUnitId(activationContext.ActivationGuid));
        if (entry == nullptr)
        {
            return;
        }

        vector<EntityEntryBaseSPtr> fts;
        fts.push_back(move(entry));

        auto work = make_shared<MultipleFailoverUnitWorkWithInput<ServiceTypeRegisteredInput>>(activityId, move(input));

        ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(work, fts, factory);
    }
    else
    {
        /*
            Filter over the LFUM with a read lock prior to enqueing for write locks

            NOTE: It is important to consider fts that are just in the inserted state
            Another thread could have a write lock and be in the process of performing an insert
            so a write lock does need to be acquired for final validation

            TODO: This code has much in common with the other hosting event processors
            Move this into the hosting adapter - Create a hosting event handler
        */
        function<bool(ReadOnlyLockedFailoverUnitPtr&)> filter = [&registration](ReadOnlyLockedFailoverUnitPtr & ft)
        {
            ASSERT_IF(ft.IsEntityDeleted, "Deleted should never be passed to the filter");
            return (!ft || ft->ServiceDescription.Type == registration->ServiceTypeId);
        };

        CreateAndStartWork(activityId, filter, move(input), factory, ra_);
    }
}

void HostingEventHandler::ProcessRuntimeClosed(std::wstring const & hostId, std::wstring const & runtimeId)
{
    ASSERT_IFNOT(runtimeId.size() > 0, "Empty runtime id during Runtime failure processing");
    ASSERT_IFNOT(hostId.size() > 0, "Empty host id during Runtime failure processing");
    
    auto activityId = CreateActivityId();
    RAEventSource::Events->HostingProcessClosedEvent(ra_.NodeInstanceIdStr, HostingEventName::RuntimeDown, hostId, runtimeId, activityId);

    if (!CanProcessEvent(activityId, false))
    {
        return;
    }

    /*
        NOTE: It is important to consider fts that are just in the inserted state
        Another thread could have a write lock and be in the process of performing an insert or even adding the STR
        so a write lock does need to be acquired for final validation
    */
    function<bool(ReadOnlyLockedFailoverUnitPtr&)> filter = [&runtimeId, &hostId](ReadOnlyLockedFailoverUnitPtr & ft)
    {
        ASSERT_IF(ft.IsEntityDeleted, "Deleted should never be passed to the filter");
        if (!ft || ft->ServiceTypeRegistration == nullptr)
        {
            return true;
        }

        return ft->RuntimeId == runtimeId && ft->AppHostId == hostId;
    };

    auto factory = [](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
    {
        auto handler = [](HandlerParameters & handlerParameters, JobItemContextBase & context)
        {
            return handlerParameters.RA.RuntimeClosedProcessor(handlerParameters, context);
        };

        RuntimeClosedJobItem::Parameters jobItemParameters(
            entry,
            work,
            handler,
            static_cast<JobItemCheck::Enum>(JobItemCheck::FTIsNotNull | JobItemCheck::FTIsOpen | JobItemCheck::RAIsOpenOrClosing),
            *JobItemDescription::RuntimeClosed,
            move(JobItemContextBase()));

        return make_shared<RuntimeClosedJobItem>(move(jobItemParameters));
    };

    RuntimeClosedInput input(hostId, runtimeId);
    CreateAndStartWork(activityId, filter, move(input), factory, ra_);
}

void HostingEventHandler::ProcessAppHostClosed(wstring const & hostId, ActivityDescription const & activityDescription)
{
    ASSERT_IFNOT(hostId.size() > 0, "Empty apphost id during AppHost failure processing");

    RAEventSource::Events->HostingProcessClosedEvent(ra_.NodeInstanceIdStr, HostingEventName::AppHostDown, hostId, wstring(), activityDescription.ActivityId.ToString());
    if (!CanProcessEvent(activityDescription.ActivityId.ToString(), false))
    {
        return;
    }

    /*
        NOTE: It is important to consider fts that are just in the inserted state
        Another thread could have a write lock and be in the process of performing an insert or even adding the STR
        so a write lock does need to be acquired for final validation
    */
    function<bool(ReadOnlyLockedFailoverUnitPtr&)> filter = [&hostId](ReadOnlyLockedFailoverUnitPtr & ft)
    {
        ASSERT_IF(ft.IsEntityDeleted, "Deleted should never be passed to the filter");
        if (!ft || ft->ServiceTypeRegistration == nullptr)
        {
            return true;
        }

        return ft->AppHostId == hostId;
    };

    Diagnostics::FilterEntityMapPerformanceData performanceData(ra_.ClockSPtr);
    auto fts = FilterUnderReadLockOverMap(ra_.LocalFailoverUnitMapObj, JobItemCheck::None, filter, performanceData);

    auto factory = [](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
    {
        auto handler = [](HandlerParameters & handlerParameters, JobItemContextBase & context)
        {
            return handlerParameters.RA.AppHostClosedProcessor(handlerParameters, context);
        };

        AppHostClosedJobItem::Parameters jobItemParameters(
            entry,
            work,
            handler,
            static_cast<JobItemCheck::Enum>(JobItemCheck::FTIsNotNull | JobItemCheck::RAIsOpenOrClosing),
            *JobItemDescription::AppHostClosed,
            move(JobItemContextBase()));

        return make_shared<AppHostClosedJobItem>(move(jobItemParameters));
    };

    AppHostClosedInput input(activityDescription, hostId);
    CreateAndStartWork(activityDescription.ActivityId.ToString(), filter, move(input), factory, ra_);
}

wstring HostingEventHandler::CreateActivityId() const
{
    // TODO: When the entire infrastructure moves to activity id use it everywhere
    return ActivityId().ToString();
}

bool HostingEventHandler::CanProcessEvent(wstring const & activityId, bool doesEventCauseReplicaOpen)
{
    StringLiteral const * reason = nullptr;

    if (!ra_.StateObj.IsFMReady)
    {
        reason = &RANotReady;
    }
    else if (doesEventCauseReplicaOpen)
    {
        if (!ra_.NodeStateObj.GetNodeDeactivationState(*FailoverManagerId::Fm).IsActivated)
        {
            reason = &RANotActivated;
        }

        if (ra_.StateObj.IsUpgrading)
        {
            reason = &RAUpgrading;
        }
    }

    if (reason != nullptr)
    {
        RAEventSource::Events->HostingIgnoreEvent(ra_.NodeInstanceIdStr, activityId, *reason);
        return false;
    }

    return true;
}
