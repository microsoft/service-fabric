// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;
    using System.Collections.Generic;

    /// <summary>
    /// <para>
    /// Contains information for an unplaced replica.
    /// </para>
    /// </summary>
    public class UnplacedReplicaInformation
    {
        /// <summary>
        /// <para> 
        /// Gets the name of the service whose replica could not be placed.
        /// </para>
        /// </summary>
        /// <value>
        /// <para> The string to which ServiceName will be set. </para>
        /// </value>
        public string ServiceName { get; private set; }

        /// <summary>
        /// <para> 
        /// Gets the Partition Id (as a Guid) of the service whose replica could not be placed.
        /// </para>
        /// </summary>
        /// <value>
        /// <para> The Guid to which PartitionId will be set. </para>
        /// </value>
        public Guid PartitionId { get; private set; }

        /// <summary>
        /// <para> 
        /// Gets the reasons (as a list of strings) why a service's replicas could not be placed.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The reasons why service replicas could not be placed.</para>
        /// </value>
        public IList<string> UnplacedReplicaReasons { get; private set; }

        /// <summary>
        /// <para> 
        /// Constructor that creates an object of UnplacedReplicaInformation.
        /// </para>
        /// </summary>
        /// <param name="serviceName">
        /// <para>The name of the service whose replica could not be placed. </para>
        /// </param>
        /// <param name="partitionId">
        /// <para>The Partition Id (as a Guid) of the service whose replica could not be placed. </para>
        /// </param>
        /// <param name="reasonsList">
        /// <para>The reasons (as a list of strings) why a service's replicas could not be placed. </para>
        /// </param>
        public UnplacedReplicaInformation(string serviceName, Guid partitionId, IList<string> reasonsList)
        {
            ServiceName = serviceName;
            PartitionId = partitionId;
            UnplacedReplicaReasons = reasonsList;
        }

        internal UnplacedReplicaInformation()
        {
            ServiceName = "";
            PartitionId = new Guid();
            UnplacedReplicaReasons = new List<string>();
        }

        internal static unsafe UnplacedReplicaInformation CreateFromNative(
            NativeClient.IFabricGetUnplacedReplicaInformationResult nativeResult)
        {
            var retval = CreateFromNative(*(NativeTypes.FABRIC_UNPLACED_REPLICA_INFORMATION*)nativeResult.get_UnplacedReplicaInformation());
            GC.KeepAlive(nativeResult);

            return retval;
        }

        internal static unsafe UnplacedReplicaInformation CreateFromNative(
            NativeTypes.FABRIC_UNPLACED_REPLICA_INFORMATION nativeUnplacedReplicaInformation)
        {
            IList<string> ReasonsList;

            if (nativeUnplacedReplicaInformation.UnplacedReplicaReasons == IntPtr.Zero)
            {
                ReasonsList = new List<string>();
            }
            else
            {
                ReasonsList =
                NativeTypes.FromNativeStringList(*(NativeTypes.FABRIC_STRING_LIST*)nativeUnplacedReplicaInformation.UnplacedReplicaReasons);
            }

            return new UnplacedReplicaInformation(NativeTypes.FromNativeString(nativeUnplacedReplicaInformation.ServiceName),
                                                  nativeUnplacedReplicaInformation.PartitionId,
                                                  ReasonsList);
        }
    }
}