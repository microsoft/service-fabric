// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.FaultAnalysis.Service.Common;
    using System.Fabric.Interop;
    using System.Fabric.JsonSerializerImpl;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Reflection;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using Newtonsoft.Json.Linq;

    internal static class ChaosUtil
    {
        public const string ChaosTelemetryStopEventId = "ChaosStop";
        internal static readonly Guid ChaosOperationID = new Guid("9181EC3E-E964-41C1-A5D8-866818CBEA92");
        private const string TraceType = "ChaosUtil";
        private static JsonSerializerSettings chaosEventsDescriptionSerializationSettings;

        static ChaosUtil()
        {
            chaosEventsDescriptionSerializationSettings = new JsonSerializerSettings
            {
                DateTimeZoneHandling = DateTimeZoneHandling.Utc,
                Converters = new List<JsonConverter>
                {
                    new Newtonsoft.Json.Converters.IsoDateTimeConverter
                    {
                        DateTimeFormat = "yyyy'-'MM'-'dd'T'HH':'mm':'ss.fff'Z'"
                    },
                    new StringEnumConverter(),
                    new ChaosParametersJsonConverter()
                }
            };
        }

        /// <summary>
        /// Registers the passed in ChaosEvent in the Reliable Dictionary,
        /// updates ChaosStatus in the Reliable Dictioanty,
        /// and do some post processing if postAction is provided, for example postAction can trace.
        /// </summary>
        /// <param name="stateManager">The statemanager which manages the event and status RD's.</param>
        /// <param name="chaosEvent">The chaos event to register</param>
        /// <param name="chaosStatus">ChaosStatus that should be registered with in the status RD.</param>
        /// <param name="postAction">Other than registering the chaos event and updating the chaos status, whatever action needs to be taken in the same transaction.</param>
        /// <param name="partition">The FAS partition.</param>
        /// <param name="cancellationToken">The cancellation token that was passed into (or created inside) the contaning method.</param>
        /// <returns></returns>
        public static async Task RegisterChaosEventAndUpdateChaosStatusAsync(
            this IReliableStateManager stateManager,
            ChaosEvent chaosEvent,
            ChaosStatus chaosStatus,
            IStatefulServicePartition partition,
            CancellationToken cancellationToken,
            Action postAction = null)
        {
            await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                ChaosOperationID,
                () => RegisterChaosEventAndUpdateChaosStatusPrivateAsync(stateManager, chaosEvent, chaosStatus, postAction, cancellationToken),
                partition,
                "RegisterChaosEventPrivateAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                cancellationToken).ConfigureAwait(false);
        }

        public static async Task UpdateLastEventKeyAsync(
            this IReliableDictionary<string, byte[]> lastKeyDictionary,
            ITransaction tx,
            long lastKey,
            CancellationToken cancellationToken)
        {
            var stamp = new DateTime(lastKey);
            TestabilityTrace.TraceSource.WriteInfo("StartTrek", "Enter UpdateLastEventKeyAsync, timestamp={0}", stamp);

            var keyInBytes = BitConverter.GetBytes(lastKey);
            await lastKeyDictionary.AddOrUpdateAsync(tx, FASConstants.ChaosLastEventKeyName, keyInBytes, (k, v) => keyInBytes, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false);
        }

        public static async Task RegisterCurrentStatusAsync(
            this IReliableStateManager stateManager,
            ChaosStatus currentStatus,
            IStatefulServicePartition partition,
            CancellationToken cancellationToken)
        {
            await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                ChaosOperationID,
                () => RegisterCurrentStatusPrivateAsync(stateManager, currentStatus, cancellationToken),
                partition,
                "RegisterCurrentStatusPrivateAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                cancellationToken).ConfigureAwait(false);
        }

        public static async Task<ChaosStatus> GetCurrentStatusAsync(
            this IReliableDictionary<string, byte[]> statusDictionary,
            ITransaction tx,
            IStatefulServicePartition partition,
            CancellationToken cancellationToken)
        {
            return await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                ChaosOperationID,
                () => GetCurrentStatusPrivateAsync(statusDictionary, tx, cancellationToken),
                partition,
                "GetCurrentStatusPrivateAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                cancellationToken).ConfigureAwait(false);
        }

        public static async Task<long> GetLastStartTimeInTicksAsync(
            this IReliableDictionary<string, byte[]> startimeDictionary,
            ITransaction tx,
            IStatefulServicePartition partition,
            CancellationToken cancellationToken)
        {
            return await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                Guid.NewGuid(),
                () => GetLastStartTimeInTicksPrivateAsync(startimeDictionary, tx, cancellationToken),
                partition,
                "GetLastStartTimeInTicksPrivateAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                cancellationToken).ConfigureAwait(false);
        }

        public static async Task<DateTime> GetUtcTimeStampAsync(
            this IReliableStateManager stateManager,
            string timeStampRDName,
            string timeStampKey,
            IStatefulServicePartition partition,
            CancellationToken cancellationToken)
        {
            return await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                Guid.NewGuid(),
                () => GetUtcTimeStampPrivateAsync(stateManager, timeStampRDName, timeStampKey, cancellationToken),
                partition,
                "GetTimestampPrivateAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                cancellationToken).ConfigureAwait(false);
        }

        public static async Task RemoveKeyFromReliableDictionaryAsync(
            this IReliableStateManager stateManager,
            string dictionaryName,
            string key,
            IStatefulServicePartition partition,
            CancellationToken cancellationToken)
        {
            await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                Guid.NewGuid(),
                () => RemoveKeyFromReliableDictionaryPrivateAsync(stateManager, dictionaryName, key, cancellationToken),
                partition,
                "RemoveKeyFromReliableDictionaryAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                cancellationToken).ConfigureAwait(false);
        }

        public static async Task SetUtcTimeStampAsync(
            this IReliableStateManager stateManager,
            string timeStampRDName,
            string timeStampKey,
            DateTime timeStamp,
            IStatefulServicePartition partition,
            CancellationToken cancellationToken)
        {
            await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                Guid.NewGuid(),
                () => SetUtcTimeStampPrivateAsync(stateManager, timeStampRDName, timeStampKey, timeStamp, cancellationToken),
                partition,
                "SetUtcTimeStampPrivateAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                cancellationToken).ConfigureAwait(false);
        }

        public static async Task AddOrUpdateAsync<TKey>(
            this IReliableDictionary<TKey, byte[]> dictionary,
            IReliableStateManager stateManager,
            TKey key,
            byte[] value,
            IStatefulServicePartition partition,
            CancellationToken cancellationToken) where TKey : IComparable<TKey>, IEquatable<TKey>
        {
            await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                ChaosOperationID,
                () => AddOrUpdatePrivateAsync(dictionary, stateManager, key, value, cancellationToken),
                partition,
                "GetCurrentStatusPrivateAsync",
                FASConstants.MaxRetriesForReliableDictionary,
                cancellationToken).ConfigureAwait(false);
        }

        public static async Task<bool> ChaosWasRunningAsync(
            this IReliableStateManager stateManager,
            IStatefulServicePartition partition,
            CancellationToken cancellationToken)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Enter ChaosWasRunningAsync");

            return await FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                ChaosOperationID,
                () => ChaosWasRunningAsyncPrivate(stateManager, cancellationToken),
                partition,
                "ShouldStopAsyncPrivate",
                FASConstants.MaxRetriesForReliableDictionary,
                cancellationToken);
        }

        public static bool IsExpectedException(Exception exception)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Inside IsExpectedException={0}", exception);

            var fabricException = exception as FabricException;
            if (fabricException != null)
            {
                var innerErrorCode = fabricException.ErrorCode;
                return FASConstants.ExpectedErrorCodes.Contains(innerErrorCode)
                    || FASConstants.ExpectedErrorCodesInternal.Contains((NativeTypes.FABRIC_ERROR_CODE)innerErrorCode);
            }
            else
            {
                return FASConstants.ExpectedExceptions.Contains(exception.GetType());
            }
        }

        public static bool InTerminalState(Task task)
        {
            return task.IsCompleted || task.IsCanceled || task.IsFaulted;
        }

        public static async Task ValidateChaosTargetFilterAsync(
            FabricClient fabricClient,
            ChaosParameters chaosParameters,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            // If chaosParameters == null, there is no input to validate
            // If chaosParameters.Context == null, there is no DisableInputValidationForChaosTargetFilterKey internal key, so whether the input is valid depends on ChaosTargetFilter
            // If chaosParameters.Context != null, we need to check if DisableInputValidationForChaosTargetFilterKey internal key is there, if yes we do not validate input, if on the other had
            // DisableInputValidationForChaosTargetFilterKey is not present then validation depends on ChaosTargetFilter. If ChaosTargetFilter is null, there is nothing to validate.
            if (chaosParameters != null && chaosParameters.Context != null && !chaosParameters.Context.ContainsKey(ChaosConstants.DisableInputValidationForChaosTargetFilterKey) && chaosParameters.ChaosTargetFilter != null)
            {
                var includedNodeTypes = chaosParameters.ChaosTargetFilter.NodeTypeInclusionList;

                if (includedNodeTypes != null && includedNodeTypes.Any())
                {
                    var nodeList =
                        await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => fabricClient.QueryManager.GetNodeListAsync(null, requestTimeout, cancellationToken), operationTimeout, cancellationToken).ConfigureAwait(false);

                    HashSet<string> invalidNodeTypeSet = new HashSet<string>(includedNodeTypes);

                    foreach (var node in nodeList)
                    {
                        if (invalidNodeTypeSet.Contains(node.NodeType))
                        {
                            invalidNodeTypeSet.Remove(node.NodeType);
                        }
                    }

                    if (invalidNodeTypeSet.Any())
                    {
                        var invalidNodeTypes = string.Join(",", invalidNodeTypeSet);

                        throw new ArgumentException(string.Format(StringResources.ChaosApiError_InvalidNodeType, invalidNodeTypes));
                    }
                }

                var includedApplications = chaosParameters.ChaosTargetFilter.ApplicationInclusionList;

                if (includedApplications != null && includedApplications.Any())
                {
                    var applicationList =
                        await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => fabricClient.QueryManager.GetApplicationListAsync(null, requestTimeout, cancellationToken), operationTimeout, cancellationToken).ConfigureAwait(false);

                    var nonexistentApplications = new List<string>();

                    foreach (var app in includedApplications)
                    {
                        var foundApplication =
                            applicationList.FirstOrDefault(a => a.ApplicationName.OriginalString.Equals(app, StringComparison.Ordinal));

                        if (foundApplication == null)
                        {
                            nonexistentApplications.Add(app);
                        }
                    }

                    if (nonexistentApplications.Any())
                    {
                        throw new ArgumentException(string.Format(StringResources.ChaosApiError_InvalidApplicationName, string.Join(",", nonexistentApplications)));
                    }
                }
            }
        }

        #region GetChaosEvents

        public static ChaosEventsDescription DeserializeChaosEventsDescriptionFromNative(string chaosEventsDescriptionJson)
        {
            JObject eventsDescriptionDTO = (JObject)JsonConvert.DeserializeObject(chaosEventsDescriptionJson, chaosEventsDescriptionSerializationSettings);

            var eventsDescription = JsonConvert.DeserializeObject<ChaosEventsDescription>(chaosEventsDescriptionJson, chaosEventsDescriptionSerializationSettings);

            JToken pagingDescriptionToken = eventsDescriptionDTO["QueryPagingDescription"];
            if (pagingDescriptionToken != null)
            {
                if (eventsDescriptionDTO["QueryPagingDescription"]["ContinuationToken"] != null)
                {
                    eventsDescription.ContinuationToken = (string)eventsDescriptionDTO["QueryPagingDescription"]["ContinuationToken"];
                }

                if (eventsDescriptionDTO["QueryPagingDescription"]["MaxResults"] != null)
                {
                    eventsDescription.MaxResults = (long)eventsDescriptionDTO["QueryPagingDescription"]["MaxResults"];
                }
            }

            return eventsDescription;
        }

        public static string SerializeChaosEvents(ChaosEventsSegment events)
        {
                // we will convert the chaos events to a dto that follows the required format in native.
                JsonSerializer serializer = JsonSerializer.Create(chaosEventsDescriptionSerializationSettings);

                JObject eventsDTO = JObject.FromObject(events, serializer);

                for (int i = 0; i < ((JArray)eventsDTO["History"]).Count; ++i)
                {
                    // turns each ChaosEvent in history from { ChaosEvent stuff} into { ChaosEvent: { ChaosEvent stuff } }
                    JObject chaosEventWrapper = new JObject();
                    JObject chaosEventDTO = (JObject)((JArray)eventsDTO["History"])[i];

                    // Reason needs to be no multiple of four so it doesnt cause decompression error
                    IDictionary<string, JToken> chaosEventDict = chaosEventDTO;
                    if (chaosEventDict.ContainsKey("Reason"))
                    {
                        chaosEventDTO["Reason"] = ChaosUtility.MakeLengthNotMultipleOfFourIgnoreReasonLength(chaosEventDTO.GetValue("Reason").Value<string>());
                    }

                    JProperty kindProperty = new JProperty(chaosEventDTO.Property("Kind"));
                    chaosEventDTO.Property("Kind").Remove();
                    chaosEventDTO.AddFirst(kindProperty);

                    chaosEventWrapper["ChaosEvent"] = chaosEventDTO;
                    ((JArray)eventsDTO["History"])[i] = chaosEventWrapper;
                }

                return JsonConvert.SerializeObject(eventsDTO, chaosEventsDescriptionSerializationSettings);
        }

        #endregion

        internal static void ThrowNotReady(string message)
        {
            throw FaultAnalysisServiceUtility.CreateException(TraceType, NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY, message);
        }

        internal static void ThrowAlreadyRunning(string message = default(string))
        {
            throw FaultAnalysisServiceUtility.CreateException(TraceType, NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_CHAOS_ALREADY_RUNNING, StringResources.ChaosError_ChaosAlreadyRunning);
        }

        private static async Task RegisterChaosEventAndUpdateChaosStatusPrivateAsync(
            this IReliableStateManager stateManager,
            ChaosEvent chaosEvent,
            ChaosStatus currentStatus,
            Action postAction,
            CancellationToken cancellationToken)
        {
            var eventsDictionary = await stateManager.GetOrAddAsync<IReliableDictionary<long, byte[]>>(FASConstants.ChaosEventsDictionaryName).ConfigureAwait(false);
            var lastEventKeyDictionary = await stateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosLastEventKeyDictionaryName).ConfigureAwait(false);
            var statusDictionary = await stateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosStatusDictionaryName).ConfigureAwait(false);

            using (ITransaction tx = stateManager.CreateTransaction())
            {
                long eventKey = chaosEvent.TimeStampUtc.Ticks;
                if (await eventsDictionary.ContainsKeyAsync(tx, eventKey, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false))
                {
                    ++eventKey;
                    chaosEvent.TimeStampUtc = chaosEvent.TimeStampUtc.AddTicks(1);
                }

                byte[] chaosEventInBytes = chaosEvent.ToBytes();

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "RegisterChaosEventAndUpdateChaosStatusPrivateAsync attempting to add event == {0} with key {1}.", chaosEvent, eventKey);

                await eventsDictionary.AddOrUpdateAsync(tx, eventKey, chaosEventInBytes, (k, v) => chaosEventInBytes, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false);

                await lastEventKeyDictionary.UpdateLastEventKeyAsync(tx, chaosEvent.TimeStampUtc.Ticks, cancellationToken).ConfigureAwait(false);

                // Update status
                byte[] statusInBytes = BitConverter.GetBytes((int)currentStatus);

                if (await statusDictionary.ContainsKeyAsync(tx, FASConstants.ChaosStatusKeyName, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false))
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "RegisterChaosEventAndUpdateChaosStatusPrivateAsync attempting to update status == {0} with key {1}.", currentStatus, FASConstants.ChaosStatusKeyName);

                    await statusDictionary.SetAsync(tx, FASConstants.ChaosStatusKeyName, statusInBytes, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false);
                }
                else
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "RegisterChaosEventAndUpdateChaosStatusPrivateAsync attempting to add status == {0} with key {1}.", currentStatus, FASConstants.ChaosStatusKeyName);

                    await statusDictionary.AddAsync(tx, FASConstants.ChaosStatusKeyName, statusInBytes, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false);
                }

                if (postAction != null)
                {
                    postAction();
                }

                TestabilityTrace.TraceSource.WriteInfo(TraceType, "RegisterChaosEventAndUpdateChaosStatusPrivateAsync stored event == {0} and status {1}.", chaosEvent, currentStatus);

                await tx.CommitAsync().ConfigureAwait(false);
            }
        }

        private static async Task RegisterCurrentStatusPrivateAsync(
            IReliableStateManager stateManager,
            ChaosStatus currentStatus,
            CancellationToken cancellationToken)
        {
            var statusDictionary = await stateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosStatusDictionaryName).ConfigureAwait(false);

            using (ITransaction tx = stateManager.CreateTransaction())
            {
                byte[] statusInBytes = BitConverter.GetBytes((int)currentStatus);

                if (await statusDictionary.ContainsKeyAsync(tx, FASConstants.ChaosStatusKeyName, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false))
                {
                    await statusDictionary.SetAsync(tx, FASConstants.ChaosStatusKeyName, statusInBytes, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false);
                }
                else
                {
                    await statusDictionary.AddAsync(tx, FASConstants.ChaosStatusKeyName, statusInBytes, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false);
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }
        }

        private static async Task<ChaosStatus> GetCurrentStatusPrivateAsync(
            IReliableDictionary<string, byte[]> statusDictionary,
            ITransaction tx,
            CancellationToken cancellationToken)
        {
            ChaosStatus status = ChaosStatus.Stopped;

            if (await statusDictionary.ContainsKeyAsync(tx, FASConstants.ChaosStatusKeyName, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false))
            {
                var conditionalStatus = await statusDictionary.TryGetValueAsync(tx, FASConstants.ChaosStatusKeyName, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false);

                if (conditionalStatus.HasValue)
                {
                    status = (ChaosStatus)BitConverter.ToInt32(conditionalStatus.Value, 0);
                }
            }
            else
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "GetCurrentStatus was called, but there was no entry with key == {0}.", FASConstants.ChaosStatusKeyName);
            }

            return status;
        }

        private static async Task<long> GetLastStartTimeInTicksPrivateAsync(
            IReliableDictionary<string, byte[]> starttimeDictionary,
            ITransaction tx,
            CancellationToken cancellationToken)
        {
            var conditionalStartTime = await starttimeDictionary.TryGetValueAsync(
                                                                                    tx,
                                                                                    FASConstants.StartTimeKey,
                                                                                    FASConstants.ReliableDictionaryTimeout,
                                                                                    cancellationToken).ConfigureAwait(false);

            if (!conditionalStartTime.HasValue)
            {
                return -1;
            }

            byte[] startTimeInBytes = conditionalStartTime.Value;
            var startTime = new DateTime(BitConverter.ToInt64(startTimeInBytes, 0));

            return startTime.Ticks;
        }

        private static async Task<DateTime> GetUtcTimeStampPrivateAsync(
            this IReliableStateManager stateManager,
            string timeStampDictionaryName,
            string timeStampKey,
            CancellationToken cancellationToken)
        {
            IReliableDictionary<string, byte[]> timeStampRD = await stateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(timeStampDictionaryName).ConfigureAwait(false);
            using (var tx = stateManager.CreateTransaction())
            {
                var conditionalTimeStamp = await timeStampRD.TryGetValueAsync(
                                                                                tx,
                                                                                timeStampKey,
                                                                                FASConstants.ReliableDictionaryTimeout,
                                                                                cancellationToken).ConfigureAwait(false);

                // If no value could be retrieved from the RD, we return UtcNow
                if (!conditionalTimeStamp.HasValue)
                {
                    return DateTime.UtcNow;
                }

                await tx.CommitAsync().ConfigureAwait(false);

                return new DateTime(BitConverter.ToInt64(conditionalTimeStamp.Value, 0));
            }
        }

        private static async Task RemoveKeyFromReliableDictionaryPrivateAsync(
            this IReliableStateManager stateManager,
            string dictionaryName,
            string keyToRemove,
            CancellationToken ctok)
        {
            IReliableDictionary<string, byte[]> dictionary = await stateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(dictionaryName).ConfigureAwait(false);

            using (var tx = stateManager.CreateTransaction())
            {
                await dictionary.TryRemoveAsync(tx, keyToRemove).ConfigureAwait(false);
                await tx.CommitAsync().ConfigureAwait(false);
            }
        }

        private static async Task SetUtcTimeStampPrivateAsync(
            this IReliableStateManager stateManager,
            string timeStampDictionaryName,
            string timeStampKey,
            DateTime timeStamp,
            CancellationToken cancellationToken)
        {
            IReliableDictionary<string, byte[]> timeStampRD = await stateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(timeStampDictionaryName).ConfigureAwait(false);
            using (var tx = stateManager.CreateTransaction())
            {
                byte[] timestampInBytes = BitConverter.GetBytes(timeStamp.Ticks);
                await timeStampRD.AddOrUpdateAsync(tx, timeStampKey, timestampInBytes, (k, v) => timestampInBytes, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false);

                await tx.CommitAsync().ConfigureAwait(false);
            }
        }

        private static async Task AddOrUpdatePrivateAsync<TKey>(
            IReliableDictionary<TKey, byte[]> dictionary,
            IReliableStateManager stateManager,
            TKey key,
            byte[] value,
            CancellationToken cancellationToken) where TKey : IComparable<TKey>, IEquatable<TKey>
        {
            using (ITransaction tx = stateManager.CreateTransaction())
            {
                await dictionary.AddOrUpdateAsync(tx, key, value, (k, v) => value, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false);

                await tx.CommitAsync().ConfigureAwait(false);
            }
        }

        private static async Task<bool> ChaosWasRunningAsyncPrivate(
            IReliableStateManager stateManager,
            CancellationToken cancellationToken)
        {
            // If Chaos was previously stopped, no need to do anything, return immediately; otherwise move on...
            bool shouldReturn = false;

            var lastEventKeyDict = await stateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(FASConstants.ChaosLastEventKeyDictionaryName).ConfigureAwait(false);

            using (var tx = stateManager.CreateTransaction())
            {
                if (await lastEventKeyDict.ContainsKeyAsync(tx, FASConstants.ChaosLastEventKeyName, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false))
                {
                    var lastEventKey = await lastEventKeyDict.TryGetValueAsync(tx, FASConstants.ChaosLastEventKeyName, LockMode.Update, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false);

                    if (lastEventKey.HasValue)
                    {
                        var lastKey = BitConverter.ToInt64(lastEventKey.Value, 0);
                        var eventsDict = await stateManager.GetOrAddAsync<IReliableDictionary<long, byte[]>>(FASConstants.ChaosEventsDictionaryName).ConfigureAwait(false);
                        var lastEventResult = await eventsDict.TryGetValueAsync(tx, lastKey, FASConstants.ReliableDictionaryTimeout, cancellationToken).ConfigureAwait(false);

                        if (lastEventResult.HasValue)
                        {
                            var lastEvent = ByteSerializationHelper.GetEventFromBytes(lastEventResult.Value);

                            // Starts only if the lastevent was a StoppedEvent, because if it was not stopped, it should already be
                            // running; if the user need to change the params, he need to stop it first, then start with the
                            // new parameter
                            bool wasStopped = lastEvent is StoppedEvent;
                            if (wasStopped)
                            {
                                shouldReturn = true;
                            }
                        }
                    }
                    else
                    {
                        shouldReturn = true;
                    }
                }
                else
                {
                    shouldReturn = true;
                }

                await tx.CommitAsync().ConfigureAwait(false);

                if (shouldReturn)
                {
                    return false;
                }
            }

            return true;
        }
    }
}