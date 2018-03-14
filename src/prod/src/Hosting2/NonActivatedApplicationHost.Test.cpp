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
#include "Hosting2/DummyStatelessServiceFactory.Test.h"
#include "Hosting2/FabricNodeHost.Test.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Transport;
using namespace Fabric;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;

const StringLiteral TraceType("NonActivatedApplicationHostTest");

// ********************************************************************************************************************

class NonActivatedApplicationHostTestHelper
{
    DENY_COPY(NonActivatedApplicationHostTestHelper)

public:
    NonActivatedApplicationHostTestHelper();
    
    ApplicationHostRegistrationTableUPtr const & GetApplicationHostRegistrationTable();
    RuntimeRegistrationTableUPtr const & GetRuntimeRegistrationTable();
    FabricRuntimeManagerUPtr const & GetFabricRuntimeManager();

    bool Setup();
    bool Cleanup();

    void VerifyApplicationHostRegistration(wstring const & hostId);
    void VerifyRuntimeRegistration(wstring const & runtimeId);
    void VerifyServiceTypeRegistration(
        VersionedServiceTypeIdentifier const & versionedServiceTypeId, 
        ServiceDescription const & serviceDescription,
        ServicePackageActivationContext const & activationContext,
        ErrorCode const expectedErrorCode);

public:
    shared_ptr<TestFabricNodeHost> const fabricNodeHost_;
};

// ********************************************************************************************************************

class NonActivatedApplicationHostTestClass
{
protected:
    NonActivatedApplicationHostTestClass() : testHelper_() { BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup);
    ~NonActivatedApplicationHostTestClass() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);


    NonActivatedApplicationHostTestHelper testHelper_;
};

// ********************************************************************************************************************

class ParallelCreateFabricRuntimeTestClass
{
protected:
    ParallelCreateFabricRuntimeTestClass() :testHelper_(),
        testContext_() 
    {
        testContext_ = make_shared<TestContext>();
        BOOST_REQUIRE(Setup());
    }
    TEST_CLASS_SETUP(Setup);
    ~ParallelCreateFabricRuntimeTestClass() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);

    struct TestContext
    {
        TestContext()
            : waitEvent_(false),
            numRuntimesRemaining_(NumRuntimes),
            numFailed_(0)
        {
        }

        ManualResetEvent waitEvent_;
        Common::atomic_long numRuntimesRemaining_;
        Common::atomic_long numFailed_;
        static const int TimeoutInMillis = 10000;
        static const int NumRuntimes = 10;
    };

    static void FabricCreateRuntime(shared_ptr<TestContext> testContext);

    NonActivatedApplicationHostTestHelper testHelper_;
    shared_ptr<TestContext> testContext_;
};


// ********************************************************************************************************************

class ServiceTypeStatusManagerTestClass
{
protected:
    ServiceTypeStatusManagerTestClass() : testHelper_() { BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup);
    ~ServiceTypeStatusManagerTestClass() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);

    NonActivatedApplicationHostTestHelper testHelper_;
    TimeSpan graceInterval_;
};

// ********************************************************************************************************************
// NonActivatedApplicationHostTestHelper Implementation
//

NonActivatedApplicationHostTestHelper::NonActivatedApplicationHostTestHelper()
    : fabricNodeHost_(make_shared<TestFabricNodeHost>())
{
}

bool NonActivatedApplicationHostTestHelper::Setup()
{
    return fabricNodeHost_->Open();
}

bool NonActivatedApplicationHostTestHelper::Cleanup()
{
    return fabricNodeHost_->Close();
}

ApplicationHostRegistrationTableUPtr const & NonActivatedApplicationHostTestHelper::GetApplicationHostRegistrationTable()
{
    return fabricNodeHost_->GetHosting().ApplicationHostManagerObj->Test_ApplicationHostRegistrationTable;
}

RuntimeRegistrationTableUPtr const & NonActivatedApplicationHostTestHelper::GetRuntimeRegistrationTable()
{
    return fabricNodeHost_->GetHosting().FabricRuntimeManagerObj->Test_RuntimeRegistrationTable;
}

FabricRuntimeManagerUPtr const & NonActivatedApplicationHostTestHelper::GetFabricRuntimeManager()
{
    return fabricNodeHost_->GetHosting().FabricRuntimeManagerObj;
}

