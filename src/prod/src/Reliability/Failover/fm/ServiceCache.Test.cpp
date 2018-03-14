// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace FailoverManagerUnitTest
{
    using namespace Common;
    using namespace Client;
    using namespace std;
    using namespace Reliability;
    using namespace Reliability::FailoverManagerComponent;
    using namespace Federation;

    class TestServiceCache
    {
    protected:

        TestServiceCache() { BOOST_REQUIRE(MethodSetup()); }
        TEST_METHOD_SETUP(MethodSetup);

        ~TestServiceCache() { BOOST_REQUIRE(MethodCleanup()); }
        TEST_METHOD_CLEANUP(MethodCleanup);

        ComponentRootSPtr root_;
        FailoverManagerSPtr fm_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestServiceCacheSuite,TestServiceCache)

    BOOST_AUTO_TEST_CASE(CreateServiceTest)
    {
        FailoverManager & fm = *fm_;

        ServiceModel::ApplicationIdentifier appId;
        ServiceModel::ApplicationIdentifier::FromString(L"TestApp_App0", appId);

        wstring serviceTypeName = L"TestServiceType";
        ServiceModel::ServiceTypeIdentifier typeId(ServiceModel::ServicePackageIdentifier(appId, L"TestPackage"), serviceTypeName);

        AutoResetEvent wait(false);

        int64 maxServiceCount = 1000;
        int64 currentServiceCount = 0;

        int64 maxThreadCount = 50;
        int64 activeThreadCount = maxThreadCount;

        for (int i = 0; i < maxThreadCount; i++)
        {
            auto root = fm.CreateComponentRoot();
            Threadpool::Post([&fm, &typeId, &maxServiceCount, &currentServiceCount, &activeThreadCount, &wait, root]()
            {
                while (InterlockedIncrement64(&currentServiceCount) <= maxServiceCount)
                {
                    wstring serviceName = L"CreateServiceTest" + StringUtility::ToWString<int64>(currentServiceCount);

                    ServiceDescription serviceDescription(
                        serviceName,
                        0, // Instance
                        0, // UpdateVersion
                        1, // PartitionCount
                        3, // TargetReplicaSetSize
                        2, // MinReplicaSetSize
                        true, // IsStateful
                        true, // HasPersistedState
                        TimeSpan::FromSeconds(60.0), // ReplicaRestartWaitDuration
                        TimeSpan::MaxValue, // QuorumLossWaitDuration
                        TimeSpan::FromSeconds(300.0), // StandByReplicaKeepDuration
                        typeId, // ServiceTypeId
                        vector<ServiceCorrelationDescription>(),
                        wstring(), // PlacementConstraints
                        0, // ScaleoutCount
                        vector<ServiceLoadMetricDescription>(),
                        0, // DefaultMoveCost
                        vector<byte>());

                    vector<ConsistencyUnitDescription> consistencyUnitDescriptions;
                    consistencyUnitDescriptions.push_back(ConsistencyUnitDescription(Guid::NewGuid()));

                    ActivityId activityId(Guid::NewGuid());
                    fm.MessageEvents.CreateServiceRequest(activityId, serviceDescription.Name, serviceDescription.Instance);

                    ErrorCode error = fm.ServiceCacheObj.CreateService(move(serviceDescription), move(consistencyUnitDescriptions), false /* IsCreateFromRebuild */);

                    fm.MessageEvents.CreateServiceReply(activityId, error);
                }

                if (InterlockedDecrement64(&activeThreadCount) == 0)
                {
                    wait.Set();
                }
            });
        }

        wait.WaitOne();
    }

    BOOST_AUTO_TEST_CASE(CreateServiceAsyncTest)
    {
        FailoverManager & fm = *fm_;

        ServiceModel::ApplicationIdentifier appId;
        ServiceModel::ApplicationIdentifier::FromString(L"TestApp_App0", appId);

        wstring serviceTypeName = L"TestServiceType";
        ServiceModel::ServiceTypeIdentifier typeId(ServiceModel::ServicePackageIdentifier(appId, L"TestPackage"), serviceTypeName);

        int64 maxServiceCount = 1000;
        int64 currentServiceCount = 0;
        int64 createdServiceCount = 0;

        int64 maxThreadCount = 50;
        int64 activeThreadCount = maxThreadCount;

        AutoResetEvent threadWait(false);
        AutoResetEvent serviceWait(false);

        for (int i = 0; i < maxThreadCount; i++)
        {
            auto root = fm.CreateComponentRoot();
            Threadpool::Post([&fm, &typeId, &maxServiceCount, &currentServiceCount, &createdServiceCount, &activeThreadCount, &threadWait, &serviceWait, root]()
            {
                while (InterlockedIncrement64(&currentServiceCount) <= maxServiceCount)
                {
                    wstring serviceName = L"CreateServiceAsyncTest" + StringUtility::ToWString<int64>(currentServiceCount);

                    ServiceDescription serviceDescription(
                        serviceName,
                        0, // Instance
                        0, // UpdateVersion
                        1, // PartitionCount
                        3, // TargetReplicaSetSize
                        2, // MinReplicaSetSize
                        true, // IsStateful
                        true, // HasPersistedState
                        TimeSpan::FromSeconds(60.0), // ReplicaRestartWaitDuration
                        TimeSpan::MaxValue, // QuorumLossWaitDuration
                        TimeSpan::FromSeconds(300.0), // StandByReplicaKeepDuration
                        typeId, // ServiceTypeId
                        vector<ServiceCorrelationDescription>(),
                        wstring(), // PlacementConstraints
                        0, // ScaleoutCount
                        vector<ServiceLoadMetricDescription>(),
                        0, // DefaultMoveCost
                        vector<byte>());

                    vector<ConsistencyUnitDescription> consistencyUnitDescriptions;
                    consistencyUnitDescriptions.push_back(ConsistencyUnitDescription(Guid::NewGuid()));

                    ActivityId activityId(Guid::NewGuid());
                    fm.MessageEvents.CreateServiceRequest(activityId, serviceDescription.Name, serviceDescription.Instance);

                    fm.ServiceCacheObj.BeginCreateService(
                        move(serviceDescription),
                        move(consistencyUnitDescriptions),
                        false, /* IsCreateFromRebuild */
                        false, /* IsServiceUpdateNeeded */
                        [&fm, &activityId, &maxServiceCount, &createdServiceCount, &serviceWait, root](AsyncOperationSPtr const& operation)
                        {
                            ErrorCode error = fm.ServiceCacheObj.EndCreateService(operation);

                            fm.MessageEvents.CreateServiceReply(activityId, error);

                            if (InterlockedIncrement64(&createdServiceCount) == maxServiceCount)
                            {
                                serviceWait.Set();
                            }
                        },
                        fm.CreateAsyncOperationRoot());
                }

                if (InterlockedDecrement64(&activeThreadCount) == 0)
                {
                    threadWait.Set();
                }
            });
        }

        threadWait.WaitOne();
        serviceWait.WaitOne();
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestServiceCache::MethodSetup()
    {
        FailoverConfig::Test_Reset();
        FailoverConfig & config = FailoverConfig::GetConfig();
        config.ServiceLocationBroadcastInterval = TimeSpan::MaxValue;
        config.IsTestMode = true;
        config.DummyPLBEnabled = true;

        root_ = make_shared<ComponentRoot>();
        fm_ = TestHelper::CreateFauxFM(*root_);

        return true;
    }

    bool TestServiceCache::MethodCleanup()
    {
        FailoverConfig::Test_Reset();

        return true;
    }

}
