// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Diagnostics;
    using System.Threading;
    using System.Threading.Tasks;

    internal class RetryHelper
    {
        // Back-off delay initial values
        private readonly int retryStartDelayMs;
        private readonly int exponentialBackoffFactor;
        private readonly int maxTotalDelayMs;
        private readonly int maxSingleDelayMs;
        private readonly Stopwatch watch;

        /// <summary>Constructor to save RetryHelpers context</summary>
        /// <param name="retryStartDelayMs">Initial back-off in milliseconds</param>
        /// <param name="exponentialBackoffFactor">Exponential factor to increase delay</param>
        /// <param name="maxSingleDelayMs">Max delay for a back-off in milliseconds</param>
        /// <param name="maxTotalDelayMs">Max total delay before <exception cref="TimeoutException" /> is thrown in milliseconds</param>
        public RetryHelper(int retryStartDelayMs, int exponentialBackoffFactor, int maxSingleDelayMs, int maxTotalDelayMs)
        {
            this.retryStartDelayMs = retryStartDelayMs;
            this.exponentialBackoffFactor = exponentialBackoffFactor;
            this.maxTotalDelayMs = maxTotalDelayMs;
            this.maxSingleDelayMs = maxSingleDelayMs;
            this.DelayMs = this.RetryStartDelayMs;

            this.watch = new Stopwatch();
            this.watch.Start();
        }

        protected int RetryStartDelayMs
        {
            get { return this.retryStartDelayMs; }
        }

        protected int ExponentialBackoffFactor
        {
            get { return this.exponentialBackoffFactor; }
        }

        protected int MaxTotalDelayMs
        {
            get { return this.maxTotalDelayMs; }
        }

        protected int MaxSingleDelayMs
        {
            get { return this.maxSingleDelayMs; }
        }

        protected int WatchElapsedMs
        {
            get { return (int) this.watch.ElapsedMilliseconds; }
        }

        // Current single retry delay in ms
        protected int DelayMs { get; private set; }

        /// <summary>
        /// Function to do an exponential back-off based on the RetryHelper settings
        /// </summary>
        /// <param name="e">The retriable exception</param>
        /// <param name="cancellationToken">Cancelation token to cancel the possible delay</param>
        /// <exception cref="TimeoutException">Throws when the total time elapsed is more than max delay milliseconds</exception>
        /// <exception cref="TaskCanceledException">Throws if cancelation is requested on the input cancelation token</exception>
        public async Task ExponentialBackoffAsync(Exception e = null, CancellationToken cancellationToken = default(CancellationToken))
        {
            int timeElapsed = this.WatchElapsedMs;

            if (timeElapsed >= this.MaxTotalDelayMs)
            {
                throw new TimeoutException(
                    string.Format(
                        "Cannot backoff as reached max retry delay. MaxTotalDelay {0} : Time Elapsed {1} : LastDelay {2}",
                        this.MaxTotalDelayMs,
                        this.WatchElapsedMs,
                        this.DelayMs),
                    e);
            }

            // If the next delay jump causes a totalDelay > maxTotalDelay, then delay time = time left to reach max
            this.DelayMs = Math.Min(this.DelayMs, this.MaxTotalDelayMs - timeElapsed);

            await Task.Delay(this.DelayMs, cancellationToken).ConfigureAwait(false);

            // New delay will be the min of the next exponential jump and the maxSingleDelay allowed
            this.DelayMs = Math.Min(this.DelayMs * this.ExponentialBackoffFactor, this.MaxSingleDelayMs);
        }

        /// <summary>
        /// Function to do an exponential back-off based on the RetryHelper settings
        /// </summary>
        /// <param name="cancellationToken">Cancelation token to cancel the possible delay</param>
        /// <exception cref="TimeoutException">Throws when the total time elapsed is more than max delay milliseconds</exception>
        /// <exception cref="TaskCanceledException">Throws if cancelation is requested on the input cancelation token</exception>
        public async Task ExponentialBackoffAsync(CancellationToken cancellationToken)
        {
            await this.ExponentialBackoffAsync(null, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Function to reset the retry helper to its initial values
        /// </summary>
        public void Reset()
        {
            this.DelayMs = this.RetryStartDelayMs;
            this.watch.Restart();
        }
    }
}