// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            class ReconfigurationPerformanceData
            {
            public:
                ReconfigurationPerformanceData() :
                    phase0Duration_(Common::TimeSpan::Zero),
                    phase1Duration_(Common::TimeSpan::Zero),
                    phase2Duration_(Common::TimeSpan::Zero),
                    phase3Duration_(Common::TimeSpan::Zero),
                    phase4Duration_(Common::TimeSpan::Zero)
                {
                }

                __declspec(property(get = get_Phase0Duration, put = set_Phase0Duration)) Common::TimeSpan Phase0Duration;
                Common::TimeSpan get_Phase0Duration() const { return phase0Duration_; }
                void set_Phase0Duration(Common::TimeSpan const& value) { phase0Duration_ = value; }

                __declspec(property(get = get_Phase1Duration)) Common::TimeSpan Phase1Duration;
                Common::TimeSpan get_Phase1Duration() const { return phase1Duration_; }

                __declspec(property(get = get_Phase2Duration)) Common::TimeSpan Phase2Duration;
                Common::TimeSpan get_Phase2Duration() const { return phase2Duration_; }

                __declspec(property(get = get_Phase3Duration)) Common::TimeSpan Phase3Duration;
                Common::TimeSpan get_Phase3Duration() const { return phase3Duration_; }

                __declspec(property(get = get_Phase4Duration)) Common::TimeSpan Phase4Duration;
                Common::TimeSpan get_Phase4Duration() const { return phase4Duration_; }

                __declspec(property(get = get_TotalDuration)) Common::TimeSpan TotalDuration;
                Common::TimeSpan get_TotalDuration() const { return totalDuration_; }

                __declspec(property(get = get_StartTime)) Common::StopwatchTime StartTime;
                Common::StopwatchTime get_StartTime() const { return startTime_; }
                void Test_SetStartTime(Common::StopwatchTime now) { startTime_ = now; phaseStartTime_ = now; }

                __declspec(property(get = get_PhaseStartTime)) Common::StopwatchTime PhaseStartTime;
                Common::StopwatchTime get_PhaseStartTime() const { return phaseStartTime_; }

                Common::TimeSpan GetCurrentPhaseElapsedTime(Common::StopwatchTime now) const
                {
                    if (phaseStartTime_ == Common::StopwatchTime::Zero)
                    {
                        Common::Assert::TestAssert("asked to get elapsed time when not reconfiguring");
                        return Common::TimeSpan::Zero;
                    }

                    return now - phaseStartTime_;
                }

                Common::TimeSpan GetCurrentPhaseElapsedTime(Infrastructure::IClock const & clock) const
                {
                    return GetCurrentPhaseElapsedTime(clock.Now());
                }

                // Elapsed time is always the time measured from this machine
                Common::TimeSpan GetElapsedTime(Common::StopwatchTime now) const
                {
                    if (startTime_ == Common::StopwatchTime::Zero)
                    {
                        Common::Assert::TestAssert("asked to get elapsed time when not reconfiguring");
                        return Common::TimeSpan::Zero;
                    }

                    return now - startTime_;
                }

                Common::TimeSpan GetElapsedTime(Infrastructure::IClock const & clock) const
                {
                    return GetElapsedTime(clock.Now());
                }

                void OnStarted(Common::TimeSpan phase0Duration, Infrastructure::IClock const & clock)
                {
                    AssertIsNotStarted();

                    auto now = clock.Now();
                    phaseStartTime_ = now;
                    startTime_ = now;
                    phase0Duration_ = phase0Duration;
                }

                void OnPhase1Finished(Infrastructure::IClock const & clock)
                {
                    UpdatePhaseTime(clock, phase1Duration_);
                }
                
                void OnPhase2Finished(Infrastructure::IClock const & clock)
                {
                    UpdatePhaseTime(clock, phase2Duration_);
                }

                void OnPhase3Finished(Infrastructure::IClock const & clock)
                {
                    UpdatePhaseTime(clock, phase3Duration_);
                }

                void OnFinishedAtPhase0(Infrastructure::IClock const & clock)
                {
                    UpdatePhaseTimeAndFinish(clock, phase0Duration_, false);
                }

                void OnFinishedAtPhase4(Infrastructure::IClock const & clock)
                {
                    UpdatePhaseTimeAndFinish(clock, phase4Duration_, true);
                }

                void OnFinishedAtPhase1(Infrastructure::IClock const & clock)
                {
                    UpdatePhaseTimeAndFinish(clock, phase1Duration_, true);
                }

                static std::string AddField(Common::TraceEvent & traceEvent, std::string const &)
                {
                    size_t index = 0;
                    traceEvent.AddEventField<double>("phase0DurationMs", index);
                    traceEvent.AddEventField<double>("phase1DurationMs", index);
                    traceEvent.AddEventField<double>("phase2DurationMs", index);
                    traceEvent.AddEventField<double>("phase3DurationMs", index);
                    traceEvent.AddEventField<double>("phase4DurationMs", index);
                    traceEvent.AddEventField<double>("totalDurationMs", index);

                    return "Phase0Duration: {0}ms. Phase1Duration: {1}ms. Phase2Duration: {2}ms. Phase3Duration: {3}ms. Phase4Duration: {4}ms. TotalDuration: {5}ms. ";
                }

                void FillEventData(Common::TraceEventContext & context) const
                {
                    context.WriteCopy<double>(phase0Duration_.TotalMillisecondsAsDouble()); 
                    context.WriteCopy<double>(phase1Duration_.TotalMillisecondsAsDouble());
                    context.WriteCopy<double>(phase2Duration_.TotalMillisecondsAsDouble());
                    context.WriteCopy<double>(phase3Duration_.TotalMillisecondsAsDouble());
                    context.WriteCopy<double>(phase4Duration_.TotalMillisecondsAsDouble());
                    context.WriteCopy<double>(totalDuration_.TotalMillisecondsAsDouble());
                }

                void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
                {
                    auto msg = Common::wformatString(
                        "Phase0Duration: {0}ms. Phase1Duration: {1}ms. Phase2Duration: {2}ms. Phase3Duration: {3}ms. Phase4Duration: {4}ms. TotalDuration: {5}ms. ",
                        phase0Duration_.TotalMillisecondsAsDouble(),
                        phase1Duration_.TotalMillisecondsAsDouble(),
                        phase2Duration_.TotalMillisecondsAsDouble(),
                        phase3Duration_.TotalMillisecondsAsDouble(),
                        phase4Duration_.TotalMillisecondsAsDouble(),
                        totalDuration_.TotalMillisecondsAsDouble());

                    w << msg;
                }

            private:
                void AssertIsStarted() const
                {
                    TESTASSERT_IF(startTime_ == Common::StopwatchTime::Zero, "Should be started");
                }

                void AssertIsNotStarted() const
                {
                    TESTASSERT_IF(startTime_ != Common::StopwatchTime::Zero, "Should not be started");
                }

                void AssertIsFinished() const
                {
                    TESTASSERT_IF(totalDuration_ == Common::TimeSpan::Zero, "Should have finished");
                }

                Common::StopwatchTime UpdatePhaseTime(Infrastructure::IClock const & clock, Common::TimeSpan & phaseDuration)
                {
                    AssertIsStarted();
                    TESTASSERT_IF(phaseDuration != Common::TimeSpan::Zero, "Updating a phase duration which is non zero");
					StopwatchTime now = clock.Now();
                    phaseDuration = now - phaseStartTime_;
                    phaseStartTime_ = now;
                    return now;
                }

                void UpdatePhaseTimeAndFinish(Infrastructure::IClock const & clock, Common::TimeSpan & phaseDuration, bool addPhase0Duration)
                {
                    /* 
                        The phase0 duration can come from a different machine or the same machine.
                        Different machine in case it is swap primary and the new primary is continuing the reconfig
                        Same machine if it is abort swap primary

                        For the first case add the phase0 duration to the total time
                        For the second case do not do so
                    */
                    auto now = UpdatePhaseTime(clock, phaseDuration);
                    totalDuration_ = now - startTime_;
                    if (addPhase0Duration)
                    {
                        totalDuration_ = totalDuration_ + phase0Duration_;
                    }
                }

                Common::StopwatchTime startTime_;
                Common::StopwatchTime phaseStartTime_;

                Common::TimeSpan phase0Duration_;
                Common::TimeSpan phase1Duration_;
                Common::TimeSpan phase2Duration_;
                Common::TimeSpan phase3Duration_;
                Common::TimeSpan phase4Duration_;
                Common::TimeSpan totalDuration_;
            };
        }
    }
}
