// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.Validator
{
    // Interface representing the tracing methods for DCA plugin validators
    public interface ITraceWriter
    {
        void WriteError(string traceTag, string format, params object[] args);

        void WriteWarning(string traceTag, string format, params object[] args);
        
        void WriteInfo(string traceTag, string format, params object[] args);
    }
}