// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Linq;
    using Newtonsoft.Json;
    using Tools.EtlReader;

    /// <summary>
    /// Consumers which can push data into Syslog
    /// </summary>
    internal class SyslogConsumer : IDcaConsumer, IEtlFileSink
    {
        // This is the Default Identity that we use while logging to syslog.
        private const string DefaultLogIdentity = "ServiceFabric";

        private const string TimeStampFieldName = "TimeStamp";

        private const string TimeStampFormat = "yyyy-MM-ddTHH:mm:ss.fffZ";

        // This is the Default log facility that we will use while logging.
        private const SyslogFacility DefaultLogFacility = SyslogFacility.Local0;

        private const bool DefaultEnabledStateValue = false;

        private LocalSysLogger sysLogWriter;

        // Whether or not the object has been disposed
        private bool disposed;

        // ETW manifest cache. Used for decoding ETW events.
        private ManifestCache manifestCache;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        private string logIdentity;

        private SyslogFacility syslogFacility;

        private bool isEnabled;

        public SyslogConsumer(ConsumerInitializationParameters initParam)
        {
            this.logSourceId = String.Concat(initParam.ApplicationInstanceId, "_", initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);

            this.PopulateSettings(initParam);

            if (this.isEnabled)
            {
                this.traceSource.WriteInfo(this.logSourceId, "SyslogConsumer is Enabled");
                this.sysLogWriter = new LocalSysLogger(this.logIdentity, this.syslogFacility);
            }
        }

        public bool NeedsDecodedEvent
        {
            get
            {
                // We want the raw events. We'll decode it ourselves.
                return false;
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            if (this.sysLogWriter != null)
            {
                this.sysLogWriter.Dispose();
            }

            GC.SuppressFinalize(this);

            this.disposed = true;
        }

        public void FlushData()
        {
        }

        public object GetDataSink()
        {
            // Return a sink if we are enabled, otherwise return null.
            return this.isEnabled ? (IEtlFileSink)this : null;
        }

        public void OnEtwEventsAvailable(IEnumerable<EventRecord> etwEvents)
        {
            if (!this.isEnabled)
            {
                return;
            }

            foreach (var oneEvent in etwEvents)
            {
                // Currently, we only support processing Platform Events.
                if (!IsPlatformEvent(oneEvent))
                {
                    continue;
                }

                this.ProcessEvent(oneEvent);
            }
        }

        public void SetEtwManifestCache(ManifestCache manifestCache)
        {
            this.manifestCache = manifestCache;
        }

        public void OnEtwEventsAvailable(IEnumerable<DecodedEtwEvent> etwEvents)
        {
        }

        public void OnEtwEventProcessingPeriodStart()
        {
        }

        public void OnEtwEventProcessingPeriodStop()
        {
        }

        private void PopulateSettings(ConsumerInitializationParameters initParam)
        {
            var configReader = new ConfigReader(initParam.ApplicationInstanceId);

            this.isEnabled = configReader.GetUnencryptedConfigValue(
                                                   initParam.SectionName,
                                                   SyslogConstant.EnabledParamName,
                                                   DefaultEnabledStateValue);

            this.logIdentity = configReader.GetUnencryptedConfigValue(
                                                   initParam.SectionName,
                                                   SyslogConstant.IdentityParamName,
                                                   DefaultLogIdentity);

            var facilityString = configReader.GetUnencryptedConfigValue(
                                                   initParam.SectionName,
                                                   SyslogConstant.FacilityParamName,
                                                   DefaultLogFacility.ToString());

            // It is bound to succeed since the param is already validated to be the right enum type in validator.
            this.syslogFacility = (SyslogFacility)Enum.Parse(typeof(SyslogFacility), facilityString, true);

            this.traceSource.WriteInfo(
                this.logSourceId,
                string.Format("SyslogConsumer Populated with these settings. Enabled: {0}, Identity: {1}, Facility: {2}", this.isEnabled, this.logIdentity, this.syslogFacility));
        }

        private void ProcessEvent(EventRecord oneEventRecord)
        {
            // Get the event definition
            var eventDefinition = this.manifestCache.GetEventDefinition(oneEventRecord);

            var keyValuesMap = this.ConvertEventToKvp(oneEventRecord, eventDefinition);

            if (keyValuesMap == null || !keyValuesMap.Any())
            {
                return;
            }

            // serialize the data into a json string
            var message = JsonConvert.SerializeObject(keyValuesMap);

            // Push to syslog.
            this.sysLogWriter.LogMessage(message, ConvertToSyslogSeverity((EventLevel)oneEventRecord.EventHeader.EventDescriptor.Level));
        }

        private Dictionary<string, string> ConvertEventToKvp(EventRecord eventRecord, EventDefinition eventDefinition)
        {
            var keyValueDictionary = new Dictionary<string, string>();

            try
            {
                var dataReader = new ApplicationDataReader(eventRecord.UserData, eventRecord.UserDataLength);

                foreach (FieldDefinition fieldDef in eventDefinition.Fields)
                {
                    string propertyValue = null;
                    switch (fieldDef.Type)
                    {
                        case FieldDefinition.FieldType.UnicodeString:
                            {
                                propertyValue = dataReader.ReadUnicodeString();
                                break;
                            }

                        case FieldDefinition.FieldType.AnsiString:
                            {
                                propertyValue = dataReader.ReadAnsiString();
                                break;
                            }

                        case FieldDefinition.FieldType.Boolean:
                            {
                                propertyValue = dataReader.ReadBoolean().ToString();
                                break;
                            }

                        case FieldDefinition.FieldType.UInt8:
                            {
                                propertyValue = dataReader.ReadUInt8().ToString();
                                break;
                            }

                        case FieldDefinition.FieldType.UInt16:
                            {
                                propertyValue = dataReader.ReadUInt16().ToString();
                                break;
                            }

                        case FieldDefinition.FieldType.UInt32:
                            {
                                unchecked
                                {
                                    propertyValue = dataReader.ReadUInt32().ToString();
                                }
                                break;
                            }

                        case FieldDefinition.FieldType.UInt64:
                            {
                                unchecked
                                {
                                    propertyValue = dataReader.ReadUInt64().ToString();
                                }
                                break;
                            }

                        case FieldDefinition.FieldType.Int8:
                            {
                                propertyValue = dataReader.ReadInt8().ToString();
                                break;
                            }

                        case FieldDefinition.FieldType.Int16:
                            {
                                propertyValue = dataReader.ReadInt16().ToString();
                                break;
                            }

                        case FieldDefinition.FieldType.Int32:
                            {
                                propertyValue = dataReader.ReadInt32().ToString();
                                break;
                            }

                        case FieldDefinition.FieldType.Int64:
                            {
                                propertyValue = dataReader.ReadInt64().ToString();
                                break;
                            }

                        case FieldDefinition.FieldType.HexInt32:
                            {
                                unchecked
                                {
                                    propertyValue = dataReader.ReadUInt32().ToString();
                                }
                                break;
                            }

                        case FieldDefinition.FieldType.HexInt64:
                            {
                                unchecked
                                {
                                    propertyValue = dataReader.ReadUInt64().ToString();
                                }
                                break;
                            }

                        case FieldDefinition.FieldType.Float:
                            {
                                propertyValue = dataReader.ReadFloat().ToString();
                                break;
                            }

                        case FieldDefinition.FieldType.Double:
                            {
                                propertyValue = dataReader.ReadDouble().ToString();
                                break;
                            }

                        case FieldDefinition.FieldType.DateTime:
                            {
                                propertyValue = dataReader.ReadFileTime().ToString("yyyy-dd-M--HH-mm-ss");
                                break;
                            }

                        case FieldDefinition.FieldType.Guid:
                            {
                                propertyValue = dataReader.ReadGuid().ToString();
                                break;
                            }

                        default:
                            {
                                this.traceSource.WriteError(
                                    logSourceId,
                                    "Event of type {0}.{1} has an unsupported field of type {2}.",
                                    eventDefinition.TaskName,
                                    eventDefinition.EventName,
                                    fieldDef.Type);
                                break;
                            }
                    }

                    if (propertyValue != null)
                    {
                        keyValueDictionary[fieldDef.Name] = propertyValue;
                    }
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    logSourceId,
                    "Failed to get all the fields of event of type {0}.{1}. Exception info: {2}.",
                    eventDefinition.TaskName,
                    eventDefinition.EventName,
                    e);

                return null;
            }

            // our Platform event can't have a field names TimeStamp today. Still keeping this check till we add a unit test for that.
            if (!keyValueDictionary.ContainsKey(TimeStampFieldName))
            {
                keyValueDictionary.Add(TimeStampFieldName, DateTime.FromFileTimeUtc(eventRecord.EventHeader.TimeStamp).ToString(TimeStampFormat));
            }

            return keyValueDictionary;
        }

        private static bool IsPlatformEvent(EventRecord eventRecord)
        {
            if ((eventRecord.EventHeader.EventDescriptor.Keyword & TraceEvent.OperationalChannelKeywordMask) != 0)
            {
                return true;
            }

            return false;
        }

        private static SyslogSeverity ConvertToSyslogSeverity(EventLevel eventLevel)
        {
            if (eventLevel == EventLevel.Critical)
            {
                return SyslogSeverity.Critical;
            }

            if (eventLevel == EventLevel.Error)
            {
                return SyslogSeverity.Error;
            }

            if (eventLevel == EventLevel.Warning)
            {
                return SyslogSeverity.Warning;
            }

            // I don't think we are using LogAlways, but if we are, then mapping it to informational.
            if (eventLevel == EventLevel.Informational || eventLevel == EventLevel.LogAlways)
            {
                return SyslogSeverity.Informational;
            }

            if (eventLevel == EventLevel.Verbose)
            {
                return SyslogSeverity.Debug;
            }

            return SyslogSeverity.Notice;
        }
    }
}