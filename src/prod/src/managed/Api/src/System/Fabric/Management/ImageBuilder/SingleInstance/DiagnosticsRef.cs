// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class DiagnosticsRef
    {
        public DiagnosticsRef()
        {
            enabled = false;
            sinkRefs = null;
        }

        public bool enabled;
        public List<string> sinkRefs;
    }
}