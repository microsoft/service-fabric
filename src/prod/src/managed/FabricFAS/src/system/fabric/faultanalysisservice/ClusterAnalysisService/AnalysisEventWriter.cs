// ----------------------------------------------------------------------
//  <copyright file="AnalysisEventWriter.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.ClusterAnalysis
{
    using System;
    using System.Fabric.Common.Tracing;
    using System.Fabric.FaultAnalysis.Service.ClusterAnalysis.Services;
    using global::ClusterAnalysis.AnalysisCore.AnalysisEvents;
    using global::ClusterAnalysis.AnalysisCore.Insights;
    using global::ClusterAnalysis.AnalysisCore.Insights.PartitionInsight;
    using global::ClusterAnalysis.Common.Log;

    internal class AnalysisEventTraceWriter : IAnalysisEventTraceWriter
    {
        private const string AnalysisEventWriterIdentifier = "AnalysisEventWriter";
        private static AnalysisEventTraceWriter instance = new AnalysisEventTraceWriter();
        private ILogger logger;

        private AnalysisEventTraceWriter()
        {
            this.logger = ClusterAnalysisLogProvider.LogProvider.CreateLoggerInstance(AnalysisEventWriterIdentifier);
        }

        public static AnalysisEventTraceWriter Instance
        {
            get
            {
                return instance;
            }
        }

        public void WriteTraceEvent(FabricAnalysisEvent analysisEvent)
        {
            var primaryMoveAnalysisEvent = analysisEvent as PrimaryMoveAnalysisEvent;
            if (primaryMoveAnalysisEvent != null)
            {
                FabricEvents.Events.PrimaryMoveAnalysisEvent(
                    primaryMoveAnalysisEvent.InstanceId,
                    primaryMoveAnalysisEvent.PartitionId,
                    primaryMoveAnalysisEvent.PrimaryMoveCompleteTimeStamp,
                    primaryMoveAnalysisEvent.Delay.TotalSeconds,
                    primaryMoveAnalysisEvent.Duration.TotalSeconds,
                    primaryMoveAnalysisEvent.PreviousNode,
                    primaryMoveAnalysisEvent.CurrentNode,
                    primaryMoveAnalysisEvent.Reason.ToString(),
                    primaryMoveAnalysisEvent.CorrelatedEventString);

                // Write the 'Join' event which correlates the Reconfiguration completed Event with the why the primary moved analysis Event.
                FabricEvents.Events.CorrelationOperational(
                    Guid.NewGuid(),
                    primaryMoveAnalysisEvent.TriggerReconfigurationCompletedTraceRecord.EventInstanceId,
                    primaryMoveAnalysisEvent.TriggerReconfigurationCompletedTraceRecord.GetType().Name,
                    primaryMoveAnalysisEvent.InstanceId,
                    primaryMoveAnalysisEvent.GetType().Name);
            }
            else
            {
                this.logger.LogError("Unknown analysisEventType : {0}.", analysisEvent.GetType());
            }
        }
    }
}