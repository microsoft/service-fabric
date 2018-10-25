// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// This enum indicates the state of a test command.
    /// </summary>
    public enum TestCommandProgressState
    {
        /// <summary>
        /// The test command state is invalid.
        /// </summary>
        Invalid = NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_INVALID,
        
        /// <summary>
        /// The test command is in progress.
        /// </summary>
        Running = NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_RUNNING,

        /// <summary>
        /// The test command is rolling back internal system state because it encountered a fatal error or was cancelled by the user.  "RollingBack"
        /// does not refer to user state.  For example, if CancelTestCommandAsync() is called on a command of type TestCommandType.PartitionDataLoss,
        /// a state of "RollingBack" does not mean service data is being restored (assuming the command has progressed far enough to cause data loss).  
        /// It means the system is rolling back/cleaning up internal system state associated with the command.
        /// </summary>
        RollingBack = NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_ROLLING_BACK,

        /// <summary>
        /// The test command has completed successfully and is no longer running.
        /// </summary>
        Completed = NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_COMPLETED,

        /// <summary>
        /// The test command has failed and is no longer running
        /// </summary>
        Faulted = NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_FAULTED,

        /// <summary>
        /// The test command was cancelled by the user using CancelTestCommandAsync()
        /// </summary>
        Cancelled = NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_CANCELLED,

        /// <summary>
        /// The test command was cancelled by the user using CancelTestCommandAsync(), with the force parameter set to true
        /// </summary>
        ForceCancelled = NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_FORCE_CANCELLED
    }

    internal static class TestCommandStateHelper
    {
        internal static NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE ToNative(TestCommandProgressState state)
        {
            switch (state)
            {
                case TestCommandProgressState.Running:
                    return NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_RUNNING;
                case TestCommandProgressState.RollingBack:
                    return NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_ROLLING_BACK;
                case TestCommandProgressState.Completed:
                    return NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_COMPLETED;
                case TestCommandProgressState.Faulted:
                    return NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_FAULTED;
                case TestCommandProgressState.Cancelled:
                    return NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_CANCELLED;
                case TestCommandProgressState.ForceCancelled:
                    return NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_FORCE_CANCELLED;
                default:
                    return NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_INVALID;
            }
        }

        internal static TestCommandProgressState FromNative(NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE state)
        {
            switch (state)
            {
                case NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_RUNNING:
                    return TestCommandProgressState.Running;
                case NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_COMPLETED:
                    return TestCommandProgressState.Completed;
                case NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_ROLLING_BACK:
                    return TestCommandProgressState.RollingBack;
                case NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_FAULTED:
                    return TestCommandProgressState.Faulted;
                case NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_CANCELLED:
                    return TestCommandProgressState.Cancelled;
                case NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE.FABRIC_TEST_COMMAND_PROGRESS_STATE_FORCE_CANCELLED:
                    return TestCommandProgressState.ForceCancelled;
                default:
                    return TestCommandProgressState.Invalid;
            }
        }
    }
}