// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class DiagnosticsSinkBase
    {
        public DiagnosticsSinkBase()
        {
            this.name = null;
            this.kind = null;
            this.description = null;
        }

        public string name;
        public string kind;
        public string description;
    }
}
