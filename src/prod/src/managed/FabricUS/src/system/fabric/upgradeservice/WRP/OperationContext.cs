// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Threading;

    public interface IOperationContext
    {
        TimeSpan OperationTimeout { get; set; }
        CancellationToken CancellationToken { get; set; }
        string ContinuationToken { get; set; }
        TimeSpan GetRemainingTime();
        TimeSpan GetRemainingTimeOrThrow();
    }

    public class OperationContext : IOperationContext
    {
        private readonly TimeoutHelper timeoutHelper;

        public OperationContext(
            CancellationToken cancellationToken,
            TimeSpan operationTimeout,
            string continuationToken = null)
        {
            this.CancellationToken = cancellationToken;
            this.OperationTimeout = operationTimeout;
            this.ContinuationToken = continuationToken;
            this.timeoutHelper = new TimeoutHelper(operationTimeout, operationTimeout);
        }

        public OperationContext(
            CancellationToken cancellationToken,
            string continuationToken = null)
            : this(cancellationToken, TimeSpan.MaxValue, continuationToken)
        {
        }

        public TimeSpan OperationTimeout { get; set; }

        public CancellationToken CancellationToken { get; set; }

        public string ContinuationToken { get; set; }

        public TimeSpan GetRemainingTime()
        {
            return this.timeoutHelper.GetRemainingTime();
        }

        public TimeSpan GetRemainingTimeOrThrow()
        {
            this.timeoutHelper.ThrowIfExpired();
            return this.GetRemainingTime();
        }
    }
}