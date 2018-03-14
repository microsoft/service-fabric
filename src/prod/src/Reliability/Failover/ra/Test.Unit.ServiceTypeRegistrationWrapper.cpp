// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;
using namespace ServiceModel;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

namespace
{
    const Guid DefaultPartitionId(L"F423D5D9-8339-4D8E-BC3F-003AB06B44FB");
}

class TestServiceTypeRegistrationWrapper
{
protected:
    TestServiceTypeRegistrationWrapper()
    {
        BOOST_REQUIRE(TestSetup());
    }

    ~TestServiceTypeRegistrationWrapper()
    {
        BOOST_REQUIRE(TestCleanup());
    }

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    void Initialize(bool isSharedActivationContext)
    {
        isSharedActivationContext_ = isSharedActivationContext;

        sd_ = Default::GetInstance().SL1_SP1_STContext.SD;
        sd_.PackageActivationMode = isSharedActivationContext ? ServicePackageActivationMode::SharedProcess : ServicePackageActivationMode::ExclusiveProcess;

        fd_ = FailoverUnitDescription(
            FailoverUnitId(DefaultPartitionId),
            ConsistencyUnitDescription(Default::GetInstance().SP1_FTContext.CUID));

        strWrapper_ = make_unique<ServiceTypeRegistrationWrapper>(fd_, sd_);
    }

    void Initialize(bool isSharedActivationContext, RetryableErrorStateName::Enum state, int currentFailureCount, int warningThreshold, int dropThreshold)
    {
        Initialize(isSharedActivationContext);
        SetupRetryableErrorState(state, currentFailureCount, warningThreshold, dropThreshold);
    }

    void SetupIncrementError(ErrorCodeValue::Enum inputError)
    {
        utContext_->Hosting.IncrementApi.SetCompleteSynchronouslyWithError(inputError);
        utContext_->Hosting.ErrorToReturnFromOperation = inputError;
    }

    ErrorCodeValue::Enum AddRegistration()
    {
        return strWrapper_->AddServiceTypeRegistration(CreateRegistration(), utContext_->RA.HostingAdapterObj, *queue_);
    }

    ServiceTypeReadContext const & GetServiceTypeReadContext() const
    {
        return Default::GetInstance().SL1_SP1_STContext;
    }

    Hosting2::ServiceTypeRegistrationSPtr CreateRegistration() const
    {
        return GetServiceTypeReadContext().CreateServiceTypeRegistration(GetServiceTypeReadContext().ServicePackageVersionInstance);
    }

    ServiceModel::ServiceTypeIdentifier CreateServiceTypeId() const
    {
        return sd_.Type;
    }

    ErrorCodeValue::Enum AddRegistrationAndDrain()
    {
        auto rv = AddRegistration();

        Drain();

        return rv;
    }

    void MarkReplicaDownAndDrain()
    {
        strWrapper_->OnReplicaDown(GetHostingAdapter(), *queue_);

        Drain();
    }

    void MarkReplicaClosedAndDrain()
    {
        strWrapper_->OnReplicaClosed(GetHostingAdapter(), *queue_);

        Drain();
    }

    void VerifyNoHostingAction() const
    {
        utContext_->Hosting.IncrementApi.VerifyNoCalls();
        utContext_->Hosting.DecrementApi.VerifyNoCalls();
    }

    void VerifyIncrement() const
    {
        VerifyServiceTypeVectorCall(utContext_->Hosting.IncrementApi, L"increment");
    }

    void VerifyDecrement() const
    {
        VerifyServiceTypeVectorCall(utContext_->Hosting.DecrementApi, L"decrement");
    }

    void VerifyServiceTypeVectorCall(HostingStub::IncrementDecrementApiStub const & callList, wstring const & tag) const
    {
        auto actual = callList.GetParametersAt(0);
        Verify::AreEqual(CreateServiceTypeId(), actual.ServiceTypeId, wformatString("ST Id {0}", tag));
        Verify::AreEqual(sd_.CreateActivationContext(fd_.FailoverUnitId.Guid), actual.ActivationContext, wformatString("ActivationContext {0}", tag));
    }

    void VerifyCodePackageCountIsZero() const
    {
        Verify::AreEqualLogOnError(true, GetHostingAdapter().ServiceTypeMapObj.IsEmpty, L"Adapter is empty");
    }

    void VerifyCodePackageCountIsOne() const
    {
        Verify::AreEqualLogOnError(false, GetHostingAdapter().ServiceTypeMapObj.IsEmpty, L"Adapter is not empty");
        Verify::AreEqualLogOnError(1, GetHostingAdapter().ServiceTypeMapObj.GetServiceTypeCount(GetServiceTypeReadContext().ServiceTypeId), L"Count");
    }

