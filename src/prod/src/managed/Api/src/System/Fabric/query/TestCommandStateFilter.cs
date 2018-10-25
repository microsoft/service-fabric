// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// This is used when calling GetTestCommandStatusListAsync(), and indicates the filter to use on TestCommandState's.    Multiple filter values may be specified.
    /// </summary>
    [Serializable]
    [Flags]
    public enum TestCommandStateFilter
    {
        /// <summary>
        /// The default filter.
        /// </summary>
        Default = NativeTypes.FABRIC_TEST_COMMAND_STATE_FILTER.FABRIC_TEST_COMMAND_STATE_FILTER_DEFAULT,

        /// <summary>
        /// Indicates not to do any filtering on TestCommandState.
        /// </summary>
        All = NativeTypes.FABRIC_TEST_COMMAND_STATE_FILTER.FABRIC_TEST_COMMAND_STATE_FILTER_ALL,

        /// <summary>
        /// This filter selects Running test commands.
        /// </summary>
        Running = NativeTypes.FABRIC_TEST_COMMAND_STATE_FILTER.FABRIC_TEST_COMMAND_STATE_FILTER_RUNNING,

        /// <summary>
        /// This filter selects RollingBack test commands.
        /// </summary>
        RollingBack = NativeTypes.FABRIC_TEST_COMMAND_STATE_FILTER.FABRIC_TEST_COMMAND_STATE_FILTER_ROLLING_BACK,

        /// <summary>
        /// This filter selects Completed test commands.
        /// </summary>
        CompletedSuccessfully = NativeTypes.FABRIC_TEST_COMMAND_STATE_FILTER.FABRIC_TEST_COMMAND_STATE_FILTER_COMPLETED_SUCCESSFULLY,

        /// <summary>
        /// This filter selects Failed test commands.
        /// </summary>
        Failed = NativeTypes.FABRIC_TEST_COMMAND_STATE_FILTER.FABRIC_TEST_COMMAND_STATE_FILTER_FAILED,

        /// <summary>
        /// This filter selects Cancelled test commands.
        /// </summary>
        Cancelled = NativeTypes.FABRIC_TEST_COMMAND_STATE_FILTER.FABRIC_TEST_COMMAND_STATE_FILTER_CANCELLED,

        /// <summary>
        /// This filter selects ForceCancelled test commands.
        /// </summary>
        ForceCancelled = NativeTypes.FABRIC_TEST_COMMAND_STATE_FILTER.FABRIC_TEST_COMMAND_STATE_FILTER_FORCE_CANCELLED
    }
}