// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// This is used when calling GetTestCommandStatusListAsync(), and indicates the filter to use on TestCommandType's.    Multiple filter values may be specified.
    /// </summary>
    [Serializable]
    [Flags]
    public enum TestCommandTypeFilter
    {
        /// <summary>
        /// The default filter.
        /// </summary>
        Default = NativeTypes.FABRIC_TEST_COMMAND_TYPE_FILTER.FABRIC_TEST_COMMAND_TYPE_FILTER_DEFAULT,

        /// <summary>
        /// Indicates to not do any filtering on TestCommandType.
        /// </summary>
        All = NativeTypes.FABRIC_TEST_COMMAND_TYPE_FILTER.FABRIC_TEST_COMMAND_TYPE_FILTER_ALL,

        /// <summary>
        /// Indicates to select test commands of TestCommandType PartitionDataLoss.
        /// </summary>
        PartitionDataLoss = NativeTypes.FABRIC_TEST_COMMAND_TYPE_FILTER.FABRIC_TEST_COMMAND_TYPE_FILTER_PARTITION_DATA_LOSS,

        /// <summary>
        /// Indicates to select test commands of TestCommandType PartitionQuorumLoss.
        /// </summary>
        PartitionQuorumLoss = NativeTypes.FABRIC_TEST_COMMAND_TYPE_FILTER.FABRIC_TEST_COMMAND_TYPE_FILTER_PARTITION_QUORUM_LOSS,

        /// <summary>
        /// Indicates to select test commands of TestCommandType PartitionRestart.
        /// </summary>
        PartitionRestart = NativeTypes.FABRIC_TEST_COMMAND_TYPE_FILTER.FABRIC_TEST_COMMAND_TYPE_FILTER_PARTITION_RESTART,

        /// <summary>
        /// Indicates to select test commands of TestCommandType for node transitions.
        /// </summary>
        NodeTransition = NativeTypes.FABRIC_TEST_COMMAND_TYPE_FILTER.FABRIC_TEST_COMMAND_TYPE_FILTER_NODE_TRANSITION,
    }
}