// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Description;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a service type.</para>
    /// </summary>
    public class ServiceType
    {
        internal ServiceType(
            ServiceTypeDescription serviceTypeDescription,
            string serviceManifestVersion,
            string serviceManifestName,
            bool isServiceGroup = false)
        {
            this.ServiceTypeDescription = serviceTypeDescription;
            this.ServiceManifestVersion = serviceManifestVersion;
            this.ServiceManifestName = serviceManifestName;
            this.IsServiceGroup = isServiceGroup;
        }

        private ServiceType() { }

        /// <summary>
        /// <para>Gets the service type description.</para>
        /// </summary>
        /// <value>
        /// <para>The service type description.</para>
        /// </value>
        public ServiceTypeDescription ServiceTypeDescription { get; private set; }

        /// <summary>
        /// <para>Gets the service manifest version.</para>
        /// </summary>
        /// <value>
        /// <para>The service manifest version.</para>
        /// </value>
        public string ServiceManifestVersion { get; private set; }

        /// <summary>
        /// <para>Gets the service manifest name</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string ServiceManifestName { get; private set; }

        /// <summary>
        /// <para> 
        /// Flag represent whether the service is a service group.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The boolean indicates whether the service is a service group.</para>
        /// </value>
        public bool IsServiceGroup { get; private set; }

        internal static unsafe ServiceType CreateFromNative(
           NativeTypes.FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM nativeResultItem)
        {
            ServiceTypeDescription serviceTypeDescription;
            if (ServiceTypeDescription.TryCreateFromNative(nativeResultItem.ServiceTypeDescription, out serviceTypeDescription))
            {
                string serviceManifestName = null;
                bool isServiceGroup = false;
                if (nativeResultItem.Reserved != IntPtr.Zero)
                {
                    var castedEx1 = (NativeTypes.FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM_EX1*)(nativeResultItem.Reserved);
                    serviceManifestName = NativeTypes.FromNativeString(castedEx1->ServiceManifestName);

                    if (castedEx1->Reserved != IntPtr.Zero)
                    {
                        var castedEx2 = (NativeTypes.FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM_EX2*)(castedEx1->Reserved);
                        isServiceGroup = NativeTypes.FromBOOLEAN(castedEx2->IsServiceGroup);
                    }
                }

                return new ServiceType(
                    serviceTypeDescription,
                    NativeTypes.FromNativeString(nativeResultItem.ServiceManifestVersion),
                    serviceManifestName,
                    isServiceGroup);
            }
            else
            {
                return null;
            }
        }
    }
}