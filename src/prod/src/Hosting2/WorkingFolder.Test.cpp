// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Hosting2/FabricNodeHost.Test.h"
#include "Management/ImageModel/ImageModel.h"


using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Query;
using namespace Management::ImageModel;
using namespace Reliability;

const StringLiteral TraceType("WorkingFolderTest");

// ********************************************************************************************************************

class WorkingFolderTestClass
{
protected:
    WorkingFolderTestClass()
        : fabricNodeHost_(make_shared<TestFabricNodeHost>())
        , servicePackageNames_()
        , serviceDescription_()
    {
        applicationName_ = *WorkingFolderAppName;
        applicationId_ = *WorkingFolderAppId;

        servicePackageNames_.push_back(L"WorkingFolderTestService");
        servicePackageNames_.push_back(L"WorkingFolderTestService1");

        serviceDescription_.ApplicationName = std::move(wstring(applicationName_));
        serviceDescription_.Name = L"fabric:/WorkingFolderApp/WorkingFolderService";

        BOOST_REQUIRE(Setup());
    }

    TEST_CLASS_SETUP(Setup);
    ~WorkingFolderTestClass() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);

    HostingSubsystem & GetHosting();
    FabricRuntimeManagerUPtr const & GetFabricRuntimeManager();
    HostingQueryManagerUPtr & GetHostingQueryManager();
    GuestServiceTypeHostManagerUPtr const & GetTypeHostManager();
    ApplicationHostActivationTableUPtr const & GetApplicationHostActivationTable();

    void VerifyRegistrations();
    void VerifyHostIsolation();
    void VerifyAnalyzeUpgrade();
    void VerifyDeployedServicePackages();
    void VerifyDeployedServiceTypes();
    void VerifyDeployedCodePackages();
    void VerifyGuestServiceTypeHostCount(size_t count);

    void VerifyDeployedServicePackagesResult(vector<DeployedServiceManifestQueryResult> const & servicePackageResult);
    void VerifyDeployedServiceTypesResult(vector<DeployedServiceTypeQueryResult> const & serviceTypeResult, wstring const & serviceTypeName);
    void VerifyDeployedCodePackagesResult(vector<DeployedCodePackageQueryResult> const & codePackageResult, wstring const & codePackageName);

    void VerifyConsoleRedirection();
    void VerifyRegistrationsRemoved();

    void ActivateRMReporting();
    void VerifyRMReports(int numberOfUpUpdates, int numberOfDownUpdates);

    void ActivatePackage(
        VersionedServiceTypeIdentifier const & versionedServiceTypeId,
        ServicePackageActivationContext const & activationContext);

    void ReportDuplicate(wstring arg1, wstring arg2, wstring arg3);
    static wstring GetServicePackageInstanceKey(wstring const & servicePackageName, wstring const & activationId);

    wstring GetProgramPath(
        Management::ImageModel::StoreLayoutSpecification const & storeLayout,
        wstring const & serviceManifestName,
        wstring const & codePackageName,
        ExeEntryPointDescription const & entryPoint);

    shared_ptr<TestFabricNodeHost> fabricNodeHost_;

    wstring applicationName_;
    wstring applicationId_;
    vector<wstring> servicePackageNames_;
    vector<ServiceTypeDescription> typesPerPackage_;
    vector<CodePackageDescription> codePackagesPerServicePackage_;
    ApplicationUpgradeSpecification appUpgradeSpecification_;
    ServiceDescription serviceDescription_;

    ServicePackageInstanceIdentifier lastActivatedPackage_;
    vector<ServicePackageInstanceIdentifier> activatedServicePackages_;
    vector<pair<VersionedServiceTypeIdentifier, ServicePackageInstanceIdentifier>> expectedRegistrations_;
    vector<pair<VersionedServiceTypeIdentifier, ServicePackageActivationContext>> activatedServiceTypeIds_;
    set<wstring, IsLessCaseInsensitiveComparer<wstring>> expectedRuntimeIds_;

    static int PackageActivationCount;
    static GlobalWString WorkingFolderAppTypeName;
    static GlobalWString WorkingFolderAppId;
    static GlobalWString WorkingFolderAppName;
    static GlobalWString ActivatorServiceTypeName;
    static GlobalWString ManifestVersion;

    static ServicePackageVersionInstance PackageVersion;
};

int WorkingFolderTestClass::PackageActivationCount = 3;
GlobalWString WorkingFolderTestClass::WorkingFolderAppTypeName = make_global<wstring>(L"WorkingFolderTestApp");
GlobalWString WorkingFolderTestClass::WorkingFolderAppId = make_global<wstring>(L"WorkingFolderTestApp_App0");
GlobalWString WorkingFolderTestClass::WorkingFolderAppName = make_global<wstring>(L"fabric:/WorkingFolderApp");
GlobalWString WorkingFolderTestClass::ActivatorServiceTypeName = make_global<wstring>(L"ImplicitServiceType-1");
GlobalWString WorkingFolderTestClass::ManifestVersion = make_global<wstring>(L"1.0");

ServicePackageVersionInstance WorkingFolderTestClass::PackageVersion(ServicePackageVersion(ApplicationVersion(RolloutVersion(1, 0)), RolloutVersion(1, 0)), 1);

//WorkingFolderTestClass::WorkingFolderAppTypeName

// ********************************************************************************************************************
// NonActivatedApplicationHostTestClass Implementation
//