    void VerifyServiceTypeId(ServiceModel::ServiceTypeIdentifier const & actual) const
    {
        auto expected = CreateServiceTypeId();

        Verify::AreEqualLogOnError(expected, actual, L"ServiceTypeid");        
    }

    void Drain()
    {
        queue_->ExecuteAllActions(L"a", entity_, utContext_->RA);
        queue_ = make_unique<StateMachineActionQueue>();
    }

    void Reset()
    {
        utContext_->Hosting.Reset();
    }

    void VerifyPropertiesAreInvalid() const
    {
        Verify::AreEqualLogOnError(L"", strWrapper_->AppHostId, L"AppHostId");
        Verify::AreEqualLogOnError(L"", strWrapper_->RuntimeId, L"RuntimeId");
        Verify::AreEqualLogOnError(false, strWrapper_->IsRuntimeActive, L"IsRuntimeActive");
        Verify::IsTrue(nullptr == strWrapper_->ServiceTypeRegistration, L"STR");
    }

    void VerifyPropertiesAreValid() const
    {
        Verify::IsTrue(L"" != strWrapper_->AppHostId, L"AppHostId");
        Verify::IsTrue(L"" != strWrapper_->RuntimeId, L"RuntimeId");
        Verify::IsTrue(strWrapper_->IsRuntimeActive, L"IsRuntimeActive");
        Verify::IsTrue(nullptr != strWrapper_->ServiceTypeRegistration, L"STR");
    }

    Hosting::HostingAdapter const & GetHostingAdapter() const
    {
        return utContext_->RA.HostingAdapterObj;
    }

    Hosting::HostingAdapter & GetHostingAdapter() 
    {
        return utContext_->RA.HostingAdapterObj;
    }

    void SetupRetryableErrorState(RetryableErrorStateName::Enum state, int currentFailureCount, int warningThreshold, int dropThreshold)
    {
        strWrapper_->RetryableErrorStateObj.Test_Set(state, currentFailureCount);
        strWrapper_->RetryableErrorStateObj.Test_SetThresholds(state, warningThreshold, dropThreshold, INT_MAX, INT_MAX);
    }

    Common::ErrorCode TryGetAndAddServiceTypeRegistration(Hosting2::ServiceTypeRegistrationSPtr const & incomingRegistration, __out std::pair<Health::ReplicaHealthEvent::Enum, Health::IHealthDescriptorUPtr> & healthReportInformation)
    {
        ErrorCode rv =  strWrapper_->TryGetAndAddServiceTypeRegistration(incomingRegistration, utContext_->RA.HostingAdapterObj, *queue_, FailoverConfig::GetConfig(), healthReportInformation);
        Drain();
        return rv;
    }

    int GetRetryableErrorStateCurrentFailureCount()
    {
        return strWrapper_->RetryableErrorStateObj.CurrentFailureCount;
    }

    void ValidateFailureCount(int expectedFailureCount)
    {
        int actualFailureCount = GetRetryableErrorStateCurrentFailureCount();
        Verify::AreEqual(expectedFailureCount, actualFailureCount, L"FailureCount should match");
    }

    void ValidateHealthEvent(Health::ReplicaHealthEvent::Enum event, std::pair<Health::ReplicaHealthEvent::Enum, Health::IHealthDescriptorUPtr> & healthReportInformation)
    {
        Verify::AreEqual(event, healthReportInformation.first, L"HealthEvent should match");
    }

    void TryGetAndAddSTRAndVerify(Hosting2::ServiceTypeRegistrationSPtr const & incomingRegistration, Common::ErrorCode expectedError, Health::ReplicaHealthEvent::Enum expectedEvent, int expectedFailureCount, std::wstring const & message)
    {
        auto healthReportInformation = make_pair<Health::ReplicaHealthEvent::Enum, Health::IHealthDescriptorUPtr>(Health::ReplicaHealthEvent::OK, make_unique<ClearWarningErrorHealthReportDescriptor>());
        auto actualError = TryGetAndAddServiceTypeRegistration(incomingRegistration, healthReportInformation);
        Verify::AreEqualLogOnError(expectedError.ReadValue(), actualError.ReadValue(), message);

        ValidateFailureCount(expectedFailureCount);

        ValidateHealthEvent(expectedEvent, healthReportInformation);
    }

