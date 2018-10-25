// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>
    /// For internal use only.
    /// </para>
    /// </summary>

    [Serializable]
    [DataContract]
    public sealed class TokenServiceMetadata
    {
        /// <summary>
        /// <para>
        /// For internal use only.
        /// </para>
        /// </summary>
        /// <param name="metadata">
        /// <para>The metadata.</para>
        /// </param>
        /// <param name="serviceName">
        /// <para>The service name.</para>
        /// </param>
        /// <param name="serviceDnsName">
        /// <para>The service Dns name.</para>
        /// </param>
        public TokenServiceMetadata(string metadata, string serviceName, string serviceDnsName)
        {
            ServiceName = serviceName;
            ServiceDnsName = serviceDnsName;
            Metadata = metadata;
        }

        /// <summary>
        /// <para>
        /// For internal use only.
        /// Gets or sets the service name.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The service name.</para>
        /// </value>
        [DataMember(IsRequired = true, Name = "ServiceName")]
        public string ServiceName
        {
            get;
            set;
        }

        /// <summary>
        /// <para>
        /// For internal use only.
        /// Gets or sets the service Dns name.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The service Dns name.</para>
        /// </value>
        [DataMember(IsRequired = true, Name = "ServiceDnsName")]
        public string ServiceDnsName
        {
            get;
            set;
        }

        /// <summary>
        /// <para>
        /// For internal use only.
        /// Gets or sets the metadata.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The metadata.</para>
        /// </value>
        [DataMember(IsRequired = true, Name = "Metadata")]
        public string Metadata
        {
            get;
            set;
        }

        internal unsafe void ToNative(PinCollection pin, out NativeTypes.FABRIC_TOKEN_SERVICE_METADATA serviceMetadata)
        {
            serviceMetadata.Metadata = pin.AddObject(this.Metadata);
            serviceMetadata.ServiceName = pin.AddObject(this.ServiceName);
            serviceMetadata.ServiceDnsName = pin.AddObject(this.ServiceDnsName);
            serviceMetadata.Reserved = IntPtr.Zero;
        }
    }
}