// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using ServiceModel::ServicePackageIdentifier;
using ServiceModel::ServiceTypeIdentifier;

namespace
{
    Global<wstring> const Empty = make_global<wstring>(L"");
}

ServiceTypeRegistrationWrapper::ServiceTypeRegistrationWrapper(
    FailoverUnitDescription const & ftDesc,
    ServiceDescription const & sdDesc) :
    ftDesc_(ftDesc),
    sdDesc_(sdDesc),
    servicePackageInUse_(false)
{
}
    

ServiceTypeRegistrationWrapper::ServiceTypeRegistrationWrapper(
    FailoverUnitDescription const & ftDesc,
    ServiceDescription const & sdDesc,
    ServiceTypeRegistrationWrapper const & other) :
    ftDesc_(ftDesc),
    sdDesc_(sdDesc),
    servicePackageInUse_(other.servicePackageInUse_),
    serviceTypeRegistration_(other.serviceTypeRegistration_),
    retryableErrorState_(other.retryableErrorState_)
{
}

ServiceTypeRegistrationWrapper & ServiceTypeRegistrationWrapper::operator=(ServiceTypeRegistrationWrapper const & other)
{
    if (this != &other)
    {
        servicePackageInUse_ = other.servicePackageInUse_;
        serviceTypeRegistration_ = other.serviceTypeRegistration_;
        retryableErrorState_ = other.retryableErrorState_;
    }

    return *this;
}

wstring const & ServiceTypeRegistrationWrapper::get_AppHostId() const
{
    if (serviceTypeRegistration_)
    {
        return serviceTypeRegistration_->HostId;
    }
    else
    {
        return *Empty;
    }
}

wstring const & ServiceTypeRegistrationWrapper::get_RuntimeId() const
{
    if (serviceTypeRegistration_)
    {
        return serviceTypeRegistration_->RuntimeId;
    }
    else
    {
        return *Empty;
    }
}

ErrorCodeValue::Enum ServiceTypeRegistrationWrapper::AddServiceTypeRegistration(
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    Hosting::HostingAdapter & hosting,
    Infrastructure::StateMachineActionQueue & queue)
{
    /*
        The usage count is incremented only once for an FT

        It is not decremented until the replica leaves the node

        This is so that hosting knows not to deactivate a service package if a code package
        hosting all the replicas crashes.

        Consider: ServiePackage SP1 with code packages CP1, CP2 and W (watchdog)
        
        Let CP1 host SP and SP2 host SV replicas

        Now if only SV replicas are on the node and CP2 crashes it is fine for hosting to deactivate W

        However if there are SP replicas and CP1 crashes then hosting should not deactivate W because
        RA will reopen CP1

        Thus, RA increments the ref count when a replica opens for the first time and decrements it 
        only when the replica is dropped (for SV down = dropped)
     */
    if (!servicePackageInUse_)
    {
        auto error = hosting.IncrementUsageCount(registration->ServiceTypeId, CreateActivationContext(), queue);
        if (!error.IsSuccess())
        {
            return error.ReadValue();
        }

        servicePackageInUse_ = true;
    }
    
    /*
        The registration is updated only once and that needs to go
        to the service type map so that it can send message to the FM 
        if needed
    */
    if (serviceTypeRegistration_ == nullptr)
    {
        serviceTypeRegistration_ = registration;
        hosting.OnServiceTypeRegistrationAdded(registration, queue);
    }

    return ErrorCodeValue::Success;
}

void ServiceTypeRegistrationWrapper::OnReplicaDown(
    Hosting::HostingAdapter & hosting,
    Infrastructure::StateMachineActionQueue & queue)
{
    ReleaseServiceTypeRegistration(hosting, queue);
}

void ServiceTypeRegistrationWrapper::OnReplicaClosed(
    Hosting::HostingAdapter & hosting,
    Infrastructure::StateMachineActionQueue & queue)
{
    ReleaseServiceTypeRegistration(hosting, queue);
    retryableErrorState_.EnterState(RetryableErrorStateName::None);

    if (servicePackageInUse_)
    {
        hosting.DecrementUsageCount(sdDesc_.Type, CreateActivationContext(), queue);
        servicePackageInUse_ = false;
    }
}

