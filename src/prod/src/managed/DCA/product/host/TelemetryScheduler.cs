// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Fabric.Dca;

    internal class TelemetryScheduler
    {
        public const double DefaultTelemetryInitialSendDelayInHours = 0.5;
        public static readonly int DefaultTelemetryDailyPushes = 3;

        public TelemetryScheduler(IConfigReader configReader)
        {
            this.SetupFromSettingsFile(configReader, DateTime.Now.ToLocalTime().TimeOfDay);
        }

        public int DailyPushes { get; private set; }

        public TimeSpan PushTime { get; private set; }

        public TimeSpan SendDelay { get; private set; }

        public bool IsTestOnlyTelemetryPushTimeSet { get; private set; }

        public TimeSpan SendInterval
        {
            get
            {
                return TimeSpan.FromHours(24.0 / this.DailyPushes);
            }
        }

        public TimeSpan InitialSendDelay
        {
            get
            {
                return TimeSpan.FromHours(Math.Min(DefaultTelemetryInitialSendDelayInHours, this.SendInterval.TotalHours));
            }
        }

        public void SetupFromSettingsFile(IConfigReader configReader, TimeSpan currentTimeOfTheDay)
        {
            // reading the number of daily pushes
            this.DailyPushes = TelemetryScheduler.ReadTelemetryDailyPushes(configReader);

            // parsing time from telemetryPushTimeParamValue
            var telemetryPushTimeParamValue = configReader.GetUnencryptedConfigValue(
                          ConfigReader.DiagnosticsSectionName,
                          ConfigReader.TestOnlyTelemetryPushTimeParamName,
                          string.Empty);

            TimeSpan telemetryPushTime;
            if (!TimeSpan.TryParse(telemetryPushTimeParamValue, out telemetryPushTime))
            {
                // the first batch of telemetry is sent after 30min that the cluster came up
                // the time of the push time will be than be 30min + 24h - (24/dailyPushes)
                TimeSpan totalTimeToPushTime = this.InitialSendDelay.Add(TimeSpan.FromHours(24.0 - this.SendInterval.TotalHours));
                this.PushTime = (currentTimeOfTheDay.Add(totalTimeToPushTime) < TimeSpan.FromDays(1)) ? currentTimeOfTheDay.Add(totalTimeToPushTime) : currentTimeOfTheDay.Add(totalTimeToPushTime).Subtract(TimeSpan.FromDays(1));
                this.IsTestOnlyTelemetryPushTimeSet = false;
            }
            else
            {
                this.PushTime = telemetryPushTime;
                this.IsTestOnlyTelemetryPushTimeSet = true;
            }

            this.UpdateScheduler(false, false, TimeSpan.FromHours(0.0), currentTimeOfTheDay);
        }

        public void UpdateScheduler(bool persisted, bool telemetrySentAtLeasOnce, TimeSpan persistedLastBatchPushTime, TimeSpan currentTimeOfTheDay)
        {
            this.SendDelay = this.UpdateSendDelayPushTime(persisted, telemetrySentAtLeasOnce, persistedLastBatchPushTime, currentTimeOfTheDay);
        }

        private static int ReadTelemetryDailyPushes(IConfigReader configReader)
        {
            var telemetryDailyPushesParam = configReader.GetUnencryptedConfigValue(
                ConfigReader.DiagnosticsSectionName,
                ConfigReader.TestOnlyTelemetryDailyPushesParamName,
                string.Empty);

            int telemetryDailyPushes;
            if (!int.TryParse(telemetryDailyPushesParam, out telemetryDailyPushes) || telemetryDailyPushes <= 0)
            {
                if (telemetryDailyPushesParam != string.Empty)
                {
                    throw new ArgumentException(
                        string.Format(
                            "Invalid value of '{0}' for parameter '{1}', under '{2}' section. Expecting an integer value greater than zero.",
                            telemetryDailyPushesParam,
                            ConfigReader.DiagnosticsSectionName,
                            ConfigReader.TestOnlyTelemetryDailyPushesParamName),
                        ConfigReader.TestOnlyTelemetryDailyPushesParamName);
                }

                return TelemetryScheduler.DefaultTelemetryDailyPushes;
            }

            return telemetryDailyPushes;
        }

        private TimeSpan UpdateSendDelayPushTime(bool persisted, bool telemetrySentAtLeastOnce, TimeSpan persistedLastBatchPushTime, TimeSpan currentTimeOfTheDay)
        {
            if (!persisted)
            {
                return this.ComputeSendDelayIgnoringPersistedPushTime(currentTimeOfTheDay);
            }

            // Ignores the persisted time to send if no telemetry was sent yet.
            // Also allows resetting the push time by modifying the Settings.xml file and 
            // modifying or adding TestOnlyTelemetryPushTime parameter and restarting DCA.
            if (!telemetrySentAtLeastOnce ||
               (this.IsTestOnlyTelemetryPushTimeSet && (persistedLastBatchPushTime != this.PushTime)))
            {
                return this.ComputeSendDelayIgnoringPersistedPushTime(currentTimeOfTheDay);
            }

            // if persisted and sent at least once and not TestOnlyTelemetryPush not explicit provided in the settings file
            this.PushTime = persistedLastBatchPushTime;

            return this.ComputeDelayFromLastPushTime(this.DailyPushes, persistedLastBatchPushTime, currentTimeOfTheDay);
        }

        private TimeSpan ComputeSendDelayIgnoringPersistedPushTime(TimeSpan currentTimeOfTheDay)
        {
            // if testonly push time is set the first batch is sent only at the specified time
            if (this.IsTestOnlyTelemetryPushTimeSet)
            {
                return this.PushTime > currentTimeOfTheDay ? this.PushTime.Subtract(currentTimeOfTheDay) : this.PushTime.Add(TimeSpan.FromDays(1).Subtract(currentTimeOfTheDay));
            }

            // send after the speficied initial delay
            return this.InitialSendDelay;
        }

        private TimeSpan ComputeDelayFromLastPushTime(int telemetryDailyPushes, TimeSpan pushTime, TimeSpan currentTimeOfTheDay)
        {
            TimeSpan timeToLastBatch = pushTime.Subtract(currentTimeOfTheDay);

            if (currentTimeOfTheDay > pushTime)
            {
                timeToLastBatch = TimeSpan.FromDays(1).Subtract(currentTimeOfTheDay).Add(pushTime);
            }

            double pushesRemaining = timeToLastBatch.TotalHours / this.SendInterval.TotalHours;

            return TimeSpan.FromHours((pushesRemaining - ((int)pushesRemaining)) * this.SendInterval.TotalHours);
        }
    }
}