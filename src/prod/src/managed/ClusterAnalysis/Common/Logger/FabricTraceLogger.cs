// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Log
{
    using System;
    using ClusterAnalysis.Common.Util;
        
    internal class FabricTraceLogger : ILogger
    {
        private readonly string traceId;

        private readonly string traceType;

        public FabricTraceLogger(string id, string type)
        {
            Assert.IsNotEmptyOrNull(type, "type can't be null/empty");
            this.traceId = id ?? string.Empty;
            this.traceType = type;
        }

        /// <inheritdoc />
        public void LogMessage(string format, params object[] message)
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc />
        public void LogWarning(string format, params object[] message)
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc />
        public void LogError(string format, params object[] message)
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc />
        public void Flush()
        {
            // No-op.
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            var other = obj as FabricTraceLogger;
            if (other == null)
            {
                return false;
            }

            return this.traceId == other.traceId && this.traceType == other.traceType;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                return ((this.traceId != null ? this.traceId.GetHashCode() : 0) * 397) ^ (this.traceType != null ? this.traceType.GetHashCode() : 0);
            }
        }

        /// <inheritdoc />
        protected bool Equals(FabricTraceLogger other)
        {
            return string.Equals(this.traceId, other.traceId) && string.Equals(this.traceType, other.traceType);
        }
    }
}