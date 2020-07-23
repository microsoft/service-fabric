// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    typedef std::map<ApplicationHostIsolationContext, ApplicationHostProxySPtr> HostProxyMap;

    class ApplicationHostActivationTable :
        public Common::RootedObject,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ApplicationHostActivationTable)

        public:
            ApplicationHostActivationTable(Common::ComponentRoot const & root, HostingSubsystem & hosting);
            virtual ~ApplicationHostActivationTable();

            Common::ErrorCode Add(ApplicationHostProxySPtr const & hostProxy);
            Common::ErrorCode Remove(std::wstring const & hostId, __out ApplicationHostProxySPtr & hostProxy);
            Common::ErrorCode Find(std::wstring const & hostId, __out ApplicationHostProxySPtr & hostProxy);
            Common::ErrorCode Find(ApplicationHostIsolationContext const & isolationContext, __out ApplicationHostProxySPtr & hostProxy); 
            Common::ErrorCode FindApplicationHostByCodePackageInstanceId(std::wstring const & codePackageInstanceId, __out ApplicationHostProxySPtr & hostProxy);
            std::vector<ApplicationHostProxySPtr> Close();

            void StartReportingToRM(std::wstring const & hostIdRM);
            void TimerCallbackRM(Common::TimerSPtr const & timer);
            void OnRequestSentToRM(AsyncOperationSPtr operation, bool expectedCompletedSynchronously, Common::TimerSPtr const & timer);
            void AddUpdateForRM(ApplicationHostProxySPtr const & appProxy, bool isUp);
            Transport::MessageUPtr CreateApplicationHostEventMessage(std::vector<Management::ResourceMonitor::ApplicationHostEvent> && updatesForRM);
            void Test_GetAllRMReports(std::vector<Management::ResourceMonitor::ApplicationHostEvent> & pending, std::vector<Management::ResourceMonitor::ApplicationHostEvent> & ongoing);

            void Test_GetHostProxyMap(HostProxyMap & proxyMap);

        private:
            typedef std::map<std::wstring, ApplicationHostProxySPtr, Common::IsLessCaseInsensitiveComparer<std::wstring>> HostIdIndexMap;
            typedef std::map<std::wstring, ApplicationHostProxySPtr, Common::IsLessCaseInsensitiveComparer<std::wstring>> CodePackageInstanceIdIndexMap;

        private:
            bool isClosed_;
            Common::RwLock lock_;
            HostProxyMap map_;
            HostIdIndexMap hostIdIndex_;
            CodePackageInstanceIdIndexMap codePackageInstanceIdIndexMap_;

            HostingSubsystem & hosting_;

            //this is used for notifying resource monitor service
            std::vector<Management::ResourceMonitor::ApplicationHostEvent> pendingRMUpdates_;
            std::vector<Management::ResourceMonitor::ApplicationHostEvent> ongoingRMUpdates_;
            Common::TimerSPtr timerRM_;
            std::wstring hostIdRM_;
    };
}
