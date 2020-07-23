// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Globalization;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Common;    

    internal class CommandProcessor
    {
        private static readonly TraceType TraceType = new TraceType("Command");

        private readonly IInfrastructureAgentWrapper infrastructureServiceAgent;
        private PendingCommandContextMap pendingStartCommands;
        private PendingCommandContextMap pendingFinishCommands;

        public CommandProcessor(IInfrastructureAgentWrapper infrastructureServiceAgent)
        {
            this.infrastructureServiceAgent = infrastructureServiceAgent;
            this.pendingStartCommands = new PendingCommandContextMap();
            this.pendingFinishCommands = new PendingCommandContextMap();
        }

        public void CancelAllContexts()
        {
            this.pendingStartCommands.Cancel();
            this.pendingFinishCommands.Cancel();
        }

        public Task ScheduleProcessCommand(
            InfrastructureServiceCommand command,
            TimeSpan timeout,
            CancellationToken cancelToken)
        {
            var commandContext = new PendingCommandContext(command, timeout, cancelToken);

            return Task.Factory.StartNew(
                () => this.ProcessCommand(commandContext), 
                cancelToken,
                TaskCreationOptions.PreferFairness,
                TaskScheduler.Default);
        }

        public Task OnReportStartTaskSuccess(string taskId, long instanceId)
        {
            PendingCommandContext commandContext = this.pendingStartCommands.Get(taskId);

            if (commandContext != null)
            {
                if (commandContext.ShouldInjectFault)
                {
                    return Task.Factory.StartNew(() => { throw new InvalidOperationException("OnReportStartTaskSuccess: Injected Fault"); });
                }

                if (commandContext.InstanceId == instanceId)
                {
                    commandContext.Complete();

                    // Leave start context in map for correlation with finish context
                }
                else
                {
                    // Not the instance we were expecting, request again with
                    // the current instance
                    this.ProcessStartCommand(commandContext);
                }
            }

            return Utility.CreateCompletedTask<object>(null);
        }

        public Task OnReportFinishTaskSuccess(string taskId, long instanceId)
        {
            PendingCommandContext startContext = this.pendingStartCommands.Remove(taskId);

            if (startContext != null)
            {
                startContext.Cancel();
            }

            PendingCommandContext finishContext = this.pendingFinishCommands.Get(taskId);

            if (finishContext != null)
            {
                if (finishContext.ShouldInjectFault)
                {
                    return Task.Factory.StartNew(() => { throw new InvalidOperationException("OnReportFinishTaskSuccess: Injected Fault"); });
                }

                if (finishContext.InstanceId == instanceId)
                {
                    finishContext.Complete();
                    this.pendingFinishCommands.Remove(taskId);
                }
                else
                {
                    // Not the instance we were expecting, request again with
                    // the current instance
                    this.ProcessFinishCommand(finishContext);
                }
            }

            return Utility.CreateCompletedTask<object>(null);
        }

        public Task OnReportTaskFailure(string taskId, long instanceId)
        {
            PendingCommandContext startContext = this.pendingStartCommands.Get(taskId);
            PendingCommandContext finishContext = this.pendingFinishCommands.Get(taskId);

            if (startContext != null)
            {
                if (startContext.ShouldInjectFault)
                {
                    return Task.Factory.StartNew(() => { throw new InvalidOperationException("OnReportTaskFailure: Injected Fault"); });
                }

                if (startContext.InstanceId == instanceId)
                {
                    startContext.Fail(Trace.CreateException(
                        TraceType,
                        NativeTypes.FABRIC_ERROR_CODE.E_FAIL, 
                        "Task failure reported"));
                    this.pendingStartCommands.Remove(startContext.MapKey);
                }
                else if (finishContext != null)
                {
                    startContext.Cancel();
                    this.pendingStartCommands.Remove(startContext.MapKey);
                }
                else if (finishContext == null)
                {
                    this.ProcessStartCommand(startContext);
                }
            }

            if (finishContext != null)
            {
                if (finishContext.ShouldInjectFault)
                {
                    return Task.Factory.StartNew(() => { throw new InvalidOperationException("OnReportTaskFailure: Injected Fault"); });
                }

                if (finishContext.InstanceId == instanceId)
                {
                    finishContext.Fail(Trace.CreateException(
                        TraceType,
                        NativeTypes.FABRIC_ERROR_CODE.E_FAIL, 
                        "Task failure reported"));
                    this.pendingFinishCommands.Remove(finishContext.MapKey);
                }
                else
                {
                    this.ProcessFinishCommand(finishContext);
                }
            }

            return Utility.CreateCompletedTask<object>(null);
        }

        private void ProcessCommand(PendingCommandContext commandContext)
        {
            if (commandContext.StartCommand != null)
            {
                if (!this.pendingStartCommands.TryAdd(commandContext))
                {
                    string message = string.Format(
                        CultureInfo.InvariantCulture,
                        "Stale start command {0}:{1}",
                        commandContext.TaskId,
                        commandContext.InstanceId);
                    throw Trace.CreateException(
                        TraceType,
                        NativeTypes.FABRIC_ERROR_CODE.E_ABORT,
                        message);
                }

                this.ProcessStartCommand(commandContext);
            }
            else if (commandContext.FinishCommand != null)
            {
                PendingCommandContext startContext = this.pendingStartCommands.Remove(commandContext.MapKey);

                if (startContext != null)
                {
                    commandContext.FinishCommand.TryUpdateInstanceId(startContext.InstanceId);
                    startContext.Cancel();
                }
                else
                {
                    commandContext.FinishCommand.TryUpdateInstanceId(DateTime.UtcNow.Ticks);
                }

                if (!this.pendingFinishCommands.TryAdd(commandContext))
                {
                    string message = string.Format(
                        CultureInfo.InvariantCulture,
                        "Stale finish command {0}:{1}",
                        commandContext.TaskId,
                        commandContext.InstanceId);
                    throw Trace.CreateException(
                        TraceType,
                        NativeTypes.FABRIC_ERROR_CODE.E_ABORT,
                        message);
                }

                this.ProcessFinishCommand(commandContext);
            }
            else
            {
                throw Trace.CreateException(
                    TraceType, 
                    NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG,
                    "Invalid Infrastructure Service command");
            }
        }

        private void ProcessStartCommand(PendingCommandContext commandContext)
        {
            var task = this.infrastructureServiceAgent.StartInfrastructureTaskAsync(
                commandContext.StartCommand.TaskDescription,
                commandContext.RemainingTimeout,
                commandContext.CancelToken);
            task.ContinueWith((prev) => 
                {
                    this.VerifyQueryResults(
                        commandContext, 
                        commandContext.StartCommand.TaskId, 
                        commandContext.StartCommand.InstanceId, 
                        commandContext.StartCommand.TaskDescription.NodeTasks);
                    this.OnInfrastructureTaskRequestComplete(prev, commandContext);
                });
        }

        private void ProcessFinishCommand(PendingCommandContext commandContext)
        {
            var task = this.infrastructureServiceAgent.FinishInfrastructureTaskAsync(
                commandContext.TaskId,
                commandContext.InstanceId,
                commandContext.RemainingTimeout,
                commandContext.CancelToken);
            task.ContinueWith((prev) =>
            {
                this.VerifyQueryResults(
                    commandContext,
                    commandContext.TaskId,
                    commandContext.InstanceId,
                    null);
                this.OnInfrastructureTaskRequestComplete(prev, commandContext);
            });
        }

        private void OnInfrastructureTaskRequestComplete(Task prevTask, PendingCommandContext commandContext)
        {
            var ex = prevTask.Exception;

            if (ex != null)
            {
                Exception translated = Utility.TryTranslateManagedExceptionToCOM(ex.InnerException);
                commandContext.Fail(translated == null ? ex.InnerException : translated);
            }
            else
            {
                // Do nothing: wait for the ack from CM
            }
        }

        private void VerifyQueryResults(
            PendingCommandContext commandContext,
            string taskId,
            long instanceId,
            ReadOnlyCollection<NodeTaskDescription> nodeTasks)
        {
            var task = this.infrastructureServiceAgent.QueryInfrastructureTaskAsync(
                commandContext.RemainingTimeout,
                commandContext.CancelToken);

            var queryResult = task.Result;

            bool isTaskFound = false;

            StringBuilder resultBuilder = new StringBuilder();
            foreach (var it in queryResult.Items)
            {
                if (taskId == it.Description.TaskId && instanceId <= it.Description.InstanceId)
                {
                    isTaskFound = true;

                    if (nodeTasks != null && !NodeTaskDescription.AreEqual(nodeTasks, it.Description.NodeTasks))
                    {
                        isTaskFound = false;
                    }
                }

                StringBuilder nodeBuilder = new StringBuilder();
                foreach (var node in it.Description.NodeTasks)
                {
                    nodeBuilder.AppendFormat(CultureInfo.InvariantCulture,
                        " {0}:{1} ",
                        node.NodeName, 
                        node.TaskType);
                }

                resultBuilder.AppendFormat(CultureInfo.InvariantCulture,
                    "Task = '{0}:{1}({2})[{3}]'{4}",
                    it.Description.TaskId,
                    it.Description.InstanceId,
                    it.State,
                    nodeBuilder.ToString(),
                    Environment.NewLine);
            }

            Trace.WriteInfo(
                TraceType,
                "QueryResults = {0}",
                resultBuilder.ToString());

            if (!isTaskFound)
            {
                throw Trace.CreateException(
                    TraceType,
                    NativeTypes.FABRIC_ERROR_CODE.E_FAIL,
                    "VerifyQueryResults({0}, {1}, {2}) failed",
                    taskId,
                    instanceId,
                    nodeTasks);
            }
        }

        private class PendingCommandContext
        {
            private readonly InfrastructureServiceCommand command;
            private readonly Stopwatch stopwatch;
            private readonly TimeSpan timeout;
            private readonly Timer timer;
            private readonly CancellationToken cancelToken;
            private TaskCompletionSource<object> completionTask;
            private EventHandler faultHandler;
            private readonly object handlerLock;
            private bool injectReplyFault;

            public PendingCommandContext(
                InfrastructureServiceCommand command,
                TimeSpan timeout,
                CancellationToken cancelToken)
            {
                this.command = command;

                this.stopwatch = new Stopwatch();
                this.timeout = timeout;
                this.timer = new Timer(this.OnTimeout, null, Timeout.Infinite, Timeout.Infinite);
                this.cancelToken = cancelToken;

                this.faultHandler = null;
                this.handlerLock = new object();

                // Attempt to reply with a failure to the CM
                // upon first processing each notification to
                // cover reply notification reply logic at the CM
                //
                this.injectReplyFault = true;
            }

            public StartTaskCommand StartCommand
            {
                get { return this.command as StartTaskCommand; }
            }

            public FinishTaskCommand FinishCommand
            {
                get { return this.command as FinishTaskCommand; }
            }

            public string TaskId
            {
                get { return this.command.TaskId; }
            }

            public long InstanceId
            {
                get { return this.command.InstanceId; }
            }

            public TimeSpan RemainingTimeout
            {
                get 
                {
                    if (this.stopwatch.Elapsed > this.timeout)
                    {
                        this.stopwatch.Stop();
                        return TimeSpan.Zero;
                    }
                    else
                    {
                        return this.timeout - this.stopwatch.Elapsed;
                    }
                }
            }

            public bool ShouldInjectFault
            {
                get
                {
                    var tmp = this.injectReplyFault;
                    this.injectReplyFault = false;
                    return tmp;
                }
            }

            public CancellationToken CancelToken
            {
                get { return this.cancelToken; }
            }

            public string MapKey
            {
                get { return this.command.TaskId; }
            }

            public bool TryStartContext(EventHandler handler)
            {
                bool isStarted = false;

                // Block the root Task from completing until we explicitly complete this context
                this.completionTask = new TaskCompletionSource<object>(TaskCreationOptions.AttachedToParent);
                this.cancelToken.Register(this.Cancel);

                lock (this.completionTask)
                {
                    if (!this.completionTask.Task.IsCompleted)
                    {
                        isStarted = true;
                        this.stopwatch.Start();
                        this.timer.Change(this.timeout, TimeSpan.FromMilliseconds(-1));

                        this.faultHandler = handler;
                    }
                }

                return isStarted;
            }
            
            public void Complete()
            {
                lock (this.completionTask)
                {
                    this.completionTask.TrySetResult(null);
                }

                // Do not stop timers in this case. Allow timeouts
                // to occur and remove this context if needed since
                // we may keep them around after completion.
            }

            public void Cancel()
            {
                this.Fail(Trace.CreateException(
                    TraceType,
                    NativeTypes.FABRIC_ERROR_CODE.E_ABORT,
                    "Command for task {0}:{1} aborted",
                    this.command.TaskId,
                    this.command.InstanceId));
            }

            public void Fail(Exception ex)
            {
                lock (this.completionTask)
                {
                    this.completionTask.TrySetException(ex);
                }
                
                this.Cleanup();
            }
            
            private void OnTimeout(object o)
            {
                this.Fail(Trace.CreateException(
                    TraceType,
                    NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT,
                    "Command for task {0}:{1} timed out",
                    this.command.TaskId,
                    this.command.InstanceId));
            }

            private void Cleanup()
            {
                this.timer.Change(TimeSpan.FromMilliseconds(-1), TimeSpan.FromMilliseconds(-1));
                this.stopwatch.Stop();
            
                EventHandler handler = null;

                lock (this.handlerLock)
                {
                    handler = this.faultHandler;
                    this.faultHandler = null;
                }

                if (handler != null)
                {
                    handler(this, null);
                }
            }
        }

        private class PendingCommandContextMap
        {
            private Dictionary<string, PendingCommandContext> contexts;

            public PendingCommandContextMap()
            {
                this.contexts = new Dictionary<string, PendingCommandContext>();
            }

            public void Cancel()
            {
                Dictionary<string, PendingCommandContext> snap = null;
                lock (this.contexts)
                {
                    snap = new Dictionary<string, PendingCommandContext>(this.contexts);
                    this.contexts.Clear();
                }

                foreach (var context in snap.Values)
                {
                    context.Cancel();
                }
            }

            public bool TryAdd(PendingCommandContext context)
            {
                bool isAdded = false;
                PendingCommandContext oldContext = null;

                lock (this.contexts)
                {
                    if (this.contexts.ContainsKey(context.MapKey))
                    {
                        if (this.contexts[context.MapKey].InstanceId <= context.InstanceId)
                        {
                            oldContext = this.contexts[context.MapKey];

                            if (context.TryStartContext(this.ContextFaultHandler))
                            {
                                this.contexts[context.MapKey] = context;
                                isAdded = true;
                            }
                        }
                    }
                    else
                    {
                        if (context.TryStartContext(this.ContextFaultHandler))
                        {
                            this.contexts.Add(context.MapKey, context);
                            isAdded = true;
                        }
                    }
                }

                if (oldContext != null)
                {
                    oldContext.Cancel();
                }

                return isAdded;
            }

            public PendingCommandContext Get(string key)
            {
                PendingCommandContext context = null;

                lock (this.contexts)
                {
                    if (this.contexts.ContainsKey(key))
                    {
                        context = this.contexts[key];
                    }
                }

                return context;
            }

            public PendingCommandContext Remove(string key)
            {
                PendingCommandContext context = null;

                lock (this.contexts)
                {
                    if (this.contexts.ContainsKey(key))
                    {
                        context = this.contexts[key];
                        this.contexts.Remove(key);
                    }
                }

                return context;
            }

            private void ContextFaultHandler(object src, EventArgs args)
            {
                this.Remove(((PendingCommandContext)src).MapKey);
            }
        }
    }
}