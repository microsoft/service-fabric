// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.IO;
    using Newtonsoft.Json;
    using TelemetryAggregation;

    // This class has the Dictionary containing all telemetry
    [JsonObject(MemberSerialization.Fields)]
    public class TelemetryCollection : IEnumerable<TraceAggregator>
    {
        private const string LogSourceId = "TelemetryCollection";

        // Dictionary containing the telemetry events
        private readonly Dictionary<TraceAggregationKey, TraceAggregator> telemetryAggregationCollection;

        public TelemetryCollection(TelemetryIdentifiers telemetryIdentifiers, int dailyNumTelPushes)
        {
            this.DailyNumberOfTelemetryPushes = dailyNumTelPushes;
            this.telemetryAggregationCollection = new Dictionary<TraceAggregationKey, TraceAggregator>();
            this.TelemetryIds = telemetryIdentifiers;
            this.Clear();
        }

        [JsonConstructor]
        private TelemetryCollection()
        {
        }

        public delegate void LogErrorDelegate(string type, string format, params object[] args);

        // returns the number of telemetry events collected and aggregated
        public int Count
        {
            get
            {
                return this.telemetryAggregationCollection.Count;
            }
        }

        public int DailyNumberOfTelemetryPushes { get; private set; }

        public int TelemetryBatchNumber { get; private set; }

        public TimeSpan PushTime { get; set; }

        public int TotalPushes { get; private set; }

        // cluster/node unique identifiers
        public TelemetryIdentifiers TelemetryIds { get; set; }

        public static TelemetryCollection LoadTelemetryCollection(string filePath, LogErrorDelegate logErrorDelegate)
        {
            TelemetryCollection telemetryCollection = null;

            if (File.Exists(filePath))
            {
                try
                {
                    using (StreamReader file = File.OpenText(filePath))
                    {
                        // using converter to properly serialize dictionary keys
                        string serializedObject = file.ReadToEnd();
                        JsonSerializerSettings settings = new JsonSerializerSettings() { ConstructorHandling = ConstructorHandling.AllowNonPublicDefaultConstructor };
                        settings.ContractResolver = new DictionaryAsArrayResolver();

                        telemetryCollection = JsonConvert.DeserializeObject<TelemetryCollection>(serializedObject, settings);
                    }
                }
                catch (JsonException e)
                {
                    logErrorDelegate(LogSourceId, "Error deserializing file :{0} - Exception {1}", filePath, e);
                }
                catch (Exception e)
                {
                    logErrorDelegate(LogSourceId, "Error loading persisted telemetry from file: {0} - Exception {1}", filePath, e);
                }
            }

            return telemetryCollection;
        }

        // tries to read the persisted serialization. If fails returns a new instance (empty)
        public static TelemetryCollection LoadOrCreateTelemetryCollection(string filePath, TelemetryIdentifiers telemetryIdentifier, int dailyNumberOfTelemetryPushes, out bool telCollectionloadedFromDisk, LogErrorDelegate logErrorDelegate)
        {
            TelemetryCollection telemetryCollection = TelemetryCollection.LoadTelemetryCollection(filePath, logErrorDelegate);

            // if loaded from file and configuration on the push rate not changed add telemetryIdentifier, otherwise create a new one
            if (telemetryCollection != null && telemetryCollection.DailyNumberOfTelemetryPushes == dailyNumberOfTelemetryPushes)
            {
                telemetryCollection.TelemetryIds = telemetryIdentifier;
                telCollectionloadedFromDisk = true;
            }
            else
            {
                telemetryCollection = new TelemetryCollection(telemetryIdentifier, dailyNumberOfTelemetryPushes);
                telCollectionloadedFromDisk = false;
            }

            return telemetryCollection;
        }

        public void Aggregate(TraceAggregationConfig traceAggregationConfig, string diffFieldValueString, IEnumerable<KeyValuePair<string, double>> numericalFields, IEnumerable<KeyValuePair<string, string>> stringFields, DateTime timestamp)
        {
            TraceAggregator traceAggregator;
            TraceAggregationKey traceAggregationKey = new TraceAggregationKey(traceAggregationConfig, diffFieldValueString);
            if (!this.telemetryAggregationCollection.TryGetValue(traceAggregationKey, out traceAggregator))
            {
                // initializes the aggregator for the first time the trace is seen
                traceAggregator = new TraceAggregator(traceAggregationConfig);

                // include the aggregator to the collection
                this.telemetryAggregationCollection[traceAggregationKey] = traceAggregator;
            }

            // aggregating field values
            traceAggregator.Aggregate(numericalFields);
            traceAggregator.Aggregate(stringFields);
            traceAggregator.TraceTimestamp = timestamp;
        }

        // save this instance of telemetrycollection to a file
        // this method first saves the data in a tmp file
        // and then replaces the existing one (if it exists) to reduce the chances of corrupting the file.
        public void SaveToFile(string filePath, LogErrorDelegate logErrorDelegate)
        {
            try
            {
                string filePathTemp = string.Format("{0}.tmp", filePath);

                // saving current collection in a json serialization format on temporary file
                using (StreamWriter file = File.CreateText(filePathTemp))
                {
                    // using converter to properly serialize dictionary keys
                    JsonSerializerSettings settings = new JsonSerializerSettings();
                    settings.ContractResolver = new DictionaryAsArrayResolver();
                    string json = JsonConvert.SerializeObject(this, settings);

                    // writing json to file
                    file.Write(json);
                }

                // create or replace file
                File.Copy(filePathTemp, filePath, true);

                // remove temporary file
                File.Delete(filePathTemp);
            }
            catch (Exception e)
            {
                logErrorDelegate(LogSourceId, "Error when trying to save telemetry aggregation to file: {0} - Exception: {1}", filePath, e);
            }
        }

        public void Clear()
        {
            this.telemetryAggregationCollection.Clear();
            this.TelemetryBatchNumber = 1;
        }

        public void IncrementTelemetryBatchNumber()
        {
            this.TelemetryBatchNumber++;
            this.TotalPushes++;
            this.ClearIfAllDailyTelemetryBatchesSent();
        }

        public IEnumerator<TraceAggregator> GetEnumerator()
        {
            return this.telemetryAggregationCollection.Values.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }

        private void ClearIfAllDailyTelemetryBatchesSent()
        {
            if (this.TelemetryBatchNumber > this.DailyNumberOfTelemetryPushes)
            {
                this.Clear();
            }
        }
    }
}
