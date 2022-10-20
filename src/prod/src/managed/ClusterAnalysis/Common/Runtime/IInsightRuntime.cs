// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Runtime
{
    using System;
    using ClusterAnalysis.Common.Config;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.PerformanceTracker;
    using ClusterAnalysis.Common.Store;

    /// <summary>
    /// Insight Runtime contract
    /// </summary>
    public interface IInsightRuntime : IServiceProvider
    {
        /// <summary>
        /// Gets the performance session manager.
        /// </summary>
        IPerformanceSessionManager PerformanceSessionManager { get; }

        /// <summary>
        /// Gets the Task runner.
        /// </summary>
        ITaskRunner TaskRunner { get; }

        /// <summary>
        /// Add a service
        /// </summary>
        /// <param name="t">type of service</param>
        /// <param name="serviceObj">service object</param>
        void AddService(Type t, object serviceObj);

        /// <summary>
        /// Get Persistent StructuredStore Provider Instance
        /// </summary>
        /// <returns></returns>
        IStoreProvider GetStoreProvider();

        /// <summary>
        /// Get Log provider
        /// </summary>
        /// <returns></returns>
        ILogProvider GetLogProvider();

        /// <summary>
        /// Get the configuration object encapsulating different configuration values.
        /// </summary>
        /// <returns></returns>
        Config GetCurrentConfig();

        /// <summary>
        /// Find out if we are currently in Test Mode.
        /// </summary>
        /// <remarks>
        /// TODO: Consider moving it out of this generic interface into implementation. Kids can cast and check.
        /// </remarks>
        /// <returns></returns>
        bool IsTestMode();
    }
}