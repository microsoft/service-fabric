// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Test
{
    using System;
    using System.Fabric.Common.Tracing;
    using System.Diagnostics.Tracing;


    internal class ErrorAndWarningFreeTraceEventSource : FabricEvents.ExtensionsEvents
    {
        public ErrorAndWarningFreeTraceEventSource(EventTask taskId = FabricEvents.Tasks.Test)
            : base(taskId)
        {
        }

        protected override void WriteError(string id, string type, string message)
        {
            base.WriteError(id, type, message);
            throw new Exception(message);
        }

        protected override void WriteWarning(string id, string type, string message)
        {
            base.WriteWarning(id, type, message);
            throw new Exception(message);
        }
    }
}