// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class ResourceDescription
    {
        public ResourceDescription()
        {
            this.memoryInGB = 0;
            this.cpu = 0;
        }

        public double memoryInGB;

        public double cpu;
    }

    internal class Resources
    {
        public Resources()
        {
            this.requests = null;
            this.limits = null;
        }

        public ResourceDescription requests;

        public ResourceDescription limits;

        public void Validate(string containerName)
        {
            if (this.requests == null)
            {
                throw new FabricApplicationException(String.Format("required parameter 'resources.requests' not specified for container {0}", containerName));
            }

            if (this.requests.cpu == 0)
            {
                throw new FabricApplicationException(String.Format("required parameter 'resources.requests.cpu' not specified for container {0}", containerName));
            }

            if (this.requests.memoryInGB == 0)
            {
                throw new FabricApplicationException(String.Format("required parameter 'resources.requests.memoryInGB' not specified for container {0}", containerName));
            }
        }
    }
}
