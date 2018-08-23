// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a service notification containing details about a service whose replica or instance endpoints have changed. Notifications are dispatched by the <see cref="System.Fabric.FabricClient.ServiceManagementClient.ServiceNotificationFilterMatched" /> event.</para>
    /// </summary>
    public sealed class ServiceNotification
    {
        internal ServiceNotification()
        {
        }

        /// <summary>
        /// <para>Gets the name of the service.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service.</para>
        /// </value>
        public Uri ServiceName { get; internal set; }

        /// <summary>
        /// <para>Gets the partition ID of the service.</para>
        /// </summary>
        /// <value>
        /// <para>The partition ID.</para>
        /// </value>
        public Guid PartitionId { get; internal set; }

        /// <summary>
        /// <para>Gets the new replica or instance endpoints published by the service. The collection will be empty if the service has been deleted.</para>
        /// </summary>
        /// <value>
        /// <para>A list of endpoints published by the service.</para>
        /// </value>
        public ICollection<ResolvedServiceEndpoint> Endpoints { get; internal set; }

        /// <summary>
        /// <para>Gets the version of this notification event. The version can be used to order any two notification events.</para>
        /// </summary>
        /// <value>
        /// <para>The version of this notification event.</para>
        /// </value>
        public ServiceEndpointsVersion Version { get; internal set; }

        /// <summary>
        /// <para>Gets the detailed partition information for the service. This property can be null in certain cases where detailed partition information is 
        /// unavailable - such as when the service notification event is for a service deletion event.</para>
        /// </summary>
        /// <value>
        /// <para>The detailed partition information of the service.</para>
        /// </value>
        public ServicePartitionInformation PartitionInfo { get; internal set; }

        internal static unsafe ServiceNotification FromNative(NativeClient.IFabricServiceNotification nativeResult)
        {
            var nativeNotification = (NativeTypes.FABRIC_SERVICE_NOTIFICATION*)nativeResult.get_Notification();
            var managedNotification = new ServiceNotification();

            managedNotification.ServiceName = NativeTypes.FromNativeUri(nativeNotification->ServiceName);

            managedNotification.PartitionId = nativeNotification->PartitionId;

            if (nativeNotification->EndpointCount > 0)
            {
                managedNotification.Endpoints = InteropHelpers.ResolvedServiceEndpointCollectionHelper.CreateFromNative((int)nativeNotification->EndpointCount, nativeNotification->Endpoints);
            }
            else
            {
                managedNotification.Endpoints = new List<ResolvedServiceEndpoint>(0);
            }

            var nativeVersion = nativeResult.GetVersion();
            managedNotification.Version = new ServiceEndpointsVersion(nativeVersion);

            if (nativeNotification->PartitionInfo != IntPtr.Zero)
            {
                managedNotification.PartitionInfo = ServicePartitionInformation.FromNative(nativeNotification->PartitionInfo);
            }
            else
            {
                managedNotification.PartitionInfo = null;
            }

            GC.KeepAlive(nativeResult);

            return managedNotification;
        }
    }
}