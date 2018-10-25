// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;

    internal class Service
    {   
        internal class ServiceProperty
        {
            public ServiceProperty()
            {
                this.description = null;
                this.codePackages = null;
                this.replicaCount = 1;
                this.networkRefs = null;
                this.diagnostics = null;
            }

            public string description;
            public List<CodePackage> codePackages;
            public int replicaCount;
            public List<NetworkRef> networkRefs;
            public DiagnosticsRef diagnostics;
        }

        public Service()
        {
            this.name = null;
            this.properties = new ServiceProperty();
        }

        public string description
        {
            get
            {
                return this.properties.description;
            }
        }

        public List<CodePackage> codePackages
        {
            get
            {
                return this.properties.codePackages;
            }
        }

        public int replicaCount
        {
            get
            {
                return this.properties.replicaCount;
            }
        }

        public List<NetworkRef> networkRefs
        {
            get
            {
                return this.properties.networkRefs;
            }
        }

        public DiagnosticsRef diagnostics
        {
            get
            {
                return this.properties.diagnostics;
            }
        }

        public string name;
        public ServiceProperty properties;
        
        public bool UsesReliableCollection()
        {
            return this.properties.codePackages.Any(cp => cp.reliableCollectionsRefs.Count != 0);
        }

        public bool UsesReplicatedStoreVolume()
        {
            return this.properties.codePackages.Any(cp => cp.UsesReplicatedStoreVolume());
        }
    };
}
