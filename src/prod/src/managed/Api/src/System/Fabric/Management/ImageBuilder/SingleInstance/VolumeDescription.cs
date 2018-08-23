// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;

    internal abstract class VolumeDescription
    {
        public VolumeDescription(string volumeName)
        {
            this.volumeName = volumeName;
            this.description = null;
        }

        public string volumeName;
        public string description;
        public abstract string PluginName { get; }
        public abstract IEnumerable<DriverOptionType> DriverOptions { get; }

        public virtual void Validate(string codePackageName)
        {
            if (string.IsNullOrEmpty(this.volumeName))
            {
                throw new FabricApplicationException(
                    String.Format(
                        "required parameter 'volumeName' not specified for volume description in code package {0}",
                        codePackageName));
            }
        }

        public virtual bool IsReplicatedStoreVolume()
        {
            return false;
        }
    };
}
