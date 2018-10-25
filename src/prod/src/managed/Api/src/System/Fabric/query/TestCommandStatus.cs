// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Common.Serialization;

namespace System.Fabric.Query
{
    /// <summary>
    /// This class represents the status of a test command.  Calling <see cref="System.Fabric.FabricClient.TestManagementClient.GetTestCommandStatusListAsync(System.Fabric.Query.TestCommandStateFilter,System.Fabric.Query.TestCommandTypeFilter,System.TimeSpan,System.Threading.CancellationToken)"/> returns an IList of this type of object.
    /// </summary>
    public sealed class TestCommandStatus
    {
        private Guid operationId;
        private TestCommandProgressState state;
        private TestCommandType type;

        internal TestCommandStatus()
        { }

        internal TestCommandStatus(Guid operationId, TestCommandProgressState actionState, TestCommandType actionType)
        {
            this.operationId = operationId;
            this.state = actionState;
            this.type = actionType;
        }

        /// <summary>
        /// The OperationId of the test command.  This Guid was provided by the user when starting the test operation.
        /// </summary>
        /// <value>A Guid representing the OperationId.</value>        
        public Guid OperationId
        {
            get
            {
                return this.operationId;
            }

            internal set
            {
                this.operationId = value;
            }
        }

        /// <summary>
        /// The current state of the test command.
        /// </summary>
        /// <value>A TestCommandProgressState with the current state.</value>
        public TestCommandProgressState State
        {
            get
            {
                return this.state;
            }

            internal set
            {
                this.state = value;
            }
        }

        /// <summary>
        /// The type of the test command.
        /// </summary>
        /// <value>A TestCommandType object.</value>
        [JsonCustomization(PropertyName = "Type")]
        public TestCommandType TestCommandType
        {
            get
            {
                return this.type;
            }

            internal set
            {
                this.type = value;
            }
        }

        /// <summary>
        /// Formats OperationId, State, and ActionType into a string.
        /// </summary>
        /// <value>The formatted string.</value>
        public override string ToString()
        {
            return string.Format("OperationId={0}, State={1}, ActionType={2}", this.operationId, this.state, this.type);
        }
    }
}