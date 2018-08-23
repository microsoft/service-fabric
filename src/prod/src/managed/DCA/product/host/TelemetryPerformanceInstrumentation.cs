// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using TelemetryAggregation;

    public class TelemetryPerformanceInstrumentation
    {
        private const string TaskNameStr = "FabricDCA";
        private const string EventNameStr = "TelemetryPerformanceInstrumentation";
        private const string ProcessingEventsMeasuredTimeStr = "ProcessingEventsMeasuredTimeInMs";
        private readonly Stopwatch processingEventsMeasuredTimeStopWatch = new Stopwatch();

        public TelemetryPerformanceInstrumentation()
        {
            string[] metrics = new string[] { ProcessingEventsMeasuredTimeStr };
            string[] properties = new string[] { };
            List<Aggregation.Kinds> metricAggr = new List<Aggregation.Kinds>()
            {
                Aggregation.Kinds.Snapshot,
                Aggregation.Kinds.Minimum,
                Aggregation.Kinds.Maximum,
                Aggregation.Kinds.Count,
                Aggregation.Kinds.Sum,
                Aggregation.Kinds.Average,
                Aggregation.Kinds.Variance
            };

            this.TraceAggregationCfg = TraceAggregationConfig.CreateTraceAggregatorConfig(
                TaskNameStr,
                EventNameStr,
                string.Empty,
                properties,
                new List<Aggregation.Kinds>(),
                metrics,
                metricAggr);
        }

        public TraceAggregationConfig TraceAggregationCfg { get; }

        public IEnumerable<KeyValuePair<string, double>> Metrics
        {
            get
            {
                return new List<KeyValuePair<string, double>>()
                {
                    new KeyValuePair<string, double>(ProcessingEventsMeasuredTimeStr, this.processingEventsMeasuredTimeStopWatch.ElapsedMilliseconds)
                };
            }
        }

        public void ProcessingEventsMeasuredTimeStart()
        {
            this.processingEventsMeasuredTimeStopWatch.Start();
        }

        public void ProcessingEventsMeasuredTimeStop()
        {
            this.processingEventsMeasuredTimeStopWatch.Stop();
        }

        public void ProcessingEventsMeasuredTimeReset()
        {
            this.processingEventsMeasuredTimeStopWatch.Reset();
        }
    }
}