void ServiceTypeRegistrationWrapper::ReleaseServiceTypeRegistration(
    Hosting::HostingAdapter & hosting,
    Infrastructure::StateMachineActionQueue & queue)
{
    if (serviceTypeRegistration_ != nullptr)
    {
        hosting.OnServiceTypeRegistrationRemoved(serviceTypeRegistration_, queue);
        serviceTypeRegistration_ = nullptr;
    }
}

void ServiceTypeRegistrationWrapper::AssertInvariants(
    FailoverUnit const & owner) const
{
    TESTASSERT_IF(serviceTypeRegistration_ != nullptr && servicePackageInUse_ == false, "If there is service type registration then service package must be in use {0}", owner);
}

std::string ServiceTypeRegistrationWrapper::AddField(Common::TraceEvent& traceEvent, std::string const& name)
{
    string format = "{0}";
    size_t index = 0;

    traceEvent.AddEventField<ServiceModel::ServicePackageVersionInstance>(format, name + ".spvi", index);

    return format;
}

void ServiceTypeRegistrationWrapper::FillEventData(Common::TraceEventContext& context) const
{
    context.Write(serviceTypeRegistration_ == nullptr ? ServiceModel::ServicePackageVersionInstance::Invalid : serviceTypeRegistration_->VersionedServiceTypeId.servicePackageVersionInstance);
}

void ServiceTypeRegistrationWrapper::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w << (serviceTypeRegistration_ == nullptr ? ServiceModel::ServicePackageVersionInstance::Invalid : serviceTypeRegistration_->VersionedServiceTypeId.servicePackageVersionInstance);
}

void ServiceTypeRegistrationWrapper::Test_SetServiceTypeRegistration(Hosting2::ServiceTypeRegistrationSPtr const & registration)
{
    ASSERT_IF(serviceTypeRegistration_ != nullptr, "registration already set");
    serviceTypeRegistration_ = registration;
    servicePackageInUse_ = true;
}

ServiceModel::ServicePackageActivationContext ServiceTypeRegistrationWrapper::CreateActivationContext() const
{
    return sdDesc_.CreateActivationContext(ftDesc_.FailoverUnitId.Guid);
}

void ServiceTypeRegistrationWrapper::Start(Reliability::ReconfigurationAgentComponent::ReplicaStates::Enum replicaState)
{
    auto newState = RetryableErrorStateName::None;
    if (replicaState == ReplicaStates::InCreate)
    {
        newState = RetryableErrorStateName::FindServiceRegistrationAtOpen;
    }
    else if (replicaState == ReplicaStates::StandBy)
    {
        newState = RetryableErrorStateName::FindServiceRegistrationAtReopen;
    }
    else if (replicaState == ReplicaStates::InDrop)
    {
        newState = RetryableErrorStateName::FindServiceRegistrationAtDrop;
    }
    else
    {
        Assert::TestAssert("RetryableErrorState {0} is not allowed here.", replicaState);
    }

    RetryableErrorStateEnterState(newState);
}

void ServiceTypeRegistrationWrapper::ReleaseServiceTypeRegistrationAndStart(
    Hosting::HostingAdapter & hosting,
    Infrastructure::StateMachineActionQueue & queue,
    Reliability::ReconfigurationAgentComponent::ReplicaStates::Enum replicaState)
{
    ReleaseServiceTypeRegistration(hosting, queue);
    Start(replicaState);
}

void ServiceTypeRegistrationWrapper::RetryableErrorStateEnterState(RetryableErrorStateName::Enum state)
{
    retryableErrorState_.EnterState(state);
}

RetryableErrorAction::Enum ServiceTypeRegistrationWrapper::RetryableErrorStateOnFailure(Reliability::FailoverConfig const & config)
{
    return retryableErrorState_.OnFailure(config);
}

RetryableErrorAction::Enum ServiceTypeRegistrationWrapper::RetryableErrorStateOnSuccessAndTransitionTo(
    RetryableErrorStateName::Enum state,
    Reliability::FailoverConfig const & config)
{
    return retryableErrorState_.OnSuccessAndTransitionTo(state, config);
}

