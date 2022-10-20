// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Interop;
using System.Threading;
using System.Threading.Tasks;

namespace System.Fabric.InfrastructureService
{
    internal sealed class TestInfrastructureCoordinator : IInfrastructureCoordinator
    {
        private static readonly TraceType TraceType = new TraceType("TestInfrastructureCoordinator");

        private readonly Guid partitionId;
        private readonly object stateLock = new object();
        private readonly CommandProcessor commandProcessor;

        private TaskCompletionSource<object> runTaskCompletionSource;

        public TestInfrastructureCoordinator(IInfrastructureAgentWrapper infrastructureServiceAgent, Guid partitionId)
        {
            this.commandProcessor = new CommandProcessor(infrastructureServiceAgent);
            this.partitionId = partitionId;
        }

        public Task RunAsync(int primaryEpoch, CancellationToken token)
        {
            var runtime = FabricRuntime.Create();

            Trace.ConsoleWriteLine(TraceType, "Running test coordinator {0}:{1}",
                runtime.CodePackageActivationContext.CodePackageName,
                runtime.CodePackageActivationContext.CodePackageVersion);

            runtime = null;

            lock (this.stateLock)
            {
                if (this.runTaskCompletionSource != null)
                    throw new InvalidOperationException("Coordinator has already been started");

                token.Register(this.OnCancelRequested);

                this.runTaskCompletionSource = new TaskCompletionSource<object>();
                return this.runTaskCompletionSource.Task;
            }
        }

        private void OnCancelRequested()
        {
            lock (this.stateLock)
            {
                this.commandProcessor.CancelAllContexts();

                this.runTaskCompletionSource.SetCanceled();
                this.runTaskCompletionSource = null;
            }
        }

        private Task<string> CheckPrimary()
        {
            TaskCompletionSource<string> notPrimaryTask = null;

            lock (this.stateLock)
            {
                if (this.runTaskCompletionSource == null)
                {
                    notPrimaryTask = new TaskCompletionSource<string>(TaskCreationOptions.AttachedToParent);
                    notPrimaryTask.TrySetException(Trace.CreateException(
                        TraceType,
                        NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_PRIMARY,
                        "Replica is not primary"));
                }
            }

            return notPrimaryTask == null ? null : notPrimaryTask.Task;
        }

        public Task ReportStartTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "ReportStartTaskSuccess: {0}:{1}", taskId, instanceId);

            var notPrimaryTask = this.CheckPrimary();
            if (notPrimaryTask != null)
            {
                return notPrimaryTask;
            }

            // Need to do synchronization to make sure that a call doesn't get past CheckPrimary and then
            // ChangeRole to secondary starts and finishes, and then the outstanding operation executes
            // as secondary when it's supposed to be a primary-only operation?

            return this.commandProcessor.OnReportStartTaskSuccess(taskId, instanceId);
        }

        public Task ReportFinishTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "ReportFinishTaskSuccess: {0}:{1}", taskId, instanceId);

            var notPrimaryTask = this.CheckPrimary();
            if (notPrimaryTask != null)
            {
                return notPrimaryTask;
            }

            return this.commandProcessor.OnReportFinishTaskSuccess(taskId, instanceId);
        }

        public Task ReportTaskFailureAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "ReportTaskFailure: {0}:{1}", taskId, instanceId);

            var notPrimaryTask = this.CheckPrimary();
            if (notPrimaryTask != null)
            {
                return notPrimaryTask;
            }

            return this.commandProcessor.OnReportTaskFailure(taskId, instanceId);
        }

        public Task<string> RunCommandAsync(bool isAdminCommand, string input, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "RunCommand: {0} (isAdminCommand = {1})", input, isAdminCommand);

            var notPrimaryTask = this.CheckPrimary();
            if (notPrimaryTask != null)
            {
                return notPrimaryTask;
            }

            // TestFabricClient.cpp depends on this string format for verification.
            // If you change this string, be sure to update the test.
            string replyMessage = string.Format(
                "[TestInfrastructureCoordinator reply for RunCommandAsync({0},'{1}')]",
                isAdminCommand,
                input);

            if (isAdminCommand)
            {
                if (string.Equals(input, "echo", StringComparison.OrdinalIgnoreCase))
                {
                    return Utility.CreateCompletedTask(replyMessage);
                }
                if (string.Equals(input, "emptyresponse", StringComparison.OrdinalIgnoreCase))
                {
                    return Utility.CreateCompletedTask<string>(null);
                }

                var command = InfrastructureServiceCommand.TryParseCommand(this.partitionId, input);

                return this.commandProcessor.ScheduleProcessCommand(command, timeout, cancellationToken)
                    .Then(_ => replyMessage);
            }
            else
            {
                return Utility.CreateCompletedTask(replyMessage);
            }
        }
    }
}