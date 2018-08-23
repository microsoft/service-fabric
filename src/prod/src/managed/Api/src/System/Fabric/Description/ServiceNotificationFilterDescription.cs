// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a class that is used to describe a filter for service notification delivery, registered 
    /// via <see cref="System.Fabric.FabricClient.ServiceManagementClient.RegisterServiceNotificationFilterAsync(System.Fabric.Description.ServiceNotificationFilterDescription)" />.</para>
    /// </summary>
    public class ServiceNotificationFilterDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" /> class.</para>
        /// </summary>
        public ServiceNotificationFilterDescription()
        {
            this.Name = null;
            this.MatchNamePrefix = false;
            this.MatchPrimaryChangeOnly = false;
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" /> class.</para>
        /// </summary>
        /// <param name="name"><para>The name for which service notifications should be delivered.</para></param>
        /// <param name="matchNamePrefix"><para>Flag that indicates whether all service notifications for which Name is a prefix will be delivered.</para></param>
        /// <param name="matchPrimaryChangeOnly"><para>Flag that indicates whether to match the service notification delivery by filtering out notifications in which the primary replica endpoint has not changed. This flag has no effect for stateless services.</para></param>
        public ServiceNotificationFilterDescription(Uri name, bool matchNamePrefix, bool matchPrimaryChangeOnly)
        {
            this.Name = name;
            this.MatchNamePrefix = matchNamePrefix;
            this.MatchPrimaryChangeOnly = matchPrimaryChangeOnly;
        }

        /// <summary>
        /// <para>Gets the name for which service notifications should be delivered. The name must be in the fabric: scheme. The root name ("fabric:") is allowed.</para>
        /// </summary>
        /// <value>
        /// <para>The name for which service notifications should be delivered.</para>
        /// </value>
        public Uri Name { get; set; }

        /// <summary>
        /// <para>Gets a value that indicates whether all service notifications for which Name is a prefix will be delivered.</para>
        /// </summary>
        /// <remarks>
        /// The prefix match occurs as a URI segment prefix match rather than a string prefix match against service names. A filter with the name "fabric:/abc" will prefix match services named "fabric:/abc" and "fabric:/abc/def", but not a service named "fabric:/abc_def".
        /// </remarks>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if all service notification for which Name is a prefix will be delivered; otherwise, 
        /// <languageKeyword>false</languageKeyword>. The default is <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool MatchNamePrefix { get; set; }

        /// <summary>
        /// <para>Gets a value that indicates whether to match the service notification delivery by filtering out notifications in which the primary 
        /// replica endpoint has not changed. This flag has no effect for stateless services.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the filtering matches primary change only; otherwise, <languageKeyword>false</languageKeyword>. 
        /// The default is <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool MatchPrimaryChangeOnly { get; set; }

        internal unsafe IntPtr ToNative(PinCollection pin)
        {

            var nativeDescription = new NativeTypes.FABRIC_SERVICE_NOTIFICATION_FILTER_DESCRIPTION[1];

            nativeDescription[0].Name = pin.AddObject(this.Name);

            UInt32 flags = 0;

            if (this.MatchNamePrefix)
            {
                flags |= (UInt32)NativeTypes.FABRIC_SERVICE_NOTIFICATION_FILTER_FLAGS.FABRIC_SERVICE_NOTIFICATION_FILTER_FLAGS_NAME_PREFIX;
            }

            if (this.MatchPrimaryChangeOnly)
            {
                flags |= (UInt32)NativeTypes.FABRIC_SERVICE_NOTIFICATION_FILTER_FLAGS.FABRIC_SERVICE_NOTIFICATION_FILTER_FLAGS_PRIMARY_ONLY;
            }

            nativeDescription[0].Flags = (NativeTypes.FABRIC_SERVICE_NOTIFICATION_FILTER_FLAGS)flags;

            return pin.AddBlittable(nativeDescription);
        }
    }
}