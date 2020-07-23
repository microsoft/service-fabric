// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.PerformanceTracker
{
    using System.Collections.Generic;

    public interface IPerformanceSessionManager
    {
        /// <summary>
        /// Get a collection of all the sessions
        /// </summary>
        IReadOnlyList<PerformanceSession> Sessions { get; }

        /// <summary>
        /// Create a new Session
        /// </summary>
        /// <param name="sessionName"></param>
        /// <param name="timeProvider"></param>
        /// <returns></returns>
        PerformanceSession CreateSession(string sessionName, ITimeProvider timeProvider);

        /// <summary>
        /// Create a new Session
        /// </summary>
        /// <param name="sessionName"></param>
        /// <returns></returns>
        PerformanceSession CreateSession(string sessionName);
    }
}