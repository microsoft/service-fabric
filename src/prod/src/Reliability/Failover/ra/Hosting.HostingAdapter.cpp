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
    wstring const ServiceTypeWorkManagerName(L"ServiceType");
}

HostingAdapter::HostingAdapter(IHostingSubsystem & hosting, ReconfigurationAgent & ra) :
hosting_(hosting),
bgmr_(
    ra.NodeInstanceIdStr + L"_" + ServiceTypeWorkManagerName,
    ServiceTypeWorkManagerName,
    [this](wstring const & activityId, ReconfigurationAgent & , BackgroundWorkManagerWithRetry & )
    {
        OnRetry(activityId);
    },    
    ra.Config.PerNodeMinimumIntervalBetweenServiceTypeMessageToFMEntry,
    ra.Config.ServiceTypeMessageRetryIntervalEntry,
    ra),
ra_(ra),
serviceTypeMap_(ra),
eventHandler_(ra),
existingReplicaFindServiceTypeRegistrationErrorList_(FindServiceTypeRegistrationErrorList::CreateErrorListForExistingReplica()),
newReplicaFindServiceTypeRegistrationErrorList_(FindServiceTypeRegistrationErrorList::CreateErrorListForNewReplica())
{
}

void HostingAdapter::Close()
{
    bgmr_.Close();
}

FindServiceTypeRegistrationErrorCode HostingAdapter::FindServiceTypeRegistration(
    bool isNewReplica,
    Reliability::FailoverUnitId const & ftId,
    Reliability::ServiceDescription const & description,
    __out Hosting2::ServiceTypeRegistrationSPtr & registration)
{
    auto error = ra_.GetServiceTypeRegistration(ftId, description, registration);

    auto const errorList = isNewReplica ? &newReplicaFindServiceTypeRegistrationErrorList_ : &existingReplicaFindServiceTypeRegistrationErrorList_;
    return errorList->TranslateError(std::move(error));
}

void HostingAdapter::AddTerminateServiceHostAction(
    TerminateServiceHostReason::Enum reason,
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    Infrastructure::StateMachineActionQueue & queue)
{
    queue.Enqueue(make_unique<TerminateHostStateMachineAction>(reason, registration));
}

void HostingAdapter::TerminateServiceHost(
    std::wstring const & activityId,
    TerminateServiceHostReason::Enum reason,
    Hosting2::ServiceTypeRegistrationSPtr const & registration)
{
    Infrastructure::RAEventSource::Events->HostingTerminateServiceHost(ra_.NodeInstanceIdStr, registration->HostId, registration->HostProcessId, reason, activityId);

    hosting_.BeginTerminateServiceHost(
        registration->HostId,
        [this](AsyncOperationSPtr const & op)
        {
            hosting_.EndTerminateServiceHost(op);
        },
        ra_.Root.CreateAsyncOperationRoot());
}

ErrorCode HostingAdapter::IncrementUsageCount(
    ServiceModel::ServiceTypeIdentifier const& typeId, 
    ServiceModel::ServicePackageActivationContext const & activationContext,
    Infrastructure::StateMachineActionQueue& queue)
{    
    auto error = hosting_.IncrementUsageCount(typeId, activationContext);
    if (!error.IsSuccess())
    { 
        return error;
    }

    /*
        it is possible that persistence may fail
        if persistence fails we need to undo the increment call made above
        enqueue an action so that it is modeled similar to other such changes
    */
    queue.Enqueue(make_unique<IncrementUsageCountStateMachineAction>(typeId, activationContext));
    return error;
}

void HostingAdapter::DecrementUsageCount(
    ServiceModel::ServiceTypeIdentifier const & typeId,
    ServiceModel::ServicePackageActivationContext const & activationContext,
    Infrastructure::StateMachineActionQueue & queue)
{
    queue.Enqueue(make_unique<DecrementUsageCountStateMachineAction>(typeId, activationContext));
}

void HostingAdapter::OnServiceTypeRegistrationAdded(
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    Infrastructure::StateMachineActionQueue & queue)
{
    AddServiceTypeRegistrationChangeAction(registration, queue, true);
}

void HostingAdapter::OnServiceTypeRegistrationAdded(
    std::wstring const & activityId,
    Hosting2::ServiceTypeRegistrationSPtr const& registration)
{
    serviceTypeMap_.OnServiceTypeRegistrationAdded(activityId, registration, bgmr_);
}

void HostingAdapter::OnServiceTypeRegistrationRemoved(
    Hosting2::ServiceTypeRegistrationSPtr const& registration, 
    Infrastructure::StateMachineActionQueue& queue)
{
    AddServiceTypeRegistrationChangeAction(registration, queue, false);
}

void HostingAdapter::OnServiceTypeRegistrationRemoved(
    std::wstring const & activityId,
    Hosting2::ServiceTypeRegistrationSPtr const& registration)
{
    serviceTypeMap_.OnServiceTypeRegistrationRemoved(activityId, registration);
}

void HostingAdapter::OnRetry(
    std::wstring const & activityId)
{
    serviceTypeMap_.OnWorkManagerRetry(activityId, bgmr_, ra_);
}

void HostingAdapter::ProcessServiceTypeNotificationReply(Reliability::ServiceTypeNotificationReplyMessageBody const& body)
{
    serviceTypeMap_.OnReplyFromFM(body.Infos);
}

void HostingAdapter::AddServiceTypeRegistrationChangeAction(
    Hosting2::ServiceTypeRegistrationSPtr const& registration, 
    Infrastructure::StateMachineActionQueue& queue, 
    bool isAdd)
{
    ASSERT_IF(registration == nullptr, "Cannot pass in null");

    if (registration->ServiceTypeId == ServiceTypeIdentifier::FailoverManagerServiceTypeId)
    {
        return;
    }

    queue.Enqueue(make_unique<ServiceTypeRegistrationChangeStateMachineAction>(registration, isAdd));
}

HostingAdapter::IncrementUsageCountStateMachineAction::IncrementUsageCountStateMachineAction(
    ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
    ServiceModel::ServicePackageActivationContext const & activationContext) : 
    serviceTypeId_(serviceTypeId),
    activationContext_(activationContext)
{
}

void HostingAdapter::IncrementUsageCountStateMachineAction::OnPerformAction(
    std::wstring const &,
    Infrastructure::EntityEntryBaseSPtr const &,
    ReconfigurationAgent &)
{
}

void HostingAdapter::IncrementUsageCountStateMachineAction::OnCancelAction(ReconfigurationAgent & ra)
{
    ra.HostingObj.DecrementUsageCount(serviceTypeId_, activationContext_);
}

HostingAdapter::DecrementUsageCountStateMachineAction::DecrementUsageCountStateMachineAction(
    ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
    ServiceModel::ServicePackageActivationContext const & activationContext) : 
    serviceTypeId_(serviceTypeId),
    activationContext_(activationContext)
{
}

void HostingAdapter::DecrementUsageCountStateMachineAction::OnPerformAction(
    std::wstring const &,
    Infrastructure::EntityEntryBaseSPtr const &,
    ReconfigurationAgent & ra)
{
    ra.HostingObj.DecrementUsageCount(serviceTypeId_, activationContext_);
}

void HostingAdapter::DecrementUsageCountStateMachineAction::OnCancelAction(ReconfigurationAgent &)
{
}
