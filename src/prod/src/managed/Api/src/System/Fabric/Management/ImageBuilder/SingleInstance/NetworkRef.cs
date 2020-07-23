// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class NetworkRef
    {
        public NetworkRef()
        {
            name = string.Empty;
            endpointRefs = null;
        }

        public string name;
        public List<EndpointRef> endpointRefs;
    }
}