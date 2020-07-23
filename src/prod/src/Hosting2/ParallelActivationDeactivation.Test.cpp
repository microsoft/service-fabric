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

const StringLiteral TraceType("ParallelActivationDeactivationTest");

// ********************************************************************************************************************

class ParallelActivationDeactivationTest
{
protected:
    ParallelActivationDeactivationTest()
        : fabricNodeHost_(make_shared<TestFabricNodeHost>())
        , servicePackageId_(AppId, ServicePackageName)
        , serviceTypeIds_()
        , versionedServiceTypeIds_()
        , serviceDescription_()
    {
        auto error = ApplicationIdentifier::FromString(AppId, applicationId_);
        VERIFY_IS_TRUE(error.IsSuccess());

        ServiceTypeIdentifier serviceTypeId(servicePackageId_, ServiceTypeName);
        ServiceTypeIdentifier implicitServiceTypeId(servicePackageId_, ImplicitServiceTypeName);
        
        serviceTypeIds_.push_back(serviceTypeId);
        serviceTypeIds_.push_back(implicitServiceTypeId);

        versionedServiceTypeIds_.push_back(VersionedServiceTypeIdentifier(serviceTypeId, PackageVersion));
        versionedServiceTypeIds_.push_back(VersionedServiceTypeIdentifier(implicitServiceTypeId, PackageVersion));

        serviceDescription_.ApplicationName = L"fabric:/ParallelActivationDeactivationTest";
        serviceDescription_.Name = L"fabric:/TestApp/TestService";

        BOOST_REQUIRE(Setup());
    }

    TEST_CLASS_SETUP(Setup);

    ~ParallelActivationDeactivationTest() { BOOST_REQUIRE(Cleanup()); }
    
    TEST_CLASS_CLEANUP(Cleanup);

    void RunActivationDeactivation(int activationCount);
    void RunOverlappedActivationDeactivation(int activationCount);

    static void TransitionIntoActivationDeactivation(Application2SPtr & application,  int iterCount);
    static void TransitionOutOfActivationDeactivation(Application2SPtr & application, int iterCount);

    void GetServiceTypeInstanceIds(
        vector<ServicePackageActivationContext> const activationContextList,
        vector<ServiceTypeInstanceIdentifier> & serviceTypeInstanceIds);

    void GetActivationContextList(int activationCount, vector<ServicePackageActivationContext> & activationContextList);

    void WaitForDeactivation(vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds);
    Application2SPtr GetApplicationWhenActive();

    void VerifyGuestServiceTypeHostCount(size_t count);

    HostingSubsystem & GetHosting();
    ServiceTypeStateManagerUPtr const & GetServiceTypeStateManager();
    GuestServiceTypeHostManagerUPtr const & GetTypeHostManager();

    wstring GetProgramPath(
        StoreLayoutSpecification const & storeLayout,
        wstring const & serviceManifestName,
        wstring const & codePackageName,
        ExeEntryPointDescription const & entryPoint);

    shared_ptr<TestFabricNodeHost> fabricNodeHost_;

    ApplicationIdentifier applicationId_;
    ServicePackageIdentifier servicePackageId_;
    vector<ServiceTypeIdentifier> serviceTypeIds_;
    vector<VersionedServiceTypeIdentifier> versionedServiceTypeIds_;
    ServiceDescription serviceDescription_;

    static GlobalWString AppTypeName;
    static GlobalWString AppId;
    static GlobalWString ServicePackageName;
    static GlobalWString ServiceTypeName;
    static GlobalWString ImplicitServiceTypeName;
    static GlobalWString ManifestVersion;

    static ServicePackageVersionInstance PackageVersion;
};

