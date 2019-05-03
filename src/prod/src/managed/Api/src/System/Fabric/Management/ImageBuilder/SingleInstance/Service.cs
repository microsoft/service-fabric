// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.Fabric.Management.ImageBuilder.DockerCompose;
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
                this.autoScalingPolicies = null;
                this.replicaCount = 1;
                this.networkRefs = null;
                this.diagnostics = null;
            }

            public string description;
            public List<CodePackage> codePackages;
            public List<ScalingPolicy> autoScalingPolicies;
            public int replicaCount;
            public List<NetworkRef> networkRefs;
            public DiagnosticsRef diagnostics;
        }

        public Service()
        {
            this.name = null;
            this.properties = new ServiceProperty();
            this.dnsName = "";
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

        public List<ScalingPolicy> autoScalingPolicies
        {
            get
            {
                return this.properties.autoScalingPolicies;
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

        public bool HasPersistedState()
        {
            if (!UsesReliableCollection())
                return true; // stateless services always have persisted state true

            List<CodePackage> codePackagesWithReliableCollectionRefs = this.properties.codePackages.FindAll(cp => cp.reliableCollectionsRefs.Count != 0);
            List<CodePackage> codePackagesWithPersistedState = codePackagesWithReliableCollectionRefs.FindAll(cp => cp.HasPersistedState() == true);
            if (codePackagesWithPersistedState.Count == codePackagesWithReliableCollectionRefs.Count)
                return true; // All code packages have persisted state true

            if (codePackagesWithPersistedState.Count == 0)
                return false; // All code packages have persisted state false

            throw new FabricComposeException(String.Format("doNotPersistState config values are not consistent across code packages for service '{0}'", this.name));
        }

        public string name;
        public ServiceProperty properties;
        private CodePackageDebugParameters debugParams;
        public string dnsName;

        internal bool HasRelieableCollectionRefs()
        {
            return this.properties.codePackages.Any(cp => cp.reliableCollectionsRefs.Count != 0);
        }

        public bool UsesReliableCollection()
        {
            return (debugParams == null || !debugParams.DisableReliableCollectionReplicationMode) && HasRelieableCollectionRefs();
        }

        public bool UsesReplicatedStoreVolume()
        {
            return this.properties.codePackages.Any(cp => cp.UsesReplicatedStoreVolume());
        }

        internal void SetDebugParameters(CodePackageDebugParameters serviceDebugParams)
        {
            this.debugParams = serviceDebugParams;
        }

        internal string GetUriScheme()
        {
            return HasRelieableCollectionRefs() ? "http" : "";
        }
    };
}
