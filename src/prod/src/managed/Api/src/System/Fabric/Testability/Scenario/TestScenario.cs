// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Scenario
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Base class for TestScenarios which defines all the common methods and functionality.
    /// </summary>
    public abstract class TestScenario : IDisposable
    {
        internal static readonly FabricEvents.ExtensionsEvents Log = TestabilityTrace.TraceSource;

        private static readonly int MaxConsecutiveFailures = 5;

        private const string TraceType = "TestScenario";
        private bool disposed = false;

        private int consecutiveFailures;
        private Stopwatch stopwatch;

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="fabricClient">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        /// <param name="testScenarioParameters">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        protected internal TestScenario(FabricClient fabricClient, TestScenarioParameters testScenarioParameters)
        {
            ThrowIf.Null(fabricClient, "fabricClient");
            ThrowIf.Null(testScenarioParameters, "testScenarioParameters");

            this.ScenarioParameters = testScenarioParameters;
            this.FabricClient = fabricClient;

            this.consecutiveFailures = 0;
        }

        internal FabricTestContext TestContext { get; private set; }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <value>
        /// Returns <see cref="System.Fabric.Testability.Scenario.TestScenarioParameters" />.
        /// </value>
        protected TestScenarioParameters ScenarioParameters { get; private set; }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <value>
        /// Returns <see cref="System.Fabric.FabricClient" />.
        /// </value>
        protected FabricClient FabricClient { get; private set; }

        /// <summary>
        /// Event handler which exposes the current progress of the test. When handled it gives you the ProgressChangedEventArgs
        /// which gives out a total % of the test which has already completed and the last operation (string) that was performed 
        /// which can be useful for logging.
        /// </summary>
        public ProgressChangedEventHandler ProgressChanged;
        
        /// <summary>
        /// This method executes the test with the specified parameter set passed into the constructor.
        /// </summary>
        /// <param name="token">the Cancellation token for the async operation.</param>
        /// <exception cref= "System.Fabric.FabricValidationException" >If any service does not stabilize within the specified timeout.</exception>
        /// <returns>Returns a Task which represents the asynchronous operation.</returns>
        public async Task ExecuteAsync(CancellationToken token)
        {
            DateTime testStartTime = DateTime.UtcNow;
            try
            {
                this.stopwatch = new Stopwatch();
                this.stopwatch.Start();

                this.TestContext = this.FabricClient.FaultManager.TestContext;

                // Call derived class method OnExecuteAsync()
                await this.OnExecuteAsync(token);

                this.ReportProgress("Scenario complete");

                await this.ValidateScenarioAtExitAsync(token);

                Log.WriteInfo(TraceType, "ExecuteAsync(): Scenario completed.");
            }
            catch (OperationCanceledException)
            {
                Log.WriteWarning(TraceType, "ExecuteAsync(): Cancellation was requested while running the scenario.");
                throw;
            }
            catch (Exception e)
            {
                Log.WriteError(TraceType, "ExecuteAsync(): Exception was thrown while running the scenario: {0}", e);
                throw;
            }
        }

        /// <summary>
        /// This method will dispose the TestScenario.
        /// </summary>
        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            // call Dispose(true) of derived class.
            // wait for derived class to finish clean up.
            this.OnDispose(true);

            GC.SuppressFinalize(this);
            this.disposed = true;
        }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="token">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        /// <returns>A task representing the execution of the TestScenario.</returns>
        protected abstract Task OnExecuteAsync(CancellationToken token);

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="disposing">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        protected abstract void OnDispose(bool disposing);

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="token">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        /// <returns>A task representing the validation operation - implemented by the derived TestScenario - that needs to run when OnExecuteAsync has completed.</returns>
        protected abstract Task ValidateScenarioAtExitAsync(CancellationToken token);

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <returns>This API supports the Service Fabric platform and is not meant to be called from your code</returns>
        protected TimeSpan GetElapsedTime()
        {
            return this.stopwatch.Elapsed;
        }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="progressReport">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        protected void ReportProgress(string progressReport)
        {
            var handler = this.ProgressChanged;
            if (handler != null)
            {
                int progress = this.ScenarioParameters.TimeToRun == TimeSpan.MaxValue ?
                    -1 : (int)(this.GetElapsedTime().TotalSeconds * 100 / this.ScenarioParameters.TimeToRun.TotalSeconds);

                // Even though TimeToRun has been passed the test might still continue running to complete the final set of faults/validation and cleanup
                // In this case it looks weird to report progress as greater than 100% hence limiting the max progress to 100% here. 
                progress = Math.Min(progress, 100);

                handler(this, new ProgressChangedEventArgs(progress, progressReport));
            }
        }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="format">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        /// <param name="args">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        protected void ReportProgress(string format, params object[] args)
        {
            string report = StringHelper.Format(format, args);
            Log.WriteInfo(TraceType, report);

            var handler = this.ProgressChanged;
            if (handler != null)
            {
                int progress = this.ScenarioParameters.TimeToRun == TimeSpan.MaxValue ?
                    -1 : (int)(this.GetElapsedTime().TotalSeconds * 100 / this.ScenarioParameters.TimeToRun.TotalSeconds);

                handler(this, new ProgressChangedEventArgs(progress, report));
            }
        }

        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        /// <param name="t">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        /// <param name="actionId">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        /// <param name="actionName">This API supports the Service Fabric platform and is not meant to be called from your code</param>
        protected void HandleTaskComplete(Task t, string actionId, string actionName)
        {
            Log.WriteInfoWithId("Action '{0}' completed with status: '{1}'.", actionName, t.Status.ToString());
            if (t.Exception != null)
            {
                t.Exception.Handle(ex =>
                {
                    var exceptionToTrace = ex is AggregateException ? (ex as AggregateException).Flatten().InnerException : ex;

                    if (exceptionToTrace is FabricValidationException)
                    {
                        // Return false since FabricValidationException should not be handled
                        return false;
                    }
                    else
                    {
                        this.ReportProgress(
                            "Action {0}. Failed with exception {1}, HResult {2}. Ignoring since state might have changed during action",
                            actionName,
                            exceptionToTrace.GetType().Name,
                            exceptionToTrace.HResult);

                        Log.WriteInfoWithId(
                            TraceType,
                            actionId,
                            "Action {0}. Failed with exception {1}.",
                            actionName,
                            exceptionToTrace);

                        // Exception will be propagated if total consecutive failures greater than MaxConsecutiveFailures
                        return this.RecordError(); // throwing the exception if false
                    }
                });
            }
            else
            {
                this.RecordSuccess();
            }
        }

        private bool RecordError()
        {
            int result = Interlocked.Increment(ref this.consecutiveFailures);
            return result < MaxConsecutiveFailures;
        }

        private void RecordSuccess()
        {
            Interlocked.Exchange(ref this.consecutiveFailures, 0);
        }
    }
}