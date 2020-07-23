// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper 
{
    using MdsUploaderTest;

    // Class containing methods to create helper objects for ETW event sinks
    internal static class EtlConsumerHelper
    {
        // Creates a helper object that decodes ETW events and writes the decoded
        // events to text files
        public static IEtlToCsvFileWriter CreateEtlToCsvFileWriter(
                                              ITraceEventSourceFactory traceEventSourceFactory, 
                                              string logSourceId, 
                                              string fabricNodeId, 
                                              string etwCsvFolder,
                                              bool disableDtrCompression)
        {
            MdsUploaderTest.EtwCsvFolder = etwCsvFolder;
            return ((IEtlToCsvFileWriter)new EtlToCsvFileWriter(
                                                  traceEventSourceFactory,
                                                  logSourceId,
                                                  fabricNodeId,
                                                  etwCsvFolder,
                                                  disableDtrCompression));
        }
    }
}