void NonActivatedApplicationHostTestHelper::VerifyApplicationHostRegistration(wstring const & hostId)
{
    ApplicationHostRegistrationSPtr registration;

    ApplicationHostRegistrationTableUPtr const & table = GetApplicationHostRegistrationTable();
    auto error = table->Find(hostId, registration);
    if (error.IsSuccess())
    {
        Trace.WriteNoise(
            TraceType,
            "VerifyApplicationHostRegistration: ErrorCode={0}, Registration={1}",
            error,
            *registration);
    }
    else
    {
         Trace.WriteError(
            TraceType,
            "VerifyApplicationHostRegistration: ErrorCode={0}, HostId={1}",
            error,
            hostId);
    }

    VERIFY_IS_TRUE(error.IsSuccess(), L"VerifyApplicationHostRegistration");
}

void NonActivatedApplicationHostTestHelper::VerifyRuntimeRegistration(wstring const & runtimeId)
{
    RuntimeRegistrationSPtr registration;

    RuntimeRegistrationTableUPtr const & table = GetRuntimeRegistrationTable();
    auto error = table->Find(runtimeId, registration);
    if (error.IsSuccess())
    {
        Trace.WriteNoise(
            TraceType,
            "VerifyRuntimeRegistration: ErrorCode={0}, Registration={1}",
            error,
            *registration);
    }
    else
    {
         Trace.WriteError(
            TraceType,
            "VerifyRuntimeRegistration: ErrorCode={0}, RuntimeId={1}",
            error,
            runtimeId);
    }

    VERIFY_IS_TRUE(error.IsSuccess(), L"VerifyRuntimeRegistration");
}

void NonActivatedApplicationHostTestHelper::VerifyServiceTypeRegistration(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServiceDescription const & serviceDescription,
    ServicePackageActivationContext const & activationContext,
    ErrorCode const expectedErrorCode)
{
    FabricRuntimeManagerUPtr const & runtimeManager = GetFabricRuntimeManager();
    ServiceTypeRegistrationSPtr registration;
    uint64 sequenceNumber;
    auto error = runtimeManager->FindServiceTypeRegistration(versionedServiceTypeId, serviceDescription, activationContext, sequenceNumber, registration);

    Trace.WriteNoise(
        TraceType,
        "VerifyServiceTypeRegistration: VersionedServiceTypeIdentifer={0},  ErrorCode={1}, Expected={2}",
        versionedServiceTypeId,
        error,
        expectedErrorCode);

    if (registration)
    {
        Trace.WriteNoise(
        TraceType,
        "VerifyServiceTypeRegistration: Result={0}",
        *registration);
    }

    VERIFY_IS_TRUE(error.IsError(expectedErrorCode.ReadValue()), L"VerifyServiceTypeRegistration");
}

// ********************************************************************************************************************
// NonActivatedApplicationHostTestClass Implementation
//




// Create and Register Runtime
BOOST_FIXTURE_TEST_SUITE(NonActivatedApplicationHostTestClassSuite,NonActivatedApplicationHostTestClass)

BOOST_AUTO_TEST_CASE(OpenCloseTest)
{
    // create NonActivatedApplicationHost
    ApplicationHostSPtr host;
    auto runtimeServiceAddress = testHelper_.fabricNodeHost_->GetHosting().RuntimeServiceAddress;
    auto error = NonActivatedApplicationHost::Create(nullptr, runtimeServiceAddress, host);

    Trace.WriteTrace(
        error.ToLogLevel(),
        TraceType,
        "NonActivatedApplicationHost::Create: ErrorCode={0}",
        error);
    VERIFY_IS_TRUE(error.IsSuccess(), L"NonActivatedApplicationHost::Create");

    // Open the host -- this registers the host with FabricNode
    auto openWaiter = make_shared<AsyncOperationWaiter>();

    host->BeginOpen(
        Hosting2::HostingConfig::GetConfig().CreateFabricRuntimeTimeout,
        [this, openWaiter, host] (AsyncOperationSPtr const & operation) { openWaiter->SetError(host->EndOpen(operation)); openWaiter->Set(); },
        AsyncOperationSPtr());

    openWaiter->WaitOne();
    error = openWaiter->GetError();
    Trace.WriteTrace(
        error.ToLogLevel(),
        TraceType,
        "NonActivatedApplicationHost::Open: ErrorCode={0}",
        error);
    VERIFY_IS_TRUE(error.IsSuccess(), L"NonActivatedApplicationHost::Open");

    this->testHelper_.VerifyApplicationHostRegistration(host->Id);
    
    // close application host - this should unregister the host
    auto closeWaiter = make_shared<AsyncOperationWaiter>();
    host->BeginClose(
        Hosting2::HostingConfig::GetConfig().CreateFabricRuntimeTimeout,
        [this, closeWaiter, host] (AsyncOperationSPtr const & operation) { closeWaiter->SetError(host->EndClose(operation)); closeWaiter->Set(); },
        AsyncOperationSPtr());
    
    closeWaiter->WaitOne();
    error = closeWaiter->GetError();
    Trace.WriteTrace(
        error.ToLogLevel(),
        TraceType,
        "NonActivatedApplicationHost::Close: ErrorCode={0}",
        error);
    VERIFY_IS_TRUE(error.IsSuccess(), L"NonActivatedApplicationHost::Close");
}

