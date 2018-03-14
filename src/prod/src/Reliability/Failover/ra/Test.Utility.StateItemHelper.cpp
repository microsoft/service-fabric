// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Test.Utility.EventWriterStub.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace ReliabilityUnitTest;
using namespace StateItemHelper;
using namespace StateManagement;
using namespace Diagnostics;

void ReliabilityUnitTest::StateItemHelper::RAStateItem::WriteToTextWriter(Common::TextWriter & w, ReliabilityUnitTest::StateItemHelper::RAStateItem::Enum const & e)
{
    switch (e)
    {
    case None:
        w << "None"; return;
    case FMMessages:
        w << "FMMessages"; return;
    case FMMMessages:
        w << "FMMMessages"; return;
    case RAMessages:
        w << "RAMessages"; return;
    case RAPMessages:
        w << "RAPMessages"; return;
    case MessageRetryTimer:
        w << "MessageRetry"; return;
    case FMMessageRetry:
        w << "ReplicaUpRetry"; return;
    case StateCleanupWorkManager:
        w << "StateCleanup"; return;
    case ReconfigurationMessageRetryWorkManager:
        w << "ReconfigMessageRetry"; return;
    case ReplicaCloseRetryWorkManager:
        w << "ReplicaCloseRetry"; return;
    case ReplicaOpenRetryWorkManager:
        w << "ReplicaOpenRetryWorkManager"; return;
    case UpdateServiceDescriptionRetryWorkManager:
        w << "UpdateServiceDescriptionRetryWorkManager"; return;
    case Hosting:
        w << "Hosting"; return;
    case Health:
        w << "Health"; return;
    case RequestReceiverContexts:
        w << "RequestReceiver"; return;
    case PerfCounters:
        w << "PerfCounters"; return;
    case LastReplicaUpState:
        w << "LastReplicaUpState"; return;
    case Diagnostics:
        w << "Diagnostics"; return;
    case RALifeCycleState:
        w << "RALifeCycleState"; return;default:
        Verify::Fail(wformatString("unknown enum StateItemhelper {0}", (int)e));
    }
}

FMMessageRetryHelper::FMMessageRetryHelper(
    ReconfigurationAgent& ra) : 
    ra_(ra),
    StateItemHelperBase(RAStateItem::FMMessageRetry)
{
}

void FMMessageRetryHelper::Reset()
{
}

void FMMessageRetryHelper::Request(Reliability::FailoverManagerId const & fm)
{
    ra_.GetFMMessageRetryComponent(fm).Request(DefaultActivityId);
}

LastReplicaUpStateHelper::LastReplicaUpStateHelper(ReconfigurationAgent & ra)
: StateItemHelperBase(RAStateItem::LastReplicaUpState),
  ra_(ra)
{
}

void LastReplicaUpStateHelper::Reset()
{
}


void LastReplicaUpStateHelper::ValidateLastReplicaUpReplyState(Reliability::FailoverManagerId const & fm, bool isReceived)
{
    auto actual = GetPendingReplicaUploadState(fm).IsComplete;
    Verify::AreEqual(isReceived, actual, wformatString("LastReplicaUp {0}", fm));
}

void LastReplicaUpStateHelper::ValidateToBeUploadedReplicaSetIsEmpty(FailoverManagerId const & fm)
{
    auto actual = GetPendingReplicaUploadState(fm).HasPendingReplicas;
    Verify::AreEqual(false, actual, wformatString("LastReplicaUp {0}: To be uploaded set is not empty", fm));
}

void LastReplicaUpStateHelper::Request(Reliability::FailoverManagerId const & fm)
{
    GetPendingReplicaUploadStateProcessor(fm).Test_OnTimer(L"a");
}

Node::PendingReplicaUploadState & LastReplicaUpStateHelper::GetPendingReplicaUploadState(Reliability::FailoverManagerId const & fm)
{
    return GetPendingReplicaUploadStateProcessor(fm).Test_GetPendingReplicaUploadState();
}

Node::PendingReplicaUploadStateProcessor & LastReplicaUpStateHelper::GetPendingReplicaUploadStateProcessor(Reliability::FailoverManagerId const & fm)
{
    return ra_.NodeStateObj.GetPendingReplicaUploadStateProcessor(fm);
}

