// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Threading;
using System.Threading.Tasks;

namespace System.Fabric.InfrastructureService
{
    internal class NullCoordinator : IInfrastructureCoordinator
    {
        private static readonly TraceType TraceType = new TraceType("NullCoordinator");

        public Task ReportFinishTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "ReportFinishTaskSuccessAsync({0},{1})", taskId, instanceId);
            return Task.FromResult(true);
        }

        public Task ReportStartTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "ReportStartTaskSuccessAsync({0},{1})", taskId, instanceId);
            return Task.FromResult(true);
        }

        public Task ReportTaskFailureAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "ReportTaskFailureAsync({0},{1})", taskId, instanceId);
            return Task.FromResult(true);
        }

        public Task RunAsync(int primaryEpoch, CancellationToken token)
        {
            Trace.WriteInfo(TraceType, "RunAsync(0x{0:X})", primaryEpoch);

            TaskCompletionSource<object> tcs = new TaskCompletionSource<object>();

            token.Register(() =>
            {
                Trace.WriteInfo(TraceType, "Stopping");
                tcs.TrySetCanceled();
            });

            return tcs.Task;
        }

        public Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "RunCommandAsync({0},{1})", isAdminCommand, command);
            return TaskUtility.ThrowAsync(new NotImplementedException());
        }
    }
}