GlobalWString ParallelActivationDeactivationTest::AppTypeName = make_global<wstring>(L"ParallelActivationDeactivationTestApp");
GlobalWString ParallelActivationDeactivationTest::AppId = make_global<wstring>(L"ParallelActivationDeactivationTestApp_App0");
GlobalWString ParallelActivationDeactivationTest::ServiceTypeName = make_global<wstring>(L"TestServiceType");
GlobalWString ParallelActivationDeactivationTest::ImplicitServiceTypeName = make_global<wstring>(L"TestImplicitServiceType");
GlobalWString ParallelActivationDeactivationTest::ServicePackageName = make_global<wstring>(L"ParallelActivationDeactivationTestService");
GlobalWString ParallelActivationDeactivationTest::ManifestVersion = make_global<wstring>(L"1.0");

ServicePackageVersionInstance ParallelActivationDeactivationTest::PackageVersion(ServicePackageVersion(ApplicationVersion(RolloutVersion(1, 0)), RolloutVersion(1, 0)), 1);

BOOST_FIXTURE_TEST_SUITE(ParallelActivationDeactivationTestClassSuite, ParallelActivationDeactivationTest)

BOOST_AUTO_TEST_CASE(VerifyActivationDeactivationRefCounting)
{
    HostingSubsystemHolder hostingHolder(GetHosting(), GetHosting().Root.CreateComponentRoot());
    
    auto application = make_shared<Application2>(hostingHolder, applicationId_, serviceDescription_.ApplicationName);
    auto waiter = make_shared<AsyncOperationWaiter>();

    auto error = application->TransitionToActivating();
    ASSERT_IFNOT(error.IsSuccess(), "application->TransitionToActivating() failed.");

    error = application->TransitionToActive();
    ASSERT_IFNOT(error.IsSuccess(), "application->TransitionToActive() failed.");

    ParallelActivationDeactivationTest::TransitionIntoActivationDeactivation(application, 10);
    ParallelActivationDeactivationTest::TransitionOutOfActivationDeactivation(application, 10);

    ParallelActivationDeactivationTest::TransitionIntoActivationDeactivation(application, 5);

    application->Abort();

    for (int i = 4; i >= 0; i--)
    {
        error = application->TransitionToActive();
        ASSERT_IFNOT(error.IsError(ErrorCodeValue::ObjectClosed), "application->TransitionToActivating() failed.");
        
        uint64 state, refCount;
        application->GetStateAndRefCount(state, refCount);

        auto expectedState = (i == 0) ? Application2::Aborted : Application2::Modifying_PackageActivationDeactivation;

        ASSERT_IFNOT(state == expectedState, "state == expectedState");
        ASSERT_IFNOT(refCount == i, "refCount == i");
    }
}

BOOST_AUTO_TEST_CASE(ActivationDeactivation)
{
    RunActivationDeactivation(50);
}

BOOST_AUTO_TEST_CASE(OverlappedActivationDeactivation)
{
    RunOverlappedActivationDeactivation(50);
}

BOOST_AUTO_TEST_SUITE_END()

void ParallelActivationDeactivationTest::TransitionIntoActivationDeactivation(Application2SPtr & application, int iterCount)
{
    for (int i = 1; i <= iterCount; i++)
    {
        auto error = application->TransitionToModifying_PackageActivationDeactivation();
        ASSERT_IFNOT(error.IsSuccess(), "application->TransitionToModifying_PackageActivationDeactivation() failed.");
        
        uint64 state, refCount;
        application->GetStateAndRefCount(state, refCount);

        ASSERT_IFNOT(state == Application2::Modifying_PackageActivationDeactivation, "state == Application2::Modifying_PackageActivationDeactivation");
        ASSERT_IFNOT(refCount == i, "refCount == i");
    }
}

void ParallelActivationDeactivationTest::TransitionOutOfActivationDeactivation(Application2SPtr & application, int iterCount)
{
    for (int i = (iterCount - 1); i >= 0; i--)
    {
        auto error = application->TransitionToActive();

        auto expectedError = (i == 0) ? ErrorCodeValue::Success : ErrorCodeValue::OperationsPending;

        ASSERT_IFNOT(error.IsError(expectedError), "error.IsError(expectedError)");

        uint64 state, refCount;
        application->GetStateAndRefCount(state, refCount);

        auto expectedState = (i == 0) ? Application2::Active : Application2::Modifying_PackageActivationDeactivation;

        ASSERT_IFNOT(state == expectedState, "state == expectedState");
        ASSERT_IFNOT(refCount == i, "refCount == i");
    }
}

