// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    using System;
    using System.Fabric.Description;
    using System.Runtime.Serialization;

    [Serializable]
    [DataContract]
    internal class WinFabricCreateServiceFromTemplate
    {
        [DataMember(Name = "ApplicationName", IsRequired = true)]
        public string ApplicationName
        {
            get;
            set;
        }

        [DataMember(Name = "ServiceName", IsRequired = true)]
        public string ServiceName
        {
            get;
            set;
        }

        [DataMember(Name = "ServiceTypeName", IsRequired = true)]
        public string ServiceTypeName
        {
            get;
            set;
        }

        [DataMember(Name = "ServiceDnsName", IsRequired = false)]
        public string ServiceDnsName
        {
            get;
            set;
        }

        [DataMember(Name = "ServicePackageActivationMode", IsRequired = false)]
        public ServicePackageActivationMode ServicePackageActivationMode
        {
            get;
            set;
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays", Justification = "Match product")]
        [DataMember(Name = "InitializationData")]
        public byte[] InitializationData
        {
            get;
            set;
        }
    }
}


#pragma warning restore 1591