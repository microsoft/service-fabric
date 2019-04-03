// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Runtime
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Threading;
    using ClusterAnalysis.Common.Config;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.PerformanceTracker;
    using ClusterAnalysis.Common.Store;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Provide runtime for insight stack
    /// </summary>
    public class DefaultInsightRuntime : IInsightRuntime
    {
        private IDictionary<Type, object> serviceDictionary = new Dictionary<Type, object>();

        private ILogProvider logProvider;

        private IStoreProvider storeProvider;

        private Config config;

        private bool testMode;

        private DefaultInsightRuntime(ILogProvider logProvider, IStoreProvider storeProvider, Config config, IPerformanceSessionManager perfSessionalManager, ITaskRunner runner, bool isTest, CancellationToken token)
        {
            Assert.IsNotNull(logProvider, "Log Provider can't be null");
            Assert.IsNotNull(storeProvider, "store provider can't be null");
            Assert.IsNotNull(config, "Config can't be null");
            Assert.IsNotNull(perfSessionalManager, "perfSessionalManager");
            Assert.IsNotNull(runner, "runner");
            this.logProvider = logProvider;
            this.storeProvider = storeProvider;
            this.config = config;
            this.PerformanceSessionManager = perfSessionalManager;
            this.TaskRunner = runner;
            this.Token = token;
            this.testMode = isTest;
        }

        /// <summary>
        /// Token
        /// </summary>
        public CancellationToken Token { get; }

        /// <inheritdoc />
        public IPerformanceSessionManager PerformanceSessionManager { get; }

        /// <inheritdoc />
        public ITaskRunner TaskRunner { get; }

        /// <summary>
        /// Expose the only instance of runtime
        /// </summary>
        public static IInsightRuntime GetInstance(ILogProvider logProvider, IStoreProvider storeProvider, Config config, IPerformanceSessionManager perfSessionMgr, ITaskRunner runner, CancellationToken token, bool isTest = false)
        {
            return new DefaultInsightRuntime(logProvider, storeProvider, config, perfSessionMgr, runner, isTest, token);
        }

        /// <inheritdoc />
        public void AddService(Type t, object serviceObj)
        {
            Assert.IsNotNull(serviceObj, "Service object can't be null");
            if (this.serviceDictionary.ContainsKey(t))
            {
                throw new Exception(string.Format(CultureInfo.InvariantCulture, "key of Type '{0}', already exists", t));
            }

            this.serviceDictionary[t] = serviceObj;
        }

        /// <inheritdoc />
        public object GetService(Type t)
        {
            return !this.serviceDictionary.ContainsKey(t) ? null : this.serviceDictionary[t];
        }

        /// <inheritdoc />
        public IStoreProvider GetStoreProvider()
        {
            return this.storeProvider;
        }

        /// <inheritdoc />
        public ILogProvider GetLogProvider()
        {
            return this.logProvider;
        }

        /// <inheritdoc />
        public Config GetCurrentConfig()
        {
            return this.config;
        }

        /// <inheritdoc />
        public bool IsTestMode()
        {
            return this.testMode;
        }
    }
}