RetryableErrorAction::Enum ServiceTypeRegistrationWrapper::ProcessFindServiceType(Hosting::FindServiceTypeRegistrationErrorCode fsrError, bool isIncremented, Reliability::FailoverConfig const & config)
{
    if (fsrError.IsSuccess())
    {
        return isIncremented ? RetryableErrorStateOnSuccessAndTransitionTo(RetryableErrorStateName::None, config) : RetryableErrorAction::None;
    }
    else if (fsrError.IsRetryable)
    {
        return RetryableErrorStateOnFailure(config);
    }
    else
    {
        return RetryableErrorAction::None;
    }
}

bool ServiceTypeRegistrationWrapper::TryAddServiceTypeRegistration(
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    Hosting::HostingAdapter & hosting,
    Infrastructure::StateMachineActionQueue & queue)
{
    auto retValue = AddServiceTypeRegistration(registration, hosting, queue);

    return retValue == ErrorCodeValue::Success;
}

Common::ErrorCode ServiceTypeRegistrationWrapper::TryGetAndAddServiceTypeRegistration(
    Hosting2::ServiceTypeRegistrationSPtr const & incomingRegistration,
    Hosting::HostingAdapter & hosting,
    Infrastructure::StateMachineActionQueue & queue,
    FailoverConfig const & config,
    __out std::pair<Health::ReplicaHealthEvent::Enum, Health::IHealthDescriptorUPtr> & healthReportInformation)
{
     TESTASSERT_IF(retryableErrorState_.CurrentState == RetryableErrorStateName::None, "retryableErrorState_ must have retryable error state here.");

     /*
     There are two scenarios to consider here:
     - Normal retry during replica open or replica close where the registration does not exist.
     In this case a call to hosting should be made to find the service type registration
     This will prompt hosting to start activation
 
     - A call from ServiceTypeRegistered when hosting has found the registration
     At this time, if the FT does not have the registration it can simply take the registration
     provided by hosting
     */
     Hosting2::ServiceTypeRegistrationSPtr serviceTypeRegistration = incomingRegistration;
     Hosting::FindServiceTypeRegistrationErrorCode fsrError;
     if (serviceTypeRegistration == nullptr)
     {
         fsrError = hosting.FindServiceTypeRegistration(retryableErrorState_.CurrentState == RetryableErrorStateName::FindServiceRegistrationAtOpen, ftDesc_.FailoverUnitId, sdDesc_, serviceTypeRegistration);
     }

    bool isIncremented = false;
    if (fsrError.IsSuccess())
    {
        /*
        It is possible that the STR was received but incrementing the ref count failed
        (example - the host came up, registered and crashed)
        These are transient issues and need to be retried
        */
        ASSERT_IF(serviceTypeRegistration == nullptr, "Hosting returned null service type registration but success.");
        isIncremented = TryAddServiceTypeRegistration(serviceTypeRegistration, hosting, queue);
    }

    auto action = ProcessFindServiceType(fsrError, isIncremented, config);

    if (action == RetryableErrorAction::ReportHealthWarning)
    {
		healthReportInformation = make_pair<Health::ReplicaHealthEvent::Enum, Health::IHealthDescriptorUPtr>(Health::ReplicaHealthEvent::ServiceTypeRegistrationWarning, make_unique<TransitionHealthReportDescriptor>(fsrError.Error));
    }
    else if (action == RetryableErrorAction::ClearHealthReport)
    {
        healthReportInformation = make_pair<Health::ReplicaHealthEvent::Enum, Health::IHealthDescriptorUPtr>(Health::ReplicaHealthEvent::ClearServiceTypeRegistrationWarning, make_unique<ClearWarningErrorHealthReportDescriptor>());
    }

    if (fsrError.IsSuccess())
    {
        return isIncremented ? ErrorCode(ErrorCodeValue::Success) : ErrorCode(ErrorCodeValue::RAServiceTypeNotRegistered);;
    }
    else if ((action == RetryableErrorAction::Drop) || !fsrError.IsRetryable)
    {
        return fsrError.Error;
    }
    else
    {
        return ErrorCode(ErrorCodeValue::RAServiceTypeNotRegistered);
    }
}