    void ValidationRetryableStateInformation(RetryableErrorStateName::Enum expectedState)
    {
        auto actualState = strWrapper_->RetryableErrorStateObj.CurrentState;
        auto actualFailureCount = strWrapper_->RetryableErrorStateObj.CurrentFailureCount;
        Verify::AreEqualLogOnError(expectedState, actualState, L"RetryableErrorState.CurrentState should mactch");
        Verify::AreEqual(0, actualFailureCount, L"RetryableErrorState.CurrentFailureCount should match");
    }

    unique_ptr<StateMachineActionQueue> queue_;
    UnitTestContextUPtr utContext_;
    ServiceDescription sd_;
    FailoverUnitDescription fd_;
    unique_ptr<ServiceTypeRegistrationWrapper> strWrapper_;
    bool isSharedActivationContext_;
    EntityEntryBaseSPtr entity_;
    Storage::Api::IKeyValueStoreSPtr store_;
};

bool TestServiceTypeRegistrationWrapper::TestSetup()
{
    store_ = make_shared<Storage::InMemoryKeyValueStore>();
    entity_ = make_shared<TestEntityEntry>(L"a", store_);
    queue_ = make_unique<StateMachineActionQueue>();
    utContext_ = UnitTestContext::Create(UnitTestContext::Option::StubJobQueueManager | UnitTestContext::Option::TestAssertEnabled);
    return true;
}

bool TestServiceTypeRegistrationWrapper::TestCleanup()
{
    Drain();
    queue_->AbandonAllActions(utContext_->RA);
    return utContext_->Cleanup();
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestServiceTypeRegistrationWrapperSuite, TestServiceTypeRegistrationWrapper)

BOOST_AUTO_TEST_CASE(InitialObjectPropertiesTest)
{
    Initialize(true);

    VerifyPropertiesAreInvalid();
}

BOOST_AUTO_TEST_CASE(ReplicaDownWithNoRegistrationIsNoOp)
{
    Initialize(true);

    MarkReplicaDownAndDrain();

    VerifyNoHostingAction();
    VerifyCodePackageCountIsZero();
}

BOOST_AUTO_TEST_CASE(ReplicaClosedWithNoRegistrationIsNoOp)
{
    Initialize(true);

    MarkReplicaClosedAndDrain();

    VerifyNoHostingAction();
    VerifyCodePackageCountIsZero();
}

BOOST_AUTO_TEST_CASE(AddClosedAndThenReplicaDownWithNoRegistrationIsNoOp)
{
    Initialize(true);

    AddRegistrationAndDrain();
    MarkReplicaClosedAndDrain();

    Reset();

    MarkReplicaDownAndDrain();

    VerifyNoHostingAction();
    VerifyCodePackageCountIsZero();
}

BOOST_AUTO_TEST_CASE(AddClosedAndThenReplicaClosedWithNoRegistrationIsNoOp)
{
    Initialize(true);

    AddRegistrationAndDrain();
    MarkReplicaClosedAndDrain();

    Reset();
    
    MarkReplicaClosedAndDrain();

    VerifyNoHostingAction();
    VerifyCodePackageCountIsZero();
}

BOOST_AUTO_TEST_CASE(AddFromClosedIncrementsServiceTypeRegistration)
{
    Initialize(true);

    AddRegistrationAndDrain();

    VerifyIncrement();
}

BOOST_AUTO_TEST_CASE(DuplicateAddDoesNotIncrementAgain)
{
    Initialize(true);

    AddRegistrationAndDrain();

    Reset();

    AddRegistrationAndDrain();

    VerifyNoHostingAction();
}

BOOST_AUTO_TEST_CASE(IncrementHasActivationContext)
{
    Initialize(false);

    AddRegistrationAndDrain();

    VerifyIncrement();
}

BOOST_AUTO_TEST_CASE(PropertiesAreSetAfterStrIsAdded)
{
    Initialize(true);

    AddRegistrationAndDrain();

    VerifyPropertiesAreValid();
}

BOOST_AUTO_TEST_CASE(DuplicateAddDoesNotUpdateRegistrationAgain)
{
    Initialize(true);

    AddRegistrationAndDrain();

    auto initial = strWrapper_->ServiceTypeRegistration;

    AddRegistrationAndDrain();

    Verify::AreEqualLogOnError(initial, strWrapper_->ServiceTypeRegistration, L"STR must not change");
}

BOOST_AUTO_TEST_CASE(AddCausesHostingEvent)
{
    Initialize(true);

    AddRegistrationAndDrain();

    VerifyCodePackageCountIsOne();
}

BOOST_AUTO_TEST_CASE(DuplicateAddDoesNotCauseHostingEvent)
{
    Initialize(true);

    AddRegistrationAndDrain();

    Reset();

    AddRegistrationAndDrain();

    VerifyCodePackageCountIsOne();
}

