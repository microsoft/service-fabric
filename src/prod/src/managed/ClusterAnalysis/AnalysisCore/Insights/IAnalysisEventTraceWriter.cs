// ----------------------------------------------------------------------
//  <copyright file="IAnalysisEventTraceWriter.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights
{
    using ClusterAnalysis.AnalysisCore.AnalysisEvents;

    /// <summary>
    /// Define a contract for emitting structured trace for an Analysis event.
    /// </summary>
    public interface IAnalysisEventTraceWriter
    {
        /// <summary>
        /// Write the structured event containing the Analysis result
        /// </summary>
        /// <param name="analysisEvent"></param>
        /// <returns></returns>
        void WriteTraceEvent(FabricAnalysisEvent analysisEvent);
    }
}