RetryTimerStateHelper::RetryTimerStateHelper(Infrastructure::RetryTimer & timer, RAStateItem::Enum stateItem)
: timer_(timer),
  stateItem_(stateItem)
{
}

void RetryTimerStateHelper::CancelTimer()
{
    int64 seq = timer_.SequenceNumber;
    timer_.TryCancel(seq);
}

void RetryTimerStateHelper::ValidateTimerIsSet()
{
    Verify::IsTrueLogOnError(timer_.IsSet, wformatString("{0}: Timer Is Set"));
}

void RetryTimerStateHelper::ValidateTimerIsNotSet()
{
    Verify::IsTrueLogOnError(!timer_.IsSet, wformatString("{0}: Timer Is Not Set"));
}

void RetryTimerStateHelper::FireTimer()
{
    Assert::CodingError("To be impl");
}

void RetryTimerStateHelper::Reset()
{
}

void RetryTimerStateHelper::Set()
{
    timer_.Set();
}

MultipleFTBackgroundWorkManagerStateHelper::MultipleFTBackgroundWorkManagerStateHelper(RAStateItem::Enum item, Infrastructure::MultipleEntityBackgroundWorkManager & workManager)
: StateItemHelperBase(item),
  workManager_(workManager),
  timerStateHelper_(workManager.BackgroundWorkManager.RetryTimerObj, item)
{
}

void MultipleFTBackgroundWorkManagerStateHelper::ValidateIsSetButEmpty()
{
    timerStateHelper_.ValidateTimerIsSet();
    Verify::IsTrue(workManager_.FTSet.IsEmpty, wformatString("Set {0} Is Empty", stateItem_));
}

void MultipleFTBackgroundWorkManagerStateHelper::ValidateTimerIsSet()
{
    timerStateHelper_.ValidateTimerIsSet();
}

void MultipleFTBackgroundWorkManagerStateHelper::ValidateTimerIsNotSet()
{
    timerStateHelper_.ValidateTimerIsNotSet();
}

void MultipleFTBackgroundWorkManagerStateHelper::Reset()
{
    timerStateHelper_.Reset();
}

void MultipleFTBackgroundWorkManagerStateHelper::RequestWork()
{
    workManager_.BackgroundWorkManager.Request(DefaultActivityId);
}

BackgroundWorkManagerWithRetryStateHelper::BackgroundWorkManagerWithRetryStateHelper(RAStateItem::Enum item, Infrastructure::BackgroundWorkManagerWithRetry & bgmr)
: StateItemHelperBase(item),
  workManager_(bgmr),
  timerStateHelper_(bgmr.RetryTimerObj, item)
{
}

void BackgroundWorkManagerWithRetryStateHelper::Reset()
{
    timerStateHelper_.Reset();
}

void BackgroundWorkManagerWithRetryStateHelper::RequestWork()
{
    workManager_.Request(DefaultActivityId);
}

void BackgroundWorkManagerWithRetryStateHelper::ValidateTimerIsSet()
{
    timerStateHelper_.ValidateTimerIsSet();
}

void BackgroundWorkManagerWithRetryStateHelper::ValidateTimerIsNotSet()
{
    timerStateHelper_.ValidateTimerIsNotSet();
}

HostingStateHelper::HostingStateHelper(HostingStub & hosting)
: StateItemHelperBase(RAStateItem::Hosting),
  hosting_(hosting)
{
}

void HostingStateHelper::ValidateServiceModelUpgradeSpecification(ServiceModel::ApplicationUpgradeSpecification const & actual, size_t packageCountExpected, wstring const & tag)
{
    Verify::AreEqualLogOnError(Default::GetInstance().ApplicationIdentifier, actual.ApplicationId, wformatString("{0}: ApplicationId", tag));
    Verify::AreEqualLogOnError(Default::GetInstance().ApplicationName, actual.AppName, wformatString("{0}: AppName", tag));
    Verify::AreEqualLogOnError(Default::GetInstance().UpgradedApplicationVersion, actual.AppVersion, wformatString("{0}: AppVersion", tag));
    Verify::AreEqualLogOnError(packageCountExpected, actual.PackageUpgrades.size(), wformatString("{0}: Package Count", tag));
}

