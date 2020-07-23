// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Config
{
    using System;
    using System.Globalization;

    /// <summary>
    ///  Config Class
    /// </summary>
    public class Config
    {
        /// <summary>
        /// Current Run Mode
        /// </summary>
        public RunMode RunMode { get; set; }

        /// <summary>
        /// Gets or Set the current runtime context
        /// </summary>
        public RuntimeContext RuntimeContext { get; set; }

        /// <summary>
        /// Gets or sets the config manager to get access to Named configuration.
        /// </summary>
        public NamedConfigManager NamedConfigManager { get; set; }

        /// <summary>
        /// Gets the Name of Crash Dump Analysis Service
        /// </summary>
        public string CrashDumpAnalyzerServiceName { get; set; }

        /// <summary>
        /// Gets if Crash Dump Analysis is Enabled or not.
        /// </summary>
        public bool IsCrashDumpAnalysisEnabled { get; set; }

        /// <summary>
        /// Gets if instrumentation is enabled or not.
        /// </summary>
        /// <remarks>
        /// TODO: Refactor it out.
        /// </remarks>
        public bool IsInstrumentationEnabled { get; set; }

        /// <summary>
        /// Gets the instrumentation key
        /// </summary>
        public string InstrumentationKey { get; set; }

        /// <inheritdoc />
        public override string ToString()
        {
            return
                string.Format(
                CultureInfo.InvariantCulture,
                "RunMode: {0}, RuntimeContext: {1}",
                this.RunMode,
                this.RuntimeContext);
        }
    }
}