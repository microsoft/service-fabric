// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Log
{
    using System.Collections.Concurrent;
    using System.Globalization;
    using ClusterAnalysis.Common.Util;

    public class FabricTraceLogProvider : ILogProvider
    {
        private ConcurrentDictionary<string, ILogger> typeBasedLogObjectMap;

        private string globalTraceId;

        private string baseTraceType;

        /// <summary>
        /// Create an instance of <see cref="FabricTraceLogProvider"/>
        /// </summary>
        public FabricTraceLogProvider() : this(null, null)
        {
        }

        /// <summary>
        /// Create an instance of <see cref="FabricTraceLogProvider"/>
        /// </summary>
        /// <param name="globalTraceSessionId"></param>
        /// <param name="baseType"></param>
        public FabricTraceLogProvider(string globalTraceSessionId, string baseType)
        {
            this.typeBasedLogObjectMap = new ConcurrentDictionary<string, ILogger>();
            this.globalTraceId = globalTraceSessionId;
            this.baseTraceType = baseType;
        }

        /// <inheritdoc />
        public ILogger CreateLoggerInstance(string logIdentifier)
        {
            Assert.IsNotEmptyOrNull(logIdentifier, "Log Identifier can't be null");

            if (!string.IsNullOrEmpty(this.baseTraceType))
            {
                logIdentifier = string.Format(CultureInfo.InvariantCulture, "{0}@{1}", this.baseTraceType, logIdentifier);
            }

            return this.typeBasedLogObjectMap.GetOrAdd(
                logIdentifier,
                str => new FabricTraceLogger(this.globalTraceId, logIdentifier));
        }
    }
}