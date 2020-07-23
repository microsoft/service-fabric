// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Hosting2/FabricNodeHost.Test.h"


using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;

const StringLiteral TraceType("ServiceTypeStateManagerTest");

class ServiceTypeStateManagerTest
{
protected:
    ServiceTypeStateManagerTest() 
        : fabricNodeHost_(make_shared<TestFabricNodeHost>())
        , applicationName_(L"MyTestApplication")
        , serviceName_(L"MyTestService")
        , serviceTypeIds_()
    { 
        BOOST_REQUIRE(Setup());

        this->SetupServiceTypeIds();
    }
    
    TEST_CLASS_SETUP(Setup);
    ~ServiceTypeStateManagerTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);

    void SetupServiceTypeIds();
    vector<Guid> GetSortedGuids(int count);
    ServiceTypeStateManagerUPtr const & GetServiceTypeStateManager();

    wstring applicationName_;
    wstring serviceName_;
    vector<ServiceTypeInstanceIdentifier> serviceTypeIds_;
	shared_ptr<TestFabricNodeHost> fabricNodeHost_;

    static const int ApplicationCount = 5;
    static const wstring ServicePackageName;
};

const wstring ServiceTypeStateManagerTest::ServicePackageName = L"SvcPkgName";

vector<Guid> ServiceTypeStateManagerTest::GetSortedGuids(int count)
{
    vector<Guid> sortedGuids;

    sortedGuids.push_back(Guid::Empty());

    for (int i = 1; i <= count; i++)
    {
        sortedGuids.push_back(Guid::NewGuid());
    }

    std::sort(
        sortedGuids.begin(),
        sortedGuids.end(),
        [](Guid item1, Guid item2)
    {
        return (StringUtility::CompareCaseInsensitive(item1.ToString(), item2.ToString()) < 0);
    });

    return sortedGuids;
}

void ServiceTypeStateManagerTest::SetupServiceTypeIds()
{
    for (int appNo = 1; appNo <= ApplicationCount; appNo++)
    {
        ApplicationIdentifier appId(wformatString("AppTypeName_{0}", appNo), appNo);

        int svcPackageCount = appNo;

        auto sortedGuids = this->GetSortedGuids(svcPackageCount);

        for (int svcPkgNum = 1; svcPkgNum <= svcPackageCount; svcPkgNum++)
        {
			ServicePackageIdentifier svcPkgId(appId, ServicePackageName);
            ServicePackageActivationContext activationCtx(sortedGuids[svcPkgNum]);

			auto svcPkgActivationId = fabricNodeHost_->GetHosting().GetOrAddServicePackagePublicActivationId(
                svcPkgId, 
                activationCtx, 
                serviceName_,
                applicationName_);

			ServicePackageInstanceIdentifier svcPkgInstanceId(svcPkgId, activationCtx, svcPkgActivationId);

            int scvTypeCount = svcPkgNum;

            for (int svcTypeNum = 1; svcTypeNum <= scvTypeCount; svcTypeNum++)
            {
                serviceTypeIds_.push_back(ServiceTypeInstanceIdentifier(svcPkgInstanceId, wformatString("SvcTypeName_{0}", svcTypeNum)));
            }
        }
    }

    auto  error = this->GetServiceTypeStateManager()->OnServicePackageActivated(serviceTypeIds_, applicationName_, Guid::NewGuid().ToString());

    VERIFY_IS_TRUE(error.IsSuccess(), L"ServiceTypeIdSortOrderTest - OnServicePackageActivated() -- ErrorCode");
}

ServiceTypeStateManagerUPtr const & ServiceTypeStateManagerTest::GetServiceTypeStateManager()
{
    return fabricNodeHost_->GetHosting().FabricRuntimeManagerObj->ServiceTypeStateManagerObj;
}

bool ServiceTypeStateManagerTest::Setup()
{
    return fabricNodeHost_->Open();
}

bool ServiceTypeStateManagerTest::Cleanup()
{
    return fabricNodeHost_->Close();
}

BOOST_FIXTURE_TEST_SUITE(ServiceTypeStateManagerTestSuite, ServiceTypeStateManagerTest)