BOOST_FIXTURE_TEST_SUITE(WorkingFolderTestClassSuite,WorkingFolderTestClass)

// LINUXTODO: disable this test due to the CodePackage Name issue.
#if !defined(PLATFORM_UNIX)
BOOST_AUTO_TEST_CASE(BasicTest)
{
    // Activate the working folder test application by doing a find, do the find in a loop, untill we find registration

    //setup applicat host activation table periodic RM reporting
    ActivateRMReporting();
    for (auto const & servicePackageName : servicePackageNames_)
    {
        for (int i = 0; i <= PackageActivationCount; i++)
        {
            ServicePackageActivationContext activationContext;
            if (i != 1) { activationContext = ServicePackageActivationContext(Guid::NewGuid()); }

            ServicePackageIdentifier servicePackageId(applicationId_, servicePackageName);

            // Create a VersionedServiceTypeIdentifier to trigger package activation.
            VersionedServiceTypeIdentifier versionedServiceTypeId(ServiceTypeIdentifier(servicePackageId, *ActivatorServiceTypeName), PackageVersion);
            activatedServiceTypeIds_.push_back(make_pair(versionedServiceTypeId, activationContext));

            this->ActivatePackage(versionedServiceTypeId, activationContext);

            wstring servicePackageActivationId;
            if (this->GetHosting().TryGetServicePackagePublicActivationId(servicePackageId, activationContext, servicePackageActivationId))
            {
                ASSERT("this->GetHosting().TryGetServicePackagePublicActivationId() returned false.");
            }

            lastActivatedPackage_ = ServicePackageInstanceIdentifier(servicePackageId, activationContext, servicePackageActivationId);
            activatedServicePackages_.push_back(lastActivatedPackage_);

            this->VerifyRegistrations();
            this->VerifyGuestServiceTypeHostCount(activatedServicePackages_.size());
            this->VerifyHostIsolation();
            this->VerifyAnalyzeUpgrade();
            this->VerifyDeployedServicePackages();
            this->VerifyDeployedServiceTypes();
            this->VerifyDeployedCodePackages();
        }
    }

    this->VerifyConsoleRedirection();
    int hostCount = (int)(servicePackageNames_.size() * codePackagesPerServicePackage_.size() * (PackageActivationCount + 1));
    //we should have a report for each CP activation
    this->VerifyRMReports(hostCount * 2, hostCount);

    for (auto const & item : activatedServiceTypeIds_)
    {
        Trace.WriteNoise(TraceType, "Decrementing usage count for ServiceType {0}.", item);
        GetHosting().DecrementUsageCount(item.first.Id, item.second);
    }

    this->VerifyRegistrationsRemoved();
    this->VerifyGuestServiceTypeHostCount(0);
    //now the main entry points are down as well
    this->VerifyRMReports(hostCount * 2, hostCount * 2);
}
#endif

BOOST_AUTO_TEST_SUITE_END()

void WorkingFolderTestClass::ActivatePackage(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServicePackageActivationContext const & activationContext)
{
    bool done = false;
    int tries = 0;
    uint64 sequenceNumber;
    ServiceTypeRegistrationSPtr registration;
    bool success = false;
    while (!done)
    {
        auto error = GetHosting().FindServiceTypeRegistration(
            versionedServiceTypeId, serviceDescription_, activationContext, sequenceNumber, registration);

        if (!error.IsSuccess())
        {
            tries++;
            if (tries == 10)
            {
                Trace.WriteError(TraceType, "Failed to activate ServiceType {0}. ErrorCode={1}", versionedServiceTypeId, error);
                success = false;
                done = false;
            }
            else
            {
                Trace.WriteNoise(TraceType, "Waiting for ServiceType {0} get registered: ErrorCode={1}", versionedServiceTypeId, error);
                ::Sleep(5000);
            }
        }
        else
        {
            ::Sleep(5000); // ensure that all types are registered

            success = true;
            done = true;

            Trace.WriteNoise(TraceType, "Incrementing usage count for ServiceType {0}.", versionedServiceTypeId);

            error = GetHosting().IncrementUsageCount(versionedServiceTypeId.Id, activationContext);

            VERIFY_IS_TRUE(error.IsSuccess(), L"IncrementUsageCount");
        }
    } // Each Package activation loop.
}

void WorkingFolderTestClass::VerifyRegistrations()
{
    for (auto const & serviceType : typesPerPackage_)
    {
        ServiceTypeIdentifier serviceTypeId(lastActivatedPackage_.ServicePackageId, serviceType.ServiceTypeName);

        this->expectedRegistrations_.push_back(
            make_pair(VersionedServiceTypeIdentifier(serviceTypeId, PackageVersion), lastActivatedPackage_));
    }

    FabricRuntimeManagerUPtr const & runtimeManager = GetFabricRuntimeManager();

    int tries = 0;

    for(auto iter = this->expectedRegistrations_.begin(); iter != this->expectedRegistrations_.end(); ++iter)
    {
        ServiceTypeInstanceIdentifier serviceTypeInstanceId(
            iter->first.Id,
            iter->second.ActivationContext,
            iter->second.PublicActivationId);

        ServiceTypeState state;
        auto error = runtimeManager->ServiceTypeStateManagerObj->Test_GetStateIfPresent(serviceTypeInstanceId, state);

        VERIFY_IS_TRUE(error.IsSuccess(), L"VerifyRegistrations() -- ErrorCode");

        if (state.Status != ServiceTypeStatus::Registered_Enabled)
        {
            Trace.WriteNoise(TraceType, "VerifyRegistrations(): WAITING => ServiceTypeInstanceId=[{0}], State=[{1}]", serviceTypeInstanceId, state);

            iter--;
            tries++;

            if (tries == 10)
            {
                Trace.WriteError(TraceType, "VerifyRegistrations(): WAITING FAILED => ServiceTypeInstanceId=[{0}], State=[{1}]", serviceTypeInstanceId, state);
                ASSERT_IF(true, "VerifyRegistrations(): WAITING FAILED => ServiceTypeInstanceId=[{0}], State=[{1}]", serviceTypeInstanceId, state);
            }

            ::Sleep(5000);
        }
        else
        {
            Trace.WriteNoise(TraceType, "VerifyRegistrations(): SUCCESS => ServiceTypeInstanceId=[{0}], State=[{1}]", serviceTypeInstanceId, state);

            expectedRuntimeIds_.insert(state.RuntimeId);
        }
    }
}

