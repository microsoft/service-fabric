// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System;
    using System.Diagnostics.Tracing;

    internal class TraceConsoleEventSink : IGenericEventSink
    {
        private static readonly object SyncLock = new object();

        public void SetOption(string option)
        {
        }

        public void SetPath(string filename)
        {
        }

        public void Write(string processName, string v1, string v2, EventLevel informational, string formattedMessage)
        {
            lock (TraceConsoleEventSink.SyncLock)
            {
                Console.WriteLine(formattedMessage);
            }
        }
    }
}