// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.HttpGatewayTest
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Json;
    using System.Fabric;
    using System.Text;

    [DataContract]
    public class SharingPolicy
    {
        [DataMember]
        public string SharedPackageName;

        [DataMember]
        PackageSharingPolicyScope PackageSharingScope;

        public SharingPolicy(string packageName, PackageSharingPolicyScope scope)
        {
            this.SharedPackageName = packageName;
            this.PackageSharingScope = scope;
        }
    }

    [DataContract]
    public class PredeploymentData
    {
        [DataMember]
        public string ApplicationTypeName;

        [DataMember]
        public string ApplicationTypeVersion;

        [DataMember]
        public string ServiceManifestName;

        [DataMember]
        public string NodeName;

        [DataMember]
        public List<SharingPolicy> PackageSharingPolicy;

        public PredeploymentData(string applicationType, string applicationTypeVersion, string nodeName, string serviceManifest, List<SharingPolicy> sharingPolicies)
        {
            this.ApplicationTypeName = applicationType;
            this.ApplicationTypeVersion = applicationTypeVersion;
            this.NodeName = nodeName;
            this.ServiceManifestName = serviceManifest;
            this.PackageSharingPolicy = sharingPolicies;
        }
    }
}