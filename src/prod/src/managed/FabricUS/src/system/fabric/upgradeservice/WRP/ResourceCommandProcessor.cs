// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.WRP.Model;
    using System.Linq;
    using System.Threading.Tasks;
    
    internal interface IResourceCommandProcessor
    {
        ResourceType Type { get; }
        TraceType TraceType { get; }
        IResourceOperations Operations { get; }
        
        Task<IList<IOperationStatus>> ProcessAsync(
            IList<IOperationDescription> operations,
            IOperationContext context);
    }

    internal abstract class ResourceCommandProcessor : IResourceCommandProcessor
    {
        public abstract ResourceType Type { get; }
        public abstract TraceType TraceType { get; }
        public IResourceOperations Operations { get; } = new ResourceOperations();

        public virtual async Task<IList<IOperationStatus>> ProcessAsync(
            IList<IOperationDescription> operations,
            IOperationContext context)
        {
            operations.ThrowIfNull(nameof(operations));
            context.ThrowIfNull(nameof(context));

            Trace.WriteInfo(TraceType, "ProcessAsync called");
            if (!operations.Any())
            {
                return new List<IOperationStatus>();
            }

            var taskList = new List<Task<IOperationStatus>>();
            Task<IOperationStatus[]> operationTask = null;
            try
            {
                Trace.WriteInfo(TraceType, "ProcessAsync: processing {0} items", operations.Count);
                foreach (var operation in operations)
                {
                    taskList.Add(
                        this.CreateOperationStatusAsync(
                            operation,
                            new OperationContext(
                                context.CancellationToken,
                                context.GetRemainingTimeOrThrow(),
                                context.ContinuationToken)));
                }

                await Task.WhenAll(taskList);
            }
            catch (Exception e)
            {
                HandleException("ProcessAsync", e, this.TraceType);
                if (operationTask?.Exception != null)
                {
                    HandleException("ProcessAsync", operationTask.Exception, this.TraceType);
                }
            }

            var statuses = new List<IOperationStatus>();
            foreach (var t in taskList)
            {
                if (t.Status == TaskStatus.RanToCompletion && t.Result != null)
                {
                    statuses.Add(t.Result);
                    if (t.Result.Status != ResultStatus.InProgress)
                    {
                        Trace.WriteInfo(
                            this.TraceType,
                            this.Operations.TryRemove(t.Result)
                                ? "ProcessAsync: Operation completed for resource. OperationId: {0}. ResourceId: {1}."
                                : "ProcessAsync: Failed to remove completed operation for resource. OperationId: {0}. ResourceId: {1}.",
                            t.Result.OperationSequenceNumber,
                            t.Result.ResourceId);
                    }
                }
            }
            
            Trace.WriteInfo(TraceType, "ProcessAsync: {0} items processed successfully", statuses.Count);

            return statuses;
        }

        public abstract Task<IOperationStatus> CreateOperationStatusAsync(
            IOperationDescription operation,
            IOperationContext context);

        protected virtual bool IsTransientException(Exception ex)
        {
            Func<Exception, bool> isTransient = (exception) => {
                return FabricClientRetryErrors.All.Value.IsRetryable(exception) ||
                       exception is OperationCanceledException ||
                       exception is TimeoutException;
            };

            if (isTransient(ex))
            {
                return true;
            }

            var aggEx = ex as AggregateException;
            if (aggEx != null && aggEx.Flatten().InnerExceptions.All(inner => isTransient(inner)))
            {
                return true;
            }

            return false;
        }

        internal static void HandleException(string methodName, Exception exception, TraceType traceType)
        {
            var te = exception as TimeoutException;
            if (te != null)
            {
                Trace.WriteWarning(traceType, "{0}: Operation timed out. Exception: {1}", methodName, te.ToString());
                return;
            }

            var oce = exception as OperationCanceledException;
            if (oce != null)
            {
                Trace.WriteWarning(traceType, "{0}: One or more tasks have been canceled. Exception: {1}", methodName, oce.ToString());
                return;
            }

            var ae = exception as AggregateException;
            if (ae != null)
            {
                ae.Flatten().Handle(innerException =>
                {
                    Trace.WriteWarning(traceType, "{0}: InnerException encountered: {1}", methodName, innerException.ToString());
                    return true;
                });

                return;
            }

            Trace.WriteWarning(traceType, "{0}: Suppressing exception: {1}", methodName, exception.ToString());
        }
    }    
}