// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "FabricRuntime.h"
#include "Common/Common.h"
#include "FabricNode/fabricnode.h"
#include "Hosting2/FabricNodeHost.Test.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Fabric;
using namespace Reliability;

const StringLiteral TraceType("GuestServiceTypeHostManagerTest");

// ********************************************************************************************************************

class GuestServiceTypeHostManagerTestClass
{
protected:
    GuestServiceTypeHostManagerTestClass()
        : fabricNodeHost_(make_shared<TestFabricNodeHost>())
    {
        BOOST_REQUIRE(Setup());
    }

    ~GuestServiceTypeHostManagerTestClass() { BOOST_REQUIRE(Cleanup()); }
    
    bool Setup();
    bool Cleanup();
    
    HostingSubsystem & GetHosting();
    ApplicationHostRegistrationTableUPtr const & GetApplicationHostRegistrationTable();
    RuntimeRegistrationTableUPtr const & GetRuntimeRegistrationTable();
    FabricRuntimeManagerUPtr const & GetFabricRuntimeManager();
    GuestServiceTypeHostManagerUPtr const & GetTypeHostManager();

    void VerifyApplicationHostRegistration(wstring const & hostId, bool expectedExist, bool withRetry);
    void VerifyRuntimeRegistration(wstring const & runtimeId, bool expectedExist, bool withRetry);
    void VerifyServiceTypeRegistration(
        VersionedServiceTypeIdentifier const & versionedServiceTypeId, 
        ServiceDescription const & serviceDescription,
        ServicePackageActivationContext const & activationContext,
        bool expectedExist,
        bool withRetry);

private:
	shared_ptr<TestFabricNodeHost> const fabricNodeHost_;
};

// ********************************************************************************************************************
// GuestServiceTypeHostManagerTestClass Implementation
//

bool GuestServiceTypeHostManagerTestClass::Setup()
{
	auto res = fabricNodeHost_->Open();
    return res;
}

bool GuestServiceTypeHostManagerTestClass::Cleanup()
{
    auto res = fabricNodeHost_->Close();
    return res;
}

HostingSubsystem & GuestServiceTypeHostManagerTestClass::GetHosting()
{
    return this->fabricNodeHost_->GetHosting();
}

ApplicationHostRegistrationTableUPtr const & GuestServiceTypeHostManagerTestClass::GetApplicationHostRegistrationTable()
{
    return fabricNodeHost_->GetHosting().ApplicationHostManagerObj->Test_ApplicationHostRegistrationTable;
}

RuntimeRegistrationTableUPtr const & GuestServiceTypeHostManagerTestClass::GetRuntimeRegistrationTable()
{
    return fabricNodeHost_->GetHosting().FabricRuntimeManagerObj->Test_RuntimeRegistrationTable;
}

FabricRuntimeManagerUPtr const & GuestServiceTypeHostManagerTestClass::GetFabricRuntimeManager()
{
    return fabricNodeHost_->GetHosting().FabricRuntimeManagerObj;
}

GuestServiceTypeHostManagerUPtr const & GuestServiceTypeHostManagerTestClass::GetTypeHostManager()
{
    return fabricNodeHost_->GetHosting().GuestServiceTypeHostManagerObj;
}

void GuestServiceTypeHostManagerTestClass::VerifyApplicationHostRegistration(wstring const & hostId, bool expectedExist, bool withRetry)
{
    auto maxRetryCount = 5;
    auto retryDelayInMilliSec = 1000;

    for (auto retry = 1; retry <= maxRetryCount; retry++)
    {
        ApplicationHostRegistrationSPtr registration;

        ApplicationHostRegistrationTableUPtr const & table = GetApplicationHostRegistrationTable();
        auto error = table->Find(hostId, registration);

        auto exists = error.IsSuccess();

        if (exists == expectedExist)
        {
            return;
        }

        auto mssg = wformatString(
            "VerifyApplicationHostRegistration: HostId={0}, ErrorCode={1}, exists={2}, expectedExist={3}. RetryCount={4}.",
            hostId,
            error,
            exists,
            expectedExist,
            retry);

        Trace.WriteNoise(TraceType, "{0}", mssg);

        if (withRetry && retry < maxRetryCount)
        {
            Sleep(retryDelayInMilliSec);
            continue;
        }
        else
        {
            ASSERT_IF(true, "{0}", mssg)
        }
    }
}

void GuestServiceTypeHostManagerTestClass::VerifyRuntimeRegistration(wstring const & runtimeId, bool expectedExist, bool withRetry)
{
    auto maxRetryCount = 5;
    auto retryDelayInMilliSec = 1000;

    for (auto retry = 1; retry < maxRetryCount; retry++)
    {
        RuntimeRegistrationSPtr registration;

        RuntimeRegistrationTableUPtr const & table = GetRuntimeRegistrationTable();
        auto error = table->Find(runtimeId, registration);

        auto exists = error.IsSuccess();

        if (exists == expectedExist)
        {
            return;
        }

        auto mssg = wformatString(
            "VerifyRuntimeRegistration: runtimeId={0}, ErrorCode={1}, exists={2}, expectedExist={3}.",
            runtimeId,
            error,
            exists,
            expectedExist);

        Trace.WriteNoise(TraceType, "{0}", mssg);

        if (withRetry && retry < maxRetryCount)
        {
            Sleep(retryDelayInMilliSec);
            continue;
        }
        else
        {
            ASSERT_IF(true, "{0}", mssg)
        }
    }
}