BOOST_AUTO_TEST_CASE(ServiceTypeIdSortOrderTest)
{   
    vector<ServiceTypeInstanceIdentifier> actualEntries;
    this->GetServiceTypeStateManager()->Test_GetServiceTypeEntryInStoredOrder(actualEntries);

    // Verify the iteration order of entry map matches the oracle list.
    for (int i = 0, j = 0; i < serviceTypeIds_.size(); i++, j++)
    {
        if (actualEntries[j].ServiceTypeId.IsSystemServiceType())
        {
            i--;
            continue;
        }
        
        Trace.WriteNoise(TraceType, "ServiceTypeIdSortOrderTest - Iter=[{0}] Expected ServiceTypeId=[{1}]", i, serviceTypeIds_[i]);
        Trace.WriteNoise(TraceType, "ServiceTypeIdSortOrderTest - Iter=[{0}] Actual ServiceTypeId=[{1}]", j, actualEntries[j]);

        auto res = (serviceTypeIds_[i] == actualEntries[j]);

        if (!res)
        {
            Trace.WriteError(TraceType, "ServiceTypeIdSortOrderTest - (ExpectServiceTypeId[idx={0}] != ActualServiceTypeId[idx={1}])", i, j);
            ASSERT("ServiceTypeIdSortOrderTest - (ExpectServiceTypeId == ActualServiceTypeId)");
        }
    }
}

BOOST_AUTO_TEST_CASE(ServiceTypeQueryResultTest)
{
    for (int appNo = 1; appNo <= ApplicationCount; appNo++)
    {
        ApplicationIdentifier appId(wformatString("AppTypeName_{0}", appNo), appNo);

        int svcPackageCount = appNo;

        for (int svcTypeNum = 1; svcTypeNum <= svcPackageCount; svcTypeNum++)
        {
            auto svcTypeName = wformatString("SvcTypeName_{0}", svcTypeNum);

            // Compute expected entries
            vector<ServiceTypeInstanceIdentifier> expEntries;
            for (auto const & item : serviceTypeIds_)
            {
                if (item.ApplicationId == appId && StringUtility::AreEqualCaseInsensitive(item.ServiceTypeName, svcTypeName))
                {
                    expEntries.push_back(item);
                }
            }

            auto queryRes = this->GetServiceTypeStateManager()->GetDeployedServiceTypeQueryResult(appId, ServicePackageName, svcTypeName);

            if (expEntries.size() != queryRes.size())
            {
                Trace.WriteError(
                    TraceType, 
                    "ServiceTypeQueryResultTest - AppId=[{0}], SvcPkgName=[{1}], SvcTypeName=[{2}], ExpCount=[{3}], ActualCount=[{4}].)",
                    appId,
                    ServicePackageName,
                    svcTypeName,
                    expEntries.size(),
                    queryRes.size());

                ASSERT("expEntries.size() != queryRes.size()");
            }

            for (int i = 0; i < expEntries.size(); i++)
            {
                Trace.WriteNoise(
                    TraceType, 
                    "ServiceTypeQueryResultTest - Iter=[{0}] Expected=[ServiceTypeName={1}, ServicePackageName={2}, ServicePackageActivationId={3}]", 
                    i, 
                    expEntries[i].ServiceTypeName, 
                    expEntries[i].ServicePackageInstanceId.ServicePackageName,
                    expEntries[i].ServicePackageInstanceId.PublicActivationId);

                Trace.WriteNoise(
                    TraceType,
                    "ServiceTypeQueryResultTest - Iter=[{0}] Actual=[ServiceTypeName={1}, ServicePackageName={2}, ServicePackageActivationId={3}]",
                    i,
                    queryRes[i].ServiceTypeName,
                    queryRes[i].ServiceManifestName,
                    queryRes[i].servicePackageActivationId);

                if (!StringUtility::AreEqualCaseInsensitive(expEntries[i].ServiceTypeName, queryRes[i].ServiceTypeName) ||
                    !StringUtility::AreEqualCaseInsensitive(expEntries[i].ServicePackageInstanceId.ServicePackageName, queryRes[i].ServiceManifestName) ||
                    !(expEntries[i].ServicePackageInstanceId.PublicActivationId == queryRes[i].servicePackageActivationId))
                {
                    Trace.WriteError(TraceType, "ServiceTypeQueryResultTest - Iter=[{0}] (ExpectedQueryResult != ActualQueryResult)", i);
                    ASSERT("ServiceTypeQueryResultTest - ExpectedQueryResult != ActualQueryResult.");
                }
            }

        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