void ParallelActivationDeactivationTest::GetServiceTypeInstanceIds(
    vector<ServicePackageActivationContext> const activationContextList,
    vector<ServiceTypeInstanceIdentifier> & serviceTypeInstanceIds)
{
    for (auto activationContext : activationContextList)
    {
        wstring servicePackageActivationId;
        if (this->GetHosting().TryGetServicePackagePublicActivationId(servicePackageId_, activationContext, servicePackageActivationId))
        {
            ASSERT("this->GetHosting().TryGetServicePackagePublicActivationId() returned false.");
        }

        for (auto const & serviceTypeId : serviceTypeIds_)
        {
            serviceTypeInstanceIds.push_back(ServiceTypeInstanceIdentifier(serviceTypeId, activationContext, servicePackageActivationId));
        }
    }
}

void ParallelActivationDeactivationTest::WaitForDeactivation(vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds)
{
    auto registrationFound = true;
    auto registrationCount = 0;

    while (registrationFound)
    {
        registrationFound = false;
        registrationCount = 0;

        for (auto const & serviceTypeInstanceId : serviceTypeInstanceIds)
        {
            ServiceTypeState state;
            auto error = this->GetServiceTypeStateManager()->Test_GetStateIfPresent(serviceTypeInstanceId, state);

            if (error.IsSuccess())
            {
                registrationFound = true;
                registrationCount++;

                Sleep(1000);
            }
        }

        Trace.WriteNoise(TraceType, "RegistrationFoundCount={0}.", registrationCount);
    }

    auto appPresent = true;
    while (appPresent)
    {
        auto error = this->GetHosting().ApplicationManagerObj->Contains(applicationId_, appPresent);
        ASSERT_IFNOT(error.IsSuccess(), "this->GetHosting().ApplicationManagerObj->Contains(applicationId_, appPresent)");

        Trace.WriteNoise(TraceType, "Waiting for app to deactivate completely.");
        Sleep(1000);
    }
}

void ParallelActivationDeactivationTest::GetActivationContextList(int activationCount, vector<ServicePackageActivationContext> & activationContextList)
{
    for (int i = 1; i <= activationCount; i++)
    {
        auto guid = Guid::NewGuid();
        activationContextList.push_back(ServicePackageActivationContext(guid));
    }
}

void ParallelActivationDeactivationTest::VerifyGuestServiceTypeHostCount(size_t expectedCount)
{
    size_t typeHostCount;
    auto error = this->GetTypeHostManager()->Test_GetTypeHostCount(typeHostCount);

    ASSERT_IFNOT(error.IsSuccess(), "this->GetTypeHostManager()->Test_GetTypeHostCount(typeHostCount)");

    Trace.WriteNoise(TraceType, "typeHostCount={0}, expectedCount={1}.", typeHostCount, expectedCount);

    ASSERT_IFNOT(typeHostCount == expectedCount, "typeHostCount == expectedCount");
}