BOOST_AUTO_TEST_CASE(FailureToIncrementDoesNotChangeAnything)
{
    Initialize(true);

    auto expectedError = ErrorCodeValue::GlobalLeaseLost;
    SetupIncrementError(expectedError);

    auto actualError = AddRegistrationAndDrain();
    Verify::AreEqualLogOnError(expectedError, actualError, L"AddRegistration should fail");

    VerifyCodePackageCountIsZero();
    VerifyPropertiesAreInvalid();
}

BOOST_AUTO_TEST_CASE(ReplicaDownAfterRegistrationHasBeenAddedDoesNotDecrementUsageCount)
{
    Initialize(true);

    AddRegistrationAndDrain();

    Reset();

    MarkReplicaDownAndDrain();

    VerifyNoHostingAction();
}

BOOST_AUTO_TEST_CASE(ReplicaDownAfterRegistrationDecrementsCodePackageCount)
{
    Initialize(true);

    AddRegistrationAndDrain();

    Reset();

    MarkReplicaDownAndDrain();

    VerifyCodePackageCountIsZero();
}

BOOST_AUTO_TEST_CASE(MultipleReplicaDownAfterRegistrationIsNoOp)
{
    Initialize(true);

    AddRegistrationAndDrain();

    Reset();

    MarkReplicaDownAndDrain();

    Reset();

    MarkReplicaDownAndDrain();

    VerifyCodePackageCountIsZero();
}

BOOST_AUTO_TEST_CASE(ReplicaClosedAfterRegistrationDecrementsUsageCount)
{
    Initialize(true);

    AddRegistrationAndDrain();

    Reset();

    MarkReplicaClosedAndDrain();

    VerifyDecrement();
}

BOOST_AUTO_TEST_CASE(DecrementHasActivationContext)
{
    Initialize(false);

    AddRegistrationAndDrain();

    Reset();

    MarkReplicaClosedAndDrain();

    VerifyDecrement();
}

BOOST_AUTO_TEST_CASE(MultipleReplicaClosedAfterRegistrationIsNoOp)
{
    Initialize(true);

    AddRegistrationAndDrain();

    Reset();

    MarkReplicaClosedAndDrain();

    Reset();

    MarkReplicaClosedAndDrain();

    VerifyNoHostingAction();
}

BOOST_AUTO_TEST_CASE(ReplicaClosedAfterReplicaDownDecrementsUsageCount)
{
    Initialize(true);

    AddRegistrationAndDrain();

    MarkReplicaDownAndDrain();

    Reset();

    MarkReplicaClosedAndDrain();

    VerifyDecrement();
}

BOOST_AUTO_TEST_CASE(ReplicaClosedReducesCodePackageCount)
{
    Initialize(true);

    AddRegistrationAndDrain();

    Reset();

    MarkReplicaClosedAndDrain();

    VerifyCodePackageCountIsZero();
}

BOOST_AUTO_TEST_CASE(MultipleReplicaClosedReducesCodePackageCount)
{
    Initialize(true);

    AddRegistrationAndDrain();

    MarkReplicaClosedAndDrain();

    Reset();

    MarkReplicaClosedAndDrain();

    VerifyCodePackageCountIsZero();
}


BOOST_AUTO_TEST_CASE(FSRSuccessShouldReturnSucceessAndIncrementCalled)
{
    Initialize(true, RetryableErrorStateName::FindServiceRegistrationAtOpen, 0, 2, 4);

    auto expectedError = ErrorCode(ErrorCodeValue::Success);
    TryGetAndAddSTRAndVerify(CreateRegistration(), expectedError, Health::ReplicaHealthEvent::OK, 0, L"FindServiceTypeRegistration should succeed");

    VerifyIncrement();
}

BOOST_AUTO_TEST_CASE(FSRSuccessWithFailureToIncrementShouldReturnRAServiceTypeNotRegistered)
{
    Initialize(true, RetryableErrorStateName::FindServiceRegistrationAtOpen, 0, 2, 4);

    auto inputError = ErrorCodeValue::GlobalLeaseLost;
    SetupIncrementError(inputError);
    
    auto expectedError = ErrorCode(ErrorCodeValue::RAServiceTypeNotRegistered);
    TryGetAndAddSTRAndVerify(CreateRegistration(), expectedError, Health::ReplicaHealthEvent::OK, 0, L"FindServiceTypeRegistration should succeed");
}

