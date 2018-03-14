// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class HostingHealthManager :
        public Common::RootedObject, 
        public Common::FabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(HostingHealthManager)

    public:
        HostingHealthManager(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);
        ~HostingHealthManager();

        void InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const & healthClient);

        /* ApplicationId related methods */
        Common::ErrorCode RegisterSource(
            ServiceModel::ApplicationIdentifier const & applicationId, 
            std::wstring applicationName, 
            std::wstring const & componentId);
        void UnregisterSource(ServiceModel::ApplicationIdentifier const & applicationId, std::wstring const & componentId);        

        void ReportHealth(
            ServiceModel::ApplicationIdentifier const & applicationId,
            std::wstring const & componentId,
            Common::SystemHealthReportCode::Enum reportCode,
            std::wstring const & healthDescription,
            FABRIC_SEQUENCE_NUMBER sequenceNumber);

        /* ServicePackageInstanceId related methods */
        Common::ErrorCode RegisterSource(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            std::wstring applicationName, 
            std::wstring const & componentId);
        void UnregisterSource(ServicePackageInstanceIdentifier const & servicePackageId, std::wstring const & componentId);

        void ReportHealth(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            std::wstring const & componentId,
            Common::SystemHealthReportCode::Enum reportCode,
            std::wstring const & healthDescription,
            FABRIC_SEQUENCE_NUMBER sequenceNumber);

        /* Node level health report with TTL */
        Common::ErrorCode RegisterSource(std::wstring const & componentId);
        void UnregisterSource(std::wstring const & componentId);

        void ReportHealth(
            std::wstring const & componentId,
            Common::SystemHealthReportCode::Enum reportCode,
            std::wstring const & healthDescription,
            FABRIC_SEQUENCE_NUMBER sequenceNumber);

        void ReportHealthWithTTL(
            std::wstring const & componentId,
            Common::SystemHealthReportCode::Enum reportCode,
            std::wstring const & healthDescription,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            Common::TimeSpan const timeToLive);

        void HostingHealthManager::ForwardHealthReportFromApplicationHostToHealthManager(
            Transport::MessageUPtr && messagePtr,
            Transport::IpcReceiverContextUPtr && ipcTransportContext);

    protected:            
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        class NodeEntityManager;

        template <class TEntity> class EntityManagerBase;
        class ApplicationEntityManager;
        class ServicePackageInstanceEntityManager;        

    private:
        void HostingHealthManager::SendToHealthManagerCompletedCallback(
            Common::AsyncOperationSPtr const & asyncOperation, 
            Transport::IpcReceiverContextUPtr && context);
        std::unique_ptr<NodeEntityManager> nodeEntityManager_;
        std::unique_ptr<ApplicationEntityManager> applicationEntityManager_;
        std::unique_ptr<ServicePackageInstanceEntityManager> servicePackageEntityManager_;
        Client::HealthReportingComponentSPtr healthClient_;
        HostingSubsystem & hosting_;
    };
}