void HostingStateHelper::ValidateServiceModelUpgradePackageSpecification(std::vector<ServiceModel::ApplicationUpgradeSpecification> const & actual, size_t index, size_t packageIndex, ULONG rolloutMajorVersion, ULONG rolloutMinorVersion, wstring const & packageName, wstring const & tag)
{
    auto const & actualSpecification = actual[index].PackageUpgrades[packageIndex];

    Verify::AreEqualLogOnError(packageName, actualSpecification.ServicePackageName, wformatString("{0} {1} Package {2} Name", tag, index, packageIndex));
    Verify::AreEqualLogOnError(ServiceModel::RolloutVersion(rolloutMajorVersion, rolloutMinorVersion), actualSpecification.RolloutVersionValue, wformatString("{0} {1} Package {2} Version", tag, index, packageIndex));
}

void HostingStateHelper::ValidateDownloadCount(size_t expected)
{
    hosting_.DownloadApplicationAsyncApi.VerifyCallCount(expected);
}

void HostingStateHelper::ValidateDownloadSpecification(size_t downloadIndex, size_t packageCountExpected)
{
    auto const & actualDownloadCall = hosting_.DownloadApplicationAsyncApi.GetParametersAt(downloadIndex);
    Verify::AreEqualLogOnError(Default::GetInstance().ApplicationIdentifier, actualDownloadCall.ApplicationId, L"Download: ApplicationId");
    Verify::AreEqualLogOnError(Default::GetInstance().ApplicationName, actualDownloadCall.AppName, L"Download:AppName");
    Verify::AreEqualLogOnError(Default::GetInstance().UpgradedApplicationVersion, actualDownloadCall.AppVersion, L"Download:AppVersion");

    Verify::AreEqualLogOnError(packageCountExpected, actualDownloadCall.PackageDownloads.size(), L"Download: Package count");
}

void HostingStateHelper::ValidateDownloadPackageSpecification(size_t downloadIndex, size_t packageIndex, ULONG rolloutMajorVersion, ULONG rolloutMinorVersion, wstring const & packageName)
{
    auto const & actualSpecification = hosting_.DownloadApplicationAsyncApi.GetParametersAt(downloadIndex).PackageDownloads[packageIndex];
    Verify::AreEqualLogOnError(packageName, actualSpecification.ServicePackageName, wformatString("Download {0} Package {1} Name", downloadIndex, packageIndex));
    Verify::AreEqualLogOnError(ServiceModel::RolloutVersion(rolloutMajorVersion, rolloutMinorVersion), actualSpecification.RolloutVersionValue, wformatString("Download {0} Package {1} Version", downloadIndex, packageIndex));
}

void HostingStateHelper::ValidateUpgradeCount(size_t expected)
{
    hosting_.UpgradeApplicationAsyncApi.VerifyCallCount(expected);
}

void HostingStateHelper::ValidateUpgradeSpecification(size_t upgradeIndex, size_t packageCountExpected)
{
    auto const & actualUpgradeCall = hosting_.UpgradeApplicationAsyncApi.GetParametersAt(upgradeIndex);
    ValidateServiceModelUpgradeSpecification(actualUpgradeCall, packageCountExpected, L"Upgrade");
}

void HostingStateHelper::ValidateUpgradePackageSpecification(size_t upgradeIndex, size_t packageIndex, ULONG rolloutMajorVersion, ULONG rolloutMinorVersion, wstring const & packageName)
{
    ValidateServiceModelUpgradePackageSpecification(
        hosting_.UpgradeApplicationAsyncApi.GetParameterList(),
        upgradeIndex,
        packageIndex,
        rolloutMajorVersion,
        rolloutMinorVersion,
        packageName,
        L"Upgrade");
}

void HostingStateHelper::ValidateAnalyzeCount(size_t expected)
{
    Verify::AreEqualLogOnError(expected, hosting_.AnalyzeApplicationUpgradeCalls.size(), L"Analyze: count");
}