BOOST_AUTO_TEST_CASE(CreateFabricRuntimeTest)
{
    ComPointer<IFabricRuntime> fabricRuntime;
    HRESULT hr = ApplicationHostContainer::FabricCreateRuntime(IID_IFabricRuntime, fabricRuntime.VoidInitializationAddress());
    Trace.WriteTrace(
        FAILED(hr) ? LogLevel::Error : LogLevel::Noise,
        TraceType,
        "ApplicationHostContainer::FabricCreateRuntime: HR={0}",
        hr);
    VERIFY_IS_TRUE(!FAILED(hr), L"ApplicationHostContainer::FabricCreateRuntime");
    
    auto comFabricRuntime = static_cast<ComFabricRuntime*>(fabricRuntime.GetRawPointer());
    wstring runtimeId(comFabricRuntime->Runtime->RuntimeId);

    this->testHelper_.VerifyRuntimeRegistration(runtimeId);
}

BOOST_AUTO_TEST_CASE(StatelessFactoryRegistrationTest)
{
    // create fabric runtime
    ComPointer<IFabricRuntime> fabricRuntime;
    HRESULT hr = ApplicationHostContainer::FabricCreateRuntime(IID_IFabricRuntime, fabricRuntime.VoidInitializationAddress());
    Trace.WriteTrace(
        FAILED(hr) ? LogLevel::Error : LogLevel::Noise,
        TraceType,
        "ApplicationHostContainer::FabricCreateRuntime: HR={0}",
        hr);
    VERIFY_IS_TRUE(!FAILED(hr), L"ApplicationHostContainer::FabricCreateRuntime");

    // verify registration of fabric runtime
    auto comFabricRuntime = static_cast<ComFabricRuntime*>(fabricRuntime.GetRawPointer());
    wstring runtimeId(comFabricRuntime->Runtime->RuntimeId);
    this->testHelper_.VerifyRuntimeRegistration(runtimeId);

    // register stateless service factory
    wstring serviceTypeName = L"DummyStatelessServiceType";
    ComPointer<IFabricStatelessServiceFactory> testFactory = make_com<DummyStatelessServiceFactory, IFabricStatelessServiceFactory>(serviceTypeName);

    hr = fabricRuntime->RegisterStatelessServiceFactory(serviceTypeName.c_str(), testFactory.GetRawPointer());
    Trace.WriteTrace(
        FAILED(hr) ? LogLevel::Error : LogLevel::Noise,
        TraceType,
        "RegisterStatelessServiceFactory HR={0}",
        hr);
    VERIFY_IS_TRUE(!FAILED(hr), L"RegisterStatelessServiceFactory");

    auto servicePackageActivationId = comFabricRuntime->Runtime->GetRuntimeContext().CodeContext.CodePackageInstanceId.ServicePackageInstanceId;

    ServiceDescription serviceDescription;
    serviceDescription.ApplicationName = std::move(wstring(comFabricRuntime->Runtime->GetRuntimeContext().CodeContext.ApplicationName));
    serviceDescription.Name = L"GuestApp/GuestService";

    ServiceTypeIdentifier serviceTypeId(servicePackageActivationId.ServicePackageId, serviceTypeName);

    this->testHelper_.VerifyServiceTypeRegistration(
        VersionedServiceTypeIdentifier(
            serviceTypeId,
            ServicePackageVersionInstance()),
        serviceDescription,
        servicePackageActivationId.ActivationContext,
        ErrorCode(ErrorCodeValue::Success));

    this->testHelper_.VerifyServiceTypeRegistration(
        VersionedServiceTypeIdentifier(
            serviceTypeId,
            ServicePackageVersionInstance(ServicePackageVersion(ApplicationVersion(RolloutVersion(1,1)), RolloutVersion()),1)),
        serviceDescription,
        servicePackageActivationId.ActivationContext,
        ErrorCode(ErrorCodeValue::NotFound));

    this->testHelper_.VerifyServiceTypeRegistration(
        VersionedServiceTypeIdentifier(
            serviceTypeId,
            ServicePackageVersionInstance(ServicePackageVersion(ApplicationVersion(RolloutVersion()), RolloutVersion(1,1)), 1)),
        serviceDescription,
        servicePackageActivationId.ActivationContext,
        ErrorCode(ErrorCodeValue::NotFound));
}

