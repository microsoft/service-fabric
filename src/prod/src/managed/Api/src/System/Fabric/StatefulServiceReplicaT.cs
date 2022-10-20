// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Globalization;
    using System.Linq;
    using System.Net;
    using System.Reflection;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using System.IO;
    using System.Security;

    internal interface ISupportEvictableProgramInstances
    {
        long GetVersion(IComparable instanceId);

        bool CanEvict(IComparable instanceId);

        bool EvictInstance(IComparable instanceId, long version);
    }

    public class StatefulServiceReplica<TInstance> : StatefulServiceReplica, ISupportEvictableProgramInstances
    {
        internal const string SessionIdHeaderName = "Session";
        internal const string SessionExpirationPrefix = "Expires=";
        internal const string FabricExceptionHeaderName = "FxEE";
        internal const string MessageIdHeaderName = "MessageId";
        internal const string PartitionKeyHeaderName = "PartitionKey";

        internal static readonly string SingletonSessionId = Helpers.GenerateUniqueId(new Guid("00000000-0000-0000-C000-000000000046"));
        private readonly InstanceManager<StatefulProgramInstance> instanceManager = new InstanceManager<StatefulProgramInstance>();

        public StatefulServiceReplica()
        {
        }

        public sealed override bool CanSupportTransaction
        {
            get
            {
                return true;
            }
        }

        /// Seal the override since StateLocks aren't relevant for the side-car model
        public sealed override IEnumerable<StateLock> StateLocks
        {
            get
            {
                return Enumerable.Empty<StateLock>();
            }
        }

        internal override bool IsReplicableType
        {
            get
            {
                return false;
            }
        }

        long ISupportEvictableProgramInstances.GetVersion(IComparable instanceId)
        {
            StatefulProgramInstance instance = this.instanceManager.GetInstance(instanceId);
            return instance != null ? instance.Version : -1;
        }

        bool ISupportEvictableProgramInstances.CanEvict(IComparable instanceId)
        {
            StatefulProgramInstance instance = this.instanceManager.GetInstance(instanceId);
            return instance == null || instance.TypeInfo.CanEvict;
        }

        bool ISupportEvictableProgramInstances.EvictInstance(IComparable instanceId, long version)
        {
            return this.DeleteInstanceInternal(instanceId, true, version);
        }

        internal override StateManager CreateStateManager()
        {
            return new StatefulComponentStateManager(this);
        }

        internal static bool GetSessionInformation(object requestObj, out IComparable sessionId, out DateTime expiry)
        {
            sessionId = null;
            expiry = DateTime.MaxValue;

            object cookieOrHeader = null;
            HttpListenerRequest request = requestObj as HttpListenerRequest;
            if (request != null)
            {
                cookieOrHeader = (request.Cookies != null) ? request.Cookies[StatefulServiceReplica<TInstance>.SessionIdHeaderName] : null;
                if (cookieOrHeader == null)
                {
                    cookieOrHeader = request.Headers[StatefulServiceReplica<TInstance>.SessionIdHeaderName];
                }
            }
            else
            {
                IDictionary<string, string> headers = requestObj as IDictionary<string, string>;
                if (headers != null)
                {
                    cookieOrHeader = headers[StatefulServiceReplica<TInstance>.SessionIdHeaderName];
                }
            }

            Cookie sessionCookie = cookieOrHeader as Cookie;
            if (sessionCookie != null)
            {
                sessionId = sessionCookie.Value;
                expiry = sessionCookie.Expires == DateTime.MinValue ? DateTime.MaxValue : sessionCookie.Expires;
            }
            else
            {
                string sessionHeader = cookieOrHeader as string;
                if (sessionHeader != null)
                {
                    string[] headerParts = sessionHeader.Split(Constants.Separators.Header, StringSplitOptions.RemoveEmptyEntries);
                    if (headerParts != null)
                    {
                        if (headerParts.Length == 1)
                        {
                            sessionId = headerParts[0];
                        }
                        else if (headerParts.Length == 2 &&
                                 headerParts[1].StartsWith(StatefulServiceReplica<TInstance>.SessionExpirationPrefix, StringComparison.OrdinalIgnoreCase))
                        {
                            sessionId = headerParts[0];
                            if (DateTime.TryParse(headerParts[1].Substring(StatefulServiceReplica<TInstance>.SessionExpirationPrefix.Length), out expiry))
                            {
                                expiry = DateTime.MaxValue;
                            }
                        }
                    }
                }
            }

            return sessionId != null;
        }

        internal static void SetSessionInformation(object responseObj, IComparable sessionId, DateTime expiry)
        {
            //// TODO: We need to add domain information which will be published address
            //// sessionCookie.Domain =
            string sessionIdHeaderValue = String.Format("{0};{1}{2}", sessionId.ToString(), StatefulServiceReplica<TInstance>.SessionExpirationPrefix, expiry);
            HttpListenerResponse response = responseObj as HttpListenerResponse;
            if (response != null)
            {
                if (response.Cookies == null)
                {
                    response.Cookies = new CookieCollection();
                }

                Cookie sessionCookie = new Cookie(StatefulServiceReplica<TInstance>.SessionIdHeaderName, sessionId.ToString());
                sessionCookie.Expires = expiry;
                response.Cookies.Add(sessionCookie);
                response.Headers.Add(StatefulServiceReplica<TInstance>.SessionIdHeaderName, sessionIdHeaderValue);
            }
            else
            {
                IDictionary<string, string> headers = responseObj as IDictionary<string, string>;
                if (headers != null)
                {
                    headers[StatefulServiceReplica<TInstance>.SessionIdHeaderName] = sessionIdHeaderValue;
                }
            }
        }

        internal override bool IsDeclarativeModel
        {
            get
            {
                return true;
            }
        }

        protected internal sealed override ReplicationOperation PrepareReplicationOperation(StatefulServiceExecutionContext executionContext, ReplicationOperationType type, IEnumerable<Replicable> replicables, IDictionary<string, string> operationMetadata)
        {
            if (this.IsDisposed)
            {
                AppTrace.TraceMsg(TraceLogEventType.Information, "StatefulServiceReplicaT.PrepareReplicationOperation", "PrepareReplicationOperation called after service was disposed");
                return null;
            }

            ReplicationOperation replicationOperation = null;
            if (type == ReplicationOperationType.Replicate)
            {
                StatefulProgramInstance instance = this.instanceManager.GetInstance(StatefulProgramInstance.CurrentInstanceId);
                if (instance == null)
                {
                    AppTrace.TraceMsg(TraceLogEventType.Error, "StatefulServiceReplicaT.PrepareReplicationOperation", "Could not find StatefulProgramInstance with id '{0}'", StatefulProgramInstance.CurrentInstanceId);
                    throw new InvalidOperationException("Replication needs to be triggered in context of a program instance.");
                }

                IEnumerable<Replicable> attributedReplicables = instance.CollectAttributedReplicableMembers(replicables);
                if (attributedReplicables != null)
                {
                    replicables = Enumerable.Concat(replicables, attributedReplicables);                    
                }

                instance.SetContextInstanceId(replicables);

                IEnumerable<Replicable> invalidReplicables;
                if (!instance.ValidateReplicablesMapToTypeMembers(replicables, out invalidReplicables))
                {
                    string missingReplicables = StatefulProgramInstance.GetReplicablesString(invalidReplicables);
                    StringBuilder error = new StringBuilder();
                    error.AppendFormat("Replication cannot be triggered as following members were not found in instance of type '{0}'.", instance.TypeInfo.ReplicableType.FullName);
                    error.AppendLine(missingReplicables.ToString());
                    AppTrace.TraceMsg(TraceLogEventType.Information, "StatefulServiceReplicaT.PrepareReplicationOperation", error.ToString());
                    throw new InvalidOperationException(error.ToString());
                }

                // TODO: When we make Replicable name to be IComparable, we can remove the ToString call from here.
                replicationOperation = new ReplicationOperation(
                                                    type,
                                                    new List<Replicable>() { new Replicable<StatefulProgramInstance>(instance.Id.ToString(), new StatefulProgramInstance(this, instance.Id, instance.TypeInfo.ReplicableType, instance.InitializationParams, replicables, instance.Singleton)) },
                                                    operationMetadata);
            }
            else if (type == ReplicationOperationType.Copy)
            {
                ItemCollection<IComparable, StatefulProgramInstance> instances;
                using (this.instanceManager.GetInstances(LockPermission.Read, out instances))
                {
                    List<Replicable<StatefulProgramInstance>> replicablesToCopy = new List<Replicable<StatefulProgramInstance>>();
                    foreach (StatefulProgramInstance instance in instances)
                    {
                        // TODO: When we make Replicable name to be IComparable, we can remove the ToString call from here.
                        replicablesToCopy.Add(new Replicable<StatefulProgramInstance>(instance.Id.ToString(), instance));
                    }

                    replicationOperation = new ReplicationOperation(
                                                    type,
                                                    replicablesToCopy,
                                                    operationMetadata);
                }
            }

            if (replicationOperation == null)
            {
                replicationOperation = new ReplicationOperation(type, replicables, operationMetadata);
            }

            return replicationOperation;
        }

        protected internal sealed override IEnumerable<Replicable> ReplicationOperationComplete(StatefulServiceExecutionContext executionContext, ReplicationResult result, ReplicationOperation replicationOperation)
        {
            if (this.IsDisposed)
            {
                AppTrace.TraceMsg(TraceLogEventType.Information, "StatefulServiceReplicaT.ReplicationOperationComplete", "ReplicationOperationComplete called after service was disposed");
                return Enumerable.Empty<Replicable>();
            }

            StatefulProgramInstance instance = null;
            Replicable<StatefulProgramInstance> replicableToCommit = replicationOperation.Replicables.FirstOrDefault() as Replicable<StatefulProgramInstance>;
            if (replicableToCommit != null)
            {
                instance = this.instanceManager.GetInstance(replicableToCommit.Value.Id) as StatefulProgramInstance;
                if (instance != null)
                {
                    instance.SetContextInstanceId();
                }
                else if (replicableToCommit.Value != null)
                {
                    Debug.Assert(replicationOperation.Type == ReplicationOperationType.Evict, "Unexpected operation type");
                    replicableToCommit.Value.SetContextInstanceId();
                }
                else
                {
                    Debug.Assert(false, "replicableToCommit.Value is null");
                }
            }

            AppTrace.TraceMsg(
                TraceLogEventType.Information,
                "StatefulComponent.ReplicationOperationComplete",
                "replicationOperation.Type={0}, result={1}, replicables.Count={2}, replicableToCommit={3}, programInstance={4}",
                replicationOperation.Type,
                result,
                replicationOperation.Replicables.Count(),
                replicableToCommit != null ? replicableToCommit.Name : "Null",
                instance != null ? instance.Id : "Null");

            IEnumerable<Replicable> replicablesToCommit = null;
            if (replicationOperation.Type == ReplicationOperationType.Replicate)
            {
                if (result == ReplicationResult.Success)
                {
                    List<Replicable> changes = new List<Replicable>();

                    if (replicableToCommit.IsDisposed)
                    {
                        foreach (Replicable replicable in replicableToCommit.Value.DataContractReplicables)
                        {
                            changes.Add(replicable);
                        }
                    }
                    else
                    {
                        ItemCollection<string, Replicable> replicables;
                        using (instance.GetReplicables(LockPermission.Write, out replicables))
                        {
                            foreach (Replicable replicable in replicableToCommit.Value.DataContractReplicables)
                            {
                                if (replicables.Contains(replicable.Name))
                                {
                                    replicables.Remove(replicable.Name);
                                    replicables.Add(replicable);
                                }
                                else
                                {
                                    replicables.Add(replicable);
                                }

                                changes.Add(replicable);
                            }
                        }
                    }

                    replicablesToCommit = changes;
                }
                else
                {
                    if (instance != null)
                    {
                        AppTrace.TraceMsg(TraceLogEventType.Error, "StatefulServiceReplicaT.ReplicationOperationComplete", "Recycling StatefulProgramInstance with id '{0}'", instance.Id);
                        instance.RecycleInstance();
                    }

                    replicablesToCommit = base.ReplicationOperationComplete(executionContext, result, replicationOperation);
                }
            }

            if (replicablesToCommit == null)
            {
                replicablesToCommit = base.ReplicationOperationComplete(executionContext, result, replicationOperation);
            }

            return replicablesToCommit;
        }

        protected sealed override IEnumerable<Replicable> OnApplyReplicationOperation(StatefulServiceExecutionContext executionContext, ReplicationOperation replicationOperation)
        {
            if (this.IsDisposed)
            {
                AppTrace.TraceMsg(TraceLogEventType.Information, "StatefulServiceReplicaT.OnApplyReplicationOperation", "OnApplyReplicationOperation called after service was disposed");
                return Enumerable.Empty<Replicable>();
            }

            if (replicationOperation.Type == ReplicationOperationType.Replicate || replicationOperation.Type == ReplicationOperationType.Copy)
            {
                List<Replicable> replicablesToCommit = new List<Replicable>();

                ItemCollection<IComparable, StatefulProgramInstance> instances;
                using (this.instanceManager.GetInstances(LockPermission.Write, out instances))
                {
                    if (replicationOperation.Type == ReplicationOperationType.Copy)
                    {
                        instances.Clear();
                    }

                    int count = replicationOperation.Replicables.Count();

                    //// Bug#221476
                    //// Debug.Assert(count <= 1, string.Format(CultureInfo.InvariantCulture, "Unexpected number of replicables {0}", count));

                    Replicable<StatefulProgramInstance> replicableToApply = (Replicable<StatefulProgramInstance>)replicationOperation.Replicables.FirstOrDefault();
                    if (replicableToApply == null)
                    {
                        return replicablesToCommit;
                    }

                    StatefulProgramInstance instance = (StatefulProgramInstance)this.GetOrCreateInstance(replicableToApply.Value.Id, Type.GetType(replicableToApply.Value.TypeName), replicableToApply.Value.InitializationParams);
                    if (instance != null)
                    {
                        instance.SetContextInstanceId();
                        ItemCollection<string, Replicable> replicables;
                        using (instance.GetReplicables(LockPermission.Write, out replicables))
                        {
                            if (replicationOperation.Type == ReplicationOperationType.Copy)
                            {
                                // Overwrite the existing replicables
                                replicables.Fill(replicableToApply.Value.DataContractReplicables);
                                replicablesToCommit.AddRange(replicables);
                            }
                            else if (replicableToApply.IsDisposed)
                            {
                                /// If this ProgramInstance was disposed, then dispose all associated Replicables and add them
                                /// to replicablesToCommit so that the persistence logic can pick it up and delete from the store
                                /// if required.
                                ItemCollection<string, Replicable> instanceReplicables;
                                using (instance.GetReplicables(LockPermission.Read, out instanceReplicables))
                                {
                                    foreach (Replicable replicable in instanceReplicables)
                                    {
                                        replicable.Dispose();
                                        replicablesToCommit.Add(replicable);
                                    }
                                }

                                this.DeleteInstanceInternal(instance.Id, false, -2);
                            }
                            else
                            {
                                foreach (Replicable replicable in replicableToApply.Value.DataContractReplicables)
                                {
                                    if (!replicable.IsDisposed)
                                    {
                                        if (replicables.Contains(replicable.Name))
                                        {
                                            // TODO: Need setitem support on ItemCollection
                                            Replicable existingReplicable = replicables[replicable.Name];
                                            if (existingReplicable.SequenceNumber <= replicable.SequenceNumber)
                                            {
                                                replicables.Remove(replicable.Name);
                                                replicables.Add(replicable);
                                            }
                                        }
                                        else
                                        {
                                            replicables.Add(replicable);
                                        }

                                        replicablesToCommit.Add(replicable);
                                    }
                                    else
                                    {
                                        replicables.Remove(replicable.Name);
                                    }
                                }
                            }
                        }

                        instance.SetContextInstanceId(replicablesToCommit);
                    }
                }
                                
                return replicablesToCommit;
            }
            else if (replicationOperation.Type == ReplicationOperationType.Evict)
            {
                int count = replicationOperation.Replicables.Count();
                Debug.Assert(count <= 1, string.Format(CultureInfo.InvariantCulture, "Unexpected number of replicables {0}", count));
                Replicable<StatefulProgramInstance> replicable = (Replicable<StatefulProgramInstance>)replicationOperation.Replicables.FirstOrDefault();
                if (replicable != null)
                {
                    ItemCollection<IComparable, StatefulProgramInstance> instances;
                    using (this.instanceManager.GetInstances(LockPermission.Write, out instances))
                    {
                        foreach (Replicable<StatefulProgramInstance> instance in replicationOperation.Replicables)
                        {
                            this.DeleteInstanceInternal(instance.Value.Id, false, -2);
                        }
                    }
                }

                return replicationOperation.Replicables;
            }

            return base.OnApplyReplicationOperation(executionContext, replicationOperation);
        }

        protected override Task<string> OnReplicaRoleChangeAsync(StatefulServiceExecutionContext executionContext, ReplicaRole newRole)
        {
            Task<string> task = base.OnReplicaRoleChangeAsync(executionContext, newRole);

            // Recycle all instances this ensures that information applied to all instances is based on last consistent state which was replicated
            ItemCollection<IComparable, StatefulProgramInstance> instances;
            using (this.instanceManager.GetInstances(LockPermission.Read, out instances))
            {
                foreach (StatefulProgramInstance instance in instances)
                {
                    instance.RecycleInstance();
                }
            }

            return task;
        }

        protected override void Dispose(bool disposing)
        {
            this.instanceManager.Dispose();

            base.Dispose(disposing);
        }

        protected override Task OnOpenAsync(StatefulServiceExecutionContext executionContext)
        {
            TaskCompletionSource<LifeCycleState> taskProducer = new TaskCompletionSource<LifeCycleState>();

            // Generate proxy type, this is static typeinfo generated once per type.
            ReplicableTypeInfo typeInfo = ReplicableTypeInfo.GenerateReplicableTypeInfo(typeof(TInstance));
            if (typeInfo == null)
            {
                taskProducer.SetException(new InvalidOperationException(String.Format(CultureInfo.InvariantCulture, "Unexpected failure. ReplicableTypeInfo could not be generated for type '{0}'.", typeof(TInstance).FullName)));
            }
            else
            {
                taskProducer.SetResult(LifeCycleState.Opened);
            }

            return taskProducer.Task;
        }

        // TODO: Do we need to wire up set-cookie header? and also set cookie expiry?
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotCatchGeneralExceptionTypes,
                Justification = "Service should remain unaffected by failures in client request.")]
        protected override Task OnProcessHttpRequestAsync(StatefulServiceExecutionContext executionContext, HttpListenerContext listenerContext)
        {
            TaskCompletionSource<object> taskProducer = new TaskCompletionSource<object>();
            this.InvokeProcessRequestAsync<IProcessHttpRequest, object>(
                                taskProducer,
                                executionContext,
                                "ProcessRequestAsync",
                                new Tuple<Type, Type[], object[]>(
                                    typeof(Task),
                                    new Type[] { typeof(HttpListenerContext) },
                                    new object[] { listenerContext }));
            return taskProducer.Task;
        }

        protected override Task<Stream> OnProcessTcpRequestAsync(StatefulServiceExecutionContext executionContext, System.IO.Stream messageBody)
        {
            TaskCompletionSource<Stream> taskProducer = new TaskCompletionSource<Stream>();
            this.InvokeProcessRequestAsync<IProcessTcpRequest, Stream>(
                                taskProducer, 
                                executionContext,
                                "ProcessRequestAsync",
                                new Tuple<Type, Type[], object[]>(
                                    typeof(Task<Stream>),
                                    new Type[] { typeof(Stream) },
                                    new object[] { messageBody }));
            return taskProducer.Task;
        }

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotCatchGeneralExceptionTypes,
                Justification = "Service should remain unaffected by failures in client request.")]
        private void InvokeProcessRequestAsync<TRequestType, TResult>(TaskCompletionSource<TResult> taskProducer, StatefulServiceExecutionContext executionContext, string methodName, Tuple<Type, Type[], object[]> methodParams)
        {
            Exception exception = null;
            try
            {
                if (!typeof(IProcessHttpRequest).IsAssignableFrom(typeof(TRequestType)) &&
                    !typeof(IProcessTcpRequest).IsAssignableFrom(typeof(TRequestType)))
                {
                    taskProducer.SetException(new NotSupportedException("Method not supported by the server"));
                    return;
                }

                // Get Session Information
                StatefulProgramInstance instance = null;
                DateTime expiryTime;
                IComparable sessionId;
                object sessionObj = null;
                HttpListenerContext httpListenerContext = methodParams.Item3[methodParams.Item3.Length - 1] as HttpListenerContext;
                if (httpListenerContext != null)
                {
                    sessionObj = httpListenerContext.Request;
                }
                else
                {
                    // For TCP we smuggle the headers in IDictionary<string, string> passed via asyncState
                    StreamWrapper streamWrapper = methodParams.Item3[methodParams.Item3.Length - 1] as StreamWrapper;
                    sessionObj = streamWrapper != null ? streamWrapper.UserData : null;
                }

                if (!StatefulServiceReplica<TInstance>.GetSessionInformation(sessionObj, out sessionId, out expiryTime))
                {
                    // TODO: We do not support session expiry currently
                    sessionId = Helpers.GenerateUniqueId();
                    expiryTime = DateTime.UtcNow.AddYears(100);
                }
                else
                {
                    instance = this.instanceManager.GetInstance(sessionId);
                }

                // Create a new instance if necessary
                bool setSessionInformation = false;
                bool isExpired = sessionId != null && DateTime.UtcNow > expiryTime;
                if (instance == null)
                {
                    this.CreateInstance(sessionId, null);
                    instance = this.instanceManager.GetInstance(sessionId);
                    setSessionInformation = true;
                }

                Action<Task> processRequestContinuation = delegate(Task task)
                {
                    this.Invoke<StatefulServiceExecutionContext>(
                        (cec) =>
                        {
                            Exception exception1 = null;
                            try
                            {
                                if (setSessionInformation)
                                {
                                    if (httpListenerContext != null)
                                    {
                                        sessionObj = httpListenerContext.Response;
                                    }
                                    else
                                    {
                                        // For TCP we smuggle the headers in asyncObject
                                        sessionObj = (IDictionary<string, string>)methodParams.Item3[methodParams.Item3.Length - 1];
                                    }

                                    StatefulServiceReplica<TInstance>.SetSessionInformation(sessionObj, sessionId, expiryTime);
                                }

                                if (!task.IsFaulted)
                                {
                                    // Get the result from either the http task or task returned by Replicable method invocation.
                                    object result = null;
                                    if (typeof(IProcessTcpRequest).IsAssignableFrom(typeof(TRequestType)))
                                    {
                                        Task<Stream> tcpResponseTask = task as Task<Stream>;
                                        if (tcpResponseTask != null)
                                        {
                                            result = tcpResponseTask.Result;
                                        }
                                        else
                                        {
                                            Task<object> taskWithObj = task as Task<object>;
                                            result = (Stream)taskWithObj.Result;
                                        }
                                    }

                                    taskProducer.SetResult((TResult)result);
                                }
                                else
                                {
                                    exception1 = task.Exception.InnerException ?? task.Exception;
                                }

                                if (isExpired)
                                {
                                    this.DeleteInstance(sessionId);
                                }
                            }
                            catch (Exception e)
                            {
                                AppTrace.TraceException(e, "StatefulServiceReplicaT.InvokeProcessRequestAsync");
                                exception1 = e;
                            }
                            finally
                            {
                                if (exception1 != null)
                                {
                                    taskProducer.SetException(exception1);
                                }
                            }
                        });
                };

                if (instance != null && !isExpired)
                {
                    IEnumerable<ReplicableMethodInfo> matchingMethods = null;
                    instance.TypeInfo.TryGetMatchingMethod("ProcessRequestAsync", methodParams.Item1, methodParams.Item2, out matchingMethods);
                    int matchingMethodCount = matchingMethods != null ? matchingMethods.Count() : 0;

                    if (matchingMethodCount == 1 && matchingMethods.First().Type == ReplicableMethodInfo.MethodType.AsyncTask)
                    {
                        ReplicableMethodInfo methodInfo = matchingMethods.First();
                        Task<Tuple<object, object[]>> asyncInvocationTask = instance.InvokeAsync(methodInfo.Methods[0], methodParams.Item3);
                        asyncInvocationTask.Wait();
                        Task task = (Task)asyncInvocationTask.Result.Item1;
                        task.ContinueWith(processRequestContinuation);
                    }
                    else if (matchingMethodCount > 1)
                    {
                        exception = new AmbiguousMatchException(String.Format(CultureInfo.InvariantCulture, "Multiple methods with name '{0}' matching similar parameters were found", "ProcessRequestAsync"));
                    }
                    else if (typeof(IProcessHttpRequest).IsAssignableFrom(typeof(TRequestType)))
                    {
                        ((IProcessHttpRequest)instance.Instance).ProcessRequestAsync((HttpListenerContext)methodParams.Item3[0])
                            .ContinueWith(task => { processRequestContinuation(task); }, TaskContinuationOptions.ExecuteSynchronously);
                    }
                    else
                    {
                        ((IProcessTcpRequest)instance.Instance).ProcessRequestAsync((Stream)methodParams.Item3[0])
                            .ContinueWith(task => { processRequestContinuation(task); }, TaskContinuationOptions.ExecuteSynchronously);
                    }
                }
                else
                {
                    AppTrace.TraceMsg(TraceLogEventType.Warning, "StatefulServiceReplicaT.InvokeProcessRequestAsync", "Instance cannot be created for expired session cookie with session id '{0}'.", sessionId);
                    taskProducer.SetException(new InvalidOperationException("Instance expired. Service cannot process request"));
                }
            }
            catch (Exception e)
            {
                AppTrace.TraceException(e, "StatefulServiceReplicaT.InvokeProcessRequestAsync");
                exception = e;
            }
            finally
            {
                if (exception != null)
                {
                    taskProducer.SetException(exception);
                }
            }
        }

        protected object CreateInstance(IComparable instanceId, object[] initializationParameters)
        {
            Requires.Argument("instanceId", instanceId).NotNull();

            StatefulProgramInstance instance = this.GetOrCreateInstance(instanceId, typeof(TInstance), initializationParameters);
            return instance != null ? instance.Instance : null;
        }

        protected object GetInstance(IComparable instanceId)
        {
            object instance = null;
            ItemCollection<IComparable, StatefulProgramInstance> instances;
            using (this.instanceManager.GetInstances(LockPermission.Read, out instances))
            {
                if (instances.Contains(instanceId))
                {
                    instance = instances[instanceId];
                }
            }

            return instance;
        }

        protected bool DeleteInstance(IComparable instanceId)
        {
            return this.DeleteInstanceInternal(instanceId, true, -2);
        }

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        protected Task<Tuple<object, object[]>> InvokeAsync(IComparable instanceId, string replicationMethodName, object[] inputParameters)
        {
            Requires.Argument("instanceId", instanceId).NotNull();
            Requires.Argument("methodName", replicationMethodName).NotNullOrEmpty();

            StatefulProgramInstance instance = this.instanceManager.GetInstance(instanceId);
            if (instance == null)
            {
                throw new ArgumentException(String.Format(CultureInfo.InvariantCulture, "No instance was found with InstanceId: '{0}'", instanceId), "instanceId");
            }

            return instance.InvokeAsync(replicationMethodName, inputParameters);
        }

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        protected Task<Tuple<object, object[]>> InvokeAsync(IComparable instanceId, MethodInfo replicationMethod, object[] inputParameters)
        {
            Requires.Argument("instanceId", instanceId).NotNull();
            Requires.Argument("replicationMethod", replicationMethod).NotNull();

            StatefulProgramInstance instance = this.instanceManager.GetInstance(instanceId);
            if (instance == null)
            {
                throw new ArgumentException(String.Format(CultureInfo.InvariantCulture, "No instance was found with InstanceId: '{0}'", instanceId), "instanceId");
            }

            return instance.InvokeAsync(replicationMethod, inputParameters);
        }

        private StatefulProgramInstance GetOrCreateInstance(IComparable instanceId, Type instanceType, object[] initializationParameters)
        {
            Requires.Argument("instanceId", instanceId).NotNull();
            Requires.Argument("instanceType", instanceType).NotNull();

            StatefulServiceExecutionContext executionContext = StatefulServiceExecutionContext.Current as StatefulServiceExecutionContext;
            if (executionContext == null || executionContext.Partition == null)
            {
                throw new InvalidOperationException("Program instance cannot be created in context of a partition. Please ensure that StatefulServiceReplica.Invoke is called.");
            }

            ReplicableTypeInfo replicableTypeInfo = null;
            Dictionary<Type, ReplicableTypeInfo> replicableTypeInfos;
            using (ReplicableTypeInfo.GetReplicableTypeInfos(LockPermission.Read, out replicableTypeInfos))
            {
                replicableTypeInfos.TryGetValue(instanceType, out replicableTypeInfo);
                if (replicableTypeInfo == null)
                {
                    return null;
                }
            }

            StatefulProgramInstance instance = null;
            ItemCollection<IComparable, StatefulProgramInstance> instances;
            using (this.instanceManager.GetInstances(LockPermission.Write, out instances))
            {
                if (instances.Contains(instanceId))
                {
                    instance = instances[instanceId];
                }
                else if (this.ReplicaRole != Fabric.ReplicaRole.Primary)
                {
                    /// Since we are the secondary, create a new instance.  Resurrection will only happen on the primary which
                    /// will then replicate the state to all secondaries.
                    instance = new StatefulProgramInstance(this, instanceId, replicableTypeInfo, initializationParameters, instanceId.CompareTo(StatefulServiceReplica<TInstance>.SingletonSessionId) == 0);
                    instances.Add(instance);
                }
            }

            if (instance == null)
            {
                /// TODO: Make this async.  The asynchrony needs to be plumbed all the way through.  WCF interfaces don't support
                /// asynchrony when obtaining service instances, so we will need to live with this there but can do better for the
                /// rest.
                StatefulComponentStateManager stateManager = (StatefulComponentStateManager)this.StateManager;
                Task<IEnumerable<Replicable>> task = stateManager.ResurrectProgramInstanceAsync(executionContext, instanceId);
                Task.WaitAll(task);

                IEnumerable<Replicable> replicables = !task.IsFaulted ? task.Result : Enumerable.Empty<Replicable>();
                using (this.instanceManager.GetInstances(LockPermission.Write, out instances))
                {
                    if (instances.Contains(instanceId))
                    {
                        instance = instances[instanceId];
                    }
                    else
                    {
                        StatefulProgramInstance statefulProgramInstance = new StatefulProgramInstance(this, instanceId, replicableTypeInfo, initializationParameters, instanceId.CompareTo(StatefulServiceReplica<TInstance>.SingletonSessionId) == 0);
                        instance = statefulProgramInstance;
                        instances.Add(instance);
                        ItemCollection<string, Replicable> typeReplicables;
                        using (statefulProgramInstance.GetReplicables(LockPermission.Write, out typeReplicables))
                        {
                            foreach (Replicable replicable in replicables)
                            {
                                typeReplicables.Remove(replicable);
                                typeReplicables.Add(replicable);
                            }
                        }
                    }
                }
            }

            return instance;
        }

        private bool DeleteInstanceInternal(IComparable instanceId, bool replicate, long evictionVersion)
        {
            Requires.Argument("instanceId", instanceId).NotNull();

            StatefulServiceExecutionContext executionContext = StatefulServiceExecutionContext.Current as StatefulServiceExecutionContext;
            if (executionContext == null || executionContext.Partition == null)
            {
                throw new InvalidOperationException("Program instance cannot be obtained outside context of a partition. Please ensure that StatefulServiceReplicaT.Invoke is called.");
            }

            StatefulProgramInstance instance = null;
            ItemCollection<IComparable, StatefulProgramInstance> instances;
            using (this.instanceManager.GetInstances(LockPermission.Write, out instances))
            {
                if (instances.Contains(instanceId))
                {
                    instance = (StatefulProgramInstance)instances[instanceId];
                    if (evictionVersion != -2 && !instance.CanEvict(evictionVersion))
                    {
                        return false;
                    }

                    instances.Remove(instanceId);
                    ((IDisposable)instance).Dispose();
                }
            }

            if (instance != null)
            {
                if (replicate)
                {
                    Replicable<StatefulProgramInstance> replicable = new Replicable<StatefulProgramInstance>(instance.Id.ToString(), instance);
                    ReplicationScope replicationScope = new ReplicationScope(new Replicable[] { replicable }, ReplicationOperationType.Evict);
                    replicationScope.ReplicateAsync().ContinueWith(
                            task =>
                            {
                                ReplicationResult result = task.IsFaulted ? ReplicationResult.Failed : task.Result;
                                if (result != ReplicationResult.Success)
                                {
                                    AppTrace.TraceMsg(TraceLogEventType.Warning, "StatefulServiceReplicaT.DeleteInstanceInternal", "Replication call to dispose the instance with id {0} failed.", instance.Id);
                                }
                            },
                            TaskContinuationOptions.ExecuteSynchronously);
                }

                return true;
            }
            else
            {
                return false;
            }
        }
    }
}