void WorkingFolderTestClass::VerifyGuestServiceTypeHostCount(size_t expectedCount)
{
    size_t typeHostCount;
    auto error = this->GetTypeHostManager()->Test_GetTypeHostCount(typeHostCount);

    ASSERT_IFNOT(error.IsSuccess(), "this->GetTypeHostManager()->Test_GetTypeHostCount(typeHostCount)");

    Trace.WriteNoise(TraceType, "typeHostCount={0}, expectedCount={1}.", typeHostCount, expectedCount);

    ASSERT_IFNOT(typeHostCount == expectedCount, "typeHostCount == expectedCount");
}

void WorkingFolderTestClass::VerifyHostIsolation()
{
    // Verify that for a given service type each activation of ServicePackage has its own host and runtime.

    std::set<DWORD> hostProcessIds;
    std::set<int64> servicePackageActivationIds;
    std::set<wstring, IsLessCaseInsensitiveComparer<wstring>> hostIds;
    std::set<wstring, IsLessCaseInsensitiveComparer<wstring>> runtimeIds;
    std::set<wstring, IsLessCaseInsensitiveComparer<wstring>> codePackageNames;

    for (auto const & svcDesc : typesPerPackage_)
    {
        auto svcTypeName = svcDesc.ServiceTypeName;

        hostIds.clear();
        hostProcessIds.clear();
        runtimeIds.clear();
        servicePackageActivationIds.clear();
        codePackageNames.clear();

        for (auto const & svcPkgInstance : activatedServicePackages_)
        {
            Trace.WriteNoise(
                TraceType,
                "VerifyHostIsolation() - ServiceTypeName=[{0}], ServicePackageName=[{1}], ServicePackageActivationId=[{2}].",
                svcTypeName,
                svcPkgInstance.ServicePackageName,
                svcPkgInstance.ActivationContext);

            VersionedServiceTypeIdentifier vSvcTypeId(ServiceTypeIdentifier(svcPkgInstance.ServicePackageId, svcTypeName), PackageVersion);

            uint64 sequenceNumber;
            ServiceTypeRegistrationSPtr registration;
            auto error = GetHosting().FindServiceTypeRegistration(vSvcTypeId, serviceDescription_, svcPkgInstance.ActivationContext, sequenceNumber, registration);

            if (error.IsSuccess())
            {
                Trace.WriteNoise(TraceType, "VerifyHostIsolation() - REGISTRATION => [{0}].", registration);

                auto res1 = hostIds.insert(registration->HostId);
                if (!res1.second)
                {
                    this->ReportDuplicate(
                        L"HostId",
                        svcTypeName,
                        GetServicePackageInstanceKey(svcPkgInstance.ServicePackageName, svcPkgInstance.PublicActivationId));
                }

                auto res2 = hostProcessIds.insert(registration->HostProcessId);
                auto isImplicitServiceType = StringUtility::StartsWithCaseInsensitive(svcTypeName, wstring(L"ImplicitServiceType"));
                
                if (isImplicitServiceType)
                {
                    GuestServiceTypeHostSPtr guestTypeHost;
                    error = this->GetTypeHostManager()->Test_GetTypeHost(registration->HostId, guestTypeHost);

                    ASSERT_IFNOT(error.IsSuccess(), "Test_GetTypeHost(registration->HostId, guestTypeHost).");

                    auto runtimeId = guestTypeHost->RuntimeId;
                    
                    ASSERT_IFNOT(runtimeId == registration->RuntimeId, "runtimeId == registration->RuntimeId");
                }

                if (!res2.second && !isImplicitServiceType)
                {
                    this->ReportDuplicate(
                        L"HostProcessId",
                        svcTypeName,
                        GetServicePackageInstanceKey(svcPkgInstance.ServicePackageName, svcPkgInstance.PublicActivationId));
                }

                auto res3 = runtimeIds.insert(registration->RuntimeId);
                if (!res3.second)
                {
                    this->ReportDuplicate(
                        L"FabricRuntimeId",
                        svcTypeName,
                        GetServicePackageInstanceKey(svcPkgInstance.ServicePackageName, svcPkgInstance.PublicActivationId));
                }

                auto res4 = servicePackageActivationIds.insert(registration->ServicePackageInstanceId);
                if (!res4.second)
                {
                    this->ReportDuplicate(
                        L"ServicePackageInstanceId",
                        svcTypeName,
                        GetServicePackageInstanceKey(svcPkgInstance.ServicePackageName, svcPkgInstance.PublicActivationId));
                }

                auto res5 = codePackageNames.insert(registration->CodePackageName);
                if (res5.second && codePackageNames.size() != 1)
                {
                    Trace.WriteError(
                        TraceType,
                        "VerifyHostIsolation() - DIFFERENT CodepackageName for ServiceTypeName=[{0}], ServicePackageName=[{1}], ServicePackageActivationContext=[{2}].",
                        svcTypeName,
                        svcPkgInstance.ServicePackageName,
                        svcPkgInstance.ActivationContext);

                    VERIFY_FAIL(L"VerifyHostIsolation() - DIFFERENT CodepackageName.");
                }

            }
            else
            {
                Trace.WriteNoise(TraceType, "VerifyHostIsolation() - FAILED TO GET for REGISTRATION [{0}].", vSvcTypeId);
                VERIFY_FAIL(L"VerifyHostIsolation() - FAILED TO GET for REGISTRATION [{0}].", vSvcTypeId);
            }
        }
    }
}

