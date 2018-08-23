// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Threading;

    internal class VolumeDescriptionVolumeDisk : VolumeDescription
    {
        private const string DockerPluginName = "sfvolumedisk";

        private Lazy<IEnumerable<DriverOptionType>> driverOptions;

        public VolumeDescriptionVolumeDisk()
            : this(null)
        {
        }

        public VolumeDescriptionVolumeDisk(string volumeName)
            : base(volumeName)
        {
            this.sizeDisk = null;

            this.driverOptions = new Lazy<IEnumerable<DriverOptionType>>(
                () =>
                {
                    return new DriverOptionType[]
                    {
                        new DriverOptionType() { Name = "servicePartitionId", Value = StringConstants.PartitionIdAlias },
                        new DriverOptionType() { Name = "sizeDisk", Value = ParseSizeDisk(this.sizeDisk) },
                        new DriverOptionType() { Name = "fileSystem", Value = "NTFS" },
                    };
                },
                LazyThreadSafetyMode.PublicationOnly);
        }

        public string sizeDisk;
        
        public override string PluginName
        {
            get
            {
                return DockerPluginName;
            }
        }

        public override IEnumerable<DriverOptionType> DriverOptions
        {
            get
            {
                return this.driverOptions.Value;
            }
        }

        public override void Validate(string codePackageName)
        {
            base.Validate(codePackageName);

            if (string.IsNullOrEmpty(this.sizeDisk))
            {
                throw new FabricApplicationException(String.Format("required parameter 'sizeDisk' not specified for volume disk parameter in code package {0}", codePackageName));
            }
        }

        public override bool IsReplicatedStoreVolume()
        {
            return true;
        }

        private string ParseSizeDisk(string input)
        {
            if (input.Equals("Small")) 
            {
                return "32GB";
            } 
            else if (input.Equals("Medium"))
            {
                return "64GB";
            }
            else if (input.Equals("Large"))
            {
                return "128GB";
            }
            else
            {
                throw new FabricApplicationException(String.Format("required parameter 'sizeDisk' format {0} is not supported", input));
            }
        }
    };
}
