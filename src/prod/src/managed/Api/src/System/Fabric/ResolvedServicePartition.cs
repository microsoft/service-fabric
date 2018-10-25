// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// Contains information about the partition of the service that was resolved
    /// and the set of endpoints that can be used to access the partition.</para>
    /// </summary>
    /// <remarks>
    /// <para>
    /// The resolved service partition is obtained as a result of calling
    /// <see cref="System.Fabric.FabricClient.ServiceManagementClient.ResolveServicePartitionAsync(System.Uri)"/> and the other overloads.
    /// </para>
    /// </remarks>
    public sealed class ResolvedServicePartition
    {
        private NativeClient.IFabricResolvedServicePartitionResult nativeResult;
        private NativeClient.IInternalFabricResolvedServicePartition nativeInternalResult;
        private ResolvedServiceEndpoint endpoint;

        internal ResolvedServicePartition()
        {
        }

        private ResolvedServicePartition(NativeClient.IFabricResolvedServicePartitionResult nativeResult)
        {
            this.nativeResult = nativeResult;
            this.nativeInternalResult = (NativeClient.IInternalFabricResolvedServicePartition)nativeResult;
        }

        /// <summary>
        /// <para>Gets the endpoints of the service partition.</para>
        /// </summary>
        /// <value>
        /// <para>A collection of <see cref="System.Fabric.ResolvedServiceEndpoint" />
        /// for the specified service partition.</para>
        /// </value>
        /// <remarks>
        /// <para>A resolved service endpoint contains the role of the
        /// stateful service replica or stateless service instance
        /// and the address where this replica can be reached.</para>
        /// </remarks>
        public ICollection<ResolvedServiceEndpoint> Endpoints
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets information about the partition of the service that was resolved.</para>
        /// </summary>
        /// <value>The information about the partition of the service that was resolved.</value>
        /// <remarks>
        /// <para>
        /// The service partition can be of different <see cref="System.Fabric.ServicePartitionKind"/>.
        /// You can cast the service partition information to the correct derived type
        /// to get detailed information about the partition.
        /// </para>
        /// </remarks>
        public ServicePartitionInformation Info
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the name of the service instance.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service instance.</para>
        /// </value>
        public Uri ServiceName
        {
            get;
            internal set;
        }

        internal long FMVersion
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1305:FieldNamesMustNotUseHungarianNotation", Justification = "Not Hungarian notation.")]
            get
            {
                return Utility.WrapNativeSyncInvokeInMTA<long>(
                    () =>
                    {
                        return this.nativeInternalResult.FMVersion;
                    },
                    "InternalResolvedServicePartition.FMVersion");
            }
        }

        internal long Generation
        {
            get
            {
                return Utility.WrapNativeSyncInvokeInMTA<long>(
                    () =>
                    {
                        return this.nativeInternalResult.Generation;
                    },
                    "InternalResolvedServicePartition.Generation");
            }
        }

        internal NativeClient.IFabricResolvedServicePartitionResult NativeResult
        {
            get { return this.nativeResult; }
        }

        /// <summary>
        /// <para>Returns a single endpoint, instead of a collection of all endpoints. </para>
        /// </summary>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.ResolvedServiceEndpoint" />.</para>
        /// </returns>
        /// <remarks>
        /// <para>Many times, you only need a single endpoint instead of all the endpoints. If the service is stateless, it returns a random endpoint. 
        /// If the service is a stateful service, than it returns the endpoint to which the Primary replica of the service partition listens. Note that if 
        /// the Primary replica currently does not exist, it throws <see cref="System.Fabric.FabricException" />.</para>
        /// </remarks>
        public ResolvedServiceEndpoint GetEndpoint()
        {
            if (this.endpoint == null)
            {
                this.endpoint = Utility.WrapNativeSyncInvokeInMTA<ResolvedServiceEndpoint>(
                    () =>
                    {
                        return this.GetEndpointInternal();
                    },
                    "ResolvedServicePartition.GetEndpoint");
            }

            return this.endpoint;
        }

        /// <summary>
        /// <para>Compares two resolved service partitions and identifies which is newer. </para>
        /// </summary>
        /// <param name="other">
        /// <para>The other resolved service partition to compare.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Int32" />.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricException">
        /// <para>The two <see cref="System.Fabric.ResolvedServicePartition" /> objects are from different service partitions. This can happen if the 
        /// service is deleted and re-created with the same name and partitioning between two resolve calls.</para>
        /// </exception>
        /// <remarks>
        /// <para>The <see cref="System.Fabric.ResolvedServicePartition.CompareVersion(System.Fabric.ResolvedServicePartition)" /> method takes in a 
        /// resolved service partition (RSP) argument with the parameter <paramref name="other" /> to enable the user to identify which RSP is more 
        /// up-to-date. A returned value of 0 indicates that the two RSPs have the same version. 1 indicates that the other RSP has an older version. 
        /// -1 indicates that the other RSP has a newer version. </para>
        /// </remarks>
        public int CompareVersion(ResolvedServicePartition other)
        {
            Requires.Argument<ResolvedServicePartition>("other", other).NotNull();

            return Utility.WrapNativeSyncInvokeInMTA<int>(
                () =>
                {
                    return this.CompareVersionInternal(other);
                },
                "ResolvedServicePartition.CompareVersion");
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage(
            "Microsoft.Reliability",
            "CA2004:RemoveCallsToGCKeepAlive",
            Justification = "Ensure that the native buffers whose lifetime is bound to the interface, do not get freed prematurely.")]
        internal static unsafe ResolvedServicePartition FromNative(NativeClient.IFabricResolvedServicePartitionResult nativeResult)
        {
            if (nativeResult == null)
            {
                return null;
            }

            var partition = new ResolvedServicePartition(nativeResult);
            var nativePartition = (NativeTypes.FABRIC_RESOLVED_SERVICE_PARTITION*)nativeResult.get_Partition();

            partition.ServiceName = NativeTypes.FromNativeUri(nativePartition->ServiceName);
            if (nativePartition->EndpointCount > 0)
            {
                partition.Endpoints = InteropHelpers.ResolvedServiceEndpointCollectionHelper.CreateFromNative((int)nativePartition->EndpointCount, nativePartition->Endpoints);
            }
            else
            {
                partition.Endpoints = new List<ResolvedServiceEndpoint>(0);
            }

            partition.Info = ServicePartitionInformation.FromNative(nativePartition->Info);
            GC.KeepAlive(nativeResult);

            return partition;
        }

        internal unsafe ResolvedServiceEndpoint GetEndpointInternal()
        {
            var nativeEndpoint = (NativeTypes.FABRIC_RESOLVED_SERVICE_ENDPOINT*)this.nativeResult.GetEndpoint();
            var endpoint = new ResolvedServiceEndpoint()
            {
                Address = NativeTypes.FromNativeString(nativeEndpoint->Address),
                Role = (ServiceEndpointRole)nativeEndpoint->Role
            };

            return endpoint;
        }

        internal int CompareVersionInternal(ResolvedServicePartition other)
        {
            return this.nativeResult.CompareVersion(other.NativeResult);
        }
    }
}