void WorkingFolderTestClass::ReportDuplicate(wstring arg1, wstring arg2, wstring arg3)
{
    Trace.WriteError(
        TraceType,
        "VerifyHostIsolation() - DUPLICATE [{0}] for ServiceTypeName=[{1}], ServicePackageInstanceName=[{2}].",
        arg1,
        arg2,
        arg3);

    VERIFY_FAIL(
        L"VerifyHostIsolation() - DUPLICATE [{0}] for ServiceTypeName=[{1}], ServicePackageInstanceName=[{2}].",
        arg1,
        arg2,
        arg3);
}

wstring WorkingFolderTestClass::GetServicePackageInstanceKey(wstring const & servicePackageName, wstring const & activationIdStr)
{
    return wformatString("{0}_{1}", servicePackageName, activationIdStr);
}

void WorkingFolderTestClass::VerifyAnalyzeUpgrade()
{
    StoreLayoutSpecification storeLayout(this->fabricNodeHost_->GetImageStoreRoot());
    RunLayoutSpecification runLayout(this->fabricNodeHost_->GetDeploymentRoot());

    auto srcSvcPkgFile = storeLayout.GetServicePackageFile(*WorkingFolderAppTypeName, applicationId_, lastActivatedPackage_.ServicePackageName, L"2.0");
    auto destSvcPkgFile = runLayout.GetServicePackageFile(applicationId_, lastActivatedPackage_.ServicePackageName, L"2.0");

    auto error = File::Copy(srcSvcPkgFile, destSvcPkgFile, true);

    ASSERT_IF(!error.IsSuccess(), "VerifyAnalyzeUpgrade-File::Copy-ErrorCode");

    set<wstring, IsLessCaseInsensitiveComparer<wstring>> affectedRuntimeIds;
    error = this->fabricNodeHost_->GetHosting().ApplicationManagerObj->AnalyzeApplicationUpgrade(appUpgradeSpecification_, affectedRuntimeIds);

    ASSERT_IF(!error.IsSuccess(), "VerifyAnalyzeUpgrade-ErrorCode");

    Trace.WriteNoise(
        TraceType,
        "VerifyAnalyzeUpgrade() - ExpectedRunTimeIdCount=[{0}] ActualRuntimeIdCount=[{1}].",
        expectedRuntimeIds_.size(),
        affectedRuntimeIds.size());

    ASSERT_IF(expectedRuntimeIds_.size() != affectedRuntimeIds.size(), "expectedRuntimeIdsCount != affectedRuntimeIds.size()");

    for (auto const & runtimeId : expectedRuntimeIds_)
    {
        if (affectedRuntimeIds.find(runtimeId) == affectedRuntimeIds.end())
        {
            TRACE_LEVEL_AND_ASSERT(
                Trace.WriteError, TraceType, "VerifyAnalyzeUpgrade() - MISSING RuntimeId=[{0}].", runtimeId);
        }
    }
}

void WorkingFolderTestClass::VerifyDeployedServicePackages()
{
    vector<DeployedServiceManifestQueryResult> servicePackageResult;

    for (auto const & servicePackageName : servicePackageNames_)
    {
        QueryArgumentMap queryArgMap;
        queryArgMap.Insert(QueryResourceProperties::Application::ApplicationName, applicationName_);
        queryArgMap.Insert(QueryResourceProperties::ServiceType::ServiceManifestName, servicePackageName);

        QueryResult queryResult;
        auto error = this->GetHostingQueryManager()->GetServiceManifestListDeployedOnNode(queryArgMap, ActivityId(), queryResult);

        ASSERT_IF(!error.IsSuccess(), "VerifyDeployedServicePackages-GetServiceManifestListDeployedOnNode-ErrorCode");

        vector<DeployedServiceManifestQueryResult> svcPkgRes;
        error = queryResult.MoveList(svcPkgRes);

        ASSERT_IF(!error.IsSuccess(), "VerifyDeployedServicePackages-queryResult.MoveList-ErrorCode");

        for (auto const & res : svcPkgRes)
        {
            servicePackageResult.push_back(res);
        }
    }

    this->VerifyDeployedServicePackagesResult(servicePackageResult);

    servicePackageResult.clear();

    QueryArgumentMap queryArgMap;
    queryArgMap.Insert(QueryResourceProperties::Application::ApplicationName, applicationName_);
    queryArgMap.Insert(QueryResourceProperties::ServiceType::ServiceManifestName, L"");

    QueryResult queryResult;
    auto error = this->GetHostingQueryManager()->GetServiceManifestListDeployedOnNode(queryArgMap, ActivityId(), queryResult);

    ASSERT_IF(!error.IsSuccess(), "VerifyDeployedServicePackages-GetServiceManifestListDeployedOnNode-ErrorCode");

    error = queryResult.MoveList(servicePackageResult);

    ASSERT_IF(!error.IsSuccess(), "VerifyDeployedServicePackages-queryResult.MoveList-ErrorCode");

    this->VerifyDeployedServicePackagesResult(servicePackageResult);
}

