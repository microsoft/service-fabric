// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;

    class TestFabricClientScenarios
    {
        DENY_COPY(TestFabricClientScenarios);

    public:
        TestFabricClientScenarios(FabricTestDispatcher &, std::shared_ptr<TestFabricClient> const &);
        virtual ~TestFabricClientScenarios() {}

        __declspec (property(get=get_TestFabricClient)) std::shared_ptr<TestFabricClient> const & TestFabricClientSPtr;
        inline std::shared_ptr<TestFabricClient> const & get_TestFabricClient() const { return testClientSPtr_; }

        bool RunScenario(Common::StringCollection const & params);

        // scenario functions
        
        static bool ParallelCreateDeleteEnumerateNames(
            FabricTestDispatcher &,
            std::shared_ptr<TestFabricClient> const &, 
            Common::StringCollection const & params);

        static bool ClusterManagerLargeKeys(
            FabricTestDispatcher &,
            std::shared_ptr<TestFabricClient> const &, 
            Common::StringCollection const & params);

        static bool ResolveServicePerformance(
            FabricTestDispatcher &,
            std::shared_ptr<TestFabricClient> const &, 
            Common::StringCollection const & params);

        static bool PushNotificationsPerformance(
            FabricTestDispatcher &,
            std::shared_ptr<TestFabricClient> const &, 
            Common::StringCollection const & params);

        static bool PushNotificationsLeak(
            FabricTestDispatcher &,
            std::shared_ptr<TestFabricClient> const &, 
            Common::StringCollection const & params);

        static bool DeleteServiceLeak(
            FabricTestDispatcher &,
            std::shared_ptr<TestFabricClient> const &, 
            Common::StringCollection const & params);

        static bool ImageBuilderApplicationScale(
            FabricTestDispatcher &,
            std::shared_ptr<TestFabricClient> const &, 
            Common::StringCollection const & params);

        static bool ApplicationThroughput(
            FabricTestDispatcher &,
            std::shared_ptr<TestFabricClient> const &, 
            Common::StringCollection const & params);

    private:
        class MockServiceNotificationEventHandler;

        typedef std::function<bool(FabricTestDispatcher &, std::shared_ptr<TestFabricClient> const&, Common::StringCollection const&)> ScenarioCallback;

        void AddScenario(std::wstring const &, ScenarioCallback const &);

        static void UploadHelper(
            std::wstring const & buildPath,
            std::wstring const & appTypeName,
            std::wstring const & appTypeVersion,
            std::wstring const & serviceTypeName,
            std::wstring const & defaultServiceName,
            FabricTestDispatcher &);

        static void ProcessResolveResult(
            Common::NamingUri const & serviceName,
            Common::ErrorCode const & error,
            Api::IResolvedServicePartitionResultPtr const & result,
            Common::atomic_long & pendingCount,
            Common::atomic_long & failuresCount,
            Common::ManualResetEvent & completedEvent,
            bool printDetails);

        static void ProcessNotification(
            int expectedNotificationCount,
            Common::atomic_long & notificationSuccessCount,
            Common::atomic_long & notificationErrorCount,
            LONGLONG handlerId,
            Api::IResolvedServicePartitionResultPtr const & result,
            Common::ErrorCode const & error,
            Common::ManualResetEvent &,
            bool printDetails);

        static Reliability::ServiceDescription CreateServiceDescription(
            std::wstring const & name,
            int partitionCount);

        FabricTestDispatcher & dispatcher_;
        std::shared_ptr<TestFabricClient> testClientSPtr_;
        std::map<std::wstring, ScenarioCallback> scenarioMap_;
    };
}
