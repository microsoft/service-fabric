// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace MessageRetry;
using Infrastructure::BackgroundWorkManagerWithRetry;

FMMessageRetryComponent::FMMessageRetryComponent(Parameters & parameters) :
pendingEntityList_(
    parameters.Target, 
    parameters.DisplayName, 
    *parameters.EntitySetCollection, 
    *parameters.Throttle,
    *parameters.RA),
messageSender_(*parameters.PendingReplicaUploadState, parameters.RA->FMTransportObj, parameters.Target),
bgmr_(
    parameters.DisplayName, 
    parameters.Target.IsFmm ? L"FMMessageRetry" : L"FmmMessageRetry", 
    [this](wstring const & activityId, ReconfigurationAgent&, BackgroundWorkManagerWithRetry&) { OnBgmrRetry(activityId); },
    *parameters.MinimumIntervalBetweenWork,
    *parameters.RetryInterval,
    *parameters.RA),
ra_(*parameters.RA),
target_(parameters.Target)
{
}

void FMMessageRetryComponent::Request(
    std::wstring const & activityId)
{
    bgmr_.Request(activityId);
}

void FMMessageRetryComponent::Close()
{
    bgmr_.Close();
}

void FMMessageRetryComponent::OnBgmrRetry(wstring const & activityId)
{
    AsyncOperation::CreateAndStart<FMMessageRetryAsyncOperation>(
        activityId,
        *this, 
        [](AsyncOperationSPtr const & thisSPtr)
        {
            auto rv = AsyncOperation::End(thisSPtr);
            ASSERT_IF(!rv.IsSuccess(), "FMMessageRetry can never fail");
        },
        ra_.Root.CreateAsyncOperationRoot());
}

FMMessageRetryComponent::FMMessageRetryAsyncOperation::FMMessageRetryAsyncOperation(
    std::wstring const & activityId,
    FMMessageRetryComponent & component,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent) :
    AsyncOperation(callback, parent),
    perfData_(component.ra_.ClockSPtr),
    component_(component),
    activityId_(activityId)
{
}

void FMMessageRetryComponent::FMMessageRetryAsyncOperation::OnStart(Common::AsyncOperationSPtr const & thisSPtr)
{
    enumerationResult_ = component_.pendingEntityList_.Enumerate(perfData_);    

    component_.messageSender_.Send(activityId_, enumerationResult_.Messages, perfData_);

    component_.pendingEntityList_.BeginUpdateEntitiesAfterSend(
        activityId_,
        perfData_,
        enumerationResult_,
        [this](Common::AsyncOperationSPtr const & innerOp)
        {
            auto error = component_.pendingEntityList_.EndUpdateEntitiesAfterSend(innerOp);

            Infrastructure::RAEventSource::Events->FMMessageRetry(component_.ra_.NodeInstanceIdStr, activityId_, component_.target_, perfData_);

            component_.bgmr_.OnWorkComplete(enumerationResult_.RetryType);

            TryComplete(innerOp->Parent, error);
        },
        thisSPtr);
}