void WorkingFolderTestClass::VerifyDeployedServicePackagesResult(vector<DeployedServiceManifestQueryResult> const & servicePackageResult)
{
    Trace.WriteNoise(
        TraceType,
        "VerifyDeployedServicePackages() - ActivatedServicePackageCount=[{0}] QueryResultCount=[{1}].",
        activatedServicePackages_.size(),
        servicePackageResult.size());

    ASSERT_IF(activatedServicePackages_.size() != servicePackageResult.size(), "ActivatedServicePackageCount != ServicePackageQueryResultCount");

    set<wstring, IsLessCaseInsensitiveComparer<wstring>> activatedSvcPkgMap;
    for (auto svcPkgRes : servicePackageResult)
    {
        if (svcPkgRes.DeployedServicePackageStatus != DeploymentStatus::Active)
        {
            TRACE_LEVEL_AND_ASSERT(
                Trace.WriteError, TraceType, "VerifyDeployedServicePackages() - DEPLOYMENT STATUS not active ServicePackageResult=[{0}].", svcPkgRes);
        }

        activatedSvcPkgMap.insert(GetServicePackageInstanceKey(svcPkgRes.ServiceManifestName, svcPkgRes.ServicePackageActivationId));
    }

    for (auto const & expectedSvcPkg : activatedServicePackages_)
    {
        auto key = GetServicePackageInstanceKey(expectedSvcPkg.ServicePackageName, expectedSvcPkg.PublicActivationId);
        if (activatedSvcPkgMap.find(key) == activatedSvcPkgMap.end())
        {
            TRACE_LEVEL_AND_ASSERT(
                Trace.WriteError, TraceType, "VerifyDeployedServicePackages() - MISSING FROM QUERY ServicePackage=[{0}].", expectedSvcPkg);
        }
    }
}

void WorkingFolderTestClass::VerifyDeployedServiceTypes()
{
    for (auto const & typeDesc : typesPerPackage_)
    {
        auto serviceTypeName = typeDesc.ServiceTypeName;

        vector<DeployedServiceTypeQueryResult> serviceTypeResult;

        for (auto const & servicePackageName : servicePackageNames_)
        {
            QueryArgumentMap queryArgMap;
            queryArgMap.Insert(QueryResourceProperties::Application::ApplicationName, applicationName_);
            queryArgMap.Insert(QueryResourceProperties::ServiceType::ServiceManifestName, servicePackageName);
            queryArgMap.Insert(QueryResourceProperties::ServiceType::ServiceTypeName, serviceTypeName);

            QueryResult queryResult;
            auto error = this->GetHostingQueryManager()->GetServiceTypeListDeployedOnNode(queryArgMap, ActivityId(), queryResult);

            ASSERT_IF(!error.IsSuccess(), "VerifyDeployedServiceTypes-GetServiceManifestListDeployedOnNode-ErrorCode");

            vector<DeployedServiceTypeQueryResult> svcTypeRes;
            error = queryResult.MoveList(svcTypeRes);

            ASSERT_IF(!error.IsSuccess(), "VerifyDeployedServiceTypes-queryResult.MoveList-ErrorCode");

            for (auto const & res : svcTypeRes)
            {
                serviceTypeResult.push_back(res);
            }
        }

        this->VerifyDeployedServiceTypesResult(serviceTypeResult, serviceTypeName);

        serviceTypeResult.clear();

        QueryArgumentMap queryArgMap;
        queryArgMap.Insert(QueryResourceProperties::Application::ApplicationName, applicationName_);
        queryArgMap.Insert(QueryResourceProperties::ServiceType::ServiceManifestName, L"");
        queryArgMap.Insert(QueryResourceProperties::ServiceType::ServiceTypeName, serviceTypeName);

        QueryResult queryResult;
        auto error = this->GetHostingQueryManager()->GetServiceTypeListDeployedOnNode(queryArgMap, ActivityId(), queryResult);

        ASSERT_IF(!error.IsSuccess(), "VerifyDeployedServiceTypes-GetServiceManifestListDeployedOnNode-ErrorCode");

        error = queryResult.MoveList(serviceTypeResult);

        ASSERT_IF(!error.IsSuccess(), "VerifyDeployedServiceTypes-queryResult.MoveList-ErrorCode");

        this->VerifyDeployedServiceTypesResult(serviceTypeResult, serviceTypeName);
    }
}