void ParallelActivationDeactivationTest::RunActivationDeactivation(int activationCount)
{
    Trace.WriteNoise(TraceType, "BEGIN TEST RunActivationDeactivation(). ActivationCount={0}.", activationCount);

    vector<ServicePackageActivationContext> activationContextList;
    this->GetActivationContextList(activationCount, activationContextList);

    auto activationWaiter = make_shared<AsyncOperationWaiter>();
    atomic_uint64 pendingOperationCount(activationContextList.size());

    vector<TimerSPtr> activationTimers;
    auto versionedServiceTypeId = versionedServiceTypeIds_[0];

    Stopwatch sw;
    sw.Start();

    for (auto activationContext : activationContextList)
    {
        auto timerSPtr = Timer::Create(
            __FUNCTION__,
            [this, activationContext, &pendingOperationCount, activationWaiter, versionedServiceTypeId](TimerSPtr const & timer)
            {
                uint64 sequenceNumber;
                ServiceTypeRegistrationSPtr registration;

                auto error = GetHosting().FindServiceTypeRegistration(
                    versionedServiceTypeId, serviceDescription_, activationContext, sequenceNumber, registration);

                if (error.IsSuccess())
                {
                    auto error1 = GetHosting().IncrementUsageCount(versionedServiceTypeId.Id, activationContext);

                    if (!error1.IsSuccess() && !error1.IsError(ErrorCodeValue::InvalidState))
                    {
                        Trace.WriteNoise(TraceType, "GetHosting().IncrementUsageCount returned: {0}.", error1);
                        ASSERT_IFNOT(error1.IsSuccess(), "GetHosting().IncrementUsageCount");
                    }

                    if (error1.IsSuccess())
                    {
                        timer->Cancel();

                        if (--pendingOperationCount == 0)
                        {
                            activationWaiter->Set();
                        }
                    }
                }
            },
            false);

        timerSPtr->Change(TimeSpan::Zero, TimeSpan::FromSeconds(1));

        activationTimers.push_back(timerSPtr);
    }

    activationWaiter->WaitOne();

    sw.Stop();

    Trace.WriteNoise(TraceType, "Time to activates {0} packages={1} ms.", activationCount, sw.ElapsedMilliseconds);

    vector<Application2SPtr> applications;
    
    auto error = this->GetHosting().ApplicationManagerObj->GetApplications(applications, serviceDescription_.ApplicationName);
    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_IS_TRUE(applications.size() == 1);
    
    uint64 state, refCount;
    applications[0]->GetStateAndRefCount(state, refCount);

    VERIFY_IS_TRUE(state == Application2::Active);
    VERIFY_IS_TRUE(refCount == 0);

    auto versionedApplication = applications[0]->GetVersionedApplication();

    uint64 vAppState, vAppRefCount;
    applications[0]->GetStateAndRefCount(vAppState, vAppRefCount);

    VERIFY_IS_TRUE(vAppState == VersionedApplication::Opened);
    VERIFY_IS_TRUE(vAppRefCount == 0);

    this->VerifyGuestServiceTypeHostCount(activationCount);

    vector<ServiceTypeInstanceIdentifier> serviceTypeInstanceIds;
    this->GetServiceTypeInstanceIds(activationContextList, serviceTypeInstanceIds);

    Sleep(2000);

    sw.Restart();

    for (auto const & activationContext : activationContextList)
    {
        GetHosting().DecrementUsageCount(versionedServiceTypeId.Id, activationContext);
    }

    this->WaitForDeactivation(serviceTypeInstanceIds);

    sw.Stop();

    this->VerifyGuestServiceTypeHostCount(0);

    Trace.WriteNoise(TraceType, "Time to deactivates {0} packages={1} ms.", activationCount, sw.ElapsedMilliseconds);

    Trace.WriteNoise(TraceType, "END TEST RunActivationDeactivation().");
}

Application2SPtr ParallelActivationDeactivationTest::GetApplicationWhenActive()
{
    vector<Application2SPtr> applications;

    while (true)
    {
        applications.clear();

        auto error = this->GetHosting().ApplicationManagerObj->GetApplications(applications, serviceDescription_.ApplicationName);
        VERIFY_IS_TRUE(error.IsSuccess());

        if (applications.size() == 1)
        {
            auto state = applications[0]->GetState();
            if (state == Application2::Inactive || state == Application2::Activating)
            {
                Trace.WriteNoise(TraceType, "Waiting for application to be in active state.");
                Sleep(10);
            }
            else
            {
                break;
            }
        }
        else
        {
            Trace.WriteNoise(TraceType, "Waiting for application to be created.");
            Sleep(10);
        }
    }

    return applications[0];
}