void HostingStateHelper::ValidateAnalyzeSpecification(size_t analyzeIndex, size_t packageCountExpected)
{
    auto const & actualCall = hosting_.AnalyzeApplicationUpgradeCalls[analyzeIndex];
    ValidateServiceModelUpgradeSpecification(actualCall, packageCountExpected, L"Analyze");
}

void HostingStateHelper::ValidateAnalyzePackageSpecification(size_t analyzeIndex, size_t packageIndex, ULONG rolloutMajorVersion, ULONG rolloutMinorVersion, wstring const & packageName)
{
    ValidateServiceModelUpgradePackageSpecification(
        hosting_.AnalyzeApplicationUpgradeCalls,
        analyzeIndex,
        packageIndex,
        rolloutMajorVersion,
        rolloutMinorVersion,
        packageName,
        L"Upgrade");
}

void HostingStateHelper::SetAllAsyncApisToCompleteSynchronouslyWithSuccess()
{
    hosting_.SetAllAsyncApisToCompleteSynchronouslyWithSuccess();
}

void HostingStateHelper::ValidateNoTerminateCalls()
{
    hosting_.TerminateServiceHostApi.VerifyNoCalls();
}

void HostingStateHelper::ValidateTerminateCall(std::wstring const & hostId)
{
    vector<wstring> hostIds;
    hostIds.push_back(hostId);
    ValidateTerminateCall(hostIds);
}

void HostingStateHelper::ValidateTerminateCall(std::vector<std::wstring> const & hostIds)
{
    hosting_.TerminateServiceHostApi.VerifyCallCount(hostIds.size());
    if (hostIds.size() != hosting_.TerminateServiceHostApi.CallList.size())
    {
        return;
    }

    vector<wstring> actual;
    for (auto const & it : hosting_.TerminateServiceHostApi.CallList)
    {
        actual.push_back(it->Parameters);
    }

    Verify::VectorStrict(hostIds, actual);
}

void HostingStateHelper::Reset()
{
}

ReliabilityStateHelper::ReliabilityStateHelper(ReliabilitySubsystemStub & reliability)
: StateItemHelperBase(RAStateItem::Reliability),
  reliability_(reliability)
{
}

void ReliabilityStateHelper::Reset()
{
    reliability_.Reset();
}

void ReliabilityStateHelper::ValidateNodeUpToFM() const
{
    Verify::AreEqual(1, reliability_.NodeUpToFMCount, L"NodeUpToFM");
}

void ReliabilityStateHelper::ValidateNodeUpToFMM() const
{
    Verify::AreEqual(1, reliability_.NodeUpToFMMCount, L"NodeUpToFMM");
}

void ReliabilityStateHelper::ValidateLfumUploadCount(GenerationNumberSource::Enum source)
{
    std::vector<ReliabilitySubsystemStub::UploadLfumInfo> const & v = GetVector(source);

    Verify::AreEqual(1, v.size(), L"Lfum Uploads Count");
}

void ReliabilityStateHelper::ValidateLfumUpload(GenerationNumberSource::Enum source, GenerationInfo info, int replicaCountExpected, bool anyReplicaFoundExpected)
{
    std::vector<ReliabilitySubsystemStub::UploadLfumInfo> const & v = GetVector(source);

    Verify::AreEqual(1, v.size(), L"Lfum Uploads Count");
    Verify::AreEqual(info.Number, v[0].GenerationNumber, L"Generation Number");
    Verify::AreEqual(replicaCountExpected, v[0].FailoverUnits.size(), L"FT Size");
    Verify::AreEqual(anyReplicaFoundExpected, v[0].AnyReplicaFound, L"AnyReplicaFound");
}

void ReliabilityStateHelper::ValidateLfumUploadData(GenerationNumberSource::Enum source, size_t index, FailoverUnitInfo const & ftInfoExpected)
{
    auto const & v = GetVector(source);

    if (v.size() != 1)
    {
        Verify::Fail(L"Expect only one call to UploadLfum here");
        return;
    }

    if (v[0].FailoverUnits.size() <= index)
    {
        Verify::Fail(wformatString("Too few FTs. Index = {0}. Size = {1}", index, v.size()));
        return;
    }

    auto const & actual = v[0].FailoverUnits[index];

    Verify::Compare(ftInfoExpected, actual, L"ValidateLfumData: FTInfo");
}