void WorkingFolderTestClass::VerifyDeployedServiceTypesResult(
    vector<DeployedServiceTypeQueryResult> const & serviceTypeResult,
    wstring const & serviceTypeName)
{
    Trace.WriteNoise(
        TraceType,
        "VerifyDeployedServiceTypes() - ActivatedServicePackageCount=[{0}] QueryResultCount=[{1}].",
        activatedServicePackages_.size(),
        serviceTypeResult.size());

    ASSERT_IF(activatedServicePackages_.size() != serviceTypeResult.size(), "ActivatedServicePackageCount != ServiceTypeQueryResultCount");

    set<wstring, IsLessCaseInsensitiveComparer<wstring>> activatedSvcPkgMap;
    for (auto svcTypeRes : serviceTypeResult)
    {
        if (svcTypeRes.ServiceTypeName != serviceTypeName)
        {
            TRACE_LEVEL_AND_ASSERT(
                Trace.WriteError, TraceType, "VerifyDeployedServiceTypes() - MISMATCH ServiceTypeName. ServiceTypeResult=[{0}].", serviceTypeResult);

        }

        auto codePkgName = svcTypeRes.CodePackageName;
        wstring implicitTypePrefix(L"ImplicitServiceType");
        wstring fabricTypeHost(L"FabricTypeHost");

        if ((!StringUtility::StartsWithCaseInsensitive(serviceTypeName, implicitTypePrefix) && !StringUtility::AreEqualCaseInsensitive(codePkgName, serviceTypeName)) ||
            (StringUtility::StartsWithCaseInsensitive(serviceTypeName, implicitTypePrefix) && !StringUtility::StartsWithCaseInsensitive(codePkgName, fabricTypeHost)))
        {
            TRACE_LEVEL_AND_ASSERT(
                Trace.WriteError, TraceType, "VerifyDeployedServiceTypes() - MISMATCH CodePackageName. ServiceTypeResult=[{0}].", serviceTypeResult);
        }

        auto key = GetServicePackageInstanceKey(svcTypeRes.ServiceManifestName, svcTypeRes.servicePackageActivationId);
        activatedSvcPkgMap.insert(key);
    }

    for (auto const & expectedSvcPkg : activatedServicePackages_)
    {
        auto key = GetServicePackageInstanceKey(expectedSvcPkg.ServicePackageName, expectedSvcPkg.PublicActivationId);
        if (activatedSvcPkgMap.find(key) == activatedSvcPkgMap.end())
        {
            TRACE_LEVEL_AND_ASSERT(
                Trace.WriteError, TraceType, "VerifyDeployedServiceTypes() - MISSING FROM QUERY ServicePackage=[{0}].", expectedSvcPkg);
        }
    }
}

void WorkingFolderTestClass::VerifyDeployedCodePackages()
{
    for (auto const & codePkg : codePackagesPerServicePackage_)
    {
        auto codePackageName = codePkg.Name;

        vector<DeployedCodePackageQueryResult> codePackageResult;

        for (auto const & servicePackageName : servicePackageNames_)
        {
            QueryArgumentMap queryArgMap;
            queryArgMap.Insert(QueryResourceProperties::Application::ApplicationName, applicationName_);
            queryArgMap.Insert(QueryResourceProperties::ServiceType::ServiceManifestName, servicePackageName);
            queryArgMap.Insert(QueryResourceProperties::CodePackage::CodePackageName, codePackageName);

            QueryResult queryResult;
            auto error = this->GetHostingQueryManager()->GetCodePackageListDeployedOnNode(queryArgMap, ActivityId(), queryResult);

            ASSERT_IF(!error.IsSuccess(), "VerifyDeployedCodePackages-GetServiceManifestListDeployedOnNode-ErrorCode");

            vector<DeployedCodePackageQueryResult> codePkgRes;
            error = queryResult.MoveList(codePkgRes);

            ASSERT_IF(!error.IsSuccess(), "VerifyDeployedCodePackages-queryResult.MoveList-ErrorCode");

            for (auto const & res : codePkgRes)
            {
                codePackageResult.push_back(res);
            }
        }

        this->VerifyDeployedCodePackagesResult(codePackageResult, codePackageName);

        codePackageResult.clear();

        QueryArgumentMap queryArgMap;
        queryArgMap.Insert(QueryResourceProperties::Application::ApplicationName, applicationName_);
        queryArgMap.Insert(QueryResourceProperties::ServiceType::ServiceManifestName, L"");
        queryArgMap.Insert(QueryResourceProperties::CodePackage::CodePackageName, codePackageName);

        QueryResult queryResult;
        auto error = this->GetHostingQueryManager()->GetCodePackageListDeployedOnNode(queryArgMap, ActivityId(), queryResult);

        ASSERT_IF(!error.IsSuccess(), "VerifyDeployedCodePackages-GetServiceManifestListDeployedOnNode-ErrorCode");

        error = queryResult.MoveList(codePackageResult);

        ASSERT_IF(!error.IsSuccess(), "VerifyDeployedCodePackages-queryResult.MoveList-ErrorCode");

        this->VerifyDeployedCodePackagesResult(codePackageResult, codePackageName);
    }
}

