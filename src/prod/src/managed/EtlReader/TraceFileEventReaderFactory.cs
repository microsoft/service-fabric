// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    public class TraceFileEventReaderFactory : ITraceFileEventReaderFactory
    {
        public ITraceFileEventReader CreateTraceFileEventReader(string fileName)
        {
#if !DotNetCoreClrLinux
            return new TraceFileEventReader(fileName);
#else
            return new LttngTraceFolderEventReader(fileName);
#endif
        }
    }
}