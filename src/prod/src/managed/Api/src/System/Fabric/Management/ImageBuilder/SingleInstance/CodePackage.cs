// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.Linq;

    internal class CodePackage
    {
        public CodePackage()
        {
            this.name = null;
            this.image = null;
            this.imageRegistryCredential = null;
            this.entryPoint = null;
            this.commands = new List<string>();
            this.endpoints = new List<Endpoint>();
            this.certificateRefs = new List<CertificateRef>();
            this.environmentVariables = new List<EnvironmentVariable>();
            this.volumeRefs = new List<IndependentVolumeMount>();
            this.volumes = new List<ApplicationScopedVolume>();
            this.labels = new List<Label>();
            this.resources = null;
            this.diagnostics = null;
            this.reliableCollectionsRefs = new List<ReliableCollectionsRef>();
        }

        public string name;
        public string image;
        public string entryPoint;
        public List<string> commands;
        public ImageRegistryCredential imageRegistryCredential;
        public List<Endpoint> endpoints;
        public List<CertificateRef> certificateRefs;
        public List<EnvironmentVariable> environmentVariables;
        public List<IndependentVolumeMount> volumeRefs;
        public List<ApplicationScopedVolume> volumes;
        public List<Label> labels;
        public Resources resources;
        public DiagnosticsRef diagnostics;
        public List<ReliableCollectionsRef> reliableCollectionsRefs; // TEMPORARY - to support reliable collections demo for //BUILD

        public void Validate()
        {
            if (String.IsNullOrEmpty(this.image))
            {
                throw new FabricApplicationException("required parameter 'image' not specified for the code package");
            }

            if (this.resources == null)
            {
                throw new FabricApplicationException(String.Format("required parameter 'resources' not specified for code package {0}", name));
            }

            this.resources.Validate(name);

            var volumeMounts = Enumerable.Concat<VolumeMount>(this.volumeRefs, this.volumes);
            foreach (var volumeMount in volumeMounts)
            {
                volumeMount.Validate(name);
            }

            if (this.labels != null)
            {
                foreach (var label in this.labels)
                {
                    label.Validate(name);
                }
            }
        }

        public bool UsesReplicatedStoreVolume()
        {
            return Enumerable.Concat<VolumeMount>(this.volumeRefs, this.volumes)
                .Any(vr => vr.IsReplicatedStoreVolume());
        }
    };
}
