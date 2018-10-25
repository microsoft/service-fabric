// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca;
    using TelemetryAggregation;
    using TelemetryConfigParser;

    internal class TelemetryFilter
    {
        private const string LogSourceId = "TelemetryFilter";
        private const string ConfigFileName = "tconfig.ini";

        // This dictionary contains the white-listed events based on TaskName, and EventName.
        // The value is the complete aggregation and white-listing information.
        private readonly Dictionary<Tuple<string, string>, TraceAggregationConfig> whiteListedEventsFromConfigFile = new Dictionary<Tuple<string, string>, TraceAggregationConfig>();

        public TelemetryFilter()
        {
            List<TraceAggregationConfig> traceAggregationConfigs;
            List<ParseResult> parseResults = SyntaxAnalyzer.Parse(ConfigFileName, new string[] { }, Utility.TraceSource.WriteError, false, out traceAggregationConfigs);

            foreach (var telConfig in traceAggregationConfigs)
            {
                try
                {
                    // including the aggregator to white-list
                    this.whiteListedEventsFromConfigFile.Add(new Tuple<string, string>(telConfig.TaskName, telConfig.EventName), telConfig);
                }
                catch (ArgumentNullException e)
                {
                    Utility.TraceSource.WriteError(LogSourceId, "Unexpected null value parsed from config file: {0} - Exception: {1}.", ConfigFileName, e);
                }
                catch (ArgumentException e)
                {
                    if (telConfig.DifferentiatorFieldName == this.whiteListedEventsFromConfigFile[new Tuple<string, string>(telConfig.TaskName, telConfig.EventName)].DifferentiatorFieldName)
                    {
                        Utility.TraceSource.WriteWarning(LogSourceId, "Conflicting trace white-listing (trace already white-listed) -Trace: {0} - Exception : {1}.", telConfig, e);
                    }
                }
            }

            // Tracing any errors that may have occured during parsing
            foreach (var parseRes in parseResults)
            {
                if (parseRes.ParseCode != ParseCode.Success)
                {
                    Utility.TraceSource.WriteError(LogSourceId, "{0}({1},{2}) : Error : {3}", ConfigFileName, parseRes.LineNumber, parseRes.ColumnPos + 1, parseRes.ErrorMsg);
                }
            }
        }

        // checks whether the trace is whitelisted - if so outputs the traceAggregationConfig for the trace
        public bool TryGetWhitelist(string taskName, string eventName, out TraceAggregationConfig traceAggregationConfig)
        {
            // see if taskname and eventname are whitelisted and get aggregation configuration
            return this.whiteListedEventsFromConfigFile.TryGetValue(Tuple.Create(taskName, eventName), out traceAggregationConfig);
        }
    }
}