// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Callback
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Store;
    using ClusterAnalysis.TraceAccessLayer;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Assert = ClusterAnalysis.Common.Util.Assert;

    /// <inheritdoc />
    internal class SimpleCallbackStore : BaseCallbackStore
    {
        private static readonly TimeSpan DelayBetweenNewSignalAvailabilityCheck = TimeSpan.FromSeconds(30);

        private static readonly IReadOnlyList<Scenario> SupportedCallbacks = new List<Scenario>
        {
            Scenario.ProcessCrash,
            Scenario.QuorumLoss,
            Scenario.Reconfiguration,
            Scenario.NewReplicaHealthEvent,
            Scenario.NewPartitionHealthEvent,
            Scenario.ExpiredPartitionHealthEvent,
            Scenario.ExpiredReplicaHealthEvent,
            Scenario.RunAsyncUnhandledException
        };

        private static readonly IReadOnlyDictionary<Scenario, Type> ScenarioTypeMap = new Dictionary<Scenario, Type>
        {
            { Scenario.ProcessCrash, typeof(ProcessUnexpectedTerminationTraceRecord) },
            { Scenario.Reconfiguration, typeof(ReconfigurationCompletedTraceRecord) }
        };

        private static readonly IReadOnlyDictionary<Type, Scenario> ReverseScenarioTypeMap = new Dictionary<Type, Scenario>
        {
            { typeof(ProcessUnexpectedTerminationTraceRecord), Scenario.ProcessCrash },
            { typeof(ReconfigurationCompletedTraceRecord), Scenario.Reconfiguration }
        };

        private ITraceStoreReader traceStoreReader;

        /// <summary>
        /// Create an instance of <see cref="SimpleCallbackStore"/>
        /// </summary>
        /// <param name="logProvider"></param>
        /// <param name="traceStoreReader"></param>
        /// <param name="token"></param>
        /// <param name="storeProvider"></param>
        /// <param name="taskRunner"></param>
        /// <remarks>
        /// Keeping private to control who can create an instance.
        /// </remarks>
        internal SimpleCallbackStore(
            IStoreProvider storeProvider,
            ITaskRunner taskRunner,
            ILogProvider logProvider,
            ITraceStoreReader traceStoreReader,
            CancellationToken token) : base(storeProvider, taskRunner, logProvider, token)
        {
            Assert.IsNotNull(traceStoreReader, "Trace Store Reader can't be null");
            this.traceStoreReader = traceStoreReader;
        }

        #region Public_Abstractions

        /// <inheritdoc />
        public override IEnumerable<Scenario> GetCallbackSupportedScenario()
        {
            return SupportedCallbacks;
        }

        #endregion Public_Abstractions

        #region Protected_Abstractions

        /// <inheritdoc />
        protected override async Task<IDictionary<Scenario, IList<TraceRecord>>> GetSignalsForScenarioAsync(
            IList<Scenario> scenarios,
            Duration duration,
            CancellationToken token)
        {
            var allTypes = this.GetTypesFromScenarios(scenarios);
            ReadFilter filter = ReadFilter.CreateReadFilter(allTypes);

            var allTraceRecords = await this.traceStoreReader.ReadTraceRecordsAsync(duration, filter, token).ConfigureAwait(false);

            return this.GroupTraceRecordsByScenario(allTraceRecords);
        }

        /// <inheritdoc />
        protected override TimeSpan GetDelayBetweenCheckForNewSignalAvailability()
        {
            return DelayBetweenNewSignalAvailabilityCheck;
        }

        private IDictionary<Scenario, IList<TraceRecord>> GroupTraceRecordsByScenario(IEnumerable<TraceRecord> allTraceRecords)
        {
            IDictionary<Scenario, IList<TraceRecord>> groupedRecord = new Dictionary<Scenario, IList<TraceRecord>>();
            foreach (var oneRecord in allTraceRecords)
            {
                var recordType = oneRecord.GetType();
                if (!ReverseScenarioTypeMap.ContainsKey(recordType))
                {
                    throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Unsupported Record '{0}'", oneRecord));
                }

                if (!groupedRecord.ContainsKey(ReverseScenarioTypeMap[recordType]))
                {
                    groupedRecord[ReverseScenarioTypeMap[oneRecord.GetType()]] = new List<TraceRecord> { oneRecord };
                }
                else
                {
                    groupedRecord[ReverseScenarioTypeMap[recordType]].Add(oneRecord);
                }
            }

            return groupedRecord;
        }

        private IList<Type> GetTypesFromScenarios(IList<Scenario> scenarios)
        {
            IList<Type> types = new List<Type>();
            foreach (var oneScenario in scenarios)
            {
                if (!ScenarioTypeMap.ContainsKey(oneScenario))
                {
                    throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Scenario '{0}' not supported", oneScenario));
                }

                types.Add(ScenarioTypeMap[oneScenario]);
            }

            return types;
        }

        #endregion Protected_Abstractions
    }
}