void ReliabilityStateHelper::ValidateNoLfumUpload(GenerationNumberSource::Enum source)
{
    std::vector<ReliabilitySubsystemStub::UploadLfumInfo> const & v = GetVector(source);

    Verify::AreEqual(0, v.size(), L"Lfum Uploads Count");
}

std::vector<ReliabilitySubsystemStub::UploadLfumInfo> const & ReliabilityStateHelper::GetVector(GenerationNumberSource::Enum source)
{
    if (source == GenerationNumberSource::FMM)
    {
        return reliability_.FmmLfumUploads;
    }
    else
    {
        return reliability_.FMLfumUploads;
    }
}


RAPMessageStateItemHelper::RAPMessageStateItemHelper(RaToRapTransportStub & rapTransport)
: StateItemHelperBase(RAStateItem::RAPMessages),
  rapTransport_(rapTransport)
{
}

void RAPMessageStateItemHelper::Reset()
{
    rapTransport_.Messages.clear();
}

FMMessageStateItemHelper::FMMessageStateItemHelper(FederationWrapperStub & federation, bool isFMM)
: StateItemHelperBase(isFMM ? RAStateItem::FMMMessages : RAStateItem::FMMessages),
  isFMM_(isFMM),
  federation_(federation)
{
}

void FMMessageStateItemHelper::Reset()
{
    if (isFMM_)
    {
        federation_.FmmMessages.clear();
    }
    else
    {
        federation_.FmMessages.clear();
    }
}

RAMessageStateItemHelper::RAMessageStateItemHelper(FederationWrapperStub & federation)
: StateItemHelperBase(RAStateItem::RAMessages),
  federation_(federation)
{
}

void RAMessageStateItemHelper::Reset()
{
    federation_.OtherNodeMessages.clear();
}

EventWriterStateItemHelper::EventWriterStateItemHelper(Diagnostics::IEventWriter & writer)
    :writer_(dynamic_cast<EventWriterStub&>(writer)),
    StateItemHelperBase(RAStateItem::Diagnostics)
{
}

void EventWriterStateItemHelper::Reset()
{
    writer_.TracedEvents.clear();
}

bool EventWriterStateItemHelper::IsTracedEventsEmpty()
{
    return writer_.TracedEvents.empty();
}

HealthStateItemHelper::HealthStateItemHelper(Health::IHealthSubsystemWrapper & health)
: health_(dynamic_cast<HealthSubsystemWrapperStub&>(health)),
  StateItemHelperBase(RAStateItem::Health)
{
}

HealthSubsystemWrapperStub::ReplicaHealthEventInfo const & HealthStateItemHelper::GetLastReplicaHealthEvent()
{
    return health_.ReplicaHealthEvents[0];
}

void HealthStateItemHelper::Reset()
{
    health_.ReplicaHealthEvents.clear();
}

void HealthStateItemHelper::ValidateNoReplicaHealthEvent()
{
    Verify::IsTrueLogOnError(health_.ReplicaHealthEvents.empty(), L"IsEmpty");
}


void HealthStateItemHelper::ValidateReplicaHealthEvent(
    Health::ReplicaHealthEvent::Enum type,
    FailoverUnitId const & ftId,
    bool isStateful,
    uint64 replicaId,
    uint64 instanceId)
{
    ValidateReplicaHealthEvent(type, ftId, isStateful, replicaId, instanceId, ErrorCodeValue::Success);
}

