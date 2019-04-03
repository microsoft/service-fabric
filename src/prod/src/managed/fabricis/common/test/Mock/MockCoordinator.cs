// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using System;
    using Threading;
    using Threading.Tasks;

    internal sealed class MockCoordinator : IInfrastructureCoordinator
    {
        public Func<string, long, TimeSpan, CancellationToken, Task> ReportFinishTaskSuccessAsyncHandler;
        public Func<string, long, TimeSpan, CancellationToken, Task> ReportStartTaskSuccessAsyncHandler;
        public Func<string, long, TimeSpan, CancellationToken, Task> ReportTaskFailureAsyncHandler;
        public Func<int, CancellationToken, Task> RunAsyncHandler;
        public Func<bool, string, TimeSpan, CancellationToken, Task<string>> RunCommandAsyncHandler;

        public Task ReportFinishTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            if (this.ReportFinishTaskSuccessAsyncHandler != null)
                return this.ReportFinishTaskSuccessAsyncHandler(taskId, instanceId, timeout, cancellationToken);

            throw new NotImplementedException();
        }

        public Task ReportStartTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            if (this.ReportStartTaskSuccessAsyncHandler != null)
                return this.ReportStartTaskSuccessAsyncHandler(taskId, instanceId, timeout, cancellationToken);

            throw new NotImplementedException();
        }

        public Task ReportTaskFailureAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            if (this.ReportTaskFailureAsyncHandler != null)
                return this.ReportTaskFailureAsyncHandler(taskId, instanceId, timeout, cancellationToken);

            throw new NotImplementedException();
        }

        public Task RunAsync(int primaryEpoch, CancellationToken token)
        {
            if (this.RunAsyncHandler != null)
                return this.RunAsyncHandler(primaryEpoch, token);

            throw new NotImplementedException();
        }

        public Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            if (this.RunCommandAsyncHandler != null)
                return this.RunCommandAsyncHandler(isAdminCommand, command, timeout, cancellationToken);

            throw new NotImplementedException();
        }
    }
}