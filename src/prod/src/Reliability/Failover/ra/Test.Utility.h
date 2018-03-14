// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// Defines for things specific to this test dll
namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class UnitTestContext;

            class ProcessRunner
            {
            public:
                void Execute(std::wstring const & command, std::wstring const & args, std::wstring const & workingDirectory, bool ignoreExitCode)
                {
                    TestLog::WriteInfo(Common::wformatString("Starting {0} {1}", command, args));
                    const Common::TimeSpan CommandTimeout = Common::TimeSpan::FromSeconds(5000);
                    Common::HandleUPtr processHandle;
                    Common::HandleUPtr threadHandle;

                    // Use an empty environment block because valid env block doesn't work
                    std::vector<wchar_t> envBlock;

                    auto commandLine = Common::ProcessUtility::GetCommandLine(command, args);
                    pid_t pid;
                    auto error = Common::ProcessUtility::CreateProcess(commandLine, workingDirectory, envBlock, 0, processHandle, threadHandle, pid);
                    ASSERT_IF(!error.IsSuccess(), "Failed to create process {0} {1}", command, error);

                    DWORD exitCode = 0;
                    Common::AutoResetEvent waitCompleted(false);

                    Common::ProcessWait pwait(
                        std::move(*processHandle),
                        pid,
                        [&] (pid_t, Common::ErrorCode const& waitResult, DWORD eCode)
                        {
                            error = waitResult;
                            exitCode = eCode;
                            waitCompleted.Set();
                        },
                        CommandTimeout);
                    
                    TestLog::WriteInfo(L"waiting for process exit");
                    waitCompleted.WaitOne();

                    ASSERT_IF(!error.IsSuccess(), "Process did not terminate {0} {1}", command, error);
                    TestLog::WriteInfo(Common::wformatString("Exit code {0} {1}", exitCode, error));
                    ASSERT_IF(exitCode != 0 && !ignoreExitCode, "ExitCode is not success {0} {1}", command, exitCode);
                }
            };

            // Used to validate that a sequence of steps happens in a unit test
            template<typename T>
            class StepValidator
            {
            public:
                void OnStep(T step)
                {
                    steps_.push_back(step);
                }

                void AssertExactStepOrder(int n, ...)
                {
                    std::vector<T> expected;

                    va_list ap;

                    va_start(ap, n);
                    for (int i = 0; i < n; i++)
                    {
                        T elem = va_arg(ap, T);
                        expected.push_back(elem);
                    }

                    va_end(ap);

                    VERIFY_ARE_EQUAL(expected.size(), steps_.size());

                    for (int i = 0; i < n; i++)
                    {
                        TestLog::WriteInfo(Common::wformatString("Comparing step at index {0}. Expected: {1}. Actual: {2}", i, (int)expected[i], (int)steps_[i]));
                        VERIFY_ARE_EQUAL(expected[i], steps_[i]);
                    }
                }

                void AssertStepOrder(int n, ...)
                {
                    std::vector<T> expected;

                    va_list ap;

                    va_start(ap, n);
                    for (int i = 0; i < n; i++)
                    {
                        T elem = va_arg(ap, T);
                        expected.push_back(elem);
                    }

                    va_end(ap);

                    std::vector<size_t> indexes;
                    for (size_t i = 0; i < n; i++)
                    {
                        auto it = find(steps_.begin(), steps_.end(), expected[i]);
                        if (it == steps_.end())
                        {
                            TestLog::WriteInfo(Common::wformatString("Failed to find step at {0}", i));
                            VERIFY_IS_TRUE(false);
                            return;
                        }
                        else
                        {
                            indexes.push_back(it - steps_.begin());
                        }
                    }

                    // assert that all the indexes are in ascending order
                    for (size_t i = 0; i < n - 1; i++)
                    {
                        VERIFY_IS_TRUE(indexes[i] < indexes[i + 1]);
                    }
                }


            private:
                std::vector<T> steps_;
            };

            const DWORD DefaultAsyncTestWaitTimeoutInMs = 10000;
            const DWORD DefaultAsyncTestPollTimeoutInMs = 10;

            void BusyWaitUntil(
                std::function<bool()> processor,
                DWORD pollTimeoutInMs = DefaultAsyncTestPollTimeoutInMs,
                DWORD waitTimeoutInMs = DefaultAsyncTestWaitTimeoutInMs
                );

            const double DefaultConfidenceIntervalLowerBoundMultiplier = 0.5;
            const double DefaultConfidenceIntervalUpperBoundMultiplier = 3.0;

            void VerifyWithinConfidenceInterval(
                double expected,
                double actual,
                double lowerBoundMultiplier = DefaultConfidenceIntervalLowerBoundMultiplier,
                double upperBoundMultiplier = DefaultConfidenceIntervalUpperBoundMultiplier
                );

            std::wstring GetRandomString(std::wstring const & prefix, Common::Random& random);
        }
    }
}
