// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// The test command type of the queried test command.
    /// </summary>
    public enum TestCommandType
    {
        /// <summary>
        /// Invalid
        /// </summary>
        Invalid = NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_DEFAULT,

        /// <summary>
        /// Indicates the test command is for a data loss command
        /// </summary>
        PartitionDataLoss = NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_INVOKE_DATA_LOSS,

        /// <summary>
        /// Indicates the test command is for a quorum loss command
        /// </summary>
        PartitionQuorumLoss = NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_INVOKE_QUORUM_LOSS,

        /// <summary>
        /// Indicates the test command is for a restart partition command
        /// </summary>
        PartitionRestart = NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_INVOKE_RESTART_PARTITION,

        /// <summary>
        /// Indicates the test command is for a node transition command
        /// </summary>
        NodeTransition = NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_START_NODE_TRANSITION,
    }

    internal static class TestCommandTypeHelper
    {
        internal static TestCommandType FromNative(NativeTypes.FABRIC_TEST_COMMAND_TYPE type)
        {
            switch (type)
            {
                case NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_INVOKE_DATA_LOSS:
                    return TestCommandType.PartitionDataLoss;
                case NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_INVOKE_QUORUM_LOSS:
                    return TestCommandType.PartitionQuorumLoss;
                case NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_INVOKE_RESTART_PARTITION:
                    return TestCommandType.PartitionRestart;
                case NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_START_NODE_TRANSITION:
                    return TestCommandType.NodeTransition;
                default:
                    return TestCommandType.Invalid;
            }
        }

        internal static NativeTypes.FABRIC_TEST_COMMAND_TYPE ToNative(TestCommandType type)
        {
            switch (type)
            {
                case TestCommandType.PartitionDataLoss:
                    return NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_INVOKE_DATA_LOSS;
                case TestCommandType.PartitionQuorumLoss:
                    return NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_INVOKE_QUORUM_LOSS;
                case TestCommandType.PartitionRestart:
                    return NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_INVOKE_RESTART_PARTITION;
                case TestCommandType.NodeTransition:
                    return NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_START_NODE_TRANSITION;
                default:
                    return NativeTypes.FABRIC_TEST_COMMAND_TYPE.FABRIC_TEST_COMMAND_TYPE_DEFAULT;
            }
        }
    }
}