BOOST_AUTO_TEST_CASE(FSRSuccessShouldReturnSuccessAndClearWarningReportIfWarningReportAlreadyGenerated)
{
    Initialize(true, RetryableErrorStateName::FindServiceRegistrationAtOpen, 3, 2, INT_MAX);

    auto expectedError = ErrorCode(ErrorCodeValue::Success);
    TryGetAndAddSTRAndVerify(CreateRegistration(), expectedError, Health::ReplicaHealthEvent::ClearServiceTypeRegistrationWarning, 0, L"FindServiceTypeRegistration should succeed");
}


BOOST_AUTO_TEST_CASE(FSRWithRetryableErrorShouldIncreaseFailureCount)
{
    Initialize(true, RetryableErrorStateName::FindServiceRegistrationAtOpen, 0, 2, 4);

    utContext_->Hosting.AddFindServiceTypeExpectation(GetServiceTypeReadContext(), FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen);

    auto expectedError = ErrorCode(ErrorCodeValue::RAServiceTypeNotRegistered);
    TryGetAndAddSTRAndVerify(nullptr, expectedError, Health::ReplicaHealthEvent::OK, 1, L"FindServiceTypeRegistration should fail");
}

BOOST_AUTO_TEST_CASE(FSRWithNonRetryableErrorShouldReturnActualErrorAndFailureCountNoChange)
{
    Initialize(true, RetryableErrorStateName::FindServiceRegistrationAtOpen, 0, 2, 4);

    utContext_->Hosting.AddFindServiceTypeExpectation(GetServiceTypeReadContext(), FindServiceTypeRegistrationError::CategoryName::FatalErrorForOpen);

    auto expectedError = FindServiceTypeRegistrationError::Create(FindServiceTypeRegistrationError::CategoryName::FatalErrorForOpen);
    TryGetAndAddSTRAndVerify(nullptr, expectedError.get_Error(), Health::ReplicaHealthEvent::OK, 0, L"FindServiceTypeRegistration should fail");
}

BOOST_AUTO_TEST_CASE(FSRWithRetryableErrorShouldIncreaseFailureCountAndReportWarningIfAtWarningThreshold)
{
    Initialize(true, RetryableErrorStateName::FindServiceRegistrationAtOpen, 1, 2, 4);

    utContext_->Hosting.AddFindServiceTypeExpectation(GetServiceTypeReadContext(), FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen);

    auto expectedError = ErrorCode(ErrorCodeValue::RAServiceTypeNotRegistered);
    TryGetAndAddSTRAndVerify(nullptr, expectedError, Health::ReplicaHealthEvent::ServiceTypeRegistrationWarning, 2, L"FindServiceTypeRegistration should fail");
}

BOOST_AUTO_TEST_CASE(FSRWithRetryableErrorShouldIncreaseFailureCountAndNotReportWarningIfBeyondWarningThreshold)
{
    Initialize(true, RetryableErrorStateName::FindServiceRegistrationAtOpen, 2, 2, 4);

    utContext_->Hosting.AddFindServiceTypeExpectation(GetServiceTypeReadContext(), FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen);

    auto expectedError = ErrorCode(ErrorCodeValue::RAServiceTypeNotRegistered);
    TryGetAndAddSTRAndVerify(nullptr, expectedError, Health::ReplicaHealthEvent::OK, 3, L"FindServiceTypeRegistration should fail");
}

BOOST_AUTO_TEST_CASE(FSRWithRetryableErrorShouldIncreaseFailureCountAndReturnActualErrorIfAtDropThreshold)
{
    Initialize(true, RetryableErrorStateName::FindServiceRegistrationAtOpen, 3, INT_MAX, 4);

    utContext_->Hosting.AddFindServiceTypeExpectation(GetServiceTypeReadContext(), FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen);

    auto expectedError = FindServiceTypeRegistrationError::Create(FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpen);
    TryGetAndAddSTRAndVerify(nullptr, expectedError.get_Error(), Health::ReplicaHealthEvent::OK, 4, L"FindServiceTypeRegistration should fail");
}

BOOST_AUTO_TEST_CASE(StartCallShouldSetRetryableErrorStateCorrectly)
{
    Initialize(true);

    // For replica open
    strWrapper_->Start(Reliability::ReconfigurationAgentComponent::ReplicaStates::InCreate);
    ValidationRetryableStateInformation(RetryableErrorStateName::FindServiceRegistrationAtOpen);

    // For replica reopen
    strWrapper_->Start(Reliability::ReconfigurationAgentComponent::ReplicaStates::StandBy);
    ValidationRetryableStateInformation(RetryableErrorStateName::FindServiceRegistrationAtReopen);

    // For replica close
    strWrapper_->Start(Reliability::ReconfigurationAgentComponent::ReplicaStates::InDrop);
    ValidationRetryableStateInformation(RetryableErrorStateName::FindServiceRegistrationAtDrop);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
