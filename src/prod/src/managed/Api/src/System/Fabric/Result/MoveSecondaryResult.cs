// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// Represents move secondary replica result object.
    /// </summary>
    /// <remarks>
    /// This class returns currentSecondaryNodeName, where secondary replica was present before movement, 
    /// newSecondaryNodeName where the SelectedReplica is moved and SelectedPartition information that represents 
    /// the selected secondary replica.
    /// </remarks>
    [Serializable]
    public class MoveSecondaryResult
    {
        /// <summary>
        /// Move secondary replica result constructor.
        /// </summary>
        /// <param name="currentSecondaryNodeName">Current node name where secodary replica exist.</param>
        /// <param name="newSecondaryNodeName">New node name where secondary replica need to move.</param>
        /// <param name="selectedPartition">Selected partion result object</param>
        internal MoveSecondaryResult(string currentSecondaryNodeName, string newSecondaryNodeName, SelectedPartition selectedPartition)
        {

            ReleaseAssert.AssertIf(string.IsNullOrEmpty(currentSecondaryNodeName), "Current secondary node name cannot be null or empty");

            this.CurrentSecondaryNodeName = currentSecondaryNodeName;
            this.NewSecondaryNodeName = newSecondaryNodeName;
            ReleaseAssert.AssertIfNull(selectedPartition, "Selected partition cannot be null");
            this.SelectedPartition = selectedPartition;
        }

        /// <summary>
        /// Gets current node name for the target secondary replica.
        /// </summary>
        /// <returns>The node name for the target secondary replica.</returns>
        public string CurrentSecondaryNodeName { get; private set; }

        /// <summary>
        /// Gets node name where secondary replica will move to.
        /// </summary>
        /// <value>The name of the node where the target secondary replica will move to.</value>
        public string NewSecondaryNodeName { get; private set; }

        /// <summary>
        /// Gets the selected partition.
        /// </summary>
        /// <value>The SelectedPartition object.</value>
        public SelectedPartition SelectedPartition { get; private set; }

        /// <summary>
        /// Formats CurrentSecondaryNodeName, NewSecondaryNodeName, and SelectedPartition into a string.
        /// </summary>
        /// <returns>The formatted string.</returns>
        public override string ToString()
        {
            return string.Format(
                "CurrentSecondaryNodeName = {0}, NewSecondaryNodeName = {1}, SelectedPartition = {2}", 
                this.CurrentSecondaryNodeName, 
                this.NewSecondaryNodeName,
                this.SelectedPartition);
        }

        internal unsafe static MoveSecondaryResult CreateFromNative(IntPtr nativeResult)
        {
            NativeTypes.FABRIC_MOVE_SECONDARY_RESULT moveSecondaryResult = *(NativeTypes.FABRIC_MOVE_SECONDARY_RESULT*)nativeResult;

            string currentSecondaryNodeName = NativeTypes.FromNativeString(moveSecondaryResult.CurrentNodeName);

            string newSecondaryNodeName = NativeTypes.FromNativeString(moveSecondaryResult.NewNodeName);

            Uri serviceName = NativeTypes.FromNativeUri(moveSecondaryResult.ServiceName);

            Guid partitionId = moveSecondaryResult.PartitionId;

            SelectedPartition selectedPartition = new SelectedPartition(serviceName, partitionId);

            return new MoveSecondaryResult(currentSecondaryNodeName, newSecondaryNodeName, selectedPartition);
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeMoveSecondaryResult = new NativeTypes.FABRIC_MOVE_SECONDARY_RESULT
            {
                CurrentNodeName = pin.AddObject(this.CurrentSecondaryNodeName),
                NewNodeName = pin.AddObject(this.NewSecondaryNodeName),
                ServiceName = pin.AddObject(this.SelectedPartition.ServiceName),
                PartitionId = this.SelectedPartition.PartitionId
            };

            return pin.AddBlittable(nativeMoveSecondaryResult);
        }
    }
}