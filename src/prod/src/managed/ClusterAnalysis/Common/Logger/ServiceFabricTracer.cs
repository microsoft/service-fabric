// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Log
{
    using System.Diagnostics.Tracing;

    /// <summary>
    /// Maintain trace events for adding traces in tests. 
    /// To use this class, reference the ServiceFabricTestTrace project and include the namespace  Microsoft.ServiceFabric.TestTracing.Internal
    /// </summary>
    [EventSource(Name = "Microsoft-ServiceFabric-Events", Guid = "cbd93bc2-71e5-4566-b3a7-595d8eeca6e8")]
    public class ServiceFabricTracer : EventSource
    {
        public const EventKeywords Default = (EventKeywords)0x1;

        private static readonly ServiceFabricTracer SingletonEvents = new ServiceFabricTracer();

        private static string tracePrefix = string.Empty;

        private ServiceFabricTracer()
            : base(false)
        {
        }

        public static ServiceFabricTracer TraceEvents
        {
            get
            {
                return SingletonEvents;
            }
        }

        /// <summary>
        /// Sets an additional prefix to test level traces
        /// For example, setting TracePrefix = "E2E" would result in traces similar to the following:
        /// Test.E2E.Type
        /// </summary>
        /// <param name="newPrefix"></param>
        public static void SetTracePrefix(string newPrefix)
        {
            tracePrefix = string.Format("{0}.", newPrefix);
        }

        /// <summary>
        /// Used to write an error trace for Fabric tests
        /// </summary>
        /// <param name="id">Specify the id for the trace</param>
        /// <param name="type">Specify the type of trace</param>
        /// <param name="format">Specify the error message format</param>
        /// <param name="args">Specify the arguments for the message format</param>
        public static void WriteErrorTrace(string id, string type, string format, params object[] args)
        {
            TraceEvents.Test_ErrorText(id, tracePrefix + type, string.Format(format, args));
        }

        /// <summary>
        /// Used to write an info trace for Fabric tests
        /// </summary>
        /// <param name="id">Specify the id for the trace</param>
        /// <param name="type">Specify the type of trace</param>
        /// <param name="format">Specify the info message format</param>
        /// <param name="args">Specify the arguments for the message format</param>
        public static void WriteInfoTrace(string id, string type, string format, params object[] args)
        {
            TraceEvents.Test_InfoText(id, tracePrefix + type, string.Format(format, args));
        }

        /// <summary>
        /// Used to write an noise trace for Fabric tests
        /// </summary>
        /// <param name="id">Specify the id for the trace</param>
        /// <param name="type">Specify the type of trace</param>
        /// <param name="format">Specify the noise message format</param>
        /// <param name="args">Specify the arguments for the message format</param>
        public static void WriteNoiseTrace(string id, string type, string format, params object[] args)
        {
            TraceEvents.Test_NoiseText(id, tracePrefix + type, string.Format(format, args));
        }

        /// <summary>
        /// Used to write a warning trace for Fabric tests
        /// </summary>
        /// <param name="id">Specify the id for the trace</param>
        /// <param name="type">Specify the type of trace</param>
        /// <param name="format">Specify the warning message format</param>
        /// <param name="args">Specify the arguments for the message format</param>
        public static void WriteWarningTrace(string id, string type, string format, params object[] args)
        {
            TraceEvents.Test_WarningText(id, tracePrefix + type, string.Format(format, args));
        }

        [Event(58880, Message = "{2}", Level = EventLevel.Error, Keywords = Default)]
        public void Test_ErrorText(string id, string type, string message)
        {
            this.WriteEvent(58880, id, type, message);
        }

        [Event(58882, Message = "{2}", Level = EventLevel.Informational, Keywords = Default)]
        public void Test_InfoText(string id, string type, string message)
        {
            this.WriteEvent(58882, id, type, message);
        }

        [Event(58883, Message = "{2}", Level = EventLevel.Verbose, Keywords = Default)]
        public void Test_NoiseText(string id, string type, string message)
        {
            this.WriteEvent(58883, id, type, message);
        }

        [Event(58881, Message = "{2}", Level = EventLevel.Warning, Keywords = Default)]
        public void Test_WarningText(string id, string type, string message)
        {
            this.WriteEvent(58881, id, type, message);
        }
    }
}