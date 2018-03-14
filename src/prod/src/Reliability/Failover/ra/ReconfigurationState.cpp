// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"
#include "Diagnostics.FailoverUnitEventData.h"
#include "Diagnostics.TraceEventStateMachineAction.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

ReconfigurationState::ReconfigurationState(
    FailoverUnitDescription const * ftDesc,
    ServiceDescription const * sdDesc) :
    sdDesc_(sdDesc),
    ftDesc_(ftDesc)
{
    ASSERT_IF(sdDesc == nullptr || ftDesc == nullptr, "ft/sd desc is null");
    Reset();
}

ReconfigurationState::ReconfigurationState(
    FailoverUnitDescription const * ftDesc,
    ServiceDescription const * sdDesc, 
    ReconfigurationState const & other) :
    ftDesc_(ftDesc),
    sdDesc_(sdDesc),
    stage_(other.stage_),
    type_(other.type_),
    perfData_(other.perfData_),
    result_(other.result_)
{
    ASSERT_IF(sdDesc == nullptr || ftDesc == nullptr, "ft/sd desc is null");
}

ReconfigurationState & ReconfigurationState::operator=(ReconfigurationState const & other)
{
    if (this != &other)
    {
        stage_ = other.stage_;
        type_ = other.type_;
        perfData_ = other.perfData_;
        result_ = other.result_;
    }

    return *this;
}

void ReconfigurationState::Reset()
{
    Reset(ReconfigurationResult::Invalid);
}

void ReconfigurationState::Reset(ReconfigurationResult::Enum result)
{
    stage_ = FailoverUnitReconfigurationStage::None;
    type_ = ReconfigurationType::Other;
    perfData_ = Diagnostics::ReconfigurationPerformanceData();
    result_ = result;
}

TimeSpan ReconfigurationState::GetCurrentReconfigurationDuration(IClock const & clock) const
{
    return perfData_.GetElapsedTime(clock);
}

Common::TimeSpan ReconfigurationState::GetCurrentReconfigurationPhaseTimeElapsed(Infrastructure::IClock const & clock) const
{
    return perfData_.GetCurrentPhaseElapsedTime(clock);
}

void ReconfigurationState::Start(
    ReconfigurationType::Enum reconfigurationType,
    Common::TimeSpan phase0Duration,
    Infrastructure::IClock const & clock)
{
    Common::TimeSpan actualPhase0Duration;
    FailoverUnitReconfigurationStage::Enum reconfigStage = FailoverUnitReconfigurationStage::Phase1_GetLSN;

    switch (reconfigurationType)
    {
    case ReconfigurationType::Failover:
        reconfigStage = FailoverUnitReconfigurationStage::Phase1_GetLSN;
        actualPhase0Duration = TimeSpan::Zero;
        break;

    case ReconfigurationType::Other:
        reconfigStage = FailoverUnitReconfigurationStage::Phase2_Catchup;
        actualPhase0Duration = TimeSpan::Zero;
        break;

    case ReconfigurationType::SwapPrimary:
        if (phase0Duration == TimeSpan::MaxValue)
        {
            actualPhase0Duration = TimeSpan::Zero;
            reconfigStage = FailoverUnitReconfigurationStage::Phase0_Demote;
        }
        else
        {
            actualPhase0Duration = phase0Duration;
            reconfigStage = FailoverUnitReconfigurationStage::Phase1_GetLSN;
        }
        break;

    default:
        Assert::CodingError("Unknown reconfiguration type {0}", reconfigurationType);
    }

    AssertIfNot(FailoverUnitReconfigurationStage::None);

    Reset();

    type_ = reconfigurationType;

    perfData_.OnStarted(actualPhase0Duration, clock);
    stage_ = reconfigStage;
}


void ReconfigurationState::Start(
    bool isPromotePrimary,
    bool isSwapPrimary,
    bool isContinueSwapPrimary,
    Common::TimeSpan phase0Duration,
    Infrastructure::IClock const & clock)
{
    TESTASSERT_IF(isContinueSwapPrimary && isSwapPrimary, "Cannot be both continue swap and swap {0}", *ftDesc_);
    TESTASSERT_IF(isContinueSwapPrimary && !isPromotePrimary, "Continue swap is a promote on this node {0}", *ftDesc_);

    ReconfigurationType::Enum type = ReconfigurationType::Other;

    if (isPromotePrimary)
    {
        if (isContinueSwapPrimary)
        {
            type = ReconfigurationType::SwapPrimary;
        }
        else
        {
            type = ReconfigurationType::Failover;
        }
    }
    else
    {
        if (isSwapPrimary)
        {
            type = ReconfigurationType::SwapPrimary;
        }
        else
        {
            type = ReconfigurationType::Other;
        }
    }

    Start(type, phase0Duration, clock);
}

void ReconfigurationState::FinishDemote(
    Infrastructure::IClock const & clock)
{
    auto phase0Duration = GetCurrentReconfigurationDuration(clock);

    Reset(ReconfigurationResult::DemoteCompleted);

    perfData_.Phase0Duration = phase0Duration;
}