void HealthStateItemHelper::ValidateReplicaHealthEvent(
    Health::ReplicaHealthEvent::Enum type,
    FailoverUnitId const & ftId,
    bool isStateful,
    uint64 replicaId,
    uint64 instanceId,
    ErrorCodeValue::Enum)
{
    auto size = health_.ReplicaHealthEvents.size();
    Verify::IsTrueLogOnError(1 == size, L"HealthEvent: Count");
    if (health_.ReplicaHealthEvents.empty())
    {
        return;
    }

    HealthSubsystemWrapperStub::ReplicaHealthEventInfo const & actual = health_.ReplicaHealthEvents[0];
    Verify::AreEqualLogOnError(ftId, actual.FTId, L"HealthEvent: FT Id");
    Verify::AreEqualLogOnError(isStateful, actual.IsStateful, L"HealthEvent: IsStateful");
    Verify::AreEqualLogOnError(replicaId, actual.ReplicaId, L"HealthEvent: ReplicaId");
    Verify::AreEqualLogOnError(instanceId, actual.InstanceId, L"HealthEvent: InstanceId");
    Verify::AreEqualLogOnError(type, actual.Type, L"HealthEvent: Type");

}

RequestReceiverContextStateHelper::RequestReceiverContextStateHelper(FederationWrapperStub & federation)
: federation_(federation),
  StateItemHelperBase(StateItemHelper::RAStateItem::RequestReceiverContexts)
{
}

void RequestReceiverContextStateHelper::ValidateError(wstring const & tag, Common::ErrorCode const & expected)
{
    RequestReceiverContextState * state = FindTag(tag);

    if (state == nullptr)
    {
        return;
    }

    if (state->State != RequestReceiverContextState::Rejected)
    {
        Verify::Fail(wformatString("Request Receiver context for {0} is not rejected", tag));
        return;
    }

    Verify::AreEqualLogOnError(wformatString(expected), wformatString(state->RejectErrorCode));
}

RequestReceiverContextState * RequestReceiverContextStateHelper::FindTag(wstring const & tag)
{
    FederationWrapperStub::RequestReceiverMap & m = federation_.RequestReceiverState;
    for(auto it = begin(m); it != end(m); ++it)
    {
        if (it->second->Tag == tag)
        {
            return it->second.get();
        }
    }

    Verify::Fail(wformatString("Unable to find tag for RequestReceiverContext: {0}", tag));
    return nullptr;
}

void RequestReceiverContextStateHelper::Reset()
{
}

GenerationStateHelper::GenerationStateHelper(ReconfigurationAgent & ra)
: StateItemHelperBase(RAStateItem::Generation),
  ra_(ra)
{
}

void GenerationStateHelper::VerifyGeneration(GenerationNumberSource::Enum source, int nodeId, int sendGeneration, int receiveGeneration, int proposedGeneration)
{
    VerifyGeneration(
        source,
        GenerationInfo(sendGeneration, nodeId),
        GenerationInfo(receiveGeneration, nodeId),
        GenerationInfo(proposedGeneration, nodeId));
}

void GenerationStateHelper::VerifyGeneration(GenerationNumberSource::Enum source, GenerationInfo sendGeneration, GenerationInfo receiveGeneration, GenerationInfo proposedGeneration)
{
    VerifySendGeneration(source, sendGeneration);
    VerifyReceiveGeneration(source, receiveGeneration);
    VerifyProposedGeneration(source, proposedGeneration);
}

void GenerationStateHelper::VerifySendGeneration(GenerationNumberSource::Enum source, GenerationInfo sendGeneration)
{
    VerifyGeneration(sendGeneration, GetGeneration(source).SendGeneration);
}

void GenerationStateHelper::VerifyReceiveGeneration(GenerationNumberSource::Enum source, GenerationInfo receiveGeneration)
{
    VerifyGeneration(receiveGeneration, GetGeneration(source).ReceiveGeneration);
}

void GenerationStateHelper::VerifyProposedGeneration(GenerationNumberSource::Enum source, GenerationInfo proposedGeneration)
{
    VerifyGeneration(proposedGeneration, GetGeneration(source).ProposedGeneration);
}

void GenerationStateHelper::SetGeneration(GenerationNumberSource::Enum source, int nodeId, int sendGeneration, int receiveGeneration, int proposedGeneration)
{
    SetGeneration(
        source,
        GenerationInfo(sendGeneration, nodeId),
        GenerationInfo(receiveGeneration, nodeId),
        GenerationInfo(proposedGeneration, nodeId));
}