BOOST_AUTO_TEST_CASE(RuntimeSharingHelperTest)
{
    // create runtime sharing helper
    auto root = testHelper_.fabricNodeHost_->GetFabricNode()->CreateComponentRoot();
        
    Hosting2::IRuntimeSharingHelper & runTimeSharingHelper = testHelper_.fabricNodeHost_->GetFabricNode()->Test_RuntimeSharingHelper;
    auto & runtimeSharingHelper = dynamic_cast<RuntimeSharingHelper&>(runTimeSharingHelper);

    // create shared runtime
    ComPointer<IFabricRuntime> runtime1 = runtimeSharingHelper.GetRuntime();
    VERIFY_IS_TRUE(runtime1.GetRawPointer() != nullptr, L"RuntimeSharingHelper::GetRuntime");

    ComPointer<IFabricRuntime> runtime2 = runtimeSharingHelper.GetRuntime();
    VERIFY_IS_TRUE(runtime2.GetRawPointer() != nullptr, L"RuntimeSharingHelper::GetRuntime");

    ComPointer<IFabricRuntime> runtime3 = runtimeSharingHelper.GetRuntime();
    VERIFY_IS_TRUE(runtime3.GetRawPointer() != nullptr, L"RuntimeSharingHelper::GetRuntime");

    runtime1.Release();
    runtime2.Release();
    runtime3.Release();
}

BOOST_AUTO_TEST_SUITE_END()


bool NonActivatedApplicationHostTestClass::Setup()
{
   return testHelper_.Setup();
}

bool NonActivatedApplicationHostTestClass::Cleanup()
{
   auto retval = testHelper_.Cleanup();
   ApplicationHostContainer::GetApplicationHostContainer().Test_CloseApplicationHost();

   return retval;
}


// ********************************************************************************************************************
// ParallelCreateFabricRuntimeTestClass Implementation
//




BOOST_FIXTURE_TEST_SUITE(ParallelCreateFabricRuntimeTestClassSuite,ParallelCreateFabricRuntimeTestClass)

BOOST_AUTO_TEST_CASE(CreateFabricRuntimeTest)
{
    for(auto i = 0; i < TestContext::NumRuntimes; i++)
    {
        auto testContext = this->testContext_;
        Threadpool::Post([testContext]{ FabricCreateRuntime(testContext); });
    }

    // wait for 10 seconds for all runtimes to get created
    this->testContext_->waitEvent_.WaitOne(TestContext::TimeoutInMillis);
    VERIFY_IS_TRUE(this->testContext_->numRuntimesRemaining_.load() == 0, L"All of the CreateRuntimes completed.");
    VERIFY_IS_TRUE(this->testContext_->numFailed_.load() == 0, L"All of the runtimes created successfully.");
}

BOOST_AUTO_TEST_SUITE_END()


bool ParallelCreateFabricRuntimeTestClass::Setup()
{
   return testHelper_.Setup();
}

bool ParallelCreateFabricRuntimeTestClass::Cleanup()
{
    auto retval = testHelper_.Cleanup();
    ApplicationHostContainer::GetApplicationHostContainer().Test_CloseApplicationHost();

    return retval;
}