void ReconfigurationState::StartPhase2Catchup(IClock const & clock)
{
    SetState(FailoverUnitReconfigurationStage::Phase2_Catchup, clock);
}

void ReconfigurationState::StartAbortPhase0Demote(IClock const & clock)
{
    AssertIfNot(FailoverUnitReconfigurationStage::Phase0_Demote);
    SetState(FailoverUnitReconfigurationStage::Abort_Phase0_Demote, clock);
}

void ReconfigurationState::StartPhase3Deactivate(IClock const & clock)
{
    AssertIfNot(FailoverUnitReconfigurationStage::Phase2_Catchup);
    SetState(FailoverUnitReconfigurationStage::Phase3_Deactivate, clock);
}

void ReconfigurationState::StartPhase4Activate(IClock const & clock)
{
    TESTASSERT_IF(stage_ != FailoverUnitReconfigurationStage::Phase3_Deactivate && stage_ != FailoverUnitReconfigurationStage::Phase2_Catchup, "Unknown reconfig state on transition to phase4");
    SetState(FailoverUnitReconfigurationStage::Phase4_Activate, clock);
}

void ReconfigurationState::Finish(
    Federation::NodeInstance const & nodeInstance,
    IClock const & clock,
    StateMachineActionQueue & queue)
{
    AssertIfNot(FailoverUnitReconfigurationStage::Phase4_Activate);
    perfData_.OnFinishedAtPhase4(clock);

    TraceAndReset(nodeInstance, ReconfigurationResult::Completed, queue);
}

void ReconfigurationState::FinishWithChangeConfiguration(
    Federation::NodeInstance const & nodeInstance,
    IClock const & clock,
    StateMachineActionQueue & queue)
{
    AssertIfNot(FailoverUnitReconfigurationStage::Phase1_GetLSN);
    perfData_.OnFinishedAtPhase1(clock);

    TraceAndReset(nodeInstance, ReconfigurationResult::ChangeConfiguration, queue);
}

void ReconfigurationState::FinishAbortSwapPrimary(
    Federation::NodeInstance const & nodeInstance,
    IClock const & clock,
    StateMachineActionQueue & queue)
{
    AssertIfNot(FailoverUnitReconfigurationStage::Abort_Phase0_Demote);
    perfData_.OnFinishedAtPhase0(clock);

    TraceAndReset(nodeInstance, ReconfigurationResult::AbortSwapPrimary, queue);
}

void ReconfigurationState::SetState(FailoverUnitReconfigurationStage::Enum stage, IClock const & clock)
{
    TESTASSERT_IF(stage == FailoverUnitReconfigurationStage::None, "Cannot set to none. Use finish");

    switch (stage_)
    {
    case FailoverUnitReconfigurationStage::Phase1_GetLSN:
        perfData_.OnPhase1Finished(clock);
        break;
    case FailoverUnitReconfigurationStage::Phase2_Catchup:
        perfData_.OnPhase2Finished(clock);
        break;
    case FailoverUnitReconfigurationStage::Phase3_Deactivate:
        perfData_.OnPhase3Finished(clock);
        break;
    }

    stage_ = stage;
}

void ReconfigurationState::TraceAndReset(Federation::NodeInstance const & nodeInstance, ReconfigurationResult::Enum result, StateMachineActionQueue & queue)
{
    TESTASSERT_IF(result == ReconfigurationResult::Invalid || result == ReconfigurationResult::DemoteCompleted, "cannot be tracing DemoteCompleted or Invalid");
    Diagnostics::ReconfigurationCompleteEventData data(
        ftDesc_->FailoverUnitId.Guid,
        nodeInstance,
        sdDesc_->Type.ServiceTypeName,
        ftDesc_->CurrentConfigurationEpoch,
        type_,
        result,
        perfData_);

    auto action = make_unique<Diagnostics::TraceEventStateMachineAction<Diagnostics::ReconfigurationCompleteEventData>>(move(data));
    queue.Enqueue(move(action));

    Reset(result);
}

void ReconfigurationState::AssertIfNot(FailoverUnitReconfigurationStage::Enum stage) const
{
    TESTASSERT_IF(stage != stage_, "Unexpected reconfig state. Expected = {0}. Actual = {1}", stage, stage_);
}

void ReconfigurationState::AssertInvariants(FailoverUnit const & owner) const
{
    TESTASSERT_IF(owner.IsClosed && IsReconfiguring, "Cannot be reconfiguring when closed {0}", owner);
    TESTASSERT_IF(type_ != ReconfigurationType::Other && !IsReconfiguring, "Reconfig type must be other {0}", owner);

    //TODO: add assert for starttime + phasestarttime
}

string ReconfigurationState::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "{0}";
    size_t index = 0;

    traceEvent.AddEventField<FailoverUnitReconfigurationStage::Trace>(format, name + ".reconfigStage", index);

    return format;
}

void ReconfigurationState::FillEventData(Common::TraceEventContext & context) const
{
    context.WriteCopy<uint>(static_cast<uint>(stage_));
}

void ReconfigurationState::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write(stage_);
}
