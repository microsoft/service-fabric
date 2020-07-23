// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.LogProvider
{
    using System.Fabric.Common;
    using ClusterAnalysis.Common.Log;

    internal class EventStoreLogProvider : ILogProvider
    {
        private const string TraceType = "EventStore@";

        private static ILogProvider logProvider;

        private EventStoreLogProvider()
        {
        }

        public static ILogProvider LogProvider
        {
            get { return logProvider ?? (logProvider = new EventStoreLogProvider()); }
        }

        public ILogger CreateLoggerInstance(string logIdentifier)
        {
            return new TraceLogger(TraceType + logIdentifier);
        }

        /// <summary>
        /// Trace Logger
        /// </summary>
        private class TraceLogger : ILogger
        {
            private string typeIdentity;

            public TraceLogger(string typeIdentifier)
            {
                this.typeIdentity = typeIdentifier;
            }

            public void Flush()
            {
            }

            /// <inheritdoc />
            public void LogError(string format, params object[] message)
            {
                TestabilityTrace.TraceSource.WriteError(this.typeIdentity, format, message);
            }

            /// <inheritdoc />
            public void LogMessage(string format, params object[] message)
            {
                TestabilityTrace.TraceSource.WriteInfo(this.typeIdentity, format, message);
            }

            /// <inheritdoc />
            public void LogWarning(string format, params object[] message)
            {
                TestabilityTrace.TraceSource.WriteWarning(this.typeIdentity, format, message);
            }
        }
    }
}