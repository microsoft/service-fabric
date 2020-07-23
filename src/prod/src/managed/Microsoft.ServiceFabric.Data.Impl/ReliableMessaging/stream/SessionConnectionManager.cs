// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Fabric.ReliableMessaging;
    using System.Fabric.ReliableMessaging.Session;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;

    #endregion

    /// <summary>
    /// This class provides the key mechanism to manage 
    /// 1. End point Lifecycle: Manage partners end point info. The end point info is retrieved from Service Manager and stored in Property Manager.
    /// 2. Outbound Sessions lifecycle (Create/Clear/Find..) activities by interfacing with ReliableSession Manager
    /// 3. Inbound Session  (IReliableInboundSessionCallback) interfaces with ReliableSession Manager to manage the lifecycle of InBound Session) and to
    /// 4. Service callbacks(IReliableSessionAbortedCallback) from ReliableSession Manager.
    /// 
    /// TODO: 1. Move end point management to its own class
    /// TODO: 2. The current implementation supports source and target partners to be located in the same ring. We need to support Xrings.
    /// TODO: 3. Security (certs management)
    /// </summary>
    internal class SessionConnectionManager : IReliableInboundSessionCallback, IReliableSessionAbortedCallback
    {
        #region properties

        /// <summary>
        /// An handle to the Reliable Session Manager, to interact with the management of Inbound and Outbound sessions
        /// </summary>
        private readonly IReliableSessionManager sessionManager;

        /// <summary>
        /// The associated stream manager to register and interact with incoming and outgoing sessions.
        /// </summary>
        private readonly StreamManager streamManager;

        /// <summary>
        /// This contains a collection of normalized Partition Keys(partner service partitions) of kind 'Numbered'.
        /// The term normalized is used to denote that the Partition has been found in SM and the PartitionRange(low and high key range) has
        /// has been retrieved in case of Partitions of kind 'Numbered'.
        /// TODO: This seems to cache all partition keys not just numbered.
        /// </summary>
        private readonly ConcurrentDictionary<PartitionKey, PartitionKey> normalizedPartitionKeys;

        /// <summary>
        /// This contains a collection of resolved PartitionKey Endpoints.
        /// </summary>
        private readonly Dictionary<PartitionKey, PartitionInfo> resolvedEndpoints;

        /// <summary>
        /// The lock is used when updating resolvedEndpoints.
        /// </summary>
        private readonly object resolvedEndpointsLock;

        /// <summary>
        /// This contains a collection of active outbound sessions, that are currently in use.
        /// Each session has a one-to-one relationship with the corresponding session driver 
        /// </summary>
        private readonly ConcurrentDictionary<PartitionKey, IReliableMessagingSession> activeOutboundSessions;

        /// <summary>
        /// This provided access to the runtime services(Service Manager and Property Manager)
        /// TODO: Will need to enhance the client connection to support Certs.
        /// </summary>
        private readonly FabricClient client;

        /// <summary>
        /// Tracer Info
        /// </summary>
        private readonly string tracer;

        #endregion

        #region cstor

        /// <summary>
        /// This creates an instance of Session Connection Manager.
        /// </summary>
        /// <param name="streamManager">The associated stream manager</param>
        /// <param name="sessionManager">The underlying Release session manager</param>
        /// <param name="tracer">Tracer </param>
        internal SessionConnectionManager(StreamManager streamManager, IReliableSessionManager sessionManager, string tracer)
        {
            // Connect to the Win Fab client.
            this.client = new FabricClient();
            // Update associated stream manager and reliable sessions
            this.sessionManager = sessionManager;
            this.streamManager = streamManager;

            // Initialize required queues.
            this.normalizedPartitionKeys = new ConcurrentDictionary<PartitionKey, PartitionKey>(new PartitionKeyEqualityComparer());
            this.resolvedEndpoints = new Dictionary<PartitionKey, PartitionInfo>(new PartitionKeyEqualityComparer());
            this.activeOutboundSessions = new ConcurrentDictionary<PartitionKey, IReliableMessagingSession>(new PartitionKeyEqualityComparer());
            this.resolvedEndpointsLock = new Object();
            this.tracer = tracer;
        }

        #endregion

        #region EndPointRegistration

        /// <summary>
        /// Get the normalized Partition key for Key.Kind == Numbered.
        /// This should only be called when the parameter is either not a trivial numbered key or it is trivial and should already have been resolved
        /// </summary>
        /// <param name="key">Partition Key</param>
        /// <returns>Normalized Partition Key</returns>
        internal PartitionKey GetNormalizedPartitionKey(PartitionKey key)
        {
            PartitionKey normalizedPartition = null;
            var foundNormalized = this.normalizedPartitionKeys.TryGetValue(key, out normalizedPartition);

            if (!foundNormalized)
            {
                var validSituation = (key.Kind != PartitionKind.Numbered) ||
                                     (key.Kind == PartitionKind.Numbered && key.PartitionRange.IntegerKeyLow != key.PartitionRange.IntegerKeyHigh);

                Diagnostics.Assert(validSituation, "{0} Normalized version not found for trivial numbered partition key {1}", this.tracer, key);
                normalizedPartition = key;
            }

            return normalizedPartition;
        }

        /// <summary>
        /// Get the normalized Partition key for a given Key
        /// </summary>
        /// <param name="baseKey">Given base partition Key</param>
        /// <param name="foundKey">Found partition key</param>
        /// <returns>true if found, false otherwise</returns>
        internal bool GetNormalizedPartitionKey(PartitionKey baseKey, out PartitionKey foundKey)
        {
            return this.normalizedPartitionKeys.TryGetValue(baseKey, out foundKey);
        }

        /// <summary>
        /// Delete End point Info from Property Manager when role(primary) is shutdown.
        /// </summary>
        /// <param name="key">Partition Key to be deleted</param>
        /// <returns />
        internal async Task RevokeEnpointAsync(PartitionKey key)
        {
            var parentName = key.ServiceInstanceName;
            var propertyName = key.PublishedPropertyName("TransportEndpoint");
            var deleteSuccess = false;

            while (!deleteSuccess)
            {
                try
                {
                    await this.client.PropertyManager.DeletePropertyAsync(parentName, propertyName);
                    FabricEvents.Events.DeleteEndpointProperty("FoundAndDeleted@" + this.streamManager.TraceType, key.ToString());
                    deleteSuccess = true;
                }
                catch (FabricElementNotFoundException)
                {
                    FabricEvents.Events.DeleteEndpointProperty("NotFound@" + this.streamManager.TraceType, key.ToString());
                    deleteSuccess = true;
                }
                catch (FabricTransientException)
                {
                    FabricEvents.Events.DeleteEndpointProperty("TransientError.Retrying@" + this.streamManager.TraceType, key.ToString());
                }
                catch (TimeoutException)
                {
                    FabricEvents.Events.DeleteEndpointProperty("Timeout.Retrying@" + this.streamManager.TraceType, key.ToString());
                }
                catch (OperationCanceledException)
                {
                    FabricEvents.Events.DeleteEndpointProperty("Canceled.Retrying@" + this.streamManager.TraceType, key.ToString());
                }
                catch (FabricException fabricException)
                {
                    if (fabricException.ErrorCode == FabricErrorCode.CommunicationError)
                    {
                        FabricEvents.Events.DeleteEndpointProperty("CommunicationError.Retrying@" + this.streamManager.TraceType, key.ToString());
                    }
                    else
                    {
                        Tracing.WriteExceptionAsError("RegisterEndpointAsync.DeletePropertyAsync.UnrecoverableError", fabricException, "{0}", this.tracer);
                        throw;
                    }
                }
                catch (Exception otherException)
                {
                    Tracing.WriteExceptionAsError("RegisterEndpointAsync.DeletePropertyAsync.UnrecoverableError", otherException, "{0}", this.tracer);
                    throw;
                }
            }
        }

        /// <summary>
        /// Register my(this instance of) service endpoint info (endpoint and era)
        /// This method is called only once at startup when the service becomes primary
        /// </summary>
        /// <param name="key">Partition to register</param>
        /// <param name="endpoint">Endpoint of the partition</param>
        /// <param name="era">Era of the partition</param>
        /// <returns></returns>
        internal async Task RegisterPartitionAsync(PartitionKey key, string endpoint, Guid era)
        {
            var parentName = key.ServiceInstanceName;
            var propertyName = key.PublishedPropertyName("TransportEndpoint");
            var endPointInfo = new PartitionInfo(endpoint, era);

            FabricEvents.Events.RegisterEndpointProperty("Start@" + this.streamManager.TraceType, key.ToString(), endPointInfo.ToString());


            await this.RevokeEnpointAsync(key);

            var putSuccess = false;

            while (!putSuccess)
            {
                try
                {
                    await this.client.PropertyManager.PutPropertyAsync(parentName, propertyName, endPointInfo.Serialize());
                    putSuccess = true;
                }
                catch (FabricTransientException)
                {
                    FabricEvents.Events.RegisterEndpointProperty(
                        "TransientException.Retrying@" + this.streamManager.TraceType,
                        key.ToString(),
                        endPointInfo.ToString());
                }
                catch (TimeoutException)
                {
                    FabricEvents.Events.RegisterEndpointProperty(
                        "Timeout.Retrying@" + this.streamManager.TraceType,
                        key.ToString(),
                        endPointInfo.ToString());
                }
                catch (OperationCanceledException)
                {
                    FabricEvents.Events.RegisterEndpointProperty(
                        "Canceled.Retrying@" + this.streamManager.TraceType,
                        key.ToString(),
                        endPointInfo.ToString());
                }
                catch (FabricException fabricException)
                {
                    if (fabricException.ErrorCode == FabricErrorCode.CommunicationError)
                    {
                        FabricEvents.Events.RegisterEndpointProperty(
                            "CommunicationError.Retrying@" + this.streamManager.TraceType,
                            key.ToString(),
                            endPointInfo.ToString());
                    }
                    else
                    {
                        Tracing.WriteExceptionAsError("RegisterEndpointAsync.PutPropertyAsync.UnrecoverableError", fabricException, "{0}", this.tracer);
                        throw;
                    }
                }
                catch (Exception otherException)
                {
                    Tracing.WriteExceptionAsError("RegisterEndpointAsync.PutPropertyAsync.UnrecoverableError", otherException, "{0}", this.tracer);
                    throw;
                }
            }

            FabricEvents.Events.RegisterEndpointProperty("Finish@" + this.streamManager.TraceType, key.ToString(), endPointInfo.ToString());
        }

        // 
        /// <summary>
        /// TODO: this may throw FabricObjectClosedException
        /// Use naming to find the integer partition range for a numbered target partition given a single partition number target
        /// </summary>
        /// <param name="key">Partition Key to resolve</param>
        /// <param name="timeout">Timeout for this operation to complete by</param>
        /// <returns>Resolved (and in case of key Kind = numbered) normalized partition key</returns>
        internal async Task<PartitionKey> ResolveAndNormalizeTargetPartition(PartitionKey key, TimeSpan timeout)
        {
            FabricEvents.Events.ResolveTargetPartition("Start@" + this.streamManager.TraceType, key.ToString(), "");

            PartitionKey foundKey = null;
            var found = this.GetNormalizedPartitionKey(key, out foundKey);

            if (found)
            {
                FabricEvents.Events.ResolveTargetPartition("FoundCached@" + this.streamManager.TraceType, key.ToString(), foundKey.ToString());
                return foundKey;
            }

            var resolveSuccess = false;
            ResolvedServicePartition resolvedPartition = null;
            var resolveDelay = StreamConstants.BaseDelayForValidSessionEndpoint;

            var remainingTimeout = timeout;

            while (!resolveSuccess)
            {
                var beforeResolveAttempt = DateTime.UtcNow;

                // TODO: ResolveServicePartitionAsync seems to throw TimeoutException in unexpected ways: investigate and fix this, leaving out the timeout param for now
                try
                {
                    switch (key.Kind)
                    {
                        case PartitionKind.Singleton:
                            resolvedPartition = await this.client.ServiceManager.ResolveServicePartitionAsync(key.ServiceInstanceName);
                            break;

                        case PartitionKind.Named:
                            resolvedPartition = await this.client.ServiceManager.ResolveServicePartitionAsync(key.ServiceInstanceName, key.PartitionName);
                            break;

                        case PartitionKind.Numbered:
                            resolvedPartition =
                                await this.client.ServiceManager.ResolveServicePartitionAsync(key.ServiceInstanceName, key.PartitionRange.IntegerKeyHigh);
                            break;

                        default:
                            Diagnostics.Assert(false, "{0} Unknown partition Kind {1} in ResolveAndNormalizeTargetPartition", this.tracer, key);
                            break;
                    }

                    resolveSuccess = true;
                }
                catch (FabricTransientException)
                {
                    FabricEvents.Events.ResolveTargetPartition("TransientException.Retrying@" + this.streamManager.TraceType, key.ToString(), "");
                }
                catch (TimeoutException)
                {
                    FabricEvents.Events.ResolveTargetPartition("Timeout.Retrying@" + this.streamManager.TraceType, key.ToString(), "");
                }
                catch (OperationCanceledException)
                {
                    FabricEvents.Events.ResolveTargetPartition("Canceled.Retrying@" + this.streamManager.TraceType, key.ToString(), "");
                }
                catch (FabricException fabricException)
                {
                    if (fabricException.ErrorCode == FabricErrorCode.InvalidPartitionKey)
                    {
                        FabricEvents.Events.ResolveTargetPartition("KeyNotFound.Retrying@" + this.streamManager.TraceType, key.ToString(), "");
                    }
                    else if (fabricException.ErrorCode == FabricErrorCode.CommunicationError)
                    {
                        FabricEvents.Events.ResolveTargetPartition("CommunicationError.Retrying@" + this.streamManager.TraceType, key.ToString(), "");
                    }
                    else if (fabricException.ErrorCode == FabricErrorCode.ServiceNotFound)
                    {
                        FabricEvents.Events.ResolveTargetPartition("ServiceNotFound.Retrying@" + this.streamManager.TraceType, key.ToString(), "");
                    }
                    else
                    {
                        Tracing.WriteExceptionAsError(
                            "ResolveAndNormalizeTargetPartition.UnrecoverableError",
                            fabricException,
                            "{0} Target: {1}",
                            this.tracer,
                            key);
                        throw;
                    }
                }
                catch (Exception e)
                {
                    Tracing.WriteExceptionAsError("ResolveAndNormalizeTargetPartition.UnrecoverableError", e, "{0} Target: {1}", this.tracer, key);
                    throw;
                }

                if (!resolveSuccess)
                {
                    var afterResolveAttempt = DateTime.UtcNow;
                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeResolveAttempt, afterResolveAttempt);

                    // TODO: the behavior of ResolveServicePartitionAsync is unclear as far as timeout goes, this code needs fixing
                    if ((afterResolveAttempt - beforeResolveAttempt).TotalMilliseconds < resolveDelay)
                    {
                        await Task.Delay(resolveDelay);
                    }

                    var afterDelay = DateTime.UtcNow;
                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, afterResolveAttempt, afterDelay);
                }
            }

            PartitionKey result;

            // if we had a numbered partition stream target with a single number then we need to change the key to the range actually supported by the resolved target partition
            if ((key.Kind == PartitionKind.Numbered) && (key.PartitionRange.IntegerKeyLow == key.PartitionRange.IntegerKeyHigh))
            {
                var partitionInfo = resolvedPartition.Info as Int64RangePartitionInformation;
                result = new PartitionKey(key.ServiceInstanceName, partitionInfo.LowKey, partitionInfo.HighKey);
            }
            else
            {
                result = key;
            }

            // Add partition keys to the cache.
            this.normalizedPartitionKeys.TryAdd(key, result);
            FabricEvents.Events.ResolveTargetPartition("Finish@" + this.streamManager.TraceType, key.ToString(), result.ToString());

            return result;
        }

        /// <summary>
        /// Check if the partition primary has changed, we do this by comparing the Era.
        /// </summary>
        /// <param name="normalizedKey">PartitionKey</param>
        /// <param name="timeout">timeout for this operation to complete</param>
        /// <returns>false if same, true otherwise</returns>
        internal async Task<bool> HasPartitionEraChanged(PartitionKey normalizedKey, TimeSpan timeout)
        {
            PartitionInfo existingEndpoint = null;

            lock (this.resolvedEndpointsLock)
            {
                var endpointFound = this.resolvedEndpoints.TryGetValue(normalizedKey, out existingEndpoint);
                Diagnostics.Assert(
                    endpointFound,
                    "{0} resolvedEndpoints found no existing endpoint in SessionConnectionManager.HasEnpointChanged, for Key: {1}",
                    this.streamManager.TraceType,
                    normalizedKey);
            }

            var currentEndPoint = await this.GetEndpointAsync(normalizedKey, timeout);
            return currentEndPoint.Era != existingEndpoint.Era;
        }

        /// <summary>
        /// Get the partition end point info for the given partition key
        /// Exceptions this may throw TimeoutException, FabricObjectClosedException, more to be discovered
        /// </summary>
        /// <param name="key">Get end point for this given partition key</param>
        /// <param name="timeout">Timeout for the operation to complete</param>
        /// <returns>Partition Body</returns>
        private async Task<PartitionInfo> GetEndpointAsync(PartitionKey key, TimeSpan timeout)
        {
            PartitionInfo endpoint = null;

            var parentName = key.ServiceInstanceName;
            var propertyName = key.PublishedPropertyName("TransportEndpoint");
            var getEndpointDelay = 0;
            var getCompleted = false;

            // TODO: revisit the timeout behavior of GetPropertyAsync to clarify and adjust accordingly
            var remainingTimeout = timeout;

            FabricEvents.Events.GetEndpointProperty("Start@" + this.streamManager.TraceType, key.ToString(), "");

            while (!getCompleted)
            {
                var beforeGetPropertyAsync = DateTime.UtcNow;
                try
                {
                    var property = await this.client.PropertyManager.GetPropertyAsync(parentName, propertyName);
                    endpoint = property.GetValue<byte[]>().Deserialize<PartitionInfo>();
                    getCompleted = true;
                }
                catch (FabricElementNotFoundException)
                {
                    FabricEvents.Events.GetEndpointProperty("NotFound.Retrying@" + this.streamManager.TraceType, key.ToString(), "");
                }
                catch (FabricTransientException)
                {
                    FabricEvents.Events.GetEndpointProperty("TransientError.Retrying@" + this.streamManager.TraceType, key.ToString(), "");
                }
                catch (TimeoutException)
                {
                    FabricEvents.Events.GetEndpointProperty("Timeout.Retrying@" + this.streamManager.TraceType, key.ToString(), "");
                }
                catch (OperationCanceledException)
                {
                    FabricEvents.Events.GetEndpointProperty("Canceled.Retrying@" + this.streamManager.TraceType, key.ToString(), "");
                }
                catch (FabricException fabricException)
                {
                    if (fabricException.ErrorCode == FabricErrorCode.CommunicationError)
                    {
                        FabricEvents.Events.GetEndpointProperty("CommunicationError.Retrying@" + this.streamManager.TraceType, key.ToString(), "");
                    }
                    else
                    {
                        Tracing.WriteExceptionAsError("RegisterEndpointAsync.GetPropertyAsync.UnrecoverableError", fabricException, "{0}", this.tracer);
                        throw;
                    }
                }
                catch (Exception otherException)
                {
                    Tracing.WriteExceptionAsError("RegisterEndpointAsync.GetPropertyAsync.UnrecoverableError", otherException, "{0}", this.tracer);
                    throw;
                }

                if (endpoint == null)
                {
                    var afterGetPropertyAsync = DateTime.UtcNow;

                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeGetPropertyAsync, afterGetPropertyAsync);

                    getEndpointDelay = getEndpointDelay >= StreamConstants.MaxDelayForValidSessionEndpoint
                        ? StreamConstants.MaxDelayForValidSessionEndpoint
                        : getEndpointDelay + StreamConstants.BaseDelayForValidSessionEndpoint;
                    await Task.Delay(getEndpointDelay);
                }
            }

            bool replaced;

            // Update resolved end points cache
            lock (this.resolvedEndpointsLock)
            {
                replaced = this.resolvedEndpoints.Remove(key);
                this.resolvedEndpoints.Add(key, endpoint);
            }

            if (replaced)
            {
                FabricEvents.Events.GetEndpointProperty("FinishReplaced@" + this.streamManager.TraceType, key.ToString(), endpoint.ToString());
            }
            else
            {
                FabricEvents.Events.GetEndpointProperty("FinishNew@" + this.streamManager.TraceType, key.ToString(), endpoint.ToString());
            }

            return endpoint;
        }

        #endregion

        #region OutboundSessions

        /// <summary>
        /// Given a Partition and its endpoint, create a reliable session and return it.
        /// Exceptions this may throw TimeoutException,FabricObjectClosedException
        /// </summary>
        /// <param name="targetPartition">Target Partition</param>
        /// <param name="endpoint">Target End point</param>
        /// <returns>Reliable Messaging Session object</returns>
        private async Task<IReliableMessagingSession> CreateSession(PartitionKey targetPartition, string endpoint)
        {
            Task<IReliableMessagingSession> sessionCreateTask = null;
            IReliableMessagingSession session;

            // CreateOutboundSessionAsync is really synchronous at the implementation level, and will throw exceptions synchronously
            try
            {
                FabricEvents.Events.SetupSession(
                    "CreateSession.Start@" + this.streamManager.TraceType,
                    targetPartition.ToString(),
                    endpoint,
                    Guid.Empty.ToString());

                switch (targetPartition.Kind)
                {
                    case PartitionKind.Singleton:
                        sessionCreateTask = this.sessionManager.CreateOutboundSessionAsync(
                            targetPartition.ServiceInstanceName,
                            endpoint,
                            CancellationToken.None);
                        break;
                    case PartitionKind.Numbered:
                        sessionCreateTask = this.sessionManager.CreateOutboundSessionAsync(
                            targetPartition.ServiceInstanceName,
                            targetPartition.PartitionRange.IntegerKeyLow,
                            endpoint,
                            CancellationToken.None);
                        break;
                    case PartitionKind.Named:
                        sessionCreateTask = this.sessionManager.CreateOutboundSessionAsync(
                            targetPartition.ServiceInstanceName,
                            targetPartition.PartitionName,
                            endpoint,
                            CancellationToken.None);
                        break;
                }

                // session creation is essentially synchronous so we won't try timeout on it
                await sessionCreateTask;

                session = sessionCreateTask.Result;
            }
            catch (FabricObjectClosedException)
            {
                // this can happen if the session manager was closed by a parallel thread due to role change
                throw;
            }
            catch (Exception e)
            {
                if (e is FabricException)
                {
                    var fabricException = e as FabricException;
                    Diagnostics.Assert(
                        fabricException.ErrorCode != FabricErrorCode.ReliableSessionAlreadyExists,
                        "{0} Unexpected duplicate session in setup session for Target: {1}",
                        this.tracer,
                        targetPartition);
                }

                Tracing.WriteExceptionAsError(
                    "SetupSession",
                    e,
                    "{0} Unexpected exception in SetupSession from CreateOutboundSessionAsync for Target: {1}",
                    this.tracer,
                    targetPartition);

                throw;
            }

            return session;
        }

        /// <summary>
        /// Instantiates a Reliable Messaging Session for a given target partition.
        /// </summary>
        /// <param name="targetPartition">Target Partition</param>
        /// <param name="timeout">Timeout for the operation to complete</param>
        /// <returns>Reliable Messaging Session for the Target Partition</returns>
        internal async Task<IReliableMessagingSession> SetupSession(PartitionKey targetPartition, TimeSpan timeout)
        {
            string endpoint = null;
            var setupSuccess = false;
            IReliableMessagingSession session = null;
            var baseOpenAsyncTimeout = 0;
            var remainingTimeout = timeout;
            SessionDataSnapshot snapshot = null;

            try
            {
                while (!setupSuccess)
                {
                    var beforeGetEndpoint = DateTime.UtcNow;

                    // Get End point for this Partition
                    var partitionInfo = await this.GetEndpointAsync(targetPartition, remainingTimeout);
                    endpoint = partitionInfo.EndPoint;

                    FabricEvents.Events.SetupSession("Start@" + this.streamManager.TraceType, targetPartition.ToString(), endpoint, Guid.Empty.ToString());

                    var afterGetEndpoint = DateTime.UtcNow;

                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeGetEndpoint, afterGetEndpoint);

                    session = await this.CreateSession(targetPartition, endpoint);

                    var retry = false;

                    snapshot = session.GetDataSnapshot();

                    FabricEvents.Events.SetupSession(
                        "OpenSession.Start@" + this.streamManager.TraceType,
                        targetPartition.ToString(),
                        endpoint,
                        snapshot.SessionId.ToString());

                    var openTask = session.OpenAsync(CancellationToken.None);

                    var beforeOpenWait = DateTime.UtcNow;

                    baseOpenAsyncTimeout = baseOpenAsyncTimeout >= StreamConstants.MaxDelayForValidSessionEndpoint
                        ? StreamConstants.MaxDelayForValidSessionEndpoint
                        : baseOpenAsyncTimeout + StreamConstants.BaseDelayForValidSessionEndpoint;

                    var baseTimeSpan = new TimeSpan(0, 0, 0, 0, baseOpenAsyncTimeout);
                    var openAsyncTimeout = (baseTimeSpan < remainingTimeout) || (remainingTimeout == Timeout.InfiniteTimeSpan) ? baseTimeSpan : remainingTimeout;

                    // this timeout is a retry mechanism in case we are going to the wrong endpoint because the primary replica is moving
                    var openTaskCompleted = await TimeoutHandler.WaitWithDelay(openAsyncTimeout, openTask);

                    var afterOpenWait = DateTime.UtcNow;
                    remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, beforeOpenWait, afterOpenWait);

                    if (openTaskCompleted && openTask.Exception == null)
                    {
                        FabricEvents.Events.SetupSession(
                            "OpenSession.Finish@" + this.streamManager.TraceType,
                            targetPartition.ToString(),
                            endpoint,
                            snapshot.SessionId.ToString());
                        setupSuccess = true;
                    }
                    else if (openTask.Exception != null)
                    {
                        if (openTask.Exception.InnerException.GetType() == typeof(FabricException))
                        {
                            var fabricException = openTask.Exception.InnerException as FabricException;

                            if (fabricException.ErrorCode == FabricErrorCode.ReliableSessionManagerNotFound)
                            {
                                // Target partition primary may be moving to a different host process before session manager was created
                                retry = true;
                            }
                        }
                        else
                        {
                            Tracing.WriteExceptionAsError(
                                "SetupSession.OpenSession.UnrecoverableError",
                                openTask.Exception,
                                "{0} Target: {1}",
                                this.tracer,
                                targetPartition);
                            throw openTask.Exception;
                        }
                    }
                    else
                    {
                        FabricEvents.Events.SetupSession(
                            "OpenSession.RetryTimeout@" + this.streamManager.TraceType,
                            targetPartition.ToString(),
                            endpoint,
                            snapshot.SessionId.ToString());
                        retry = true;
                    }

                    if (retry)
                    {
                        session.Abort();
                        session = null;
                        setupSuccess = false;
                    }
                    else
                    {
                        // control should never come here if we did not succeed; we should have thrown any non-retriable exception or set retry=true for a retriable exception
                        Diagnostics.Assert(setupSuccess, "SetupSession.CodingError {0} Target: {1}", this.tracer, targetPartition);
                    }
                }
            }
            catch (TimeoutException)
            {
                if (session != null)
                {
                    // Abort the session and leave a clean slate for the next time we try to set up the session: we don't want a ReliableSessionAlreadyExists exception
                    session.Abort();
                    session = null;
                }

                throw;
            }
            catch (FabricObjectClosedException)
            {
                // this can happen if the session manager was closed by a parallel thread due to role change
                var endpointString = endpoint ?? "NullEndpoint";
                var sessionIdString = snapshot == null ? "NoSessionCreated" : snapshot.SessionId.ToString();

                FabricEvents.Events.SetupSession(
                    "ObjectClosed@" + this.streamManager.TraceType,
                    targetPartition.ToString(),
                    endpointString,
                    sessionIdString);
                throw;
            }
            catch (Exception e)
            {
                Tracing.WriteExceptionAsError("SetupSession.Failure", e, "Source: {0} Target: {1}", this.tracer, targetPartition);

                Diagnostics.Assert(
                    false,
                    "{0} Unexpected exception {1} in SessionConnectionManager.SetupSession for Target: {2}",
                    this.tracer,
                    e.GetType(),
                    targetPartition);
            }

            Diagnostics.Assert(
                session != null,
                "{0} SessionConnectionManager.SetupSession returning null session for Target: {1}",
                this.tracer,
                targetPartition);

            if (session != null)
            {
                snapshot = session.GetDataSnapshot();

                Diagnostics.Assert(
                    snapshot.IsOpenForSend,
                    "SessionConnectionManager.SetupSession returning session that is not open for send for Source: {0} Target: {1}",
                    this.tracer,
                    targetPartition);

                FabricEvents.Events.SetupSession(
                    "Finish@" + this.streamManager.TraceType,
                    targetPartition.ToString(),
                    endpoint,
                    snapshot.SessionId.ToString());
            }
            return session;
        }

        /// <summary>
        /// When the OutBoundSessionDriver is disposed, Abort and clear the corresponding outbound session. 
        /// </summary>
        /// <param name="targetPartition">Clear the Reliable MessagingSession for the target Partition</param>
        internal void ClearOutboundSession(PartitionKey targetPartition)
        {
            FabricEvents.Events.ClearOutboundSession("Start@" + this.streamManager.TraceType, targetPartition.ToString(), "");
            IReliableMessagingSession outboundSession = null;

            var normalizedPartition = this.GetNormalizedPartitionKey(targetPartition);

            var found = this.activeOutboundSessions.TryRemove(normalizedPartition, out outboundSession);

            if (found)
            {
                var snapshot = outboundSession.GetDataSnapshot();
                outboundSession.Abort();
                FabricEvents.Events.ClearOutboundSession(
                    "Aborted@" + this.streamManager.TraceType,
                    normalizedPartition.ToString(),
                    snapshot.SessionId.ToString());
            }
            else
            {
                FabricEvents.Events.ClearOutboundSession("NotFound@" + this.streamManager.TraceType, normalizedPartition.ToString(), "");
            }
        }

        /// <summary>
        /// Find or create a session for a given partition. 
        /// This is called under a critical section by FindOrCreateDriverAsync and is not designed to be re-entrant
        /// This routine must be called with a resolved and normalized partition, which FindOrCreateDriverAsync does
        /// </summary>
        /// <param name="targetPartition">Find the session for the given target partition</param>
        /// <param name="timeout">Timeout for the operation to complete</param>
        /// <returns>Return the reliable messaging session</returns>
        internal async Task<IReliableMessagingSession> FindOrCreateOutboundSessionAsync(PartitionKey targetPartition, TimeSpan timeout)
        {
            FabricEvents.Events.FindOrCreateOutboundSession("Start@" + this.streamManager.TraceType, targetPartition.ToString(), "");
            IReliableMessagingSession outboundSession = null;

            var found = this.activeOutboundSessions.TryGetValue(targetPartition, out outboundSession);

            if (found)
            {
                var snapshot = outboundSession.GetDataSnapshot();
                FabricEvents.Events.FindOrCreateOutboundSession(
                    "Found@" + this.streamManager.TraceType,
                    targetPartition.ToString(),
                    snapshot.SessionId.ToString());
            }
            else
            {
                FabricEvents.Events.FindOrCreateOutboundSession("NotFound.SettingUp@" + this.streamManager.TraceType, targetPartition.ToString(), "");

                try
                {
                    var newSession = await this.SetupSession(targetPartition, timeout);
                    var added = this.activeOutboundSessions.TryAdd(targetPartition, newSession);
                    Diagnostics.Assert(added, "Failed to add new session to cache in FindOrCreateOutboundSession");
                    var snapshot = newSession.GetDataSnapshot();
                    FabricEvents.Events.FindOrCreateOutboundSession(
                        "SetupFinish@" + this.streamManager.TraceType,
                        targetPartition.ToString(),
                        snapshot.SessionId.ToString());
                    outboundSession = newSession;
                }
                catch (TimeoutException)
                {
                    throw;
                }
                catch (FabricObjectClosedException)
                {
                    throw;
                }
                catch (Exception e)
                {
                    // TODO: overactive assert meant to discover failure modes
                    Diagnostics.Assert(false, "{0} SetupSession failure for Target: {1} with Exception: {2}", this.tracer, targetPartition, e);
                }
            }

            return outboundSession;
        }

        #endregion

        #region IReliableInboundSessionCallback

        /// <summary>
        /// Manages the Inbound Session Callback for a singleton service type.
        /// </summary>
        /// <param name="sourceServiceInstanceName">The inbound source service name</param>
        /// <param name="session">The inbound reliable messaging session</param>
        /// <returns>true if accepted, false otherwise</returns>
        public bool AcceptInboundSession(Uri sourceServiceInstanceName, IReliableMessagingSession session)
        {
            var key = new PartitionKey(sourceServiceInstanceName);
            return this.AcceptInboundSession(key, session);
        }

        /// <summary>
        /// Manages the Inbound Session Callback for a numbered service type.
        /// </summary>
        /// <param name="sourceServiceInstanceName">The inbound source service name</param>
        /// <param name="keyRange">The key range of the inbound source service partition</param>
        /// <param name="session">The inbound reliable messaging session</param>
        /// <returns>true if accepted, false otherwise</returns>
        public bool AcceptInboundSession(Uri sourceServiceInstanceName, IntegerPartitionKeyRange keyRange, IReliableMessagingSession session)
        {
            var key = new PartitionKey(sourceServiceInstanceName, keyRange.IntegerKeyLow, keyRange.IntegerKeyHigh);
            return this.AcceptInboundSession(key, session);
        }

        /// <summary>
        /// Manages the Inbound Session Callback for a named service type.
        /// </summary>
        /// <param name="sourceServiceInstanceName">The inbound source service name</param>
        /// <param name="partitionName">The name of the inbound source service partition</param>
        /// <param name="session">The inbound reliable messaging session</param>
        /// <returns>true if accepted, false otherwise</returns>
        public bool AcceptInboundSession(Uri sourceServiceInstanceName, string partitionName, IReliableMessagingSession session)
        {
            var key = new PartitionKey(sourceServiceInstanceName, partitionName);
            return this.AcceptInboundSession(key, session);
        }

        /// <summary>
        /// For the given source partition and inbound session, update(replace or create) the InboundSessionDriver cache.
        /// </summary>
        /// <param name="key">Source Partition</param>
        /// <param name="session">Corresponding inbound messaging session</param>
        /// <returns>true if accepted, false otherwise</returns>
        private bool AcceptInboundSession(PartitionKey key, IReliableMessagingSession session)
        {
            var snapshot = session.GetDataSnapshot();
            FabricEvents.Events.AcceptInboundSession("Start@" + this.streamManager.TraceType, key.ToString(), snapshot.SessionId.ToString());

            InboundSessionDriver oldDriver = null;
            var removeSuccess = this.streamManager.RuntimeResources.InboundSessionDrivers.TryRemove(key, out oldDriver);

            if (removeSuccess)
            {
                FabricEvents.Events.AcceptInboundSession("ClearedOldDriver@" + this.streamManager.TraceType, key.ToString(), oldDriver.SessionId.ToString());
                oldDriver.Dispose();
            }

            var driver = new InboundSessionDriver(key, session, this.streamManager);

            var addSuccess = this.streamManager.RuntimeResources.InboundSessionDrivers.TryAdd(key, driver);
            Diagnostics.Assert(addSuccess, "Inbound session accepted twice");

            FabricEvents.Events.AcceptInboundSession("Finish@" + this.streamManager.TraceType, key.ToString(), snapshot.SessionId.ToString());
            return true;
        }

        #endregion

        #region IReliableSessionAbortedCallback

        // TODO: Why are we ignoring the session abort callbacks.

        public void SessionAbortedByPartner(bool isInbound, Uri sourceServiceInstanceName, IReliableMessagingSession session)
        {
            this.SessionAbortedByPartner(isInbound, new PartitionKey(sourceServiceInstanceName), session);
        }

        public void SessionAbortedByPartner(bool isInbound, Uri sourceServiceInstanceName, IntegerPartitionKeyRange range, IReliableMessagingSession session)
        {
            this.SessionAbortedByPartner(isInbound, new PartitionKey(sourceServiceInstanceName, range.IntegerKeyLow, range.IntegerKeyHigh), session);
        }

        public void SessionAbortedByPartner(bool isInbound, Uri sourceServiceInstanceName, string stringPartitionKey, IReliableMessagingSession session)
        {
            this.SessionAbortedByPartner(isInbound, new PartitionKey(sourceServiceInstanceName, stringPartitionKey), session);
        }

        private void SessionAbortedByPartner(bool isInbound, PartitionKey partition, IReliableMessagingSession session)
        {
            var snapshot = session.GetDataSnapshot();

            if (isInbound)
            {
                FabricEvents.Events.SessionAbortedByPartner("Inbound@" + this.streamManager.TraceType, partition.ToString(), snapshot.SessionId.ToString());
            }
            else
            {
                FabricEvents.Events.SessionAbortedByPartner("Outbound@" + this.streamManager.TraceType, partition.ToString(), snapshot.SessionId.ToString());
            }
        }

        #endregion
    }
}