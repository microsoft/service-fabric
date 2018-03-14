// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class Deactivator : 
        public Common::RootedObject,
        public Common::FabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(Deactivator)

    public:
        Deactivator(Common::ComponentRoot const & root, __in ApplicationManager & appManager);
        virtual ~Deactivator();

        Common::ErrorCode EnsureApplicationEntry(ServiceModel::ApplicationIdentifier const & applicationId);
        Common::ErrorCode EnsureServicePackageInstanceEntry(ServicePackageInstanceIdentifier const & servicePackageId);

        Common::ErrorCode IncrementUsageCount(ServicePackageInstanceIdentifier const & servicePackageInstanceId);
        Common::ErrorCode DecrementUsageCount(ServicePackageInstanceIdentifier const & servicePackageInstanceId);

        Common::ErrorCode ScheduleDeactivationIfNotUsed(ServiceModel::ApplicationIdentifier const & applicationId, __out bool & isDeactivationScheduled);
        Common::ErrorCode ScheduleDeactivationIfNotUsed(ServicePackageInstanceIdentifier const & servicePackageId, __out bool & isDeactivationScheduled);

        void OnDeactivationFailed(ServicePackageInstanceIdentifier const & servicePackageId);
        void OnDeactivationSuccess(ServicePackageInstanceIdentifier const & servicePackageId);

        void OnDeactivationSuccess(ServiceModel::ApplicationIdentifier const & applicationId);

        bool IsUsed(ServicePackageInstanceIdentifier const & servicePackageId);
        bool IsUsed(ServiceModel::ApplicationIdentifier const & applicationId);

        void GetServicePackageInstanceEntries(
            ServiceModel::ServicePackageIdentifier const & servicePackageId,
            __out std::vector<ServicePackageInstanceIdentifier> & servicePackageInstances);

    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        template <class TIdentifier> class DeactivationEntry;
        template <class TIdentifier> class DeactivationQueue;

        typedef std::unique_ptr<DeactivationQueue<ServicePackageInstanceIdentifier>> ServicePackageInstanceDeactivationQueueUPtr;
        typedef std::unique_ptr<DeactivationQueue<ServiceModel::ApplicationIdentifier>> ApplicationDeactivationQueueUPtr;

        typedef Deactivator::DeactivationEntry<ServicePackageInstanceIdentifier> ServicePackageInstanceDeactivationEntry;
        typedef Deactivator::DeactivationEntry<ServiceModel::ApplicationIdentifier> ApplicationDeactivationEntry;

        typedef std::shared_ptr<ServicePackageInstanceDeactivationEntry> ServicePackageInstanceDeactivationEntrySPtr;
        typedef std::shared_ptr<ApplicationDeactivationEntry> ApplicationDeactivationEntrySPtr;

        class ServicePackageInstanceReplicaCountEntry;
        typedef std::shared_ptr<ServicePackageInstanceReplicaCountEntry> ServicePackageInstanceReplicaCountEntrySPtr;

        class ApplicationReplicaCountEntry;        
        typedef std::shared_ptr<ApplicationReplicaCountEntry> ApplicationReplicaCountEntrySPtr;        

        friend class ApplicationReplicaCountEntry;

    private:
        /******************** ServicePackage Deactivation ******************************/                
        void ScheduleServicePackageInstanceDeactivation(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            uint64 const sequenceNumber,
            Common::TimeSpan deactivationGraceInterval);

        void CancelServicePackageInstanceDeactivation(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            uint64 const sequenceNumber);

        bool OnDeactivateIfNotUsed(
            ServicePackageInstanceIdentifier const & servicePackageId, 
            uint64 const sequenceNumber);

        void OnServicePackageInstanceDeactivationCallback(
            Common::TimerSPtr const & timer, 
            ServicePackageInstanceIdentifier const & servicePackageId, 
            uint64 const sequenceNumber); 

        /******************** Application Deactivation ******************************/                
        void ScheduleApplicationDeactivation(
            ServiceModel::ApplicationIdentifier const & applicationId, 
            uint64 const applicationSequenceNumber);

        void CancelApplicationDeactivation(
            ServiceModel::ApplicationIdentifier const & applicationId, 
            uint64 const applicationSequenceNumber);

        bool OnDeactivateIfNotUsed(
            ServiceModel::ApplicationIdentifier const & applicationId, 
            uint64 const sequenceNumber);

        void OnApplicationDeactivationCallback(
            Common::TimerSPtr const & timer, 
            ServiceModel::ApplicationIdentifier const & applicationId, 
            uint64 const sequenceNumber); 

        void ScanForUnusedApplications();

        Common::ErrorCode GetApplicationEntry(
            ServiceModel::ApplicationIdentifier const & applicationId, 
            ApplicationReplicaCountEntrySPtr & applicationEntry);

    private:
        ApplicationManager & appManager_;
        ServicePackageInstanceDeactivationQueueUPtr pendingServicePackageDeactivations_;
        ApplicationDeactivationQueueUPtr pendingApplicationDeactivations_;
        Common::TimerSPtr unusedApplicationScanTimer_;

        Common::RwLock mapLock_;        
        std::map<ServiceModel::ApplicationIdentifier, ApplicationReplicaCountEntrySPtr> applicationMap_;
        Common::Random random_;
    };
}

