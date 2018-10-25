// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca;
    using TelemetryAggregation;
    using Tools.EtlReader;

    public static class TelemetryUtility
    {
        public static bool ReadFieldsFromEventRecord(
            EventRecord eventRecord,
            EventDefinition eventDefinition,
            TraceAggregationConfig traceAggregationConfig,
            out List<KeyValuePair<string, double>> numericalFields,
            out List<KeyValuePair<string, string>> stringFields,
            out string diffFieldValueString,
            string logSourceId)
        {
            numericalFields = new List<KeyValuePair<string, double>>();
            stringFields = new List<KeyValuePair<string, string>>();
            diffFieldValueString = string.Empty;

            // Figure out how many fields the event has
            int fieldCount = eventDefinition.Fields.Count;
            if (0 == fieldCount)
            {
                Utility.TraceSource.WriteError(
                    logSourceId,
                    "No fields found in event of type {0}.{1}.",
                    eventDefinition.TaskName,
                    eventDefinition.EventName);
                return false;
            }

            // Get the field names and values
            try
            {
                ApplicationDataReader reader = new ApplicationDataReader(
                                                       eventRecord.UserData,
                                                       eventRecord.UserDataLength);

                foreach (FieldDefinition fieldDef in eventDefinition.Fields)
                {
                    var fieldValue = TelemetryUtility.GetEtwEventRecordValue(reader, fieldDef, logSourceId);
                    if (traceAggregationConfig.FieldsAndAggregationConfigs.ContainsKey(fieldDef.Name))
                    {
                        if (null != fieldValue)
                        {
                            if (fieldValue is string)
                            {
                                stringFields.Add(new KeyValuePair<string, string>(fieldDef.Name, (string)fieldValue));
                            }
                            else
                            {
                                if (fieldValue is double)
                                {
                                    numericalFields.Add(new KeyValuePair<string, double>(fieldDef.Name, (double)fieldValue));
                                }
                                else
                                {
                                    Utility.TraceSource.WriteError(
                                        logSourceId,
                                        "Unexpected field value type read. TaskName : {0}, EventName : {1}, FieldName : {2}, FieldType : {3}",
                                        eventDefinition.TaskName,
                                        eventDefinition.EventName,
                                        fieldDef.Name,
                                        fieldDef.Type.ToString());
                                }
                            }
                        }
                    }

                    // look if field is the differentiator field
                    if (traceAggregationConfig.DifferentiatorFieldName == fieldDef.Name)
                    {
                        if (null != fieldValue)
                        {
                            diffFieldValueString = fieldValue.ToString();
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Utility.TraceSource.WriteError(
                    logSourceId,
                    "Failed to get all the fields of event of type {0}.{1}. Exception info: {2}.",
                    eventDefinition.TaskName,
                    eventDefinition.EventName,
                    e);
                return false;
            }

            return true;
        }

        // extension method for telemetryCollection to aggregate from EventRecord
        public static void AggregateEventRecord(this TelemetryCollection telemetryCollection, EventRecord eventRecord, EventDefinition eventDefinition, TraceAggregationConfig traceAggregationConfig, string logSourceId)
        {
            // Reading data from eventRecord white-listed fields
            List<KeyValuePair<string, double>> numericalFields;
            List<KeyValuePair<string, string>> stringFields;
            string diffFieldValueString;
            TelemetryUtility.ReadFieldsFromEventRecord(eventRecord, eventDefinition, traceAggregationConfig, out numericalFields, out stringFields, out diffFieldValueString, logSourceId);

            telemetryCollection.Aggregate(traceAggregationConfig, diffFieldValueString, numericalFields, stringFields, DateTime.FromFileTimeUtc(eventRecord.EventHeader.TimeStamp));
        }

        // extension method to include EnvironmentTelemetry in TelemetryCollection
        public static void Aggregate(this TelemetryCollection telemetryCollection, EnvironmentTelemetry environmentTelemetry)
        {
            telemetryCollection.Aggregate(
                environmentTelemetry.TraceAggregationCfg,
                string.Empty,
                environmentTelemetry.Metrics,
                environmentTelemetry.Properties,
                DateTime.UtcNow);
        }

        public static void Aggregate(this TelemetryCollection telemetryCollection, TelemetryPerformanceInstrumentation telPerfInstrumentation)
        {
            telemetryCollection.Aggregate(
                telPerfInstrumentation.TraceAggregationCfg,
                string.Empty,
                telPerfInstrumentation.Metrics,
                new Dictionary<string, string>(),
                DateTime.UtcNow);

            // the stop watch has to reset everytime the aggregation happens so that the accumulation in sum represents the total time
            // this is necessary so that it supports loading the accumulation from the persisted value.
            telPerfInstrumentation.ProcessingEventsMeasuredTimeReset();
        }

        // returns an object that would be either a string or a double, or null if failed
        private static object GetEtwEventRecordValue(ApplicationDataReader reader, FieldDefinition fieldDef, string logSourceId)
        {
            object value = null;

            switch (fieldDef.Type)
            {
                case FieldDefinition.FieldType.UnicodeString:
                    value = reader.ReadUnicodeString();
                    break;

                case FieldDefinition.FieldType.AnsiString:
                    value = reader.ReadAnsiString();
                    break;

                case FieldDefinition.FieldType.Boolean:
                    value = reader.ReadBoolean() ? "true" : "false";
                    break;

                case FieldDefinition.FieldType.UInt8:
                    value = (double)reader.ReadUInt8();
                    break;

                case FieldDefinition.FieldType.UInt16:
                    value = (double)reader.ReadUInt16();
                    break;

                case FieldDefinition.FieldType.UInt32:
                    value = (double)reader.ReadUInt32();
                    break;

                case FieldDefinition.FieldType.UInt64:
                    value = (double)reader.ReadUInt64();
                    break;

                case FieldDefinition.FieldType.Int8:
                    value = (double)reader.ReadInt8();
                    break;

                case FieldDefinition.FieldType.Int16:
                    value = (double)reader.ReadInt16();
                    break;

                case FieldDefinition.FieldType.Int32:
                    value = (double)reader.ReadInt32();
                    break;

                case FieldDefinition.FieldType.Int64:
                    value = (double)reader.ReadInt64();
                    break;

                case FieldDefinition.FieldType.HexInt32:
                    value = (double)reader.ReadUInt32();
                    break;

                case FieldDefinition.FieldType.HexInt64:
                    value = (double)reader.ReadUInt64();
                    break;

                case FieldDefinition.FieldType.Float:
                    value = (double)reader.ReadFloat();
                    break;

                case FieldDefinition.FieldType.Double:
                    value = (double)reader.ReadDouble();
                    break;

                case FieldDefinition.FieldType.DateTime:
                    value = reader.ReadFileTime().ToString();
                    break;

                case FieldDefinition.FieldType.Guid:
                    value = reader.ReadGuid().ToString();
                    break;

                default:
                    Utility.TraceSource.WriteError(
                        logSourceId,
                        "Unsupported field of type {0}.",
                        fieldDef.Type);
                    break;
            }

            return value;
        }
    }
}