void GenerationStateHelper::SetGeneration(GenerationNumberSource::Enum source, GenerationInfo sendGeneration, GenerationInfo receiveGeneration, GenerationInfo proposedGeneration)
{
    SetSendGeneration(source, sendGeneration);
    SetReceiveGeneration(source, receiveGeneration);
    SetProposedGeneration(source, proposedGeneration);
}

void GenerationStateHelper::SetSendGeneration(GenerationNumberSource::Enum source, GenerationInfo sendGeneration)
{
    GetGeneration(source).SendGeneration = sendGeneration.Number;
}

void GenerationStateHelper::SetReceiveGeneration(GenerationNumberSource::Enum source, GenerationInfo receiveGeneration)
{
    GetGeneration(source).ReceiveGeneration = receiveGeneration.Number;
}

void GenerationStateHelper::SetProposedGeneration(GenerationNumberSource::Enum source, GenerationInfo proposedGeneration)
{
    GetGeneration(source).ProposedGeneration = proposedGeneration.Number;
}

void GenerationStateHelper::Reset()
{
}

GenerationNumberSource::Enum GenerationStateHelper::GetOther(GenerationNumberSource::Enum source) const
{
    return source == GenerationNumberSource::FMM ? GenerationNumberSource::FM : GenerationNumberSource::FMM;
}

void GenerationStateHelper::VerifyGeneration(GenerationInfo const & expected, GenerationNumber const & actual)
{
    Verify::AreEqual(expected.Number, actual);
}

void GenerationStateHelper::SetGeneration(GenerationInfo const & value , GenerationNumber & actual)
{
    actual = value.Number;
}

GenerationState & GenerationStateHelper::GetGeneration(GenerationNumberSource::Enum source)
{
    return ra_.GenerationStateManagerObj.Test_GetGenerationState(source == GenerationNumberSource::FMM);
}

PerformanceCounterHelper::PerformanceCounterHelper(ReconfigurationAgent & ra) :
    StateItemHelperBase(RAStateItem::PerfCounters),
    perfCounters_(ra.PerfCountersSPtr)
{
}

PerformanceCounterHelper::PerformanceCounterHelper(Diagnostics::PerformanceCountersSPtr perfCounters) :
    StateItemHelperBase(RAStateItem::PerfCounters),
    perfCounters_(perfCounters)
{
}

void PerformanceCounterHelper::VerifyAveragePerformanceCounter(
    Common::PerformanceCounterData & base,
    Common::PerformanceCounterData & value,
    int expected,
    std::wstring const & name)
{
    Verify::AreEqual(1, base.RawValue, name + L"base");
    Verify::AreEqual(expected, value.RawValue, name + L"value");
}

void PerformanceCounterHelper::VerifyAveragePerformanceCounterUnchanged(
    Common::PerformanceCounterData & base,
    Common::PerformanceCounterData & value,
    std::wstring const & name)
{
    Verify::AreEqual(0, base.RawValue, name + L"base");
    Verify::AreEqual(0, value.RawValue, name + L"value");
}


void PerformanceCounterHelper::Reset()
{
}

RALifeCycleStateHelper::RALifeCycleStateHelper(ReconfigurationAgent & ra) :
ra_(ra),
StateItemHelperBase(RAStateItem::RALifeCycleState)
{
}

void RALifeCycleStateHelper::SetState(RALifeCycleState::Enum e)
{
    auto & stateObj = ra_.StateObj;

    bool isOpen = false;
    bool isClosing = false;

    switch (e)
    {
    case RALifeCycleState::Closed:        
        break;

    case RALifeCycleState::Open:
        isOpen = true;
        break;

    case RALifeCycleState::Closing:
        isOpen = false;
        isClosing = true;
        break;

    default:
        Assert::CodingError("unknown rastate {0}", static_cast<int>(e));
    }


    stateObj.Test_SetIsOpen(isOpen);
    stateObj.Test_SetIsClosing(isClosing);
    
    if (!isOpen && !isClosing)
    {
        stateObj.Test_SetNodeUpAckFromFMProcessed(false);
        stateObj.Test_SetNodeUpAckFromFmmProcessed(false);
    }
}

void RALifeCycleStateHelper::Reset()
{
}
