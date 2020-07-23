// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service
{
    using System;
    using System.Fabric;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using EventStore.Service.LogProvider;

    internal static class TaskRunner
    {
        public static Task RunAsync(string friendlyName, Func<Task> workItem, Func<string, Func<Task>, Exception, Task> callOnUnhandledException, CancellationToken token)
        {
            Assert.IsNotNull(friendlyName, "friendlyName != null");
            return Task.Run(workItem, token).ContinueWith(GetDefaultContinuation(friendlyName, workItem, callOnUnhandledException, token), token);
        }

        private static Action<Task> GetDefaultContinuation(string taskName, Func<Task> workItem, Func<string, Func<Task>, Exception, Task> unhandledExceptionHandler, CancellationToken token)
        {
            return async task =>
            {
                EventStoreLogger.Logger.LogMessage("TaskName: {0} Exited with Status : {1}", taskName, task.Status);
                if (task.Status != TaskStatus.Faulted)
                {
                    return;
                }

                EventStoreLogger.Logger.LogMessage(
                    "TaskName: {0} Faulted. Exeception {1}",
                    taskName,
                    ExtractInformationFromException(task.Exception));

                if (task.Exception.InnerException is OperationCanceledException && token.IsCancellationRequested)
                {
                    EventStoreLogger.Logger.LogMessage("TaskName: {0}. Cancellation Requested. Returning", taskName);
                    return;
                }

                if (task.Exception.InnerException is FabricObjectClosedException)
                {
                    EventStoreLogger.Logger.LogMessage("TaskName: {0}. FabricClosedException. Detail: {1}", taskName, task.Exception.InnerException);
                    return;
                }

                if (task.Exception.InnerException is FabricNotPrimaryException)
                {
                    EventStoreLogger.Logger.LogMessage("TaskName: {0}. FabricNotPrimary. Detail: {1}", taskName, task.Exception.InnerException);
                    return;
                }

                EventStoreLogger.Logger.LogWarning(
                    "TaskName: {0}. Task Encountered unexpected Exception '{1}', Data '{2}'",
                    taskName,
                    task.Exception,
                    ExtractInformationFromException(task.Exception));

                if (unhandledExceptionHandler != null)
                {
                    await unhandledExceptionHandler(taskName, workItem, task.Exception.InnerException).ConfigureAwait(false);
                    return;
                }

                Environment.FailFast(ExtractInformationFromException(task.Exception), task.Exception);
            };
        }

        /// <summary>
        /// </summary>
        /// <param name="exp"></param>
        /// <returns></returns>
        private static string ExtractInformationFromException(AggregateException exp)
        {
            StringBuilder details = new StringBuilder();
            if (exp == null)
            {
                return details.ToString();
            }

            details.AppendFormat("Exception Type: {0}, Message: {1}, Stack: {2}", exp.GetType(), exp.Message, exp.StackTrace);

            exp = exp.Flatten();
            if (exp.InnerExceptions == null)
            {
                return details.ToString();
            }

            foreach (var oneExp in exp.Flatten().InnerExceptions)
            {
                details.AppendFormat("Inner Exception Details");
                details.AppendFormat("Exception Type: {0}, Message: {1}, Stack: {2}", oneExp.GetType(), oneExp.Message, oneExp.StackTrace);
                var enumerator = oneExp.Data.GetEnumerator();
                while (enumerator.MoveNext())
                {
                    details.AppendFormat("{0}={1}", enumerator.Key, enumerator.Value);
                }
            }

            return details.ToString();
        }
    }
}