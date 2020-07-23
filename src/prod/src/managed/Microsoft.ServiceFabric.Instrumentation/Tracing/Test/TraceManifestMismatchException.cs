// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Test
{
    using System;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    internal class TraceManifestMismatchException : Exception
    {
        private TraceRecord traceRecord;

        private EtwEvent manifestEvent;

        private MismatchKind mismatchKind;

        public TraceManifestMismatchException(TraceRecord record, MismatchKind mismatch, string message) : base(message)
        {
            this.traceRecord = record;
            this.mismatchKind = mismatch;
        }

        public TraceManifestMismatchException(TraceRecord record, EtwEvent manifestEvent, MismatchKind mismatch, string message) : base(message)
        {
            this.traceRecord = record;
            this.manifestEvent = manifestEvent;
            this.mismatchKind = mismatch;
        }

        public override string ToString()
        {
            return string.Format(
                "MismatchKind: {0}, Msg: {1}, TraceRecord: {2}, ManifestRecord: {3}",
                this.mismatchKind,
                this.Message != null ? this.Message : "<N/A>",
                this.traceRecord.Identity,
                this.manifestEvent != null ? this.manifestEvent.ToString() : " Missing Manifest Event");
        }
    }
}