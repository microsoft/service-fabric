// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Diagnostics;
    using System.Fabric.ReliableMessaging;
    using System.Threading;
    using System.Threading.Tasks;

    #endregion

    /// <summary>
    /// Provides task timeout capability, to indicate if the task completed within a specified time period or timed out.
    /// </summary>
    internal class TimeoutHandler
    {
        #region helper methods

        /// <summary>
        /// Validates the timeout period.
        /// </summary>
        /// <param name="timeout">The timeout period</param>
        /// <returns>Bool</returns>
        internal static bool ValidRange(TimeSpan timeout)
        {
            // Check for overflow.
            var maxDelay = new TimeSpan(Int32.MaxValue);
            if (timeout > maxDelay)
            {
                return false;
            }

            // Support for long running txn.
            if (timeout == Timeout.InfiniteTimeSpan)
            {
                return true;
            }

            // Invalid timeout period.
            if (timeout.TotalMilliseconds < 0)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Subtracts Finish and Start timespan.  
        /// </summary>
        /// <param name="stepFinishTime">Finish time</param>
        /// <param name="stepStartTime">Start time</param>
        /// <returns>Duration</returns>
        internal static TimeSpan Subtract(DateTime stepFinishTime, DateTime stepStartTime)
        {
            var result = stepFinishTime.Subtract(stepStartTime);

            // Assert when start > finish time.
            var milliseconds = (int) result.TotalMilliseconds;
            if (milliseconds < 0)
            {
                Debugger.Break();
            }
            Diagnostics.Assert(milliseconds >= 0, string.Format("Unexpected step duration: {0}", milliseconds));

            return result;
        }

        /// <summary>
        /// Update new timeout value. 
        /// i.e original time out value subtracted by time spent already (finish - start).
        /// </summary>
        /// <param name="previousTimout">Previous time out value</param>
        /// <param name="stepStartTime">Start time, when task started.</param>
        /// <param name="stepFinishTime">Finish time, time already spent.</param>
        /// <returns>New timeout value</returns>
        internal static TimeSpan UpdateTimeout(TimeSpan previousTimout, DateTime stepStartTime, DateTime stepFinishTime)
        {
            // Return, if timeout set to infinite
            if (previousTimout == Timeout.InfiniteTimeSpan)
            {
                return Timeout.InfiniteTimeSpan;
            }

            // Calculate remaining leftover timeout period.
            var newTimeout = previousTimout - Subtract(stepFinishTime, stepStartTime);

            // Raise an timeout exception if timeout already passed
            if (newTimeout.TotalMilliseconds < 0)
            {
                throw new TimeoutException();
            }

            return newTimeout;
        }

        #endregion

        /// <summary>
        /// Timeout handler: Return true: if the task completed within the timeout period, false otherwise.
        /// Note: This routine should be used with care--in particular the management of completionTask is the caller's responsibility
        /// especially in cases where completionTask may fault (throw an exception)
        /// </summary>
        /// <param name="timeout">Timeout period</param>
        /// <param name="completionTask">Completion Task</param>
        /// <returns>True, if completion task completed before timeout expired, false otherwise</returns>
        internal static async Task<bool> WaitWithDelay(TimeSpan timeout, Task completionTask)
        {
            // Is valid timeout period
            if (ValidRange(timeout) == false)
            {
                throw new ArgumentOutOfRangeException("timeout", timeout, null);
            }

            var source = new CancellationTokenSource();
            var token = source.Token;
            Task[] tasks = {Task.Delay(timeout, token), completionTask};

            // Wait for completion task to complete or timeout task to expire.
            await Task.WhenAny(tasks);

            // Check if completion task completed before timeout period.
            if (completionTask.IsCompleted)
            {
                // Cancel the internal timer job launched by Task.Delay call.
                source.Cancel();
                return true;
            }

            // Timeout expired.
            return false;
        }
    }
}