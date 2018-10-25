// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// Represents a move primary replica result object.
    /// </summary>
    /// <remarks>
    /// This class returns the nodeName and SelectedPartition information for which move primary replica action was called. 
    /// </remarks>
    [Serializable]
    public class MovePrimaryResult
    {
        /// <summary>
        /// Move primary replica result object constructor.
        /// </summary>
        /// <param name="nodeName">Node name where primary will be moving.</param>
        /// <param name="selectedPartition">Selected partition</param>
        internal MovePrimaryResult(string nodeName, SelectedPartition selectedPartition)
        {
            this.NodeName = nodeName;
            ReleaseAssert.AssertIfNull(selectedPartition, "Selected partition cannot be null");
            this.SelectedPartition = selectedPartition;
        }

        /// <summary>
        /// Gets node name.
        /// </summary>
        /// <value>The node name.</value>
        public string NodeName { get; private set; }

        /// <summary>
        /// Gets the selected partition.
        /// </summary>
        /// <value>The SelectedPartition object.</value>
        public SelectedPartition SelectedPartition { get; private set; }

        /// <summary>
        /// Formats NodeName and SelectedPartition into a string.
        /// </summary>
        /// <returns>The formatted string.</returns>
        public override string ToString()
        {
            return string.Format("NodeName = {0}, SelectedPartition = {1}", this.NodeName, this.SelectedPartition);
        }

        internal unsafe static MovePrimaryResult CreateFromNative(IntPtr nativeResult)
        {
            NativeTypes.FABRIC_MOVE_PRIMARY_RESULT movePrimaryResult = *(NativeTypes.FABRIC_MOVE_PRIMARY_RESULT*)nativeResult;

            string nodeName = NativeTypes.FromNativeString(movePrimaryResult.NodeName);

            Uri serviceName = NativeTypes.FromNativeUri(movePrimaryResult.ServiceName);

            Guid partitionId = movePrimaryResult.PartitionId;

            SelectedPartition selectedPartition = new SelectedPartition(serviceName, partitionId);

            return new MovePrimaryResult(nodeName, selectedPartition);
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeMovePrimaryResult = new NativeTypes.FABRIC_MOVE_PRIMARY_RESULT
            {
                NodeName = pin.AddObject(this.NodeName),
                ServiceName = pin.AddObject(this.SelectedPartition.ServiceName),
                PartitionId = this.SelectedPartition.PartitionId
            };

            return pin.AddBlittable(nativeMovePrimaryResult);
        }
    }
}