void GuestServiceTypeHostManagerTestClass::VerifyServiceTypeRegistration(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServiceDescription const & serviceDescription,
    ServicePackageActivationContext const & activationContext,
    bool expectedExist, 
    bool withRetry)
{
    auto maxRetryCount = 5;
    auto retryDelayInMilliSec = 1000;

    for (auto retry = 1; retry < maxRetryCount; retry++)
    {
        auto const & runtimeManager = GetFabricRuntimeManager();
        ServiceTypeRegistrationSPtr registration;
        uint64 sequenceNumber;

        auto error = runtimeManager->FindServiceTypeRegistration(versionedServiceTypeId, serviceDescription, activationContext, sequenceNumber, registration);

        auto exists = error.IsSuccess();

        if (exists == expectedExist)
        {
            return;
        }

        auto mssg = wformatString(
            "VerifyServiceTypeRegistration: VersionedServiceTypeIdentifer={0}, ErrorCode={1}, exists={2}, expectedExist={3}.",
            versionedServiceTypeId,
            error,
            exists,
            expectedExist);

        Trace.WriteNoise(TraceType, "{0}", mssg);

        if (withRetry && retry < maxRetryCount)
        {
            Sleep(retryDelayInMilliSec);
            continue;
        }
        else
        {
            ASSERT_IF(true, "{0}", mssg)
        }
    }
}

// ********************************************************************************************************************
// Test Cases
//

BOOST_FIXTURE_TEST_SUITE(GuestServiceTypeHostManagerTestClassSuite,GuestServiceTypeHostManagerTestClass)

