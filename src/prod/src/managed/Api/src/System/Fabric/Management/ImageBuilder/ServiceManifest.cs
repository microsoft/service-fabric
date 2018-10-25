// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System.Collections.ObjectModel;
    using System.Fabric.Management.ServiceModel;

    internal class ServiceManifest
    {
        public ServiceManifest(ServiceManifestType serviceManifestType)
        {
            this.ServiceManifestType = serviceManifestType;
            this.CodePackages = new Collection<CodePackage>();
            this.ConfigPackages = new Collection<ConfigPackage>();
            this.DataPackages = new Collection<DataPackage>();
        }        

        public ServiceManifestType ServiceManifestType { get; set; }

        public Collection<CodePackage> CodePackages
        {
            get;
            private set;
        }

        public Collection<ConfigPackage> ConfigPackages
        {
            get;
            private set;
        }

        public Collection<DataPackage> DataPackages
        {
            get;
            private set;
        }

        public string Checksum { get; set; }

        public string ConflictingChecksum { get; set; }
    }
}