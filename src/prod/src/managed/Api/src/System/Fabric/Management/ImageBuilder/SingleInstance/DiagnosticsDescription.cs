// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class DiagnosticsDescription
    {
        public DiagnosticsDescription()
        {
            sinks = null;
            enabled = false;
            defaultSinkRefs = null;
        }

        public List<DiagnosticsSinkBase> sinks;
        public bool enabled;
        public List<string> defaultSinkRefs;
    }
}