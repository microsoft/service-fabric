// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    internal abstract class VolumeMount
    {
        private static bool volumeDescriptionRequired = true;
        private readonly string name;
        private readonly string destinationPath;
        private readonly bool readOnly;
        protected readonly VolumeDescription volumeDescription;

        internal static bool TestSettingVolumeDescriptionRequired
        {
            set
            {
                volumeDescriptionRequired = value;
            }
        }

        internal VolumeDescription VolumeDesc
        {
            get
            {
                return this.volumeDescription;
            }
        }

        internal string Name
        {
            get
            {
                return this.name;
            }
        }

        internal string DestinationPath
        {
            get
            {
                return this.destinationPath;
            }
        }

        internal bool ReadOnly
        {
            get
            {
                return this.readOnly;
            }
        }

        internal VolumeMount(string name, string destinationPath, bool readOnly, VolumeDescription volumeDescription)
        {
            this.name = name;
            this.destinationPath = destinationPath;
            this.readOnly = readOnly;
            this.volumeDescription = volumeDescription;
        }

        internal virtual void Validate(string codePackageName)
        {
            if (string.IsNullOrEmpty(this.name))
            {
                throw new FabricApplicationException(
                    String.Format(
                        "required parameter 'name' not specified for volume mount in code package {0}",
                        codePackageName));
            }

            if (string.IsNullOrEmpty(this.destinationPath))
            {
                throw new FabricApplicationException(
                    String.Format(
                        "required parameter 'destinationPath' not specified for volume mount with name {0} in code package {1}",
                        this.name,
                        codePackageName));
            }

            if (this.volumeDescription != null)
            {
                volumeDescription.Validate(codePackageName);
            }
            else if (volumeDescriptionRequired)
            {
                throw new FabricApplicationException(
                    String.Format(
                        "required parameter 'volumeDescription' not specified for volume mount with name {0} in code package {1}",
                        this.name,
                        codePackageName));
            }
        }

        internal bool IsReplicatedStoreVolume()
        {
            return (this.volumeDescription == null) ? false : volumeDescription.IsReplicatedStoreVolume();
        }
    }

    internal class ApplicationScopedVolume : VolumeMount
    {
        internal ApplicationScopedVolume(string name, string destinationPath, bool readOnly, VolumeDescription volumeDescription)
            : base(
                  String.Concat(name, "-", StringConstants.PartitionIdAlias),
                  destinationPath,
                  readOnly,
                  volumeDescription)
        {
        }
    }

    internal class IndependentVolumeMount : VolumeMount
    {
        internal IndependentVolumeMount(string name, string destinationPath, bool readOnly, VolumeDescription volumeDescription)
            : base(name, destinationPath, readOnly, volumeDescription)
        {
        }
    }
}