void ParallelActivationDeactivationTest::RunOverlappedActivationDeactivation(int activationCount)
{
    Trace.WriteNoise(TraceType, "BEGIN TEST RunOverlappedActivationDeactivation(). ActivationCount={0}.", activationCount);

    vector<ServicePackageActivationContext> activationContextList;
    this->GetActivationContextList(activationCount, activationContextList);

    vector<TimerSPtr> activationTimers;
    auto activationWaiter = make_shared<AsyncOperationWaiter>();
    atomic_uint64 pendingOperationCount(activationContextList.size());
    
    RwLock activationLock;
    set<ServicePackageActivationContext> packageWithReplica;
    auto versionedServiceTypeId = versionedServiceTypeIds_[0];

    Stopwatch sw;
    sw.Start();

    for (auto activationContext : activationContextList)
    {
        auto timerSPtr = Timer::Create(
            __FUNCTION__,
            [this, activationContext, activationCount, &packageWithReplica, &pendingOperationCount, &activationLock, activationWaiter, versionedServiceTypeId](TimerSPtr const & timer)
            {
                uint64 sequenceNumber;
                ServiceTypeRegistrationSPtr registration;

                auto error = GetHosting().FindServiceTypeRegistration(
                    versionedServiceTypeId, serviceDescription_, activationContext, sequenceNumber, registration);

                if (error.IsSuccess())
                {
                    bool shouldCancelTimer = true;

                    {
                        AcquireWriteLock writeLock(activationLock);

                        if (packageWithReplica.size() < (activationCount / 2))
                        {
                            auto error1 = GetHosting().IncrementUsageCount(versionedServiceTypeId.Id, activationContext);

                            if (!error1.IsSuccess() && !error1.IsError(ErrorCodeValue::InvalidState))
                            {
                                Trace.WriteNoise(TraceType, "GetHosting().IncrementUsageCount returned: {0}.", error1);
                                ASSERT_IFNOT(error1.IsSuccess(), "GetHosting().IncrementUsageCount");
                            }

                            if (error.IsSuccess())
                            {
                                packageWithReplica.insert(activationContext);
                            }
                            else
                            {
                                shouldCancelTimer = false;
                            }
                        }
                    }

                    if (shouldCancelTimer)
                    {
                        timer->Cancel();

                        if (--pendingOperationCount == 0)
                        {
                            activationWaiter->Set();
                        }
                    }
                }
            },
            false);

        timerSPtr->Change(TimeSpan::Zero, TimeSpan::FromSeconds(1));

        activationTimers.push_back(timerSPtr);
    }

    sw.Stop();

    Trace.WriteNoise(TraceType, "Time to activate {0} packages={1} ms.", activationCount, sw.ElapsedMilliseconds);

    auto application = this->GetApplicationWhenActive();

    uint64 state, refCount;
    application->GetStateAndRefCount(state, refCount);

    Trace.WriteNoise(TraceType, "Application: State={0}, RefCount={1}.", application->StateToString(state), refCount);

    auto isAppStateValid = 
        ((state == Application2::Active) && (refCount == 0)) ||
        ((state == Application2::Modifying_PackageActivationDeactivation) && (refCount > 0));

    VERIFY_IS_TRUE(isAppStateValid == true);

    auto versionedApplication = application->GetVersionedApplication();

    uint64 vAppState, vAppRefCount;
    versionedApplication->GetStateAndRefCount(vAppState, vAppRefCount);

    Trace.WriteNoise(TraceType, "VersionedApplication: State={0}, RefCount={1}.", versionedApplication->StateToString(vAppState), vAppRefCount);

    auto isVersionedAppStateValid =
        ((vAppState == VersionedApplication::Opened) && (vAppRefCount == 0)) ||
        ((vAppState == VersionedApplication::Modifying_PackageActivationDeactivation) && (vAppRefCount > 0));

    VERIFY_IS_TRUE(isVersionedAppStateValid == true);

    activationWaiter->WaitOne();

    vector<ServiceTypeInstanceIdentifier> serviceTypeInstanceIds;
    this->GetServiceTypeInstanceIds(activationContextList, serviceTypeInstanceIds);

    sw.Restart();

    for (auto const & activationContext : packageWithReplica)
    {
        this->GetHosting().DecrementUsageCount(versionedServiceTypeId.Id, activationContext);
    }

    this->WaitForDeactivation(serviceTypeInstanceIds);

    sw.Stop();

    this->VerifyGuestServiceTypeHostCount(0);

    Trace.WriteNoise(TraceType, "Total Packages={0}. With Replica={1}.", activationCount, packageWithReplica.size());
    Trace.WriteNoise(TraceType, "Time to deactivates {0} packages={1} ms.", activationCount, sw.ElapsedMilliseconds);

    Trace.WriteNoise(TraceType, "END TEST RunOverlappedActivationDeactivation()");
}

