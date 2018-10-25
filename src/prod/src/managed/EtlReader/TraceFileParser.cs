// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;

    public class TraceFileParser
    {
        private readonly ManifestCache cache;
        private readonly Action<string> traceDispatcher;

        public TraceFileParser(ManifestCache cache, Action<string> traceDispatcher)
        {
            if (cache == null)
            {
                throw new ArgumentNullException("cache");
            }

            if (traceDispatcher == null)
            {
                throw new ArgumentNullException("traceDispatcher");
            }

            this.cache = cache;
            this.traceDispatcher = traceDispatcher;
        }

        public void ParseTraces(string fileName, DateTime startTime, DateTime endTime)
        {
            using (var reader = new TraceFileEventReader(fileName))
            {
                reader.EventRead += this.OnEventRead;
                reader.ReadEvents(startTime, endTime);
            }
        }

        private void OnEventRead(object sender, EventRecordEventArgs e)
        {
            string s = this.cache.FormatEvent(e.Record);
            if (s != null)
            {
                s = s.Replace("\n", "\t");
                this.traceDispatcher(s);
            }
        }
    }
}