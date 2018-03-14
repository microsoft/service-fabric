// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ReconfigurationState
        {
        public:
            ReconfigurationState(
                FailoverUnitDescription const * ftDesc, 
                ServiceDescription const * sdDesc);

            ReconfigurationState(
                FailoverUnitDescription const * ftDesc,
                ServiceDescription const * sdDesc, 
                ReconfigurationState const & other);

            ReconfigurationState & operator=(ReconfigurationState const & other);

            __declspec(property(get = get_ReconfigurationStage)) FailoverUnitReconfigurationStage::Enum ReconfigurationStage;
            FailoverUnitReconfigurationStage::Enum get_ReconfigurationStage() const { return stage_; }
            void Test_SetReconfigurationStage(FailoverUnitReconfigurationStage::Enum value) { stage_ = value; }

            __declspec(property(get = get_IsCatchupReconfigurationStage)) bool IsCatchupReconfigurationStage;
            bool get_IsCatchupReconfigurationStage() const
            {
                return stage_ == FailoverUnitReconfigurationStage::Phase0_Demote || stage_ == FailoverUnitReconfigurationStage::Phase2_Catchup;
            }

            __declspec(property(get = get_IsReconfiguring)) bool IsReconfiguring;
            bool get_IsReconfiguring() const { return stage_ != FailoverUnitReconfigurationStage::None; }

            __declspec(property(get = get_IsSwapPrimary)) bool IsSwapPrimary;
            bool get_IsSwapPrimary() const { return type_ == ReconfigurationType::SwapPrimary; }

            __declspec(property(get = get_Phase0Duration)) Common::TimeSpan Phase0Duration;
            Common::TimeSpan get_Phase0Duration() const { return perfData_.Phase0Duration; }

            __declspec(property(get = get_StartTime)) Common::StopwatchTime StartTime;
            Common::StopwatchTime get_StartTime() const { return perfData_.StartTime; }
            void Test_SetStartTime(Common::StopwatchTime now) { perfData_.Test_SetStartTime(now); }

            __declspec(property(get = get_PhaseStartTime)) Common::StopwatchTime PhaseStartTime;
            Common::StopwatchTime get_PhaseStartTime() const { return perfData_.PhaseStartTime; }

            __declspec(property(get = get_ReconfigType)) ReconfigurationType::Enum ReconfigType;
            ReconfigurationType::Enum get_ReconfigType() const { return type_; }
            void Test_SetReconfigType(ReconfigurationType::Enum value) { type_ = value; }

            __declspec(property(get = get_Result)) ReconfigurationResult::Enum Result;
            ReconfigurationResult::Enum get_Result() const { return result_; }

            // NOTE: This is measured based on when the reconfiguration started on this node
            // In the case of swap the reconfiguration starts on the old primary and continues on the new primary
            // It is treated as effectively a new reconfig on the new primary
            Common::TimeSpan GetCurrentReconfigurationDuration(Infrastructure::IClock const & clock) const;

            Common::TimeSpan GetCurrentReconfigurationPhaseTimeElapsed(Infrastructure::IClock const & clock) const;

            void Reset();
            
            void Finish(
                Federation::NodeInstance const & nodeInstance,
                Infrastructure::IClock const & clock,
                Infrastructure::StateMachineActionQueue & queue);

            void FinishWithChangeConfiguration(
                Federation::NodeInstance const & nodeInstance,
                Infrastructure::IClock const & clock,
                Infrastructure::StateMachineActionQueue & queue);

            void FinishAbortSwapPrimary(
                Federation::NodeInstance const & nodeInstance,
                Infrastructure::IClock const & clock,
                Infrastructure::StateMachineActionQueue & queue);

            void FinishDemote(
                Infrastructure::IClock const & clock);

            // Use this to start a reconfguration
            // phase0Duration is ignored in all cases except swap primary
            // If it is max value and swap then it is running on the old primary
            // Else it is a continue swap or the promote part of swap primary
            void Start(
                ReconfigurationType::Enum reconfigType,
                Common::TimeSpan phase0Duration,
                Infrastructure::IClock const & clock);

            void Start(
                bool isPromotePrimary,
                bool isSwapPrimary,
                bool isContinueSwapPrimary,
                Common::TimeSpan phase0Duration,
                Infrastructure::IClock const & clock);

            void StartPhase2Catchup(Infrastructure::IClock const & clock);
            void StartAbortPhase0Demote(Infrastructure::IClock const & clock);
            void StartPhase3Deactivate(Infrastructure::IClock const & clock);
            void StartPhase4Activate(Infrastructure::IClock const & clock);

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void AssertInvariants(FailoverUnit const & owner) const;

        private:
            void AssertIfNot(FailoverUnitReconfigurationStage::Enum expected) const;
            
            void SetState(
                FailoverUnitReconfigurationStage::Enum stage, 
                Infrastructure::IClock const & clock);

            void TraceAndReset(
                Federation::NodeInstance const & nodeInstance,
                ReconfigurationResult::Enum result,
                Infrastructure::StateMachineActionQueue & queue);

            void Reset(
                ReconfigurationResult::Enum result);

            FailoverUnitReconfigurationStage::Enum stage_;
            ReconfigurationType::Enum type_;
            Diagnostics::ReconfigurationPerformanceData perfData_;
            ReconfigurationResult::Enum result_;
            
            Reliability::FailoverUnitDescription const * ftDesc_;
            Reliability::ServiceDescription const * sdDesc_;
        };
    }
}



