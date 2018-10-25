// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// Returns selected partition using PartitionSelector FaultManagementClient APIs. 
    /// </summary>
    /// <remarks> 
    /// This class returns information about the partition that was selected using PartitionSelector testability API.
    /// For example if the PartitionSelector was created using the RandomOf overload this class will populate 
    /// service name and partition id of the for selected partition
    /// </remarks>
    public class SelectedPartition : IEquatable<SelectedPartition>
    {
        /// <summary>
        /// Returns none partition selector.
        /// </summary>
        /// <remarks>
        /// A SelectedPartition will be none for any testability API with overloads which does not take a partition
        /// or replica selector as an input. For example if RestartNodeAsync is called with the NodeName overload
        /// instead of ReplicaSelector then PartitionSelector returned will be None
        /// </remarks>
        public static readonly SelectedPartition None = new SelectedPartition(null, Guid.Empty);

        /// <summary>
        /// Selected partition constructor
        /// </summary>
        /// <param name="serviceName">Service name URI</param>
        /// <param name="partitionId">Partition id</param>
        internal SelectedPartition(Uri serviceName, Guid partitionId)
        {       
            ReleaseAssert.AssertIf(
                ((serviceName != null && partitionId == Guid.Empty) ||
                (serviceName == null && partitionId != Guid.Empty)),
                "Both should be null or neither");
            this.ServiceName = serviceName;
            this.PartitionId = partitionId;
        }

        internal SelectedPartition() { }

        /// <summary>
        /// Gets the service name as URI
        /// </summary>
        /// <value>
        /// A URI
        /// </value>
        public Uri ServiceName { get; internal set; }

        /// <summary>
        /// Gets the partition ID as GUID
        /// </summary>
        /// <value>
        /// A GUID
        /// </value>
        public Guid PartitionId { get; internal set; }

        /// <summary>
        /// Compares this SelectedPartition with other SelectedPartition.
        /// </summary>
        /// <param name="other">A selected partition with which the current SelectedPartition is to be compared.</param>
        /// <returns>True, if both service name and partition ID of the two SelectedPartition objects match; else returns false.</returns>
        public bool Equals(SelectedPartition other)
        {
            return other != null && this.ServiceName.Equals(other.ServiceName) && this.PartitionId == other.PartitionId;
        }

        /// <summary>
        /// Gets a string representation of the SelectedPartition object
        /// </summary>
        /// <returns>A string with the format: Service Name: &lt;service-name&gt;, Partition Id: &lt;partition-id&gt;</returns>
        public override string ToString()
        {
            return string.Format("Service Name: {0}, Partition Id: {1}", this.ServiceName, this.PartitionId);
        }

        #region Interop Helpers
        internal unsafe static SelectedPartition CreateFromNative(IntPtr pointer)
        {
            NativeTypes.FABRIC_SELECTED_PARTITION nativeSelectedPartition = *(NativeTypes.FABRIC_SELECTED_PARTITION*)pointer;
            Uri serviceName = NativeTypes.FromNativeUri(nativeSelectedPartition.ServiceName);
            Guid partitionId = nativeSelectedPartition.PartitionId;

            var selectedPartition = new SelectedPartition 
            {
                ServiceName = serviceName,
                PartitionId = partitionId
            };

            return selectedPartition;
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeSelectedPartition = new NativeTypes.FABRIC_SELECTED_PARTITION();
            nativeSelectedPartition.ServiceName = pin.AddObject(this.ServiceName);
            nativeSelectedPartition.PartitionId = this.PartitionId;
            return pin.AddBlittable(nativeSelectedPartition);
        }
        #endregion
    }
}