void WorkingFolderTestClass::VerifyDeployedCodePackagesResult(
    vector<DeployedCodePackageQueryResult> const & codePackageResult,
    wstring const & codePackageName)
{
    Trace.WriteNoise(
        TraceType,
        "VerifyDeployedCodePackages() - ActivatedServicePackageCount=[{0}] QueryResultCount=[{1}].",
        activatedServicePackages_.size(),
        codePackageResult.size());

    ASSERT_IF(activatedServicePackages_.size() != codePackageResult.size(), "ActivatedServicePackageCount != CodePackageQueryResultCount");

    set<wstring, IsLessCaseInsensitiveComparer<wstring>> activatedSvcPkgMap;
    for (auto codePkgRes : codePackageResult)
    {
        if (codePkgRes.CodePackageName != codePackageName)
        {
            TRACE_LEVEL_AND_ASSERT(
                Trace.WriteError, TraceType, "VerifyDeployedCodePackages() - MISMATCH CodePackageName. CodePackageResult=[{0}].", codePkgRes);
        }

        if (codePkgRes.HostType != HostType::ExeHost)
        {
            TRACE_LEVEL_AND_ASSERT(
                Trace.WriteError, TraceType, "VerifyDeployedCodePackages() - UNEXPECTED HostType. CodePackageResult=[{0}].", codePkgRes);
        }

        if (codePkgRes.HostIsolationMode != HostIsolationMode::None)
        {
            TRACE_LEVEL_AND_ASSERT(
                Trace.WriteError, TraceType, "VerifyDeployedCodePackages() - UNEXPECTED HostIsolationMode. CodePackageResult=[{0}].", codePkgRes);
        }

        auto key = GetServicePackageInstanceKey(codePkgRes.ServiceManifestName, codePkgRes.ServicePackageActivationId);
        activatedSvcPkgMap.insert(key);
    }

    for (auto const & expectedSvcPkg : activatedServicePackages_)
    {
        auto key = GetServicePackageInstanceKey(expectedSvcPkg.ServicePackageName, expectedSvcPkg.PublicActivationId);
        if (activatedSvcPkgMap.find(key) == activatedSvcPkgMap.end())
        {
            TRACE_LEVEL_AND_ASSERT(
                Trace.WriteError, TraceType, "VerifyDeployedCodePackages() - MISSING FROM QUERY ServicePackage=[{0}].", expectedSvcPkg);
        }
    }
}

void WorkingFolderTestClass::VerifyRegistrationsRemoved()
{
    ::Sleep(10000); // ensure that all types are unregistered

    FabricRuntimeManagerUPtr const & runtimeManager = GetFabricRuntimeManager();

    int tries = 0;
    do
    {
        // Ensure that the ServicePackage is deactivated
        ServiceTypeState state;
        auto error = runtimeManager->ServiceTypeStateManagerObj->Test_GetStateIfPresent(
            ServiceTypeInstanceIdentifier(
                expectedRegistrations_[0].first.Id,
                expectedRegistrations_[0].second.ActivationContext,
                expectedRegistrations_[0].second.PublicActivationId),
            state);

        if(error.IsError(ErrorCodeValue::NotFound))
        {
            break;
        }

        ::Sleep(5000);
    }while(++tries < 10);


    for(auto iter = this->expectedRegistrations_.begin();  iter != this->expectedRegistrations_.end(); ++iter)
    {
        ServiceTypeState state;
        ServiceTypeInstanceIdentifier serviceTypeInstanceId(
            iter->first.Id,
            iter->second.ActivationContext,
            iter->second.PublicActivationId);

        auto error = runtimeManager->ServiceTypeStateManagerObj->Test_GetStateIfPresent(serviceTypeInstanceId, state);

        Trace.WriteNoise(
            TraceType,
            "Get registration for {0} (ErrorCode={1}, State={2})",
            serviceTypeInstanceId,
            error,
            state);

        ASSERT_IF(!error.IsError(ErrorCodeValue::NotFound), "VerifyExpectedRegistration-ErrorCode");
    }
}

void WorkingFolderTestClass::ActivateRMReporting()
{
    this->GetApplicationHostActivationTable()->StartReportingToRM(L"dummy");
}

void WorkingFolderTestClass::VerifyRMReports(int numberOfUpUpdates, int numberOfDownUpdates)
{
    auto & activationTable = this->GetApplicationHostActivationTable();
    std::vector<Management::ResourceMonitor::ApplicationHostEvent> pending;
    std::vector<Management::ResourceMonitor::ApplicationHostEvent> ongoing;
    activationTable->Test_GetAllRMReports(pending, ongoing);

    auto processor = [&](std::vector<Management::ResourceMonitor::ApplicationHostEvent> & events)
    {
        for (auto event : events)
        {
            if (event.IsUp)
            {
                --numberOfUpUpdates;
            }
            else
            {
                --numberOfDownUpdates;
            }
        }
    };

    processor(pending);
    processor(ongoing);

    ASSERT_IFNOT(numberOfUpUpdates == 0, "Number of up updates does not match {0}", numberOfUpUpdates);
    ASSERT_IFNOT(numberOfDownUpdates == 0, "Number of down updates does not match {0}", numberOfDownUpdates);
}

void WorkingFolderTestClass::VerifyConsoleRedirection()
{
    Management::ImageModel::RunLayoutSpecification runLayout(this->fabricNodeHost_->GetDeploymentRoot());
    wstring logFolder = runLayout.GetApplicationLogFolder(WorkingFolderAppId);
    wstring pattern = L"Test-Default_WorkingFolderTestService_S*.out";

    vector<wstring> files = Common::Directory::Find(logFolder, pattern, 1, false);
    Trace.WriteInfo(TraceType, "Found {0} files. Expected {1}", files.size(), 1);

    // Verify that the files generated are not more than the retention count.
    ASSERT_IF((files.size() != 1), "Found different number files than expected");
}

HostingSubsystem & WorkingFolderTestClass::GetHosting()
{
    return this->fabricNodeHost_->GetHosting();
}

