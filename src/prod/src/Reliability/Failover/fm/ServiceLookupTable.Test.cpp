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
    using namespace Federation;
    using namespace Reliability;
    using namespace Reliability::FailoverManagerComponent;

    class TestFMServiceLookupTable
    {
    protected:

        TestFMServiceLookupTable() { BOOST_REQUIRE(MethodSetup()); }
        TEST_METHOD_SETUP(MethodSetup);

        ~TestFMServiceLookupTable() { BOOST_REQUIRE(MethodCleanup()); }
        TEST_METHOD_CLEANUP(MethodCleanup);

        vector<ConsistencyUnitDescription> GetConsistencyUnitDescriptions(int count) const;
        LockedFailoverUnitPtr GetFailoverUnit(int64 lookupVersion);

        ComponentRootSPtr root_;
        FailoverManagerSPtr fm_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestFMServiceLookupTableSuite, TestFMServiceLookupTable)

    BOOST_AUTO_TEST_CASE(TestServiceLookupTable)
    {
        auto pageSizeLimit = static_cast<size_t>(
            Federation::FederationConfig::GetConfig().SendQueueSizeLimit * FailoverConfig::GetConfig().MessageContentBufferRatio);

        // Test an empty ServiceLookupTable.
        vector<ServiceTableEntry> entriesToUpdate;
        VersionRangeCollection versionRangesToUpdate;
        int64 endVersion;
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, vector<ConsistencyUnitId>(), VersionRangeCollection(0, 0), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(0), entriesToUpdate.size());
        VERIFY_IS_TRUE(versionRangesToUpdate.IsEmpty);
        VERIFY_ARE_EQUAL(1, endVersion);

        // Add 3 services having 5 FailoverUnits each.
        for (int i = 0; i < 3; i++)
        {
            wstring serviceName = L"TestService" + StringUtility::ToWString(i);
            wstring serviceType = L"ServiceType" + StringUtility::ToWString(i);
            ServiceModel::ServiceTypeIdentifier typeId(ServiceModel::ServicePackageIdentifier(L"TestApp_App0", L"TestPackage"), serviceType);

            wstring placementConstraints = L"";
            int scaleoutCount = 0;
            ServiceDescription serviceDescription = ServiceDescription(serviceName, 0, 0, 5, 3, 2, true, true, TimeSpan::FromSeconds(60.0), TimeSpan::MaxValue, TimeSpan::FromSeconds(300.0), typeId, vector<ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<ServiceLoadMetricDescription>(), 0, vector<byte>());
            vector<ConsistencyUnitDescription> consistencyUnitDescriptions = GetConsistencyUnitDescriptions(5);

            ErrorCode error = fm_->ServiceCacheObj.CreateService(move(serviceDescription), move(consistencyUnitDescriptions), false);
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, vector<ConsistencyUnitId>(), VersionRangeCollection(0, 0), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(15), entriesToUpdate.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(1, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(16, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(16, fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion);
        VERIFY_ARE_EQUAL(16, endVersion);

        // Update the ServiceLookupTable with an existing updated FailoverUnit.
        auto failoverUnit = GetFailoverUnit(1);
        fm_->FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit);
        fm_->FailoverUnitCacheObj.ServiceLookupTable.Update(*failoverUnit);

        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, vector<ConsistencyUnitId>(), VersionRangeCollection(0, 0), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(15), entriesToUpdate.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(1, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(17, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(17, fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion);
        VERIFY_ARE_EQUAL(17, endVersion);

        // Update the ServiceLookupTable with an existing FailoverUnit that failed persistence.
        fm_->FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit);
        fm_->FailoverUnitCacheObj.ServiceLookupTable.OnUpdateFailed(*failoverUnit);

        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, vector<ConsistencyUnitId>(), VersionRangeCollection(0, 0), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(15), entriesToUpdate.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(1, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(18, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(18, fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion);
        VERIFY_ARE_EQUAL(18, endVersion);

        // Update the ServiceLookupTable with an existing FailoverUnit that failed persistence previously.
        fm_->FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit);
        fm_->FailoverUnitCacheObj.ServiceLookupTable.Update(*failoverUnit);

        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, vector<ConsistencyUnitId>(), VersionRangeCollection(0, 0), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(15), entriesToUpdate.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(1, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(19, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(19, fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion);
        VERIFY_ARE_EQUAL(19, endVersion);

        // Three FailoverUnits getting updated simulteneously.
        auto failoverUnit1 = GetFailoverUnit(2);
        auto failoverUnit2 = GetFailoverUnit(3);
        auto failoverUnit3 = GetFailoverUnit(4);
        fm_->FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit1);
        fm_->FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit2);
        fm_->FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit3);
        fm_->FailoverUnitCacheObj.ServiceLookupTable.OnUpdateFailed(*failoverUnit1);
        fm_->FailoverUnitCacheObj.ServiceLookupTable.Update(*failoverUnit3);
        fm_->FailoverUnitCacheObj.ServiceLookupTable.Update(*failoverUnit2);

        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, vector<ConsistencyUnitId>(), VersionRangeCollection(0, 0), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(15), entriesToUpdate.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(1, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(22, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(22, fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion);
        VERIFY_ARE_EQUAL(22, endVersion);

        // Client has partial data
        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, vector<ConsistencyUnitId>(), VersionRangeCollection(1, 6), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(13), entriesToUpdate.size()); // version 2 & 5 will not be included
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(6, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(22, versionRangesToUpdate.VersionRanges[0].EndVersion);

        // Test too many entries
        pageSizeLimit = 1500;
        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, vector<ConsistencyUnitId>(), VersionRangeCollection(1, 6), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(12), entriesToUpdate.size()); // version 2 & 5 will not be included
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(6, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(21, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(21, endVersion);

        pageSizeLimit = 700;
        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, vector<ConsistencyUnitId>(), VersionRangeCollection(1, 6), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(5), entriesToUpdate.size()); // version 2 & 5 will not be included
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(6, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(11, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(11, endVersion);

        pageSizeLimit = 1024 * 1024;

        // Update the ServiceLookupTable with an existing FailoverUnit that failed persistence previously.
        fm_->FailoverUnitCacheObj.ServiceLookupTable.UpdateLookupVersion(*failoverUnit1);
        fm_->FailoverUnitCacheObj.ServiceLookupTable.Update(*failoverUnit1);

        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, vector<ConsistencyUnitId>(), VersionRangeCollection(0, 0), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(15), entriesToUpdate.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(1, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(23, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(23, fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion);
        VERIFY_ARE_EQUAL(23, endVersion);

        // Add another service.
        wstring serviceName = L"TestService4";
        wstring serviceType = L"ServiceType4";
        ServiceModel::ServiceTypeIdentifier typeId(ServiceModel::ServicePackageIdentifier(L"TestApp_App0", L"TestPackage"), serviceType);

        wstring placementConstraints = L"";
        int scaleoutCount = 0;
        ServiceDescription serviceDescription = ServiceDescription(serviceName, 0, 0, 5, 3, 2, true, true, TimeSpan::FromSeconds(60.0), TimeSpan::MaxValue, TimeSpan::FromSeconds(300.0), typeId, vector<ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<ServiceLoadMetricDescription>(), 0, vector<byte>());
        vector<ConsistencyUnitDescription> consistencyUnitDescriptions = GetConsistencyUnitDescriptions(5);

        ErrorCode error = fm_->ServiceCacheObj.CreateService(move(serviceDescription), move(consistencyUnitDescriptions), false);
        VERIFY_IS_TRUE(error.IsSuccess());

        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, vector<ConsistencyUnitId>(), VersionRangeCollection(0, 0), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(20), entriesToUpdate.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(1, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(28, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(28, fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion);
        VERIFY_ARE_EQUAL(28, endVersion);

        // Resolve request includes explicit ConsistencyUnitIds when incoming generation is equal to FM generation
        vector<ConsistencyUnitId> consistencyUnitIds;
        for (FailoverUnitId const& failoverUnitId : fm_->ServiceCacheObj.GetService(L"TestService4")->FailoverUnitIds)
        {
            consistencyUnitIds.push_back(ConsistencyUnitId(failoverUnitId.Guid));
        }

        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, consistencyUnitIds, VersionRangeCollection(0, 0), entriesToUpdate, versionRangesToUpdate, endVersion);
        
        VERIFY_ARE_EQUAL(static_cast<size_t>(20), entriesToUpdate.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(1, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(28, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(28, fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion);
        VERIFY_ARE_EQUAL(28, endVersion);

        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, fm_->Generation, consistencyUnitIds, VersionRangeCollection(1, 6), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(19), entriesToUpdate.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(6, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(28, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(28, fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion);
        VERIFY_ARE_EQUAL(28, endVersion);

        // Resolve request includes explicit ConsistencyUnitIds when incoming generation is less than FM generation
        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, GenerationNumber(), consistencyUnitIds, VersionRangeCollection(0, 0), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(20), entriesToUpdate.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(1, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(28, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(28, fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion);
        VERIFY_ARE_EQUAL(28, endVersion);

        entriesToUpdate.clear();
        fm_->FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(pageSizeLimit, GenerationNumber(), consistencyUnitIds, VersionRangeCollection(1, 6), entriesToUpdate, versionRangesToUpdate, endVersion);

        VERIFY_ARE_EQUAL(static_cast<size_t>(20), entriesToUpdate.size());
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), versionRangesToUpdate.VersionRanges.size());
        VERIFY_ARE_EQUAL(1, versionRangesToUpdate.VersionRanges[0].StartVersion);
        VERIFY_ARE_EQUAL(28, versionRangesToUpdate.VersionRanges[0].EndVersion);
        VERIFY_ARE_EQUAL(28, fm_->FailoverUnitCacheObj.ServiceLookupTable.EndVersion);
        VERIFY_ARE_EQUAL(28, endVersion);
    }

    BOOST_AUTO_TEST_CASE(TestBroadcast)
    {
        FMServiceLookupTable & lookupTable = fm_->FailoverUnitCacheObj.ServiceLookupTable;

        //
        // Initial case when ServiceLookupTable is empty
        //
        ServiceTableUpdateMessageBody body;
        bool result = lookupTable.TryGetServiceTableUpdateMessageBody(body);
        VERIFY_IS_FALSE(result);

        // Add 3 services having 5 FailoverUnits each.
        for (int i = 0; i < 3; i++)
        {
            wstring serviceName = L"TestService" + StringUtility::ToWString(i);
            wstring serviceType = L"ServiceType" + StringUtility::ToWString(i);
            ServiceModel::ServiceTypeIdentifier typeId(ServiceModel::ServicePackageIdentifier(L"TestApp_App0", L"TestPackage"), serviceType);

            wstring placementConstraints = L"";
            int scaleoutCount = 0;
            ServiceDescription serviceDescription = ServiceDescription(serviceName, 0, 0, 5, 3, 2, true, true, TimeSpan::FromSeconds(60.0), TimeSpan::MaxValue, TimeSpan::FromSeconds(300.0), typeId, vector<ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<ServiceLoadMetricDescription>(), 0, vector<byte>());
            vector<ConsistencyUnitDescription> consistencyUnitDescriptions = GetConsistencyUnitDescriptions(5);

            ErrorCode error = fm_->ServiceCacheObj.CreateService(move(serviceDescription), move(consistencyUnitDescriptions), false);
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        //
        // First broadcast should contain all the entries
        //
        ServiceTableUpdateMessageBody body1;
        result = lookupTable.TryGetServiceTableUpdateMessageBody(body1);

        VERIFY_IS_TRUE(result);
        VERIFY_ARE_EQUAL(body1.EndVersion, 16);
        VERIFY_ARE_EQUAL(body1.VersionRangeCollection.VersionRanges.size(), 1);
        VERIFY_ARE_EQUAL(body1.VersionRangeCollection.VersionRanges[0].StartVersion, 1);
        VERIFY_ARE_EQUAL(body1.VersionRangeCollection.VersionRanges[0].EndVersion, 16);
        VERIFY_ARE_EQUAL(body1.ServiceTableEntries.size(), 15);

        //
        // Second broadcast should also contain all the entries becaues each update is broadcasted twice (by default)
        //
        ServiceTableUpdateMessageBody body2;
        result = lookupTable.TryGetServiceTableUpdateMessageBody(body2);

        VERIFY_IS_TRUE(result);
        VERIFY_ARE_EQUAL(body2.EndVersion, 16);
        VERIFY_ARE_EQUAL(body2.VersionRangeCollection.VersionRanges.size(), 1);
        VERIFY_ARE_EQUAL(body2.VersionRangeCollection.VersionRanges[0].StartVersion, 1);
        VERIFY_ARE_EQUAL(body2.VersionRangeCollection.VersionRanges[0].EndVersion, 16);
        VERIFY_ARE_EQUAL(body2.ServiceTableEntries.size(), 15);

        //
        // Third broadcast should not contain any entry
        //
        ServiceTableUpdateMessageBody body3;
        result = lookupTable.TryGetServiceTableUpdateMessageBody(body3);

        VERIFY_IS_TRUE(result);
        VERIFY_ARE_EQUAL(body3.EndVersion, 16);
        VERIFY_IS_TRUE(body3.VersionRangeCollection.IsEmpty);
        VERIFY_ARE_EQUAL(body3.ServiceTableEntries.size(), 0);

        //
        // Broadcast after a single update should only contain one entry
        //
        LockedFailoverUnitPtr failoverUnit = this->GetFailoverUnit(1);
        lookupTable.UpdateLookupVersion(*failoverUnit);
        lookupTable.Update(*failoverUnit);
        failoverUnit.Release(false, false);

        ServiceTableUpdateMessageBody body4;
        result = lookupTable.TryGetServiceTableUpdateMessageBody(body4);

        VERIFY_IS_TRUE(result);
        VERIFY_ARE_EQUAL(body4.EndVersion, 17);
        VERIFY_ARE_EQUAL(body4.VersionRangeCollection.VersionRanges.size(), 1);
        VERIFY_ARE_EQUAL(body4.VersionRangeCollection.VersionRanges[0].StartVersion, 16);
        VERIFY_ARE_EQUAL(body4.VersionRangeCollection.VersionRanges[0].EndVersion, 17);
        VERIFY_ARE_EQUAL(body4.ServiceTableEntries.size(), 1);

        //
        // After another update, broadcast should only contain two entries
        //
        failoverUnit = GetFailoverUnit(2);
        lookupTable.UpdateLookupVersion(*failoverUnit);
        lookupTable.Update(*failoverUnit);
        failoverUnit.Release(false, false);

        ServiceTableUpdateMessageBody body5;
        result = lookupTable.TryGetServiceTableUpdateMessageBody(body5);

        VERIFY_IS_TRUE(result);
        VERIFY_ARE_EQUAL(body5.EndVersion, 18);
        VERIFY_ARE_EQUAL(body5.VersionRangeCollection.VersionRanges.size(), 1);
        VERIFY_ARE_EQUAL(body5.VersionRangeCollection.VersionRanges[0].StartVersion, 16);
        VERIFY_ARE_EQUAL(body5.VersionRangeCollection.VersionRanges[0].EndVersion, 18);
        VERIFY_ARE_EQUAL(body5.ServiceTableEntries.size(), 2);

        //
        // A failed update(FT1) should not be included in the broadcast. Since the failed
        // version is at the end, it will not be included in the broadcasted version
        // ranges.
        //
        failoverUnit = GetFailoverUnit(3);
        lookupTable.UpdateLookupVersion(*failoverUnit);
        lookupTable.OnUpdateFailed(*failoverUnit);
        failoverUnit.Release(false, false);

        ServiceTableUpdateMessageBody body6;
        result = lookupTable.TryGetServiceTableUpdateMessageBody(body6);

        VERIFY_IS_TRUE(result);
        VERIFY_ARE_EQUAL(body6.EndVersion, 19);
        VERIFY_ARE_EQUAL(body6.VersionRangeCollection.VersionRanges.size(), 1);
        VERIFY_ARE_EQUAL(body6.VersionRangeCollection.VersionRanges[0].StartVersion, 17);
        VERIFY_ARE_EQUAL(body6.VersionRangeCollection.VersionRanges[0].EndVersion, 19);
        VERIFY_ARE_EQUAL(body6.ServiceTableEntries.size(), 1);

        //
        // A successful update(FT1) after a failure(FT2) should create a hole in version ranges.
        // Since there are no entries to be broadcasted with version less than that of
        // the hole, only a single version range will be broadcased.
        //
        failoverUnit = GetFailoverUnit(4);
        lookupTable.UpdateLookupVersion(*failoverUnit);
        lookupTable.Update(*failoverUnit);
        failoverUnit.Release(false, false);

        ServiceTableUpdateMessageBody body7;
        result = lookupTable.TryGetServiceTableUpdateMessageBody(body7);

        VERIFY_IS_TRUE(result);
        VERIFY_ARE_EQUAL(body7.EndVersion, 20);
        VERIFY_ARE_EQUAL(body7.VersionRangeCollection.VersionRanges.size(), 1);
        VERIFY_ARE_EQUAL(body7.VersionRangeCollection.VersionRanges[0].StartVersion, 18);
        VERIFY_ARE_EQUAL(body7.VersionRangeCollection.VersionRanges[0].EndVersion, 20);
        VERIFY_ARE_EQUAL(body7.ServiceTableEntries.size(), 1);

        //
        // A failed update(FT1) followed by a successful update(FT2) should result in a hole in
        // the broadcasted version ranges.
        //
        failoverUnit = GetFailoverUnit(5);
        lookupTable.UpdateLookupVersion(*failoverUnit);
        lookupTable.OnUpdateFailed(*failoverUnit);
        failoverUnit.Release(false, false);

        failoverUnit = GetFailoverUnit(6);
        lookupTable.UpdateLookupVersion(*failoverUnit);
        lookupTable.Update(*failoverUnit);
        failoverUnit.Release(false, false);

        ServiceTableUpdateMessageBody body8;
        result = lookupTable.TryGetServiceTableUpdateMessageBody(body8);

        VERIFY_IS_TRUE(result);
        VERIFY_ARE_EQUAL(body8.EndVersion, 22);
        VERIFY_ARE_EQUAL(body8.VersionRangeCollection.VersionRanges.size(), 1);
        VERIFY_ARE_EQUAL(body8.VersionRangeCollection.VersionRanges[0].StartVersion, 19);
        VERIFY_ARE_EQUAL(body8.VersionRangeCollection.VersionRanges[0].EndVersion, 22);
        VERIFY_ARE_EQUAL(body8.ServiceTableEntries.size(), 2);

        //
        // A failed update(FT1) followed by an eventual successful update(FT1) should fill
        // the hole.
        //
        failoverUnit = GetFailoverUnit(7);
        lookupTable.UpdateLookupVersion(*failoverUnit);
        lookupTable.OnUpdateFailed(*failoverUnit);
        failoverUnit.Release(false, false);

        failoverUnit = GetFailoverUnit(8);
        lookupTable.UpdateLookupVersion(*failoverUnit);
        lookupTable.Update(*failoverUnit);
        failoverUnit.Release(false, false);

        failoverUnit = GetFailoverUnit(22);
        lookupTable.UpdateLookupVersion(*failoverUnit);
        lookupTable.Update(*failoverUnit);
        failoverUnit.Release(false, false);

        ServiceTableUpdateMessageBody body9;
        result = lookupTable.TryGetServiceTableUpdateMessageBody(body9);

        VERIFY_IS_TRUE(result);
        VERIFY_ARE_EQUAL(body9.EndVersion, 25);
        VERIFY_ARE_EQUAL(body9.VersionRangeCollection.VersionRanges.size(), 1);
        VERIFY_ARE_EQUAL(body9.VersionRangeCollection.VersionRanges[0].StartVersion, 20);
        VERIFY_ARE_EQUAL(body9.VersionRangeCollection.VersionRanges[0].EndVersion, 25);
        VERIFY_ARE_EQUAL(body9.ServiceTableEntries.size(), 3);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestFMServiceLookupTable::MethodSetup()
    {
        LoadBalancingComponent::PLBConfig::Test_Reset();
        auto & config_plb = Reliability::LoadBalancingComponent::PLBConfig::GetConfig();
        config_plb.IsTestMode = true;
        config_plb.DummyPLBEnabled = true;
        config_plb.PLBRefreshInterval = (TimeSpan::MaxValue); //set so that PLB won't spawn threads to balance empty data structures in functional tests
        config_plb.ProcessPendingUpdatesInterval = (TimeSpan::MaxValue);//set so that PLB won't spawn threads to balance empty data structures in functional tests

        FailoverConfig::Test_Reset();
        FailoverConfig & config = FailoverConfig::GetConfig();
        config.IsTestMode = true;
        config.DummyPLBEnabled = true;
        config.ServiceLocationBroadcastInterval = TimeSpan::MaxValue;
        config.PeriodicStateScanInterval = TimeSpan::MaxValue;

        root_ = make_shared<ComponentRoot>();
        fm_ = TestHelper::CreateFauxFM(*root_);

        GenerationNumber generationNumber = GenerationNumber(DateTime::Now().Ticks, fm_->Federation.Id);
        fm_->SetGeneration(generationNumber);

        return true;
    }

    bool TestFMServiceLookupTable::MethodCleanup()
    {
        fm_->Close(true /* isStoreCloseNeeded */);
        FailoverConfig::Test_Reset();

        return true;
    }

    LockedFailoverUnitPtr TestFMServiceLookupTable::GetFailoverUnit(int64 lookupVersion)
    {
        auto visitor = fm_->FailoverUnitCacheObj.CreateVisitor(false, TimeSpan::Zero);
        for (;;)
        {
            auto failoverUnit = visitor->MoveNext();
            if (!failoverUnit)
            {
                break;
            }

            if (failoverUnit->LookupVersion == lookupVersion)
            {
                return failoverUnit;
            }
        }

#if !defined(PLATFORM_UNIX)
        return nullptr;
#else
        return LockedFailoverUnitPtr();
#endif
    }

    vector<ConsistencyUnitDescription> TestFMServiceLookupTable::GetConsistencyUnitDescriptions(int count) const
    {
        vector<ConsistencyUnitDescription> consistencyUnitDescriptions;
        for (int i = 0; i < count; i++)
        {
            consistencyUnitDescriptions.push_back(ConsistencyUnitDescription());
        }

        return consistencyUnitDescriptions;
    }
}