BOOST_AUTO_TEST_CASE(BasicTest)
{
    ServiceDescription serviceDescription;
    serviceDescription.ApplicationName = L"GuestApp";
    serviceDescription.Name = L"GuestApp/GuestService";

    wstring applicationId(L"");
    wstring servicePackageName(L"GuestServicePackage");
    wstring serviceTypeName(L"GuestServiceType");

    ServicePackageIdentifier servicePackageId(applicationId, servicePackageName);

    RolloutVersion rolloutVersion(1, 0);
    ApplicationVersion appVersion(rolloutVersion);
    ServicePackageVersionInstance versionInstance(ServicePackageVersion(appVersion, rolloutVersion), 1);

    vector<GuestServiceTypeInfo> guestServiceTypes;
    guestServiceTypes.push_back(move(GuestServiceTypeInfo(L"GuestStatelessServiceTypeA", false)));
    guestServiceTypes.push_back(move(GuestServiceTypeInfo(L"GuestStatefulServiceTypeA", true)));
    guestServiceTypes.push_back(move(GuestServiceTypeInfo(L"GuestStatelessServiceTypeB", false)));
    guestServiceTypes.push_back(move(GuestServiceTypeInfo(L"GuestStatefulServiceTypeB", true)));

    HostingSubsystemHolder hostingHolder(this->GetHosting(), this->GetHosting().Root.CreateComponentRoot());

    int guestServiceTypeHostCount = 50;

    vector<wstring> hostIds;
    vector<wstring> runtimeIds;
    vector<ServicePackageActivationContext> activationContexts;

    for (int i = 0; i < guestServiceTypeHostCount; i++)
    {
        auto hostId = Guid::NewGuid().ToString();
        ServicePackageActivationContext activationContext(Guid::NewGuid());

        hostIds.push_back(hostId);
        activationContexts.push_back(activationContext);

        ServicePackageInstanceIdentifier servicePackageInstanceId(servicePackageId, activationContext, Guid::NewGuid().ToString());
        CodePackageInstanceIdentifier codePackageInstanceId(servicePackageInstanceId, Hosting2::Constants::ImplicitTypeHostCodePackageName);
        CodePackageContext codePackageContext(codePackageInstanceId, 100, 200, versionInstance, serviceDescription.ApplicationName);

        auto openWaiter = make_shared<AsyncOperationWaiter>();

        this->GetTypeHostManager()->BeginOpenGuestServiceTypeHost(
            ApplicationHostContext(hostId, ApplicationHostType::Activated_InProcess, false, false),
            guestServiceTypes,
            codePackageContext,
            move(vector<wstring>()),
            move(vector<EndpointDescription>()),
            Hosting2::HostingConfig::GetConfig().ActivationTimeout,
            [this, openWaiter](AsyncOperationSPtr const & operation)
            {
                openWaiter->SetError(this->GetTypeHostManager()->EndOpenGuestServiceTypeHost(operation));
                openWaiter->Set();
            },
            AsyncOperationSPtr());

        openWaiter->WaitOne();
        auto error = openWaiter->GetError();

        Trace.WriteNoise(TraceType, "OpenGuestServiceTypeHost: ErrorCode={0}", error);

        ASSERT_IFNOT(error.IsSuccess(), "OpenGuestServiceTypeHost");

        GuestServiceTypeHostSPtr guestTypeHost;
        error = this->GetTypeHostManager()->Test_GetTypeHost(hostId, guestTypeHost);
        
        ASSERT_IFNOT(error.IsSuccess(), "Test_GetTypeHost(hostId, guestTypeHost) after Open.");

        auto runtimeId = guestTypeHost->RuntimeId;

        runtimeIds.push_back(runtimeId);

        this->VerifyApplicationHostRegistration(hostId, true, false);
        this->VerifyRuntimeRegistration(runtimeId, true, false);

        for (auto const & guestServiceType : guestServiceTypes)
        {
            ServiceTypeIdentifier serviceTypeId(servicePackageId, guestServiceType.ServiceTypeName);
            VersionedServiceTypeIdentifier versionedServiceTypeId(serviceTypeId, versionInstance);

            this->VerifyServiceTypeRegistration(versionedServiceTypeId, serviceDescription, activationContext, true, false);
        }
    }

    size_t typeHostCountAfterOpen;
    auto error = this->GetTypeHostManager()->Test_GetTypeHostCount(typeHostCountAfterOpen);

    ASSERT_IFNOT(error.IsSuccess(), "this->GetTypeHostManager()->Test_GetTypeHostCount(typeHostCountAfterOpen)");

    Trace.WriteNoise(TraceType, "typeHostCountAfterOpen={0}", typeHostCountAfterOpen);

    ASSERT_IFNOT(typeHostCountAfterOpen == guestServiceTypeHostCount, "typeHostCountAfterOpen == guestServiceTypeHostCount");

    for (int i = 0; i < guestServiceTypeHostCount; i++)
    {
        auto hostId = hostIds.at(i);
        ApplicationHostContext appHostContext(hostId, ApplicationHostType::Activated_InProcess, false, false);

        auto runtimeId = runtimeIds.at(i);
        auto activationContext = activationContexts.at(i);

        auto shouldClose = ((i + 1) % 2 == 0);

        if (shouldClose)
        {
            auto closeWaiter = make_shared<AsyncOperationWaiter>();

            this->GetTypeHostManager()->BeginCloseGuestServiceTypeHost(
                appHostContext,
                Hosting2::HostingConfig::GetConfig().ActivationTimeout,
                [this, closeWaiter](AsyncOperationSPtr const & operation)
            {
                closeWaiter->SetError(this->GetTypeHostManager()->EndCloseGuestServiceTypeHost(operation));
                closeWaiter->Set();
            },
                AsyncOperationSPtr());

            closeWaiter->WaitOne();
            error = closeWaiter->GetError();

            Trace.WriteNoise(TraceType, "CloseGuestServiceTypeHost: ErrorCode={0}", error);

            ASSERT_IFNOT(error.IsSuccess(), "CloseGuestServiceTypeHost");
        }
        else
        {
            this->GetTypeHostManager()->AbortGuestServiceTypeHost(appHostContext);
        }        
        
        GuestServiceTypeHostSPtr guestTypeHostToIgnore;
        error = this->GetTypeHostManager()->Test_GetTypeHost(hostId, guestTypeHostToIgnore);

        ASSERT_IFNOT(error.IsError(ErrorCodeValue::NotFound), "Test_GetTypeHost(hostId, guestTypeHostToIgnore) after close.");

        this->VerifyApplicationHostRegistration(hostId, false, true);
        this->VerifyRuntimeRegistration(runtimeId, false, true);

        for (auto const & guestServiceType : guestServiceTypes)
        {
            ServiceTypeIdentifier serviceTypeId(servicePackageId, guestServiceType.ServiceTypeName);
            VersionedServiceTypeIdentifier versionedServiceTypeId(serviceTypeId, versionInstance);

            this->VerifyServiceTypeRegistration(versionedServiceTypeId, serviceDescription, activationContext, false, true);
        }
    }

    size_t typeHostCountAfterClose;
    error = this->GetTypeHostManager()->Test_GetTypeHostCount(typeHostCountAfterClose);

    ASSERT_IFNOT(error.IsSuccess(), "this->GetTypeHostManager()->Test_GetTypeHostCount(typeHostCountAfterClose)");

    Trace.WriteNoise(TraceType, "typeHostCountAfterClose={0}", typeHostCountAfterClose);

    ASSERT_IFNOT(typeHostCountAfterClose == 0, "typeHostCountAfterClose == 0");
}

BOOST_AUTO_TEST_SUITE_END()