FabricRuntimeManagerUPtr const & WorkingFolderTestClass::GetFabricRuntimeManager()
{
    return this->GetHosting().FabricRuntimeManagerObj;
}

HostingQueryManagerUPtr & WorkingFolderTestClass::GetHostingQueryManager()
{
    return this->GetHosting().Test_GetHostingQueryManager();
}

GuestServiceTypeHostManagerUPtr const & WorkingFolderTestClass::GetTypeHostManager()
{
    return this->GetHosting().GuestServiceTypeHostManagerObj;
}

ApplicationHostActivationTableUPtr const & WorkingFolderTestClass::GetApplicationHostActivationTable()
{
    return this->GetHosting().ApplicationHostManagerObj->Test_ApplicationHostActivationTable;
}

bool WorkingFolderTestClass::Setup()
{
    HostingConfig::GetConfig().EnableActivateNoWindow = true;
    HostingConfig::GetConfig().EndpointProviderEnabled = true;

    wstring nttree;
    if (!Environment::GetEnvironmentVariableW(L"_NTTREE", nttree, Common::NOTHROW()))
    {
        nttree = Directory::GetCurrentDirectoryW();
    }

    wstring typeHostPath = HostingConfig::GetConfig().FabricTypeHostPath;
    HostingConfig::GetConfig().FabricTypeHostPath = Path::Combine(nttree, typeHostPath);

    if (!this->fabricNodeHost_->Open()) { return false; }

    // copy WorkingFolderTestHost.exe to the code packages of the application
    Management::ImageModel::StoreLayoutSpecification storeLayout(this->fabricNodeHost_->GetImageStoreRoot());
    Management::ImageModel::RunLayoutSpecification runLayout(this->fabricNodeHost_->GetDeploymentRoot());

#if !defined(PLATFORM_UNIX)
    wstring workingFolderHostExePath = Path::Combine(this->fabricNodeHost_->GetTestDataRoot(), L"..\\WorkingFolderTestHost.exe");
#else
    wstring workingFolderHostExePath = Path::Combine(this->fabricNodeHost_->GetTestDataRoot(), L"../WorkingFolderTestHost.exe");
#endif

    for (auto const & servicePackageName : servicePackageNames_)
    {
       auto serviceManifestFile = storeLayout.GetServiceManifestFile(*WorkingFolderAppTypeName, servicePackageName, *ManifestVersion);

       ServiceManifestDescription serviceManifest;
       auto error = ServiceModel::Parser::ParseServiceManifest(serviceManifestFile, serviceManifest);
       if (!error.IsSuccess())
       {
           Trace.WriteError(
               TraceType,
               "Failed to parse ServiceManifest file {0}. ErrorCode={1}",
               serviceManifestFile,
               error);
           return false;
       }

       wstring programPath;
       for (auto const & codePkg : serviceManifest.CodePackages)
       {
           programPath = GetProgramPath(storeLayout, serviceManifest.Name, codePkg.Name, codePkg.EntryPoint.ExeEntryPoint);
           auto directory = Path::GetDirectoryName(programPath);
           if (!Directory::Exists(directory))
           {
               Directory::Create(directory);
           }

           error = File::Copy(workingFolderHostExePath, programPath, true);
           if (!error.IsSuccess())
           {
               Trace.WriteError(
                   TraceType,
                   "Failed to copy WorkingFolderTestHost.exe to {0}. ErrorCode={1}",
                   programPath,
                   error);
               return false;
           }
       }

       typesPerPackage_ = serviceManifest.ServiceTypeNames;
       codePackagesPerServicePackage_ = serviceManifest.CodePackages;
    }

    // Initialize application upgrade specification
    vector<wstring> codePackageNames;
    for (auto const & codePackage : codePackagesPerServicePackage_)
    {
        codePackageNames.push_back(codePackage.Name);
    }

    std::vector<ServicePackageUpgradeSpecification> packageUpgrades;

    for(auto const & servicePackageName : servicePackageNames_)
    {
        packageUpgrades.push_back(
            ServicePackageUpgradeSpecification(
            servicePackageName,
            RolloutVersion(2, 0),
            codePackageNames));
    }

    ApplicationIdentifier appId;
    ApplicationIdentifier::FromString(applicationId_, appId);

    appUpgradeSpecification_ = ApplicationUpgradeSpecification(
        appId,
        PackageVersion.Version.ApplicationVersionValue,
        applicationName_,
        PackageVersion.InstanceId,
        UpgradeType::Rolling_ForceRestart,
        false,
        false,
        std::move(packageUpgrades),
        vector<ServiceTypeRemovalSpecification>(),
        map<ServicePackageIdentifier, ServicePackageResourceGovernanceDescription>());

    return true;
}

bool WorkingFolderTestClass::Cleanup()
{
    return fabricNodeHost_->Close();
}

 wstring WorkingFolderTestClass::GetProgramPath(
     Management::ImageModel::StoreLayoutSpecification const & storeLayout,
     wstring const & serviceManifestName,
     wstring const & codePackageName,
     ExeEntryPointDescription const & entryPoint)
 {
    wstring programPath = storeLayout.GetCodePackageFolder(
             *WorkingFolderAppTypeName,
             serviceManifestName,
             codePackageName,
             *ManifestVersion);
    programPath = Path::Combine(programPath, entryPoint.Program);
    return programPath;
}