HostingSubsystem & ParallelActivationDeactivationTest::GetHosting()
{
    return this->fabricNodeHost_->GetHosting();
}

ServiceTypeStateManagerUPtr const & ParallelActivationDeactivationTest::GetServiceTypeStateManager()
{
    return this->GetHosting().FabricRuntimeManagerObj->ServiceTypeStateManagerObj;
}

GuestServiceTypeHostManagerUPtr const & ParallelActivationDeactivationTest::GetTypeHostManager()
{
    return this->GetHosting().GuestServiceTypeHostManagerObj;
}

bool ParallelActivationDeactivationTest::Setup()
{
    HostingConfig::GetConfig().EnableActivateNoWindow = true;
    HostingConfig::GetConfig().DeactivationScanInterval = TimeSpan::FromSeconds(120);

    wstring nttree;
    if (!Environment::GetEnvironmentVariableW(L"_NTTREE", nttree, Common::NOTHROW()))
    {
        nttree = Directory::GetCurrentDirectoryW();
    }

    if (!Path::IsPathRooted(HostingConfig::GetConfig().FabricTypeHostPath))
    {
        HostingConfig::GetConfig().FabricTypeHostPath = Path::Combine(nttree, HostingConfig::GetConfig().FabricTypeHostPath);
    }

    if (!this->fabricNodeHost_->Open()) { return false; }

    // copy WorkingFolderTestHost.exe to the code packages of the application
    Management::ImageModel::StoreLayoutSpecification storeLayout(this->fabricNodeHost_->GetImageStoreRoot());
    Management::ImageModel::RunLayoutSpecification runLayout(this->fabricNodeHost_->GetDeploymentRoot());

    auto serviceManifestFile = storeLayout.GetServiceManifestFile(AppTypeName, ServicePackageName, ManifestVersion);

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

        auto executablePath = HostingConfig::GetConfig().FabricTypeHostPath;
        error = File::Copy(executablePath, programPath, true);
        if (!error.IsSuccess())
        {
            Trace.WriteError(
                TraceType,
                "Failed to copy FabricTypeHost.exe to {0}. ErrorCode={1}",
                programPath,
                error);
            return false;
        }
    }

    return true;
}

bool ParallelActivationDeactivationTest::Cleanup()
{
    return fabricNodeHost_->Close();
}

wstring ParallelActivationDeactivationTest::GetProgramPath(
    StoreLayoutSpecification const & storeLayout,
    wstring const & serviceManifestName,
    wstring const & codePackageName,
    ExeEntryPointDescription const & entryPoint)
{
    auto programPath = storeLayout.GetCodePackageFolder(
        AppTypeName,
        serviceManifestName,
        codePackageName,
        ManifestVersion);

    programPath = Path::Combine(programPath, entryPoint.Program);
    return programPath;
}
