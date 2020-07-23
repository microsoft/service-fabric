// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.ClusterAnalysis.Services
{
    using System;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using global::ClusterAnalysis.Common;
    using global::ClusterAnalysis.Common.Log;
    using global::ClusterAnalysis.Common.Util;

    internal class TaskRunner : ITaskRunner
    {
        private ILogger logger;

        private Func<string, Func<Task>, Exception, Task> onUnhandledException;

        public TaskRunner(ILogProvider logProvider, Func<string, Func<Task>, Exception, Task> callOnUnhandledException)
        {
            Assert.IsNotNull(logProvider, "logProvider");
            Assert.IsNotNull(callOnUnhandledException, "callOnUnhandledException");
            this.logger = logProvider.CreateLoggerInstance("TaskRunner");
            this.onUnhandledException = callOnUnhandledException;
        }

        public Task Run(string friendlyName, Func<Task> workItem, CancellationToken token)
        {
            Assert.IsNotNull(friendlyName, "friendlyName != null");
            return Task.Run(workItem, token).ContinueWith(this.GetDefaultContinuation(friendlyName, workItem, token), token);
        }

        public Action<Task> GetDefaultContinuation(string taskName, CancellationToken token)
        {
            return this.GetDefaultContinuation(taskName, null, token);
        }

        private Action<Task> GetDefaultContinuation(string taskName, Func<Task> workItem, CancellationToken token)
        {
            return async task =>
                {
                    this.logger.LogMessage("TaskName: {0} Exited with Status : {1}", taskName, task.Status);
                    if (task.Status != TaskStatus.Faulted)
                    {
                        return;
                    }

                    this.logger.LogMessage(
                        "TaskName: {0} Fauled. Exeception {1}",
                        taskName,
                        HandyUtil.ExtractInformationFromException(task.Exception));

                    if (task.Exception.InnerException is OperationCanceledException && token.IsCancellationRequested)
                    {
                        this.logger.LogMessage("TaskName: {0}. Cancellation Requested. Returning", taskName);
                        return;
                    }

                    if (task.Exception.InnerException is FabricObjectClosedException)
                    {
                        this.logger.LogMessage("TaskName: {0}. FabricClosedException. Detail: {1}", taskName, task.Exception.InnerException);
                        return;
                    }

                    if (task.Exception.InnerException is FabricNotPrimaryException)
                    {
                        this.logger.LogMessage("TaskName: {0}. FabricNotPrimary. Detail: {1}", taskName, task.Exception.InnerException);
                        return;
                    }

                    this.logger.LogWarning(
                        "TaskName: {0}. Task Encountered unexpected Exception '{1}', Data '{2}'",
                        taskName,
                        task.Exception,
                        HandyUtil.ExtractInformationFromException(task.Exception));

                    await this.onUnhandledException(taskName, workItem, task.Exception.InnerException).ConfigureAwait(false);
                };
            }
    }
}