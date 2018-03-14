// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Hosting
        {
            class HostingAdapter
            {
                DENY_COPY(HostingAdapter);
            public:
                HostingAdapter(Hosting2::IHostingSubsystem & hosting, ReconfigurationAgent & ra);

                __declspec(property(get = get_HostingObj)) Hosting2::IHostingSubsystem & HostingObj;
                Hosting2::IHostingSubsystem & get_HostingObj() { return hosting_; }

                __declspec(property(get = get_BGMR)) Infrastructure::BackgroundWorkManagerWithRetry & BGMR;
                Infrastructure::BackgroundWorkManagerWithRetry & get_BGMR() { return bgmr_; }

                __declspec(property(get = get_EventHandler)) HostingEventHandler & EventHandler;
                HostingEventHandler & get_EventHandler() { return eventHandler_; }

                __declspec(property(get = get_ServiceTypeMapObj)) ServiceTypeMap const & ServiceTypeMapObj;
                ServiceTypeMap const & get_ServiceTypeMapObj() const { return serviceTypeMap_; }
                ServiceTypeMap & Test_GetServiceTypeMapObj() { return serviceTypeMap_; }

                FindServiceTypeRegistrationErrorCode FindServiceTypeRegistration(
                    bool isNewReplica,
                    Reliability::FailoverUnitId const & ftId,
                    Reliability::ServiceDescription const & description,
                    __out Hosting2::ServiceTypeRegistrationSPtr & registration);
    
                void AddTerminateServiceHostAction(
                    TerminateServiceHostReason::Enum reason,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration,
                    Infrastructure::StateMachineActionQueue & queue);

                void TerminateServiceHost(
                    std::wstring const & activityId,
                    TerminateServiceHostReason::Enum reason,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration);

                void Close();

                void OnServiceTypeRegistrationAdded(
                    Hosting2::ServiceTypeRegistrationSPtr const & registration,
                    Infrastructure::StateMachineActionQueue & queue);

                void OnServiceTypeRegistrationAdded(
                    std::wstring const & activityId,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration);
                
                void OnServiceTypeRegistrationRemoved(
                    Hosting2::ServiceTypeRegistrationSPtr const & registration,
                    Infrastructure::StateMachineActionQueue & queue);

                void OnServiceTypeRegistrationRemoved(
                    std::wstring const & activityId,
                    Hosting2::ServiceTypeRegistrationSPtr const & registration);

                Common::ErrorCode IncrementUsageCount(
                    ServiceModel::ServiceTypeIdentifier const & typeId,
                    ServiceModel::ServicePackageActivationContext const & activationContext,
                    Infrastructure::StateMachineActionQueue & queue);

                void DecrementUsageCount(
                    ServiceModel::ServiceTypeIdentifier const & typeId,
                    ServiceModel::ServicePackageActivationContext const & activationContext,
                    Infrastructure::StateMachineActionQueue & queue);

                void OnRetry(
                    std::wstring const & activityId);

                void ProcessServiceTypeNotificationReply(
                    Reliability::ServiceTypeNotificationReplyMessageBody const & body);

            private:
                class IncrementUsageCountStateMachineAction : public Infrastructure::StateMachineAction
                {
                    DENY_COPY(IncrementUsageCountStateMachineAction);

                public:
                    IncrementUsageCountStateMachineAction(
                        ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
                        ServiceModel::ServicePackageActivationContext const & activationContext);

                    void OnPerformAction(
                        std::wstring const & activityId,
                        Infrastructure::EntityEntryBaseSPtr const & entity,
                        ReconfigurationAgent & reconfigurationAgent);

                    void OnCancelAction(ReconfigurationAgent &);

                private:
                    ServiceModel::ServiceTypeIdentifier const serviceTypeId_;
                    ServiceModel::ServicePackageActivationContext const activationContext_;
                };

                class DecrementUsageCountStateMachineAction : public Infrastructure::StateMachineAction
                {
                    DENY_COPY(DecrementUsageCountStateMachineAction);
                public:
                    DecrementUsageCountStateMachineAction(
                        ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
                        ServiceModel::ServicePackageActivationContext const & activationContext);

                    void OnPerformAction(
                        std::wstring const & activityId,
                        Infrastructure::EntityEntryBaseSPtr const & entity,
                        ReconfigurationAgent & reconfigurationAgent);

                    void OnCancelAction(ReconfigurationAgent &);

                private:
                    ServiceModel::ServiceTypeIdentifier const serviceTypeId_;
                    ServiceModel::ServicePackageActivationContext const activationContext_;
                };

                void AddServiceTypeRegistrationChangeAction(
                    Hosting2::ServiceTypeRegistrationSPtr const& registration, 
                    Infrastructure::StateMachineActionQueue& queue, 
                    bool isAdd);

                HostingEventHandler eventHandler_;
                Hosting2::IHostingSubsystem & hosting_;
                Infrastructure::BackgroundWorkManagerWithRetry bgmr_;
                ServiceTypeMap serviceTypeMap_;
                ReconfigurationAgent & ra_;

                FindServiceTypeRegistrationErrorList existingReplicaFindServiceTypeRegistrationErrorList_;
                FindServiceTypeRegistrationErrorList newReplicaFindServiceTypeRegistrationErrorList_;                
            };
        }
    }
}