void ParallelCreateFabricRuntimeTestClass::FabricCreateRuntime(shared_ptr<TestContext> testContext)
{
    Trace.WriteInfo(
        TraceType,
        "NonActivatedApplicationHostTest::StartCreateRuntime, Remaining {0}",
        testContext->numRuntimesRemaining_.load());
    IFabricRuntime * runtime;
    HRESULT hr = ApplicationHostContainer::FabricCreateRuntime(IID_IFabricRuntime, (void**)&runtime);
    if (FAILED(hr))
    {
        Trace.WriteWarning(
        TraceType,
        "Failed creation of runtime: HR={0}",
        hr);
        testContext->numFailed_++;
    }
    else
    {
        runtime->Release();
    }
    testContext->numRuntimesRemaining_--;

    if (testContext->numRuntimesRemaining_.load() == 0)
    {
        testContext->waitEvent_.Set();
    }
}


// ********************************************************************************************************************
// ServiceTypeStatusManagerTestClass Implementation
//




BOOST_FIXTURE_TEST_SUITE(ServiceTypeStatusManagerTestClassSuite,ServiceTypeStatusManagerTestClass)

BOOST_AUTO_TEST_CASE(DisableEnableTest)
{
    // create fabric runtime
    ComPointer<IFabricRuntime> fabricRuntime;
    HRESULT hr = ApplicationHostContainer::FabricCreateRuntime(IID_IFabricRuntime, fabricRuntime.VoidInitializationAddress());
    Trace.WriteTrace(
        FAILED(hr) ? LogLevel::Error : LogLevel::Noise,
        TraceType,
        "ApplicationHostContainer::FabricCreateRuntime: HR={0}",
        hr);
    VERIFY_IS_TRUE(!FAILED(hr), L"ApplicationHostContainer::FabricCreateRuntime");

    // verify registration of fabric runtime
    auto comFabricRuntime = static_cast<ComFabricRuntime*>(fabricRuntime.GetRawPointer());
    wstring runtimeId(comFabricRuntime->Runtime->RuntimeId);
    this->testHelper_.VerifyRuntimeRegistration(runtimeId);
    
    wstring serviceTypeName = L"DummyStatelessServiceType";
    ServiceTypeIdentifier serviceTypeId(ServicePackageIdentifier(), serviceTypeName);
    ServicePackageActivationContext activationContext;
    ServiceTypeInstanceIdentifier serviceTypeInstanceId(serviceTypeId, activationContext, L"");

    ServicePackageVersionInstance servicePackageVersionInstance;
    VersionedServiceTypeIdentifier versionedServiceTypeId(serviceTypeId, servicePackageVersionInstance);
    
    // perform a find for the service which should disable the service type
    FabricRuntimeManagerUPtr const & runtimeManager = this->testHelper_.GetFabricRuntimeManager();
    ServiceTypeRegistrationSPtr registration;
    uint64 sequenceNumberBeforeRegistration;

    // set grace interval to zero
    HostingConfig::GetConfig().ServiceTypeDisableGraceInterval = TimeSpan::Zero;

    ServiceDescription serviceDescription;
    serviceDescription.Name = L"GuestService";

    auto error = runtimeManager->FindServiceTypeRegistration(
        versionedServiceTypeId,
        serviceDescription,
        activationContext,
        sequenceNumberBeforeRegistration,
        registration);

    Trace.WriteNoise(
        TraceType,
        "Find registration for {0} before registration - 1. SequenceNumber={1}, Error={2}",
        versionedServiceTypeId,
        sequenceNumberBeforeRegistration,
        error);

    VERIFY_IS_TRUE(
        error.IsError(ErrorCodeValue::HostingServiceTypeNotRegistered), 
        L"error.IsError(ErrorCodeValue::HostingServiceTypeNotRegistered)");

    // The ServiceType will be disabled on a different thread
    // Attempting another find will return disabled eventually
    for(int i = 0; i < 5; i++)
    {
        error = runtimeManager->FindServiceTypeRegistration(
            versionedServiceTypeId,
            serviceDescription,
            activationContext,
            sequenceNumberBeforeRegistration,
            registration);

        if(error.IsError(ErrorCodeValue::HostingServiceTypeDisabled))
        {
            break;
        }
        else
        {
            ::Sleep(3000);
        }
    }

    Trace.WriteNoise(
        TraceType,
        "Find registration for {0} before registration - 2. SequenceNumber={1}, Error={2}",
        versionedServiceTypeId,
        sequenceNumberBeforeRegistration,
        error);

    VERIFY_IS_TRUE(
        error.IsError(ErrorCodeValue::HostingServiceTypeDisabled), 
        L"error.IsError(ErrorCodeValue::HostingServiceTypeDisabled)");

    // now register the type
    ComPointer<IFabricStatelessServiceFactory> testFactory = make_com<DummyStatelessServiceFactory, IFabricStatelessServiceFactory>(serviceTypeName);

    hr = fabricRuntime->RegisterStatelessServiceFactory(serviceTypeName.c_str(), testFactory.GetRawPointer());
    Trace.WriteTrace(
        FAILED(hr) ? LogLevel::Error : LogLevel::Noise,
        TraceType,
        "RegisterStatelessServiceFactory HR={0}",
        hr);
    VERIFY_IS_TRUE(!FAILED(hr), L"RegisterStatelessServiceFactory");

    uint64 sequenceNumberAfterRegistration;
    error = runtimeManager->FindServiceTypeRegistration(
        versionedServiceTypeId,
        serviceDescription,
        activationContext,
        sequenceNumberAfterRegistration,
        registration);

    Trace.WriteNoise(
        TraceType,
        "Find registration for {0} after registration. SequenceNumber={1}, Error={2}",
        versionedServiceTypeId,
        sequenceNumberAfterRegistration,
        error);

    VERIFY_IS_TRUE(
        error.IsSuccess(), 
        L"error.IsSuccess()");

    // ensure that type is enabled now
    bool disabled = this->testHelper_.GetFabricRuntimeManager()->ServiceTypeStateManagerObj->Test_IsDisabled(serviceTypeInstanceId);

    Trace.WriteNoise(
        TraceType,
        "IsDisabled(ServiceTypeId:{0} SequenceNumber:{1}) = {2}",
        serviceTypeId,
        sequenceNumberAfterRegistration,
        disabled);

    VERIFY_IS_TRUE(disabled == false, L"disabled == false after registration");
}

