// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// Returns selected replica object using ReplicaSelector testability API.
    /// </summary>
    /// <remarks> 
    /// This class returns information about the selected replica selected using replica selector testability API. 
    /// If the ReplicaSelector was generated using the RandomOf overload this class will be populated with 
    /// ReplicaOrInstance ID and PartitionSelector for the selected replica
    /// </remarks>
    
    public class SelectedReplica : IEquatable<SelectedReplica>
    {
        /// <summary>
        /// Returns None replica selector. 
        /// </summary>
        /// <remarks>
        /// A SelectedReplica will be none for any testability API with overloads which does not take a replica instance id and partition
        /// selector as an input. For example if RestartNodeAsync is called with the NodeName overload
        /// instead of ReplicaSelector. Returned ReplicaSelector will be None in that case.
        /// </remarks>
        public static readonly SelectedReplica None = new SelectedReplica(0, SelectedPartition.None);

        /// <summary>
        /// Selected replica constructor 
        /// </summary>
        /// <param name="replicaOrInstanceId">Replica or instance id</param>
        /// <param name="partitionSelectorResult">Partition selector result</param>
        internal SelectedReplica(long replicaOrInstanceId, SelectedPartition partitionSelectorResult)
        {
            ReleaseAssert.AssertIf(
                ((replicaOrInstanceId != 0 && partitionSelectorResult == SelectedPartition.None) ||
                (replicaOrInstanceId == 0 && partitionSelectorResult != SelectedPartition.None)),
                "Both should be null or neither");

            this.SelectedPartition = partitionSelectorResult;
            this.ReplicaOrInstanceId = replicaOrInstanceId;
        }

        /// <summary>
        /// Gets replica or instance ID
        /// </summary>
        /// <value>
        /// <para>Returns the replica or instance ID for this SelectedReplica</para>
        /// </value>
        public long ReplicaOrInstanceId { get; private set; }
        
        /// <summary>
        /// Gets selected partition 
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.SelectedReplica"/>.</para>
        /// </value>
        public SelectedPartition SelectedPartition { get; private set; }

        /// <summary>
        /// Compares this SelectedReplica with other SelectedReplica.
        /// </summary>
        /// <param name="other">A selected replica with which the current SelectedReplica is to be compared.</param>
        /// <returns>True, if both replicaOrInstanceId and SelectedPartition, of the two SelectedReplica objects match; else returns false.</returns>
        public bool Equals(SelectedReplica other)
        {
            return other != null && this.ReplicaOrInstanceId == other.ReplicaOrInstanceId && this.SelectedPartition.Equals(other.SelectedPartition); 
        }

        /// <summary>
        /// Returns a string representation of SelectedReplica object
        /// </summary>
        /// <returns>A string with the format: ReplicaOrInstanceId = &lt;replicaOrInstanceId&gt;, SelectedPartition = &lt;selectedPartition&gt;</returns>
        public override string ToString()
        {
            return string.Format("ReplicaOrInstanceId = {0}, SelectedPartition = {1}", this.ReplicaOrInstanceId, this.SelectedPartition);
        }

        #region Interop Helpers
        internal unsafe static SelectedReplica CreateFromNative(IntPtr nativeSelectedReplica)
        {
            NativeTypes.FABRIC_SELECTED_REPLICA selectedReplica = *(NativeTypes.FABRIC_SELECTED_REPLICA*)nativeSelectedReplica;
            long replicaOrInstanceId = selectedReplica.ReplicaOrInstanceId;
            SelectedPartition selectedPartition = SelectedPartition.CreateFromNative(selectedReplica.PartitionSelected);

            return new SelectedReplica(replicaOrInstanceId, selectedPartition);
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeSelectedReplica = new NativeTypes.FABRIC_SELECTED_REPLICA();

            nativeSelectedReplica.ReplicaOrInstanceId = this.ReplicaOrInstanceId;

            nativeSelectedReplica.PartitionSelected = this.SelectedPartition.ToNative(pin);

            return pin.AddBlittable(nativeSelectedReplica);
        }
        #endregion
    }
}