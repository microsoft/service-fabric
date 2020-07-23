// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Temp
{
    using System;
    using System.Diagnostics.Tracing;


    // TODO: replace this with trace into product
    [EventSource(Name = "Microsoft-Test-WindowsFabric", Guid = TempEventSource.ProviderIdString)]
    public class TempEventSource : EventSource
    {
        public const string ProviderIdString = "{FC9E797F-436C-44C9-859E-502BBD654403}";
        public static readonly Guid ProviderId = new Guid(ProviderIdString);

        private static readonly TempEventSource SingletonInstance = new TempEventSource();

        private TempEventSource()
        {
        }

        // We are forced to make this a singleton due to the way that EventSource instances are managed by .NET.
        // If we create more than one instance of the same EventSource per AppDomain, conflicts can arise, e.g.
        // ArgumentExceptions thrown from Enable/DisableEvents.
        public static TempEventSource Instance
        {
            get { return SingletonInstance; }
        }

        [Event(
            TestTracingCode.GenericError,
            Level = EventLevel.Error,
            Message = "{0}")]
        public void OnGenericError(string message)
        {
            this.WriteEvent(TestTracingCode.GenericError, message);
        }

        [Event(
            TestTracingCode.GenericWarning,
            Level = EventLevel.Warning,
            Message = "{0}")]
        public void OnGenericWarning(string message)
        {
            this.WriteEvent(TestTracingCode.GenericWarning, message);
        }

        [Event(
            TestTracingCode.GenericInfo,
            Level = EventLevel.Informational,
            Message = "{0}")]
        public void OnGenericInfo(string message)
        {
            this.WriteEvent(TestTracingCode.GenericInfo, message);
        }
        /*
        [Event(
            TestTracingCode.GenericVerbose,
            Level = EventLevel.Verbose,
            Message = "{0}")]
        public void OnGenericVerbose(string message)
        {
            this.WriteEvent(TestTracingCode.GenericVerbose, message);
        }

        [Event(
            TestTracingCode.RemoteObjectHostAddingObject,
            Level = EventLevel.Informational,
            Message = "Remote object host adding object '{0}', name='{1}'.")]
        public void OnRemoteObjectHostAddingObject(Guid id, string name)
        {
            this.WriteEvent(TestTracingCode.RemoteObjectHostAddingObject, id, name);
        }

        [Event(
            TestTracingCode.RemoteObjectHostAddedObject,
            Level = EventLevel.Informational,
            Message = "Remote object host added object '{0}'.")]
        public void OnRemoteObjectHostAddedObject(Guid id)
        {
            this.WriteEvent(TestTracingCode.RemoteObjectHostAddedObject, id);
        }

        [Event(
            TestTracingCode.RemoteObjectHostRemovingObject,
            Level = EventLevel.Informational,
            Message = "Remote object host removing object '{0}'.")]
        public void OnRemoteObjectHostRemovingObject(Guid id)
        {
            this.WriteEvent(TestTracingCode.RemoteObjectHostRemovingObject, id);
        }

        [Event(
            TestTracingCode.RemoteObjectHostRemovedObject,
            Level = EventLevel.Informational,
            Message = "Remote object host removed object '{0}'.")]
        public void OnRemoteObjectHostRemovedObject(Guid id)
        {
            this.WriteEvent(TestTracingCode.RemoteObjectHostRemovedObject, id);
        }

        [Event(
            TestTracingCode.RemoteObjectHostInvokingObject,
            Level = EventLevel.Informational,
            Message = "Remote object host invoking command '{1}' for object '{0}'.")]
        public void OnRemoteObjectHostInvokingObject(Guid id, string name)
        {
            this.WriteEvent(TestTracingCode.RemoteObjectHostInvokingObject, id, name);
        }

        [Event(
            TestTracingCode.TraceEnvironmentVariable,
            Level = EventLevel.Informational,
            Message = "{0}='{1}'")]
        public void OnTraceEnvironmentVariable(string name, string value)
        {
            this.WriteEvent(TestTracingCode.TraceEnvironmentVariable, name, value);
        }

        [Event(
            TestTracingCode.TestActionStarted,
            Level = EventLevel.Informational,
            Message = "Test action '{0}' started, activity ID={1}, parent activity ID={2}.")]
        public void OnTestActionStarted(string name, Guid activityId, Guid parentActivityId)
        {
            this.WriteEvent(TestTracingCode.TestActionStarted, name, activityId, parentActivityId);
        }

        [Event(
            TestTracingCode.TestActionCompleted,
            Level = EventLevel.Informational,
            Message = "Test action '{0}' completed, activity ID={1}.")]
        public void OnTestActionCompleted(string name, Guid activityId)
        {
            this.WriteEvent(TestTracingCode.TestActionCompleted, name, activityId);
        }

        [Event(
            TestTracingCode.TestActionCompletedWithError,
            Level = EventLevel.Warning,
            Message = "Test action '{0}' completed with error, activity ID={1}: {2}")]
        public void OnTestActionCompletedWithError(string name, Guid activityId, string error)
        {
            this.WriteEvent(TestTracingCode.TestActionCompletedWithError, name, activityId, error);
        }

        [Event(
            TestTracingCode.TestActionSkipped,
            Level = EventLevel.Informational,
            Message = "Test action '{0}' skipped, activity ID={1}, detail: {2}.")]
        public void OnTestActionSkipped(string name, Guid activityId, string detail)
        {
            this.WriteEvent(TestTracingCode.TestActionSkipped, name, activityId, detail);
        }

        [Event(
            TestTracingCode.TestActionScopeCreated,
            Level = EventLevel.Informational,
            Message = "Test action scope '{0}' created, activity ID={1}, parent activity ID={2}.")]
        public void OnTestActionScopeCreated(string name, Guid activityId, Guid parentActivityId)
        {
            this.WriteEvent(TestTracingCode.TestActionScopeCreated, name, activityId, parentActivityId);
        }

        [Event(
            TestTracingCode.TestActionScopeDisposed,
            Level = EventLevel.Informational,
            Message = "Test action scope '{0}' disposed, activity ID={1}.")]
        public void OnTestActionScopeDisposed(string name, Guid activityId)
        {
            this.WriteEvent(TestTracingCode.TestActionScopeDisposed, name, activityId);
        }

        [Event(
            TestTracingCode.TestActionScopeCanceled,
            Level = EventLevel.Informational,
            Message = "Test action scope '{0}' canceled, activity ID={1}.")]
        public void OnTestActionScopeCanceled(string name, Guid activityId)
        {
            this.WriteEvent(TestTracingCode.TestActionScopeCanceled, name, activityId);
        }

        [Event(
            TestTracingCode.TestActionScopePausing,
            Level = EventLevel.Informational,
            Message = "Test action scope '{0}' pausing, activity ID={1}.")]
        public void OnTestActionScopePausing(string name, Guid activityId)
        {
            this.WriteEvent(TestTracingCode.TestActionScopePausing, name, activityId);
        }

        [Event(
            TestTracingCode.TestActionScopePaused,
            Level = EventLevel.Informational,
            Message = "Test action scope '{0}' paused, activity ID={1}.")]
        public void OnTestActionScopePaused(string name, Guid activityId)
        {
            this.WriteEvent(TestTracingCode.TestActionScopePaused, name, activityId);
        }

        [Event(
            TestTracingCode.TestActionScopeResumed,
            Level = EventLevel.Informational,
            Message = "Test action scope '{0}' resumed, activity ID={1}.")]
        public void OnTestActionScopeResumed(string name, Guid activityId)
        {
            this.WriteEvent(TestTracingCode.TestActionScopeResumed, name, activityId);
        }

        [Event(
            TestTracingCode.TestLoopIterationRunning,
            Level = EventLevel.Informational,
            Message = "Test loop iteration {0} started, activity ID={1}.")]
        public void OnTestLoopIterationRunning(int iteration, Guid activityId)
        {
            this.WriteEvent(TestTracingCode.TestLoopIterationRunning, iteration, activityId);
        }

        [Event(
            TestTracingCode.TestLoopLastIteration,
            Level = EventLevel.Informational,
            Message = "Test loop last iteration completing, activity ID={0}.")]
        public void OnTestLoopLastIteration(Guid activityId)
        {
            this.WriteEvent(TestTracingCode.TestLoopLastIteration, activityId);
        }

        [Event(
            TestTracingCode.TestLoopRunningValidator,
            Level = EventLevel.Informational,
            Message = "Test loop running validator '{0}', activity ID={1}.")]
        public void OnTestLoopRunningValidator(string name, Guid activityId)
        {
            this.WriteEvent(TestTracingCode.TestLoopRunningValidator, name, activityId);
        }

        [Event(
            TestTracingCode.TestLoopValidationResultAdded,
            Level = EventLevel.Informational,
            Message = "Adding validation result ID={0}, category='{1}', detail='{2}', activity ID={3}.")]
        public void OnTestLoopValidationResultAdded(int id, string category, string detail, Guid activityId)
        {
            this.WriteEvent(TestTracingCode.TestLoopValidationResultAdded, id, category, detail, activityId);
        }

        [Event(
            TestTracingCode.RetryLoopIterationStarted,
            Level = EventLevel.Informational,
            Message = "{0}: Retry Loop started for request: '{1}'.")]
        public void OnRetryLoopStarted(Guid activityId, string request)
        {
            this.WriteEvent(TestTracingCode.RetryLoopIterationStarted, activityId, request);
        }

        [Event(
            TestTracingCode.RetryLoopIterationCompleted,
            Level = EventLevel.Verbose,
            Message = "{0}: Retry Loop iteration completed. Current ContextInfo: Succeeded: '{1}' ShouldRetry: '{2}' Iteration: '{3}' ElapsedTime: '{4}'")]
        public void OnRetryLoopIterationCompleted(Guid activityId, bool succeeded, bool shouldRetry, int iteration, string elapsedTime)
        {
            this.WriteEvent(TestTracingCode.RetryLoopIterationCompleted, activityId, succeeded, shouldRetry, iteration, elapsedTime);
        }

        [Event(
            TestTracingCode.DelayStarted,
            Level = EventLevel.Informational,
            Message = "{0}: Sleeping '{1}' seconds.")]
        public void OnDelayStarted(Guid activityId, double timeout)
        {
            this.WriteEvent(TestTracingCode.DelayStarted, activityId, timeout);
        }

        [Event(
            TestTracingCode.DelayAmountUpdated,
            Level = EventLevel.Verbose,
            Message = "{0}: Increasing current delay time from '{1}' to '{2}' seconds.")]
        public void OnDelayAmountUpdated(Guid activityId, double previousTimeout, double nextTimeout)
        {
            this.WriteEvent(TestTracingCode.DelayAmountUpdated, activityId, previousTimeout, nextTimeout);
        }

        [Event(
            TestTracingCode.RetryLoopCanceled,
            Level = EventLevel.Warning,
            Message = "{0}: Cancellation was requested for request")]
        public void OnRetryLoopCanceled(Guid activityId)
        {
            this.WriteEvent(TestTracingCode.RetryLoopCanceled, activityId);
        }

        [Event(
            TestTracingCode.RetryConditionExhausted,
            Level = EventLevel.Error,
            Message = "{0}: Retry '{1}' has been exhausted for request")]
        public void OnRetryConditionExhausted(Guid activityId, string conditionType)
        {
            this.WriteEvent(TestTracingCode.RetryConditionExhausted, activityId, conditionType);
        }

        [Event(
            TestTracingCode.CountConstrainedRequestSucceeded,
            Level = EventLevel.Informational,
            Message = "{0}: Request succeeded with expected result count: '{1}'")]
        public void OnCountConstrainedRequestSucceeded(Guid activityId, int count)
        {
            this.WriteEvent(TestTracingCode.CountConstrainedRequestSucceeded, activityId, count);
        }

        [Event(
            TestTracingCode.RetryRequestedOnCountConstrainedRequest,
            Level = EventLevel.Warning,
            Message = "{0}: Request failed in result count validation. Expected result count: '{1}'. Actual result count : '{2}'")]
        public void OnRetryRequestedOnCountConstrainedRequest(Guid activityId, int expectedCount, int actualCount)
        {
            this.WriteEvent(TestTracingCode.RetryRequestedOnCountConstrainedRequest, activityId, expectedCount, actualCount);
        }

        [Event(
            TestTracingCode.CurrentProgressReceived,
            Level = EventLevel.Informational,
            Message = "{0}: Current upgrade progress received: '{1}'.")]
        public void OnCurrentProgressReceived(Guid activityId, string currentProgress)
        {
            this.WriteEvent(TestTracingCode.CurrentProgressReceived, activityId, currentProgress);
        }

        [Event(
            TestTracingCode.UpgradeCompleted,
            Level = EventLevel.Informational,
            Message = "{0}: Upgrade completed based on selected criteria.")]
        public void OnUpgradeCompleted(Guid activityId)
        {
            this.WriteEvent(TestTracingCode.UpgradeCompleted, activityId);
        }

        [Event(
            TestTracingCode.UpgradeDomainChanged,
            Level = EventLevel.Informational,
            Message = "{0}: Upgrade domain {1} status changed.")]
        public void OnUpgradeDomainChanged(Guid activityId, string domain)
        {
            this.WriteEvent(TestTracingCode.UpgradeDomainChanged, activityId, domain);
        }

        [Event(
            TestTracingCode.ValidationStarted,
            Level = EventLevel.Informational,
            Message = "{0}: Validation {1} started against latest upgrade status")]
        public void OnValidationStarted(Guid activityId, string validationName)
        {
            this.WriteEvent(TestTracingCode.ValidationStarted, activityId, validationName);
        }

        [Event(
            TestTracingCode.UpgradeTimeRemaining,
            Level = EventLevel.Informational,
            Message = "{0}: Waiting for upgrade complete with {1} remaining")]
        public void OnTraceRemainingUpgradeTime(Guid activityId, string remainingTime)
        {
            this.WriteEvent(TestTracingCode.UpgradeTimeRemaining, activityId, remainingTime);
        }

        [Event(
            TestTracingCode.PowerShellCommandResults,
            Level = EventLevel.Informational,
            Message = "Command: {0} {1}; Results: {2}")]
        public void OnPowerShellCommandResults(string command, string parameters, string results)
        {
            this.WriteEvent(TestTracingCode.PowerShellCommandResults, command, parameters, results);
        }

        [Event(
            TestTracingCode.PowerShellCommandFailure,
            Level = EventLevel.Warning,
            Message = "Command: {0} {1}; Error Message: {2}")]
        public void OnPowerShellCommandFailure(string command, string parameters, string errorMessage)
        {
            this.WriteEvent(TestTracingCode.PowerShellCommandFailure, command, parameters, errorMessage);
        }

        [Event(
            TestTracingCode.RestCommandResults,
            Level = EventLevel.Informational,
            Message = "Command: {0} {1}; Results: {2}")]
        public void OnRestCommandResults(string command, string parameters, string results)
        {
            this.WriteEvent(TestTracingCode.RestCommandResults, command, parameters, results);
        }

        [Event(
            TestTracingCode.RestCommandFailure,
            Level = EventLevel.Warning,
            Message = "Command: {0} {1}; Error Message: {2}")]
        public void OnRestCommandFailure(string command, string parameters, string errorMessage)
        {
            this.WriteEvent(TestTracingCode.RestCommandFailure, command, parameters, errorMessage);
        }
         * */
    }

    public static class TestTracingCode
    {
        // Generic events
        public const int GenericError = 1;
        public const int GenericWarning = 2;
        public const int GenericInfo = 3;
        public const int GenericVerbose = 4;

        // Remote objects
        public const int RemoteObjectHostAddingObject = 5;
        public const int RemoteObjectHostAddedObject = 6;
        public const int RemoteObjectHostRemovingObject = 7;
        public const int RemoteObjectHostRemovedObject = 8;
        public const int RemoteObjectHostInvokingObject = 9;

        // Machine info
        public const int TraceEnvironmentVariable = 20;

        // Test action events
        public const int TestActionStarted = 100;
        public const int TestActionCompleted = 101;
        public const int TestActionCompletedWithError = 102;
        public const int TestActionSkipped = 103;
        public const int TestActionScopeCreated = 104;
        public const int TestActionScopeDisposed = 105;
        public const int TestActionScopeCanceled = 106;
        public const int TestActionScopePausing = 107;
        public const int TestActionScopePaused = 108;
        public const int TestActionScopeResumed = 109;
        public const int TestLoopIterationRunning = 110;
        public const int TestLoopLastIteration = 111;
        public const int TestLoopRunningValidator = 112;
        public const int TestLoopValidationResultAdded = 113;

        // RetryLoop & Request events
        public const int RetryLoopIterationStarted = 150;
        public const int RetryLoopIterationCompleted = 151;
        public const int RetryRequestedOnRequest = 152;
        public const int RetryInvalidatedOnRequest = 153;
        public const int RequestSucceeded = 154;
        public const int DelayStarted = 155;
        public const int DelayAmountUpdated = 156;
        public const int RetryLoopCanceled = 157;
        public const int RetryConditionExhausted = 158;
        public const int CountConstrainedRequestSucceeded = 159;
        public const int RetryRequestedOnCountConstrainedRequest = 160;

        // UpgradeManager events
        public const int CurrentProgressReceived = 180;
        public const int UpgradeCompleted = 181;
        public const int UpgradeDomainChanged = 182;
        public const int ValidationStarted = 183;
        public const int UpgradeTimeRemaining = 184;

        // PowerShell client events
        public const int PowerShellCommandResults = 190;
        public const int PowerShellCommandFailure = 191;

        // Rest client events
        public const int RestCommandResults = 200;
        public const int RestCommandFailure = 201;
    }
}