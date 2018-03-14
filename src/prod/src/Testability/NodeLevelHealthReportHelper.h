// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Testability
{
    class NodeHealthReportManager :
        public Common::RootedObject,
        public Common::FabricComponent,
        public Common::TextTraceComponent<Common::TraceTaskCodes::TestabilityComponent>
    {
        DENY_COPY(NodeHealthReportManager)
    public:
        NodeHealthReportManager(
            Common::ComponentRoot const & root,
            Client::HealthReportingComponentSPtr const & healthClient,
            __in Common::FabricNodeConfigSPtr nodeConfig);
        ~NodeHealthReportManager() = default;
        void InitializeHealthReportingComponent(
            Client::HealthReportingComponentSPtr const & healthClient);

        Common::ErrorCode RegisterSource(
            std::wstring const & componentId);

        void UnregisterSource(
            std::wstring const & componentId);

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

    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        void ReportHealthEvent(
            std::wstring const & propertyName,
            Common::SystemHealthReportCode::Enum reportCode,
            std::wstring const & healthDescription,
            FABRIC_SEQUENCE_NUMBER healthSequenceNumber,
            Common::TimeSpan const timeToLive);

        void DeleteHealthEvent(
            std::wstring const & propertyName,
            FABRIC_SEQUENCE_NUMBER healthSequenceNumber);

    private:
        Client::HealthReportingComponentSPtr healthClient_;
        Common::FabricNodeConfigSPtr nodeConfig_;
        std::map<std::wstring, bool> registeredComponents_;
        Common::RwLock lock_;
        bool isClosed_;
    };
    typedef std::shared_ptr<NodeHealthReportManager> NodeHealthReportManagerSPtr;
    typedef std::unique_ptr<NodeHealthReportManager> NodeHealthReportManagerUPtr;
}
