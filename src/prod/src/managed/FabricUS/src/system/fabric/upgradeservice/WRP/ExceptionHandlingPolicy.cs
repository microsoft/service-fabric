// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using Common;
    using Health;
    using System;
    using Text;
    using Threading.Tasks;

    internal interface IExceptionHandlingPolicy
    {
        /// <summary>
        /// Gets the task name for which the policy apply.
        /// </summary>
        string TaskName { get; }

        /// <summary>
        /// Gets the health property name on which we report the health event.
        /// </summary>
        string HealthProperty { get; }

        /// <summary>
        /// <para>Gets the cotinuous failure count.</para>
        /// <para>
        /// Value is reset to 0 upon ReportSuccess()
        /// ReportError() increases the value by 1.
        /// </para>
        /// </summary>
        int ContinuousFailureCount { get; }

        /// <summary>
        /// Exception handler for the exception encountered. 
        /// </summary>
        /// <param name="ex">exception</param>
        /// <param name="canceled">flag to specify if the exception happend on a canceled thread</param>
        void ReportError(Exception ex, bool canceled);

        /// <summary>
        /// Resets the health event associated with health property to success.
        /// Resets the continuous failure count to 0.
        /// </summary>
        void ReportSuccess();
    }

    internal class ExceptionHandlingPolicy : IExceptionHandlingPolicy
    {
        private TraceType TraceType;
        private readonly IConfigStore configStore;
        private readonly string configSectionName;
        private readonly IStatefulServicePartition partition;        

        internal ExceptionHandlingPolicy(
            string healthProperty,
            string taskName,
            IConfigStore configStore,
            string configSectionName,
            IStatefulServicePartition partition)
        {
            healthProperty.ThrowIfNullOrWhiteSpace(nameof(healthProperty));
            taskName.ThrowIfNullOrWhiteSpace(nameof(taskName));
            configStore.ThrowIfNull(nameof(configStore));
            configSectionName.ThrowIfNullOrWhiteSpace(nameof(configSectionName));
            partition.ThrowIfNull(nameof(partition));

            this.configStore = configStore;
            this.configSectionName = configSectionName;
            this.partition = partition;
            this.HealthProperty = healthProperty;
            this.TaskName = taskName;

            this.ContinuousFailureCount = 0;
            this.TraceType = new TraceType(healthProperty);
        }

        public string TaskName { get; private set; }
        public string HealthProperty { get; private set; }

        public int ContinuousFailureCount { get; private set; }

        public void ReportError(Exception ex, bool canceled)
        {
            this.ContinuousFailureCount++;

            this.HandleException(ContinuousFailureCount, this.HealthProperty, ex, canceled);
        }

        public void ReportSuccess()
        {
            this.ContinuousFailureCount = 0;

            HealthInformation successHealthInfo = new HealthInformation()
            {
                HealthState = HealthState.Ok,
                SourceId = Constants.PaasCoordinator.HealthSourceId,
                Property = Constants.PaasCoordinator.SFRPPollHealthProperty,
                Description = $"{this.TaskName} completed successfully."
            };

            ReportHealth(successHealthInfo);
        }

        private void HandleException(int continousFailureCount, string healthProperty, Exception exception, bool cancelled)
        {
            if (exception == null)
            {
                return;
            }

            bool isTerminalError = continousFailureCount >= configStore.Read(configSectionName, Constants.ConfigurationSection.ContinuousFailureFaultThreshold, 15);
            string exceptionDescription = string.Empty;
            var ae = exception as AggregateException;
            if (ae != null)
            {
                StringBuilder exceptionDescriptionBuilder = new StringBuilder("AggregateException encountered:\r\n");
                ae.Flatten().Handle(
                    innerException =>
                    {
                        exceptionDescriptionBuilder.AppendFormat("InnerException: {0}\r\n", innerException.ToString());
                        if (innerException is FabricObjectClosedException)
                        {
                            isTerminalError = true;
                        }

                        return true;
                    });

                exceptionDescription = exceptionDescriptionBuilder.ToString();
            }
            else
            {
                if (exception is FabricObjectClosedException)
                {
                    isTerminalError |= exception is FabricObjectClosedException;
                }

                exceptionDescription = string.Format("Exception encountered: {0}", exception.ToString());
            }

            Trace.WriteWarning(
                TraceType,
                "{0}: {1}\r\n CountinousFailureCount:{2}",
                this.TaskName,
                exceptionDescription,
                continousFailureCount);

            if (!isTerminalError && exception is TaskCanceledException && cancelled)
            {
                Trace.WriteInfo(
                    TraceType,
                    "{0}: {1}{2} was ignored since CancellationToken has cancelled, CountinousFailureCount:{3}",
                    this.TaskName,
                    exceptionDescription,
                    Environment.NewLine,
                    continousFailureCount);
                return;
            }

            int warningThreshold = configStore.Read(configSectionName, Constants.ConfigurationSection.ContinuousFailureWarningThreshold, 3);
            int errorThreshold = configStore.Read(configSectionName, Constants.ConfigurationSection.ContinuousFailureErrorThreshold, 5);
            if (continousFailureCount >= warningThreshold)
            {
                HealthState healthState = (continousFailureCount < errorThreshold)
                    ? HealthState.Warning
                    : HealthState.Error;
                string healthDescription = exceptionDescription.Length > 4095
                    ? exceptionDescription.Substring(0, 4095)
                    : exceptionDescription;
                HealthInformation healthInfo = new HealthInformation()
                {
                    HealthState = healthState,
                    SourceId = Constants.PaasCoordinator.HealthSourceId,
                    Property = healthProperty,
                    Description = healthDescription,
                    RemoveWhenExpired = true,
                    TimeToLive = configStore.ReadTimeSpan(
                        this.configSectionName,
                        Constants.ConfigurationSection.HealthReportTTLInSeconds,
                        TimeSpan.FromSeconds(600))
                };

                this.ReportHealth(healthInfo, isTerminalError);
            }

            if (isTerminalError)
            {
                Trace.WriteError(TraceType, "Reporting transient fault since terminal error was encountered.");
                try
                {
                    this.partition.ReportFault(FaultType.Transient);
                }
                catch (Exception e)
                {
                    Trace.WriteWarning(TraceType, "Exception when reporting health: {0}", e.ToString());
                }             
            }
        }

        private void ReportHealth(HealthInformation healthInfo, bool immediate = false)
        {
            try
            {
                var sendOptions = new HealthReportSendOptions()
                {
                    Immediate = immediate
                };

                this.partition.ReportPartitionHealth(healthInfo, sendOptions);
            }
            catch (Exception e)
            {
                // TODO: RDBug 11876054:FabricUS: Handle retryable exceptions for health status publishing
                Trace.WriteWarning(TraceType, "Exception when reporting health: {0}", e.ToString());
            }
        }
    }
}
