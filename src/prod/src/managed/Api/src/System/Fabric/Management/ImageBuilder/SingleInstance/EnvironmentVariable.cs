// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class EnvironmentVariable
    {
        public EnvironmentVariable()
        {
            this.name = null;
            this.value = null;
        }

        public string name;
        public string value;
    }
}
