// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.ClusterAnalysis.Services
{
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using global::ClusterAnalysis.Common.Log;
    using global::ClusterAnalysis.Common.PerformanceTracker;

    public class PerformanceSessionManager : IPerformanceSessionManager
    {
        private ILogger logger;

        private object singleAccessLock = new object();

        private List<PerformanceSession> sessionsUnderManagement = new List<PerformanceSession>();

        private Task sessionLoggerTask;

        internal PerformanceSessionManager(ILogProvider logProvider, CancellationToken token)
        {
            this.logger = logProvider.CreateLoggerInstance("PerformanceSessionManager");
            this.sessionLoggerTask = this.CreateSessionLoggerTask(token);
        }

        public IReadOnlyList<PerformanceSession> Sessions
        {
            get
            {
                lock (this.singleAccessLock)
                {
                    return this.sessionsUnderManagement.AsReadOnly();
                }
            }
        }

        public PerformanceSession CreateSession(string sessionName, ITimeProvider timeProvider)
        {
            var session = new PerformanceSession(sessionName, timeProvider);

            lock (this.singleAccessLock)
            {
                this.sessionsUnderManagement.Add(session);
            }

            return session;
        }

        public PerformanceSession CreateSession(string sessionName)
        {
            return this.CreateSession(sessionName, new RealTimeProvider());
        }

        private Task CreateSessionLoggerTask(CancellationToken token)
        {
            return Task.Run(() => this.LogNewSessionsAsync(token), token).ContinueWith(
                task => { this.logger.LogWarning("Task Died with Exception: {0}", task.Exception.InnerExceptions); },
                TaskContinuationOptions.OnlyOnFaulted);
        }

        private async Task LogNewSessionsAsync(CancellationToken token)
        {
            while (!token.IsCancellationRequested)
            {
                lock (this.singleAccessLock)
                {
                    foreach (var oneSession in this.sessionsUnderManagement)
                    {
                        if (token.IsCancellationRequested)
                        {
                            this.logger.LogMessage("LogNewSessionsAsync:: Cancellation Requested. Exiting");
                            return;
                        }

                        if (oneSession.HasEnded)
                        {
                            this.logger.LogMessage(
                                "SessionData: Name: {0}, CreateTime: {1}, StartTime: {2}, EndTime: {3}, Properties: {4}",
                                oneSession.SessionName,
                                oneSession.SessionCreateTime,
                                oneSession.StartTime,
                                oneSession.EndTime,
                                string.Join("\n", oneSession.CacheItems));
                        }
                    }

                    this.sessionsUnderManagement.RemoveAll(session => session.HasEnded);
                }

                await Task.Delay(TimeSpan.FromMinutes(1), token).ConfigureAwait(false);
            }
        }
    }
}