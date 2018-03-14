// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ServiceTypeRegistrationWrapper
        {
        public:
            ServiceTypeRegistrationWrapper(
                FailoverUnitDescription const & ftDesc, 
                ServiceDescription const & sdDesc);

            ServiceTypeRegistrationWrapper(
                FailoverUnitDescription const & ftDesc, 
                ServiceDescription const & sdDesc, 
                ServiceTypeRegistrationWrapper const & other);
            
            ServiceTypeRegistrationWrapper & operator=(ServiceTypeRegistrationWrapper const & other);

            __declspec(property(get = get_IsServicePackageInUse)) bool IsServicePackageInUse;
            bool get_IsServicePackageInUse() const { return servicePackageInUse_; }

            __declspec (property(get = get_ServiceTypeRegistration)) Hosting2::ServiceTypeRegistrationSPtr const& ServiceTypeRegistration;
            Hosting2::ServiceTypeRegistrationSPtr const& get_ServiceTypeRegistration() const { return serviceTypeRegistration_; }

            __declspec (property(get = get_AppHostId)) std::wstring const & AppHostId;
            std::wstring const & get_AppHostId() const;

            __declspec (property(get = get_RuntimeId)) std::wstring const & RuntimeId;
            std::wstring const & get_RuntimeId() const;

            __declspec (property(get = get_IsRuntimeActive)) bool IsRuntimeActive;
            bool get_IsRuntimeActive() const { return serviceTypeRegistration_ != nullptr && !serviceTypeRegistration_->HostId.empty() && !serviceTypeRegistration_->RuntimeId.empty(); }

            __declspec(property(get = test_GetRetryableErrorStateObj)) RetryableErrorState & RetryableErrorStateObj;
            RetryableErrorState const & test_GetRetryableErrorStateObj() const { return retryableErrorState_; }
            RetryableErrorState & test_GetRetryableErrorStateObj() { return retryableErrorState_; }

            Common::ErrorCodeValue::Enum AddServiceTypeRegistration(
                Hosting2::ServiceTypeRegistrationSPtr const & registration,
                Hosting::HostingAdapter & hosting,
                Infrastructure::StateMachineActionQueue & queue);

            void OnReplicaDown(
                Hosting::HostingAdapter & hosting,
                Infrastructure::StateMachineActionQueue & queue);

            void OnReplicaClosed(
                Hosting::HostingAdapter & hosting,
                Infrastructure::StateMachineActionQueue & queue);

            void AssertInvariants(
                FailoverUnit const & owner) const;

            void Test_SetServiceTypeRegistration(Hosting2::ServiceTypeRegistrationSPtr const & registration);

            static std::string AddField(Common::TraceEvent& traceEvent, std::string const& name);
            void FillEventData(Common::TraceEventContext& context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const&) const;

            Common::ErrorCode TryGetAndAddServiceTypeRegistration(
                Hosting2::ServiceTypeRegistrationSPtr const & registration,
                Hosting::HostingAdapter & hosting,
                Infrastructure::StateMachineActionQueue & queue,
                FailoverConfig const & config,
                __out std::pair<Health::ReplicaHealthEvent::Enum, Health::IHealthDescriptorUPtr> & healthReportInformation);

            void Start(Reliability::ReconfigurationAgentComponent::ReplicaStates::Enum replicaState);

            void ReleaseServiceTypeRegistrationAndStart(
                Hosting::HostingAdapter & hosting,
                Infrastructure::StateMachineActionQueue & queue,
                Reliability::ReconfigurationAgentComponent::ReplicaStates::Enum replicaState);

        private:
            ServiceModel::ServicePackageActivationContext CreateActivationContext() const;

            void ReleaseServiceTypeRegistration(
                Hosting::HostingAdapter & hosting,
                Infrastructure::StateMachineActionQueue & queue);
            
            // Functions related to retryableErrorState_
            void RetryableErrorStateEnterState(RetryableErrorStateName::Enum state);

            RetryableErrorAction::Enum RetryableErrorStateOnFailure(Reliability::FailoverConfig const & config);

            RetryableErrorAction::Enum RetryableErrorStateOnSuccessAndTransitionTo(
                RetryableErrorStateName::Enum state,
                Reliability::FailoverConfig const & config);

            RetryableErrorAction::Enum ProcessFindServiceType(Hosting::FindServiceTypeRegistrationErrorCode fsrError, bool isIncremented, Reliability::FailoverConfig const & config);

            bool TryAddServiceTypeRegistration(
                Hosting2::ServiceTypeRegistrationSPtr const & registration,
                Hosting::HostingAdapter & hosting,
                Infrastructure::StateMachineActionQueue & queue);

            /*
                This state is stored external to this class 
            */
            FailoverUnitDescription const & ftDesc_;
            ServiceDescription const & sdDesc_;

            bool servicePackageInUse_;
            Hosting2::ServiceTypeRegistrationSPtr serviceTypeRegistration_;

            RetryableErrorState retryableErrorState_;
        };
    }
}