BOOST_AUTO_TEST_CASE(DelayedDisableTest)
{
    // create fabric runtime
    ComPointer<IFabricRuntime> fabricRuntime;
    HRESULT hr = ApplicationHostContainer::FabricCreateRuntime(IID_IFabricRuntime, fabricRuntime.VoidInitializationAddress());
    Trace.WriteTrace(
        FAILED(hr) ? LogLevel::Error : LogLevel::Noise,
        TraceType,
        "ApplicationHostContainer::FabricCreateRuntime: HR={0}",
        hr);
    VERIFY_IS_TRUE(!FAILED(hr), L"ApplicationHostContainer::FabricCreateRuntime");

    // verify registration of fabric runtime
    auto comFabricRuntime = static_cast<ComFabricRuntime*>(fabricRuntime.GetRawPointer());
    wstring runtimeId(comFabricRuntime->Runtime->RuntimeId);
    this->testHelper_.VerifyRuntimeRegistration(runtimeId);
    
    ServicePackageActivationContext activationContext;
    
    wstring serviceTypeName1 = L"ServiceType1";
    ServiceTypeIdentifier serviceTypeId1 = ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName1);
    ServiceTypeInstanceIdentifier serviceTypeInstanceId1(serviceTypeId1, activationContext, L"");

    ServicePackageVersionInstance servicePackageVersionInstance1;
    VersionedServiceTypeIdentifier versionedServiceTypeId1 = VersionedServiceTypeIdentifier(serviceTypeId1, servicePackageVersionInstance1);
    
    wstring serviceTypeName2 = L"ServiceType2";
    ServiceTypeIdentifier serviceTypeId2 = ServiceTypeIdentifier(ServicePackageIdentifier(), serviceTypeName2);
    ServiceTypeInstanceIdentifier serviceTypeInstanceId2(serviceTypeId2, activationContext, L"");

    ServicePackageVersionInstance servicePackageVersionInstance2;
    VersionedServiceTypeIdentifier versionedServiceTypeId2 = VersionedServiceTypeIdentifier(serviceTypeId2, servicePackageVersionInstance2);

    FabricRuntimeManagerUPtr const & runtimeManager = this->testHelper_.GetFabricRuntimeManager();
    ServiceTypeRegistrationSPtr registrationST1;
    ServiceTypeRegistrationSPtr registrationST2;
    uint64 sequenceNumberST1;
    uint64 sequenceNumberST2;

    // set grace interval to 20 seconds
    HostingConfig::GetConfig().ServiceTypeDisableGraceInterval = TimeSpan::FromSeconds(10);

    ServiceDescription serviceDescription;
    serviceDescription.Name = L"GuestService";

    // find service type 1
    auto error = runtimeManager->FindServiceTypeRegistration(
        versionedServiceTypeId1,
        serviceDescription,
        activationContext,
        sequenceNumberST1,
        registrationST1);

    Trace.WriteNoise(
        TraceType,
        "Find registration for {0}, SequenceNumber={1}, Error={2}",
        versionedServiceTypeId1,
        sequenceNumberST1,
        error);

    VERIFY_IS_TRUE(
        error.IsError(ErrorCodeValue::HostingServiceTypeNotRegistered), 
        L"error.IsError(ErrorCodeValue::HostingServiceTypeNotRegistered)");


    // register service type 2
    ComPointer<IFabricStatelessServiceFactory> testFactory = make_com<DummyStatelessServiceFactory, IFabricStatelessServiceFactory>(serviceTypeName2);

    hr = fabricRuntime->RegisterStatelessServiceFactory(serviceTypeName2.c_str(), testFactory.GetRawPointer());
    Trace.WriteTrace(
        FAILED(hr) ? LogLevel::Error : LogLevel::Noise,
        TraceType,
        "RegisterStatelessServiceFactory HR={0}",
        hr);
    VERIFY_IS_TRUE(!FAILED(hr), L"RegisterStatelessServiceFactory");

    error = runtimeManager->FindServiceTypeRegistration(
        versionedServiceTypeId2,
        serviceDescription,
        activationContext,
        sequenceNumberST2,
        registrationST2);

    Trace.WriteNoise(
        TraceType,
        "Find registration for {0}, SequenceNumber={1}, Error={2}",
        versionedServiceTypeId2,
        sequenceNumberST2,
        error);

    VERIFY_IS_TRUE(
        error.IsSuccess(), 
        L"error.IsSuccess()");

    // find st1 again
    error = runtimeManager->FindServiceTypeRegistration(
        versionedServiceTypeId1,
        serviceDescription,
        activationContext,
        sequenceNumberST1,
        registrationST1);

    Trace.WriteNoise(
        TraceType,
        "Find registration for {0}, SequenceNumber={1}, Error={2}",
        versionedServiceTypeId1,
        sequenceNumberST1,
        error);

    VERIFY_IS_TRUE(
        error.IsError(ErrorCodeValue::HostingServiceTypeNotRegistered), 
        L"error.IsError(ErrorCodeValue::HostingServiceTypeNotRegistered)");

    // wait 20 seconds
    ::Sleep(20000);

    // ensure that ST1 is disabled 
    error = runtimeManager->FindServiceTypeRegistration(
        versionedServiceTypeId1,
        serviceDescription,
        activationContext,
        sequenceNumberST1,
        registrationST1);

    Trace.WriteNoise(
        TraceType,
        "Find registration for {0}. SequenceNumber={1}, Error={2}",
        versionedServiceTypeId1,
        sequenceNumberST1,
        error);

    VERIFY_IS_TRUE(
        error.IsError(ErrorCodeValue::HostingServiceTypeDisabled), 
        L"error.IsError(ErrorCodeValue::HostingServiceTypeDisabled)");
}

BOOST_AUTO_TEST_SUITE_END()

bool ServiceTypeStatusManagerTestClass::Setup()
{
    graceInterval_ = HostingConfig::GetConfig().ServiceTypeDisableGraceInterval;
    return testHelper_.Setup();
}

bool ServiceTypeStatusManagerTestClass::Cleanup()
{
    HostingConfig::GetConfig().ServiceTypeDisableGraceInterval = graceInterval_;
    auto retval = testHelper_.Cleanup();
    ApplicationHostContainer::GetApplicationHostContainer().Test_CloseApplicationHost();

   return retval;
}

