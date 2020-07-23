// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Linq;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Reflection;
    using System.Text.RegularExpressions;
    using System.Xml;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;
    using Tools.EtlReader;
    using System.IO;

    internal abstract class AzureTableSelectiveEventUploader : IDcaConsumer, IEtlFileSink
    {
        // Constants
        private const string IdFieldName = "id";
        private const string DeletionEventIndicator = "DEL";
        private const string VersionFieldName = "dca_version";
        private const string InstanceFieldName = "dca_instance";
        private const string TaskNameProperty = "TaskName";
        private const string EventTypeProperty = "EventType";
        private const string EventVersionProperty = "EventVersion";
        internal const string StatusEntityRowKey = "FFFFFFFFFFFFFFFFFFFF";
        private const string OldLogDeletionTimerIdSuffix = "_OldEntityDeletionTimer";
        private const string TableInfoDelimiter = "__";
        private const char EventInfoDelimiter = '_';
        private const string DeploymentIdTableShortName = "DeploymentIds";
        private const int MaxAzureRuntimeDeploymentIDLength = 16;
        private const int ReverseChronologicalRowKeyFlag = unchecked((int) 0x80000000);

        // For some of the events, the field to be used as the partition key 
        // can contain characters that are not allowed in Azure partition keys.
        // For such events, we replace the disallowed character with an alternate.
        private static readonly Dictionary<string, CharacterReplacement> CharReplaceMap = new Dictionary<string, CharacterReplacement>()
        {
           { "Services", new CharacterReplacement() {Original = '/', Substitute = '$'} },
           { "Applications", new CharacterReplacement() {Original = '/', Substitute = '$'} },
        };

        // Regular expression describing valid table names
        private static readonly Regex RegEx = new Regex(AzureConstants.AzureTableNameRegularExpression);

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Consumer initialization parameters
        private readonly ConsumerInitializationParameters initParam;

        // Configuration reader
        private readonly ConfigReader configReader;

        // Object that implements miscellaneous utility functions
        private readonly AzureUtility azureUtility;

        // List of short names for the tables we'll be uploading events to
        private readonly HashSet<string> tableShortNames;

        // Timer to delete old logs
        private readonly DcaTimer oldLogDeletionTimer;

        // List of tables we'll be uploading events to
        private readonly HashSet<string> tableNames;

        // The deployment ID that we use as part of the table name
        private string effectiveDeploymentId;

        // Name of the deployment ID table
        private string deploymentIdTableName;

        // ETW manifest cache. Used for decoding ETW events.
        private ManifestCache manifestCache;

        // Information that helps determine whether or not we 
        // should delete old logs
        //
        // We do not want the DCA on every node in the cluster
        // to be deleting old logs from tables. So we make the
        // DCA on the FMM node do it. 
        //
        // This is only an optimization to prevent all nodes from 
        // duplicating the deletion work. So it doesn't have to be 
        // 100% accurate. If the FMM were to move from one node to
        // another, it is possible that for a particular deletion
        // pass, both nodes attempt the deletion because they both
        // think that they are the FMM node. But this should be a 
        // transient condition and eventually one of the nodes would
        // "realize" that it is no longer the FMM node.
        private DateTime fmmEventTimestampSnapshot;

        // Settings for uploading data to the table store.
        private TableUploadSettings tableUploadSettings;

        // Whether or not we are in the process of stopping
        private bool stopping;

        // Whether or not the object has been disposed
        private bool disposed;

        protected AzureTableSelectiveEventUploader(ConsumerInitializationParameters initParam)
        {
            // Initialization
            this.stopping = false;
            this.initParam = initParam;
            this.logSourceId = String.Concat(this.initParam.ApplicationInstanceId, "_", this.initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.configReader = new ConfigReader(initParam.ApplicationInstanceId);
            this.azureUtility = new AzureUtility(this.traceSource, this.logSourceId);
            this.effectiveDeploymentId = String.Empty;
            this.tableShortNames = new HashSet<string>();
            this.tableNames = new HashSet<string>();
            this.fmmEventTimestampSnapshot = DateTime.MinValue;

            // Read table-specific settings from settings.xml
            GetSettings();
            if (false == this.tableUploadSettings.Enabled)
            {
                // Upload to Azure table storage is not enabled, so return immediately
                return;
            }

            // Get the table names
            GetTableNames();

            // Create the tables
            try
            {
                CreateTables();
            }
            catch (Exception e)
            {
                const string Message =
                    "Due to an error in table creation ETW traces will not be uploaded to Azure table storage.";
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    Message);
                throw new InvalidOperationException(Message, e);
            }

            // Create a timer to delete old logs
            string timerId = String.Concat(
                                    this.logSourceId,
                                    OldLogDeletionTimerIdSuffix);
            this.oldLogDeletionTimer = new DcaTimer(
                                                timerId,
                                                this.DeleteOldLogsHandler,
                                                (this.tableUploadSettings.EntityDeletionAge < TimeSpan.FromDays(1)) ?
                                                    AzureConstants.OldLogDeletionIntervalForTest :
                                                    AzureConstants.OldLogDeletionInterval);
            this.oldLogDeletionTimer.Start();
        }

        /// <summary>
        /// Determines if the event is required to be uploaded, and so its corresponding table is recorded.
        /// </summary>
        /// <returns></returns>
        protected abstract bool IsEventConsidered(string providerName, XmlElement eventDefinition);

#if DotNetCoreClrLinux
        protected abstract bool IsEventInThisTable(EventRecord eventRecord);
#endif

        // The constituent parts of event name
        private enum TableInfoParts
        {
            Table,
            DeletionEvent
        }

        public void FlushData()
        {
        }

        private void GetTableShortNameForEvent(XmlElement evt, string providerName, string manifestFile)
        {
            string eventId = evt.GetAttribute("value");
            try
            {
                if (this.IsEventConsidered(providerName, evt))
                {
                    string symbol = evt.GetAttribute("symbol");
                    if (String.IsNullOrEmpty(symbol))
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Unable to find attribute 'symbol' for event with id {0}, provider {1} in ETW manifest {2}.",
                            eventId,
                            providerName,
                            manifestFile);
                        return;
                    }

                    int eventNameIndex = symbol.LastIndexOf(EventInfoDelimiter);
                    if (-1 != eventNameIndex)
                    {
                        symbol = symbol.Remove(eventNameIndex);
                    }
                    else
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "The value {0} of attribute 'symbol' for event with id {1}, provider {2}, ETW manifest {3} is not in the expected format.",
                            symbol,
                            eventId,
                            providerName,
                            manifestFile);
                        return;
                    }

                    int index = symbol.IndexOf(TableInfoDelimiter);
                    if (index > 0 && (index + TableInfoDelimiter.Length) < symbol.Length)
                    {
                        symbol = symbol.Substring(index + TableInfoDelimiter.Length);
                    }
                    else
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Table information not available for event with symbol {0}, event id {1}, provider {2} in ETW manifest {3}.",
                            symbol,
                            eventId,
                            providerName,
                            manifestFile);
                        return;
                    }

                    string[] tableNameParts = symbol.Split(EventInfoDelimiter);
                    if ((int)TableInfoParts.Table >= tableNameParts.Length)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "The value {0} of attribute 'symbol' for event with id {1}, provider {2}, ETW manifest {3} is not in the expected format.",
                            symbol,
                            eventId,
                            providerName,
                            manifestFile);
                        return;
                    }

                    // Get the table name
                    string tableName = tableNameParts[(int)TableInfoParts.Table];

                    this.tableShortNames.Add(tableName);
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to determine table name for event id {0} from manifest {1}, provider {2}. Exception: {3}.",
                    eventId,
                    manifestFile,
                    providerName,
                    e);
            }
        }

        private void GetTableShortNamesForProvider(XmlElement provider, string manifestFile)
        {
            string providerName = provider.GetAttribute("name");
            try
            {
                XmlElement eventElements = provider["events"];
                if (null == eventElements)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to find 'events' node for provider {0} in ETW manifest {1}.",
                        providerName,
                        manifestFile);
                    return;
                }
                foreach (XmlElement evt in eventElements.GetElementsByTagName("event"))
                {
                    GetTableShortNameForEvent(evt, providerName, manifestFile);
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to determine table names from manifest {0}, provider {1}. Exception: {2}.",
                    manifestFile,
                    providerName,
                    e);
            }
        }

        private void GetTableShortNamesFromManifest(string manifestFile)
        {
            try
            {
                XmlDocument doc = new XmlDocument { XmlResolver = null };
                XmlReaderSettings settings = new XmlReaderSettings { DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null};
                using (XmlReader reader = XmlReader.Create(manifestFile, settings))
                {
                    doc.Load(reader);
                }

                XmlElement rootNode = doc.DocumentElement;
                if (rootNode == null)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to find root node in ETW manifest {0}",
                        manifestFile);
                    return;
                }

                XmlElement instrumentationNode = rootNode["instrumentation"];
                if (instrumentationNode == null)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to find 'instrumentation' node in ETW manifest {0}",
                        manifestFile);
                    return;
                }

                XmlElement providerElements = instrumentationNode["events"];
                if (providerElements == null)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to find 'events' node in ETW manifest {0}",
                        manifestFile);
                    return;
                }

                foreach (XmlElement provider in providerElements.GetElementsByTagName("provider"))
                {
                    GetTableShortNamesForProvider(provider, manifestFile);
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to determine table names from manifest {0}. Exception: {1}.",
                    manifestFile,
                    e);
            }
        }

        private void GetTableShortNames()
        {
            // Get the directory where the manifests are located
            const string DefaultManifestFilter = "*.man";
            string assemblyLocation = typeof(AzureTableSelectiveEventUploader).GetTypeInfo().Assembly.Location;
            string manifestFileDirectory = Path.GetDirectoryName(assemblyLocation);

            // Get the names of the Windows Fabric manifests. This includes the
            // manifests for the current version as well as previous versions.
            string manifestPattern = this.configReader.GetUnencryptedConfigValue(
                                            this.initParam.SectionName,
                                            AzureConstants.TestOnlyEtwManifestFilePatternParamName,
                                            DefaultManifestFilter);
            IEnumerable<string> fabricManifests = FabricDirectory.GetFiles(manifestFileDirectory, manifestPattern);

            if (manifestPattern == DefaultManifestFilter)
            {
                fabricManifests = fabricManifests.Where(path => WinFabricManifestManager.IsFabricManifestFileName(Path.GetFileName(path)));
            }

            // Get the table names from each of the manifests
            foreach (string manifestFilePath in fabricManifests)
            {
                this.GetTableShortNamesFromManifest(manifestFilePath);
            }
        }

        private bool AtLeastOneLegacyTableExists()
        {
            bool atLeastOneLegacyTableExists = false;

            // Get the cloud storage account where we need to check for tables
            CloudStorageAccount storageAccount = this.tableUploadSettings.StorageAccountFactory.GetStorageAccount();

            CloudTableClient tableClient = AzureTableCommon.CreateNewTableClient(storageAccount);

            foreach (string tableShortName in this.tableShortNames)
            {
                // Compute the table name as per the legacy naming scheme
                string legacyTableName = String.Concat(
                                            this.tableUploadSettings.TableNamePrefix,
                                            tableShortName);

                // Check if the table name is in the correct format
                if (false == RegEx.IsMatch(legacyTableName))
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "{0} is an invalid Azure table name. Check for presence of legacy table will be skipped and we will assume that the legacy table does not exist.",
                        legacyTableName);
                    continue;
                }

                CloudTable cloudTable = tableClient.GetTableReference(legacyTableName);
                bool tableExists = false;
                try
                {
                    Utility.PerformWithRetries(
                        ctx =>
                        {
#if DotNetCoreClr
                            tableExists = cloudTable.ExistsAsync().Result;
#else
                            tableExists = cloudTable.Exists();
#endif
                        },
                        (object) null,
                        new RetriableOperationExceptionHandler(this.TableOperationExceptionHandler));
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsWarning(
                        this.logSourceId,
                        e,
                        "Unable to determine if legacy table {0} exists. We will assume that it does not exist.",
                        legacyTableName);
                    continue;
                }

                if (tableExists)
                {
                    atLeastOneLegacyTableExists = true;
                    break;
                }
            }

            return atLeastOneLegacyTableExists;
        }

        private void GetEffectiveDeploymentId()
        {
            if (false == String.IsNullOrEmpty(this.tableUploadSettings.DeploymentId))
            {
                // Deployment ID was explicitly specified in the cluster manifest. That's
                // the one we'll use.
                this.effectiveDeploymentId = this.tableUploadSettings.DeploymentId;
            }
            else
            {
                if (AzureUtility.IsAzureInterfaceAvailable())
                {
                    if (this.tableUploadSettings.TableNamePrefix.Length > AzureConstants.MaxTableNamePrefixLengthForDeploymentIdInclusion)
                    {
                        // The table name prefix specified by the user is too long to accomodate
                        // the deployment ID.
                        this.effectiveDeploymentId = String.Empty;
                    }
                    else if (AtLeastOneLegacyTableExists())
                    {
                        // The storage account already has a table with the legacy naming pattern
                        // that does not contain a deployment ID. For continuity, let us use the
                        // same table name. This means that the effective deployment ID is an
                        // empty string.
                        this.effectiveDeploymentId = String.Empty;
                    }
                    else
                    {
                        // Effective deployment ID is the deployment ID that we obtain from
                        // the Azure runtime interface.

                        // If it is too long, we truncate it. Azure deployment IDs seem to be
                        // random enough that this should almost always be good enough.
                        this.effectiveDeploymentId = AzureUtility.DeploymentId;
                        if (this.effectiveDeploymentId.Length > MaxAzureRuntimeDeploymentIDLength)
                        {
                            this.effectiveDeploymentId = this.effectiveDeploymentId.Remove(MaxAzureRuntimeDeploymentIDLength);
                        }

                        // The deployment ID on the Azure compute emulator has brackets, which
                        // are not valid characters in Azure table names. So let's replace the 
                        // opening and closing brackets with the string "OB" and "CB".
                        this.effectiveDeploymentId = this.effectiveDeploymentId.Replace("(", "OB");
                        this.effectiveDeploymentId = this.effectiveDeploymentId.Replace(")", "CB");
                    }
                }
                else
                {
                    // Azure interface is not available, so we cannot get the deployment ID
                    // from the Azure runtime. So the effective deployment ID is an empty string.
                    this.effectiveDeploymentId = String.Empty;
                }
            }
        }

        private void GetTableNamesFromShortNames()
        {
            // Figure out the deployment ID that we will use in the table name
            GetEffectiveDeploymentId();

            foreach (string tableShortName in this.tableShortNames)
            {
                // Compute the full table name
                string tableFullName = String.Concat(
                                           this.tableUploadSettings.TableNamePrefix,
                                           this.effectiveDeploymentId,
                                           tableShortName);

                // Check if the table name is in the correct format
                if (false == RegEx.IsMatch(tableFullName))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "{0} is an invalid Azure table name.",
                        tableFullName);
                    continue;
                }

                this.tableNames.Add(tableFullName);
            }
        }

        private void GetTableNames()
        {
            // First get the short names
            GetTableShortNames();

            // Now figure out the full table names
            GetTableNamesFromShortNames();

            // Figure out the name of the deployment ID table
            this.deploymentIdTableName = String.Concat(
                                             this.tableUploadSettings.TableNamePrefix,
                                             DeploymentIdTableShortName);
        }

        public object GetDataSink()
        {
            // Return a sink if we are enabled, otherwise return null.
            return this.tableUploadSettings.Enabled ? (IEtlFileSink) this : null;
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }
            this.disposed = true;

            // Keep track of the fact that the consumer is stopping
            this.stopping = true;

            if (null != this.oldLogDeletionTimer)
            {
                // Stop and dispose timers
                this.oldLogDeletionTimer.StopAndDispose();

                // Wait for timer callbacks to finish executing
                this.oldLogDeletionTimer.DisposedEvent.WaitOne();
            }

            GC.SuppressFinalize(this);
        }

        public bool NeedsDecodedEvent 
        {
            get
            {
                // We want the raw events. We'll decode it ourselves.
                return false;
            }
        }

        public void OnEtwEventsAvailable(IEnumerable<DecodedEtwEvent> etwEvents)
        {
        }

        private string ConvertEntityPropertyToString(EntityProperty entityProperty)
        {
            string result = null;
            switch (entityProperty.PropertyType)
            {
                case EdmType.Boolean:
                    {
                        result = entityProperty.BooleanValue.ToString();
                        break;
                    }
                case EdmType.DateTime:
                    {
                        result = entityProperty.DateTimeOffsetValue.ToString();
                        break;
                    }
                case EdmType.Double:
                    {
                        result = entityProperty.DoubleValue.ToString();
                        break;
                    }
                case EdmType.Guid:
                    {
                        result = entityProperty.GuidValue.ToString();
                        break;
                    }
                case EdmType.Int32:
                    {
                        result = entityProperty.Int32Value.ToString();
                        break;
                    }
                case EdmType.Int64:
                    {
                        result = entityProperty.Int64Value.ToString();
                        break;
                    }
                case EdmType.String:
                    {
                        result = entityProperty.StringValue;
                        break;
                    }
            }
            return result;
        }

        private bool VerifyVersionField(EventDefinition eventDefinition, out bool hasVersionField)
        {
            hasVersionField = false;
            foreach (FieldDefinition field in eventDefinition.Fields)
            {
                if (field.Name.Equals(VersionFieldName, StringComparison.Ordinal))
                {
                    hasVersionField = true;
                    if ((field.Type != FieldDefinition.FieldType.Int32) &&
                        (field.Type != FieldDefinition.FieldType.UInt32))
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "The field named {0} in Event of type {1}.{2} should be a 32-bit value.",
                            VersionFieldName,
                            eventDefinition.TaskName,
                            eventDefinition.EventName);
                        return false;
                    }
                }
            }
            return true;
        }

        private bool VerifyInstanceField(EventDefinition eventDefinition)
        {
            foreach (FieldDefinition field in eventDefinition.Fields)
            {
                if (field.Name.Equals(InstanceFieldName, StringComparison.Ordinal))
                {
                    if (field.Type != FieldDefinition.FieldType.UInt64)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "The field named {0} in Event of type {1}.{2} should be a 64-bit unsigned integer.",
                            InstanceFieldName,
                            eventDefinition.TaskName,
                            eventDefinition.EventName);
                        return false;
                    }
                }
            }
            return true;
        }

        private DynamicTableEntity ConvertEventToEntity(EventRecord eventRecord, EventDefinition eventDefinition)
        {
            // Figure out how many fields the event has
            int fieldCount = eventDefinition.Fields.Count();
            if (0 == fieldCount)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "No fields found in event of type {0}.{1}.",
                    eventDefinition.TaskName,
                    eventDefinition.EventName);
                return null;
            }

            // If the event has a version field, verify that it is a 32-bit value
            bool hasVersionField;
            if (false == VerifyVersionField(eventDefinition, out hasVersionField))
            {
                return null;
            }

            // If the event has an instance field, verify that it is a 64-bit unsigned int
            if (false == VerifyInstanceField(eventDefinition))
            {
                return null;
            }

            // Determine if the "id" field of the event is to be used as the partition key
            bool idFieldIsPartitionKey = false;
            if (eventDefinition.Fields[0].Name.Equals(IdFieldName, StringComparison.Ordinal))
            {
                // If the first field of the event is named "id", then use the value of that
                // field as the partition key.
                idFieldIsPartitionKey = true;
            }

            // Get the field names and values
            DynamicTableEntity tableEntity = new DynamicTableEntity();
            try
            {
                ApplicationDataReader reader = new ApplicationDataReader(
                                                       eventRecord.UserData,
                                                       eventRecord.UserDataLength);
                foreach (FieldDefinition fieldDef in eventDefinition.Fields)
                {
                    EntityProperty property = null;
                    switch (fieldDef.Type)
                    {
                        case FieldDefinition.FieldType.UnicodeString:
                            {
                                string value = reader.ReadUnicodeString();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.AnsiString:
                            {
                                string value = reader.ReadAnsiString();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.Boolean:
                            {
                                bool value = reader.ReadBoolean();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.UInt8:
                            {
                                int value = reader.ReadUInt8();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.UInt16:
                            {
                                int value = reader.ReadUInt16();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.UInt32:
                            {
                                int value;
                                unchecked
                                {
                                    value = (int)reader.ReadUInt32();
                                }
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.UInt64:
                            {
                                long value;
                                unchecked
                                {
                                    value = (long)reader.ReadUInt64();
                                }
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.Int8:
                            {
                                int value = reader.ReadInt8();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.Int16:
                            {
                                int value = reader.ReadInt16();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.Int32:
                            {
                                int value = reader.ReadInt32();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.Int64:
                            {
                                long value = reader.ReadInt64();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.HexInt32:
                            {
                                int value;
                                unchecked
                                {
                                    value = (int)reader.ReadUInt32();
                                }
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.HexInt64:
                            {
                                long value;
                                unchecked
                                {
                                    value = (long)reader.ReadUInt64();
                                }
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.Float:
                            {
                                double value = reader.ReadFloat();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.Double:
                            {
                                double value = reader.ReadDouble();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.DateTime:
                            {
                                DateTime value = reader.ReadFileTime();
                                property = new EntityProperty(value);
                                break;
                            }

                        case FieldDefinition.FieldType.Guid:
                            {
                                Guid value = reader.ReadGuid();
                                property = new EntityProperty(value);
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
                    if (null != property)
                    {
                        // The '.' character is not allowed in property names. However, the Windows Fabric tracing
                        // infrastructure uses the '.' character in the ETW event field names for encapsulated
                        // ETW records (i.e. ones that use AddField/FillEventData). Therefore, we replace the
                        // '.' character with the '_' character.
                        string propertyName = fieldDef.Name.Replace(".", "_");
                        tableEntity.Properties[propertyName] = property;
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

            tableEntity.Properties[TaskNameProperty] = new EntityProperty(eventDefinition.TaskName);
            tableEntity.Properties[EventTypeProperty] = new EntityProperty(eventDefinition.EventName);
            tableEntity.Properties[EventVersionProperty] = new EntityProperty(eventDefinition.Version);

            // If the event does not have a version field, supply a default version 0
            if (false == hasVersionField)
            {
                tableEntity.Properties[VersionFieldName] = new EntityProperty(ReverseChronologicalRowKeyFlag);
            }
            else
            {
                tableEntity.Properties[VersionFieldName].Int32Value = tableEntity.Properties[VersionFieldName].Int32Value | ReverseChronologicalRowKeyFlag;
            }

            // Initialize the partition key
            if (idFieldIsPartitionKey)
            {
                // Use the "id" field as the partition key and remove it from the
                // set of properties because we don't want another column for it in
                // the table.
                tableEntity.PartitionKey = ConvertEntityPropertyToString(tableEntity.Properties[IdFieldName]);
                tableEntity.Properties.Remove(IdFieldName);
            }
            else
            {
                // Use <TaskName>.<EventType> as the partition key
                tableEntity.PartitionKey = String.Concat(
                                                eventDefinition.TaskName,
                                                ".",
                                                eventDefinition.EventName);
            }

            // Initialize the row key
            DateTime eventTimestamp = DateTime.FromFileTimeUtc(eventRecord.EventHeader.TimeStamp);
            long rowKeyPrefix = DateTime.MaxValue.Ticks - eventTimestamp.Ticks;
            tableEntity.RowKey = String.Concat(
                                    rowKeyPrefix.ToString("D20", CultureInfo.InvariantCulture),
                                    "_",
                                    this.initParam.FabricNodeId,
                                    "_",
                                    eventRecord.BufferContext.ProcessorNumber.ToString(),
                                    "_",
                                    eventRecord.EventHeader.ProcessId.ToString(),
                                    "_",
                                    eventRecord.EventHeader.ThreadId.ToString());
            return tableEntity;
        }

        public void OnEtwEventsAvailable(IEnumerable<EventRecord> etwEvents)
        {
            foreach (EventRecord eventRecord in etwEvents)
            {
                if (this.stopping)
                {
                    // The consumer is being stopped. As far as possible, we don't want 
                    // the saving of changes to be interrupted midway due to any timeout 
                    // that may be enforced on consumer stop. Therefore, let's not save 
                    // the entities at this time. 
                    //
                    // Note that if a consumer stop is initiated while an ETL file is being 
                    // processed, that file gets processed again when the consumer is restarted.
                    // So we should get another opportunity to upload events from this ETL file 
                    // when we are restarted.
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "The consumer is being stopped. Therefore, traces will not be sent for upload to table storage.");
                    break;
                }

#if DotNetCoreClrLinux
                if (!this.IsEventInThisTable(eventRecord))
                {
                    continue;
                }
#endif
                // Get the event definition
                EventDefinition eventDefinition = this.manifestCache.GetEventDefinition(eventRecord);
                if (null == eventDefinition)
                {
                    // Unexpected event or eventsource event found. Just ignore it and move on.
                    continue;
                }

                // Figure out which table (if any) the event needs to go to
                string tableInfo = eventDefinition.TableInfo;
                if (String.IsNullOrEmpty(tableInfo))
                {
                    // Event definition does not indicate which table the event needs to go to. Skip it.
                    continue;
                }
                string[] tableInfoParts = tableInfo.Split(EventInfoDelimiter);
                if ((int)TableInfoParts.Table >= tableInfoParts.Length)
                {
                    // This event does not need to go to any table. Skip it.
                    continue;
                }

                string tableNameSuffix = tableInfoParts[(int)TableInfoParts.Table];
                string tableName = String.Concat(
                                        this.tableUploadSettings.TableNamePrefix,
                                        this.effectiveDeploymentId,
                                        tableNameSuffix);
                if (false == this.tableNames.Contains(tableName))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to upload event with task name {0}, event name {1} because table name {2} is not recognized.",
                        eventDefinition.TaskName,
                        eventDefinition.EventName,
                        tableName);
                    continue;
                }

                var storageAccountForUpload = this.tableUploadSettings.StorageAccountFactory.GetStorageAccount();
                var tableClientForUpload = AzureTableCommon.CreateNewTableClient(storageAccountForUpload);
                
                // Get a reference to the table that the event needs to go to
                CloudTable table = tableClientForUpload.GetTableReference(tableName);
                                            
                // Create an entity from this event.
                DynamicTableEntity entity = ConvertEventToEntity(eventRecord, eventDefinition);
                if (null == entity)
                {
                    // Failed to convert event to entity
                    continue;
                }

                // If the partition key might contains some characters that are not allowed for
                // Azure partition keys, replace it with an alternate.
                if (CharReplaceMap.ContainsKey(tableNameSuffix))
                {
                    CharacterReplacement replacement = CharReplaceMap[tableNameSuffix];
                    entity.PartitionKey = entity.PartitionKey.Replace(replacement.Original, replacement.Substitute);
                }

                // Check if the event is a deletion event
                bool isDeletionEvent = ((int)TableInfoParts.DeletionEvent < tableInfoParts.Length) &&
                                       tableInfoParts[(int)TableInfoParts.DeletionEvent].Equals(
                                           DeletionEventIndicator,
                                           StringComparison.Ordinal);

                // Push the entity to the table
                if (!TryWriteEntityToTable(table, entity, isDeletionEvent))
                {
                    this.traceSource.WriteError(
                         this.logSourceId,
                         "Failed to upload event of type {0}.{1} to Azure table.",
                         eventDefinition.TaskName,
                         eventDefinition.EventName); 
                }
            }
        }

        private bool TryWriteEntityToTable(
                        CloudTable table, 
                        DynamicTableEntity entity, 
                        bool entityIsDeletionEvent)
        {
            bool statusEntityConflict;
            bool success;
            do
            {
                success = true;
                statusEntityConflict = false;

                // Get the status record
                TableEntity statusEntity;
                bool isNewStatusEntity;
                if (false == GetStatusEntity(table, entity, entityIsDeletionEvent, out statusEntity, out isNewStatusEntity))
                {
                    // Error occurred while retrieving the status record. Skip this entity.
                    return false;
                }

                // The batch can contain the following entities:
                // 1. [REQUIRED] The entity representing the ETW event that we're processing
                // 2. [OPTIONAL] The entity that creates or updates the status record for this partition
                TableBatchOperation operation = new TableBatchOperation();

                // Add the entity representing the ETW event that we're processing
                operation.InsertOrReplace(entity);

                if (null != statusEntity)
                {
                    // We need to create or update the status record
                    if (isNewStatusEntity)
                    {
                        // Create new status record
                        operation.Insert(statusEntity);
                    }
                    else
                    {
                        // Update existing status record
                        operation.Merge(statusEntity);
                    }
                }

                IList<TableResult> results;
                try
                {
                    results = table.ExecuteBatchAsync(operation).Result;
                }
                catch (Exception e)
                {
                    while (e is AggregateException)
                    {
                        e = e.InnerException;
                    }

                    StorageException eSpecific = e as StorageException;
                    if (null != eSpecific)
                    {
                        // Check if the error was due to a conflicting update to the status record
                        //   #1 We thought we were inserting a new status entity into the table, but
                        //      someone else inserted the entity before we could.
                        //   #2 We were trying to update the existing status record, but someone else updated
                        //      or deleted it before we could. 
                        if ((isNewStatusEntity && 
                             (AzureTableCommon.HttpCodeEntityAlreadyExists == eSpecific.RequestInformation.HttpStatusCode)) // #1 above
                                ||
                            ((false == isNewStatusEntity) &&
                             ((AzureTableCommon.HttpCodePreconditionFailed == eSpecific.RequestInformation.HttpStatusCode) ||
                              (AzureTableCommon.HttpCodeResourceNotFound == eSpecific.RequestInformation.HttpStatusCode)))) // #2 above
                        {
                            statusEntityConflict = true;
                            continue;
                        }
                    }

                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to upload event to Azure table. Exception information: {0}",
                        e);
                    return false;
                }

                var i = 0;
                foreach (var result in results)
                {
                    if ((result.HttpStatusCode >= AzureTableCommon.HttpSuccessCodeMin) &&
                        (result.HttpStatusCode <= AzureTableCommon.HttpSuccessCodeMax))
                    {
                        continue;
                    }

                    // Error occurred while uploading the entity corresponding to the ETW event
                    success = false;
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "HTTP status code {0} was returned while uploading entity representing {1}.",
                        result.HttpStatusCode,
                        i == 0 ? "ETW event" : "status record");

                    if (i == 0)
                    {
                        continue;
                    }

                    // Check if the error was due to a conflicting update to the status record
                    // Case 1: We thought we were inserting a new status entity into the table, but
                    // someone else inserted the entity before we could.
                    // Case 2: We were trying to update the existing status record, but someone else updated
                    // it before we could. Hence the ETag on the status record was invalidated.
                    if (isNewStatusEntity && AzureTableCommon.HttpCodeEntityAlreadyExists == result.HttpStatusCode ||
                        !isNewStatusEntity && AzureTableCommon.HttpCodePreconditionFailed == result.HttpStatusCode)
                    {
                        statusEntityConflict = true;
                    }

                    i++;
                }
            }
            while (statusEntityConflict);

            return success;
        }

        private bool GetExistingStatusEntity(CloudTable table, DynamicTableEntity entity, out StatusEntity statusEntity)
        {
            statusEntity = null;

            // The row key of the status record is a special value that will 
            // not match any of the row keys of the non-status records.
            TableOperation queryStatusOperation = TableOperation.Retrieve<StatusEntity>(entity.PartitionKey, StatusEntityRowKey);
            TableResult queryResult;
            try
            {
#if DotNetCoreClr
                queryResult = table.ExecuteAsync(queryStatusOperation).Result;
#else
                queryResult = table.Execute(queryStatusOperation);
#endif
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to query status record from table {0}. Partition key: {1}, row key: {2}. Exception: {3}",
                    table.Name,
                    entity.PartitionKey,
                    entity.RowKey,
                    e);
                return false;
            }

            if ((queryResult.HttpStatusCode < AzureTableCommon.HttpSuccessCodeMin) ||
                (queryResult.HttpStatusCode > AzureTableCommon.HttpSuccessCodeMax))
            {
                if (queryResult.HttpStatusCode == AzureTableCommon.HttpCodeResourceNotFound)
                {
                    // Status record not present. This is not an error.
                    return true;
                }
                else
                {
                    // Error occurred while retrieving status record
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to query status record from table {0}. Partition key: {1}, row key: {2}. HTTP status code {3}.",
                        table.Name,
                        entity.PartitionKey,
                        entity.RowKey,
                        queryResult.HttpStatusCode);
                    return false;
                }
            }

            statusEntity = queryResult.Result as StatusEntity;
            return true;
        }

        private bool GetStatusEntity(
                        CloudTable table, 
                        DynamicTableEntity entity,
                        bool entityIsDeletionEvent,
                        out TableEntity statusEntity, 
                        out bool isNewStatusEntity)
        {
            statusEntity = null;
            isNewStatusEntity = false;

            // Check if the record representing the event contains an instance field
            long instance;
            if (false == entity.Properties.ContainsKey(InstanceFieldName))
            {
                // Instance field not present
                if (false == entityIsDeletionEvent)
                {
                    // Neither instance field is present nor is it a deletion event, so no 
                    // need for a status record.
                    return true;
                }
                else
                {
                    // It is a deletion event, so assign a dummy instance value that
                    // is large enough to always override the existing instance value in
                    // the status record.
                    unchecked
                    {
                        instance = (long) UInt64.MaxValue;
                    }
                }
            }
            else
            {
                // Instance field present
                instance = (long)entity.Properties[InstanceFieldName].Int64Value;
            }

            // If a status record already exists in the table, retrieve it
            StatusEntity existingStatusEntity;
            if (false == GetExistingStatusEntity(table, entity, out existingStatusEntity))
            {
                // Error occurred while trying to retrieve status record. Note that if the
                // status record is not present, that is NOT considered an error.
                return false;
            }

            if (null == existingStatusEntity)
            {
                // Status record doesn't exist yet. Let's create a new one.
                isNewStatusEntity = true;
                StatusEntity newStatusEntity = new StatusEntity();
                newStatusEntity.PartitionKey = entity.PartitionKey;
                newStatusEntity.RowKey = StatusEntityRowKey;

                newStatusEntity.Instance = instance;

                // If necessary, update the field that indicates deletion
                newStatusEntity.IsDeleted = entityIsDeletionEvent;

                statusEntity = newStatusEntity;
                return true;
            }
            else
            {
                UInt64 entityInstance;
                UInt64 existingStatusInstance;
                unchecked
                {
                    entityInstance = (UInt64)instance;
                    existingStatusInstance = (UInt64)existingStatusEntity.Instance;
                }
                if (entityInstance < existingStatusInstance)
                {
                    // This event does not describe the latest instance of the object.
                    // No need to update the status record in this case.
                    return true;
                }
                else if (entityInstance == existingStatusInstance)
                {
                    // This event describes the latest instance of the object. Check if
                    // we need to update the deletion status of the object.
                    if (entityIsDeletionEvent)
                    {
                        // Update the deletion status of this object
                        existingStatusEntity.IsDeleted = true;
                    }
                    else
                    {
                        // No need to update deletion status
                        return true;
                    }
                }
                else
                {
                    // We have a new instance of the object. We need to update the
                    // status record to reflect this.
                    existingStatusEntity.Instance = instance;
                    existingStatusEntity.IsDeleted = entityIsDeletionEvent;
                }
                statusEntity = existingStatusEntity;
                return true;
            }
        }

        public void SetEtwManifestCache(ManifestCache manifestCache)
        {
            this.manifestCache = manifestCache;
        }

        public void OnEtwEventProcessingPeriodStart()
        {
        }

        public void OnEtwEventProcessingPeriodStop()
        {
        }

        private void GetSettings()
        {
            // Check for values in settings.xml
            this.tableUploadSettings.Enabled = this.configReader.GetUnencryptedConfigValue(
                                                   this.initParam.SectionName,
                                                   AzureConstants.EnabledParamName,
                                                   AzureConstants.TableUploadEnabledByDefault);
            if (this.tableUploadSettings.Enabled)
            {
                this.tableUploadSettings.StorageAccountFactory = this.azureUtility.GetStorageAccountFactory(
                                    this.configReader,
                                    this.initParam.SectionName,
                                    AzureConstants.ConnectionStringParamName);

                if (this.tableUploadSettings.StorageAccountFactory == null)
                {
                    this.tableUploadSettings.Enabled = false;
                    return;
                }

                this.tableUploadSettings.TableNamePrefix = this.configReader.GetUnencryptedConfigValue(
                                                         this.initParam.SectionName,
                                                         AzureConstants.EtwTraceTablePrefixParamName,
                                                         AzureConstants.DefaultTableNamePrefix);
                this.tableUploadSettings.DeploymentId = this.configReader.GetUnencryptedConfigValue(
                                                            this.initParam.SectionName,
                                                            AzureConstants.DeploymentId,
                                                            AzureConstants.DefaultDeploymentId);
                var entityDeletionAge = TimeSpan.FromDays(this.configReader.GetUnencryptedConfigValue(
                                                this.initParam.SectionName,
                                                AzureConstants.DataDeletionAgeParamName,
                                                AzureConstants.DefaultDataDeletionAge.TotalDays));
                if (entityDeletionAge > AzureConstants.MaxDataDeletionAge)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        entityDeletionAge,
                        this.initParam.SectionName,
                        AzureConstants.DataDeletionAgeParamName,
                        AzureConstants.MaxDataDeletionAge);
                    entityDeletionAge = AzureConstants.MaxDataDeletionAge;
                }
                this.tableUploadSettings.EntityDeletionAge = entityDeletionAge;

                // Check for test settings
                var logDeletionAgeTestValue = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                  this.initParam.SectionName,
                                                  AzureConstants.TestDataDeletionAgeParamName,
                                                  0));
                if (logDeletionAgeTestValue != TimeSpan.Zero)
                {
                    if (logDeletionAgeTestValue > AzureConstants.MaxDataDeletionAge)
                    {
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                            logDeletionAgeTestValue,
                            this.initParam.SectionName,
                            AzureConstants.TestDataDeletionAgeParamName,
                            AzureConstants.MaxDataDeletionAge);
                        logDeletionAgeTestValue = AzureConstants.MaxDataDeletionAge;
                    }
                    this.tableUploadSettings.EntityDeletionAge = logDeletionAgeTestValue;
                }

                // Write settings to log
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Table storage upload enabled: ETW trace table prefix: {0}, Entity deletion age ({1}): {2}.",
                    this.tableUploadSettings.TableNamePrefix,
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? "days" : "minutes",
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? entityDeletionAge : logDeletionAgeTestValue);

            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Table storage upload not enabled");
            }
        }

        private void CreateTablesWorker(object context)
        {
            CloudStorageAccount storageAccount = (CloudStorageAccount) context;

            // Create the table client object
            CloudTableClient cloudTableClient = AzureTableCommon.CreateNewTableClient(storageAccount);

            List<string> tablesToCreate = new List<string>(this.tableNames) { this.deploymentIdTableName };
            foreach (string tableName in tablesToCreate)
            {
                // Create the table
#if DotNetCoreClr
                cloudTableClient.GetTableReference(tableName).CreateIfNotExistsAsync().Wait();
#else
                cloudTableClient.GetTableReference(tableName).CreateIfNotExists();
#endif
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Created table {0} in storage account {1}",
                    tableName,
                    this.tableUploadSettings.StorageAccountFactory.Connection.AccountName);
            }
        }

        private void AddEffectiveDeploymentIdToTable(object context)
        {
            if (String.IsNullOrEmpty(this.effectiveDeploymentId))
            {
                return;
            }

            // Construct the entity
            DeploymentTableEntity entity = new DeploymentTableEntity
            {
                PartitionKey = this.effectiveDeploymentId,
                RowKey = String.Empty,
                StartTime = DateTime.UtcNow
            };

            // Construct the table operation
            TableOperation insertOperation = TableOperation.Insert(entity);

            CloudStorageAccount storageAccount = (CloudStorageAccount) context;

            // Create the table client object
            CloudTableClient cloudTableClient = AzureTableCommon.CreateNewTableClient(storageAccount);
            
            // Get the table reference
            CloudTable cloudTable = cloudTableClient.GetTableReference(this.deploymentIdTableName);

            // Add entity to the table
            try
            {
#if DotNetCoreClr
                cloudTable.ExecuteAsync(insertOperation).Wait();
#else
                cloudTable.Execute(insertOperation);
#endif
            }
            catch (StorageException e)
            {
                // If the entity already exists in the table, we handle the exception and move on.
                // It is not an error - it just means that we already added the deployment ID to
                // the table.
                if (AzureTableCommon.HttpCodeEntityAlreadyExists != e.RequestInformation.HttpStatusCode)
                {
                    throw;
                }
            }
        }

        internal RetriableOperationExceptionHandler.Response TableOperationExceptionHandler(Exception e)
        {
            this.traceSource.WriteError(
                this.logSourceId,
                "Exception encountered when attempting to perform an operation on table {0} in storage account {1}. Exception information: {2}",
                this.tableUploadSettings.TableNamePrefix,
                this.tableUploadSettings.StorageAccountFactory.Connection.AccountName,
                e);
                
            if (e is StorageException)
            {
                // If this was a network error, we'll retry later. Else, we'll
                // just give up.
                StorageException eSpecific = (StorageException) e;
                if (Utility.IsNetworkError(eSpecific.RequestInformation.HttpStatusCode))
                {
                    return RetriableOperationExceptionHandler.Response.Retry;
                }
            }

            return RetriableOperationExceptionHandler.Response.Abort;
        }

        private void CreateTables()
        {
            CloudStorageAccount storageAccount = this.tableUploadSettings.StorageAccountFactory.GetStorageAccount();

            RetriableOperationExceptionHandler exceptionHandler = new RetriableOperationExceptionHandler(
                                                                          this.TableOperationExceptionHandler);
            Utility.PerformWithRetries(
                CreateTablesWorker,
                storageAccount,
                exceptionHandler);

            Utility.PerformWithRetries(
                AddEffectiveDeploymentIdToTable,
                storageAccount,
                exceptionHandler);
        }

        private bool ShouldDeleteOldLogs()
        {
            DateTime lastFMMEventTimestamp;
            try
            {
                // Get the timestamp of the last FMM event we saw
                lastFMMEventTimestamp = Utility.LastFmmEventTimestamp;
            }
            catch (NotImplementedException)
            {
                // Unable to determine the timestamp of the last FMM event
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "Unable to determine whether the current node is the FMM node. So multiple nodes will attempt deletion of old entities from Azure tables.");
                return true;
            }

            if (lastFMMEventTimestamp.CompareTo(this.fmmEventTimestampSnapshot) > 0)
            {
                // New FMM events were seen since the last time we checked. Therefore,
                // we're likely still on the FMM node. Hence we should attempt to delete
                // old entities.
                this.fmmEventTimestampSnapshot = lastFMMEventTimestamp;
                return true;
            }

            // No new FMM events were seen since the last time we checked. Therefore,
            // we're not on the FMM node and hence we should not attempt to delete
            // old entities.
            this.traceSource.WriteInfo(
                this.logSourceId,
                "Not attempting deletion of old entities because the current node is not the FMM node.");
            return false;
        }

        private void DeleteOldLogsHandler(object state)
        {
            if (false == ShouldDeleteOldLogs())
            {
                // Schedule the next pass
                this.oldLogDeletionTimer.Start();
                return;
            }

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Starting old entity deletion pass ...");

            var storageAccountForDeletion = this.tableUploadSettings.StorageAccountFactory.GetStorageAccount();

            // Create a table client
            CloudTableClient tableClient = AzureTableCommon.CreateNewTableClient(storageAccountForDeletion);

            // Figure out the timestamp before which all entities will be deleted
            DateTime cutoffTime = DateTime.UtcNow.Add(-this.tableUploadSettings.EntityDeletionAge);

            foreach (string tableName in this.tableNames)
            {
                if (this.stopping)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Deletion of old entities interrupted because the consumer is being stopped.");
                    break;
                }

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Starting old entity deletion pass for table {0} ...",
                    tableName);

                CloudTable table = tableClient.GetTableReference(tableName);

                // If we used status records to keep track of the table's partitions,
                // then perform deletion partition by partition.
                bool noStatusEntitiesFound;
                DeleteOldEntitiesByPartition(table, cutoffTime, out noStatusEntitiesFound);

                if (noStatusEntitiesFound)
                {
                    // We didn't find any status records, so just delete entites
                    // without regard to the partition to which they belong.
                    DeleteOldEntities(table, cutoffTime, null, null);
                }

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Completed old entity deletion pass for table {0}.",
                    tableName);
            }

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Completed old entity deletion pass.");

            // Schedule the next pass
            this.oldLogDeletionTimer.Start();
        }

        private void DeleteOldEntitiesByPartition(CloudTable table, DateTime cutoffTime, out bool noStatusEntitiesFound)
        {
            noStatusEntitiesFound = true;

            // Let's first get all the status records. This will tell us what
            // partitions exist in the table.
            TableQuery<StatusEntity> statusEntityQuery = (new TableQuery<StatusEntity>())
                                                         .Where(TableQuery.GenerateFilterCondition(
                                                                    "RowKey",
                                                                     QueryComparisons.Equal,
                                                                     StatusEntityRowKey));
#if DotNetCoreClr
            List<StatusEntity> statusEntities = new List<StatusEntity>();
            TableContinuationToken tableContinuationToken = new TableContinuationToken();
            TableQuerySegment<StatusEntity> entitiesSegment = null;
            while (entitiesSegment == null)
            {
                entitiesSegment = table.ExecuteQuerySegmentedAsync(statusEntityQuery, tableContinuationToken).Result;
                statusEntities.AddRange(entitiesSegment);
            }
#else
            IEnumerable<StatusEntity> statusEntities = table.ExecuteQuery(statusEntityQuery);
#endif
            try
            {
                foreach (StatusEntity statusEntity in statusEntities)
                {
                    if (this.stopping)
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Deletion of old entities from table {0} was interrupted because the consumer is being stopped.",
                            table.Name);
                        break;
                    }

                    // Delete old records for this partition
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Starting old entity deletion pass for table {0}, partition {1} ...",
                        table.Name,
                        statusEntity.PartitionKey);

                    noStatusEntitiesFound = false;
                    DeleteOldEntities(table, cutoffTime, statusEntity.PartitionKey, statusEntity);

                    // Check if the object corresponding to this partition has 
                    // already been deleted. If so, delete the status record too.
                    DeleteStatusEntityIfNeeded(table, statusEntity);

                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Completed old entity deletion pass for table {0}, partition {1}.",
                        table.Name,
                        statusEntity.PartitionKey);
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to query for status records from table {0}. Exception: {1}.",
                    table.Name,
                    e);
            }
        }

        private void DeleteOldEntities(CloudTable table, DateTime cutoffTime, string partitionKey, StatusEntity statusEntity)
        {
            TableQuery<DynamicTableEntity> oldEntityQuery;
            if (null != partitionKey)
            {
                // We query for entities that meet the following criteria:
                //  - partition key equals the partition key provided by the caller
                //  - event timestamp is old enough that the entity needs to be deleted
                oldEntityQuery = (new TableQuery<DynamicTableEntity>())
                                 .Where(TableQuery.CombineFilters(
                                            TableQuery.GenerateFilterCondition(
                                                "PartitionKey", 
                                                QueryComparisons.Equal, 
                                                partitionKey),
                                            TableOperators.And,
                                            CreateTimestampFilter(cutoffTime)));
            }
            else
            {
                // We query for entities that meet the following criteria:
                //  - event timestamp is old enough that the entity needs to be deleted
                oldEntityQuery = (new TableQuery<DynamicTableEntity>())
                                 .Where(CreateTimestampFilter(cutoffTime));
            }

#if DotNetCoreClr
            List<DynamicTableEntity> oldEntities = new List<DynamicTableEntity>();
            TableContinuationToken tableContinuationToken = new TableContinuationToken();
            TableQuerySegment<DynamicTableEntity> entitiesSegment = null;
            while (entitiesSegment == null)
            {
                entitiesSegment = table.ExecuteQuerySegmentedAsync(oldEntityQuery, tableContinuationToken).Result;
                oldEntities.AddRange(entitiesSegment);
            }
#else
            IEnumerable<DynamicTableEntity> oldEntities = table.ExecuteQuery(oldEntityQuery);
#endif
            try
            {
                bool atLeastOneOldEntityPreserved = false;
                foreach (DynamicTableEntity oldEntity in oldEntities)
                {
                    if (this.stopping)
                    {
                        if (null != partitionKey)
                        {
                            this.traceSource.WriteInfo(
                                this.logSourceId,
                                "Deletion of old entities from table {0} partition {1} was interrupted because the consumer is being stopped.",
                                table.Name,
                                partitionKey);
                        }
                        else
                        {
                            this.traceSource.WriteInfo(
                                this.logSourceId,
                                "Deletion of old entities from table {0} was interrupted because the consumer is being stopped.",
                                table.Name);
                        }
                        break;
                    }

                    if ((null != statusEntity) &&
                        (false == statusEntity.IsDeleted))
                    {
                        if (oldEntity.Properties.ContainsKey(InstanceFieldName))
                        {
                            // We don't delete entities associated with the current
                            // instance of the object.
                            if (oldEntity.Properties[InstanceFieldName].Int64Value == statusEntity.Instance)
                            {
                                // Skip deletion of this entity.
                                atLeastOneOldEntityPreserved = true;
                                continue;
                            }
                        }

                        if ((false == atLeastOneOldEntityPreserved) &&
                            (oldEntity.Properties[VersionFieldName].Int32Value < 0))
                        {
                            // For the tables that we cleanup on a per-partition basis, we always retain one entity that meets the deletion
                            // criteria, so that the table contains full history about that partition for the retention period.
                            // For example, consider two entities with timestamps T1 and T3, where T1 < T3 and there are no other entites
                            // in between them. Let's say the retention policy is such that any entities with timestamp older than T2 should
                            // be deleted, where T1 < T2 < T3. In this example, the entity with timestamp T1 is eligible for deletion, but
                            // we won't delete it. This is because its presence will ensure that the table can be used to reconstruct the 
                            // history between T2 and T3.
                            // Note that we do this only for entities that have been written by v3.1 or later code. Entities written by v3.1
                            // and later are arranged in reverse chronological order. So when we query for old entities we receive them in
                            // reverse chronological order, which means that the newest old entity comes first and the oldest old entity comes
                            // last. Our goal is to skip the newest old entity, so we just need to skip the first old entity that was written
                            // by v3.1 or later code.
                            atLeastOneOldEntityPreserved = true;
                            continue;
                        }
                    }

                    DeleteEntity(table, oldEntity, new[] { AzureTableCommon.HttpCodeResourceNotFound });
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to query for old entities from table {0}. Exception: {1}.",
                    table.Name,
                    e);
            }
        }

        private static string CreateTimestampFilter(DateTime cutoffTime)
        {
            // Filter for records created by V3 and older code, where row keys were in chronological order
            string cutoffTimeTicks = cutoffTime.Ticks.ToString("D20", CultureInfo.InvariantCulture);
            string v3StyleFilter = TableQuery.CombineFilters(
                                        TableQuery.GenerateFilterCondition(
                                            "RowKey", 
                                            QueryComparisons.LessThan, 
                                            cutoffTimeTicks),
                                        TableOperators.And,
                                        TableQuery.GenerateFilterConditionForInt(
                                            VersionFieldName, 
                                            QueryComparisons.GreaterThanOrEqual, 
                                            0));

            // Filter for records created by the current code, where row keys are in reverse chronological order
            cutoffTimeTicks = (DateTime.MaxValue.Ticks - cutoffTime.Ticks).ToString("D20", CultureInfo.InvariantCulture);
            string filter = TableQuery.CombineFilters(
                                        TableQuery.GenerateFilterCondition(
                                            "RowKey",
                                            QueryComparisons.GreaterThan,
                                            cutoffTimeTicks),
                                        TableOperators.And,
                                        TableQuery.GenerateFilterConditionForInt(
                                            VersionFieldName,
                                            QueryComparisons.LessThan,
                                            0));

            return TableQuery.CombineFilters(
                        v3StyleFilter,
                        TableOperators.Or,
                        filter);
        }

        private void DeleteEntity(CloudTable table, ITableEntity entity, int[] errorCodesToIgnore)
        {
            TableOperation deleteOperation = TableOperation.Delete(entity);
            TableResult deleteResult;
            try
            {
#if DotNetCoreClr
                deleteResult = table.ExecuteAsync(deleteOperation).Result;
#else
                deleteResult = table.Execute(deleteOperation);
#endif
            }
            catch (Exception e)
            {
                StorageException eSpecific = e as StorageException;
                if (null != eSpecific)
                {
                    if (false == errorCodesToIgnore.Contains(eSpecific.RequestInformation.HttpStatusCode))
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Failed to delete entity with partition key {0}, row key {1} from table {2}. Exception: {3}.",
                            entity.PartitionKey,
                            entity.RowKey,
                            table.Name,
                            e);
                    }
                }
                return;
            }
            if ((deleteResult.HttpStatusCode < AzureTableCommon.HttpSuccessCodeMin) ||
                (deleteResult.HttpStatusCode > AzureTableCommon.HttpSuccessCodeMax))
            {
                // Error occurred while deleting entity.
                // If the error code is not something that the caller asked us to ignore,
                // then we log an error message.
                if (false == errorCodesToIgnore.Contains(deleteResult.HttpStatusCode))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to delete entity with partition key {0}, row key {1} from table {2}. HTTP status code {3}.",
                        entity.PartitionKey,
                        entity.RowKey,
                        table.Name,
                        deleteResult.HttpStatusCode);
                }
            }
        }

        private void DeleteStatusEntityIfNeeded(CloudTable table, StatusEntity statusEntity)
        {
            // Check if the object corresponding to this partition has 
            // already been deleted
            if (false == statusEntity.IsDeleted)
            {
                // Object corresponding to this partition has not yet
                // been deleted. We cannot delete the status record yet.
                return;
            }

            // The object corresponding to this partition has already
            // been deleted.

            // Check if we have deleted all entities from this partition
            TableQuery<DynamicTableEntity> partitionEntityQuery = (new TableQuery<DynamicTableEntity>())
                                .Where(TableQuery.CombineFilters(
                                        TableQuery.GenerateFilterCondition(
                                            "PartitionKey",
                                            QueryComparisons.Equal,
                                            statusEntity.PartitionKey),
                                        TableOperators.And,
                                        TableQuery.GenerateFilterCondition(
                                            "RowKey",
                                            QueryComparisons.LessThan,
                                            StatusEntityRowKey)));
#if DotNetCoreClr
            List<DynamicTableEntity> partitionEntities = new List<DynamicTableEntity>();
            TableContinuationToken tableContinuationToken = new TableContinuationToken();
            TableQuerySegment<DynamicTableEntity> entitiesSegment = null;
            while (entitiesSegment == null)
            {
                entitiesSegment = table.ExecuteQuerySegmentedAsync(partitionEntityQuery, tableContinuationToken).Result;
                partitionEntities.AddRange(entitiesSegment);
            }
#else
            IEnumerable<DynamicTableEntity> partitionEntities = table.ExecuteQuery(partitionEntityQuery);
#endif
            try
            {
                if (null == partitionEntities.FirstOrDefault())
                {
                    // We have deleted all entities from the partition. So
                    // we can also delete the status record now.
                    DeleteEntity(table, statusEntity, new[] {AzureTableCommon.HttpCodePreconditionFailed});
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to determine whether table {0}, partition {1} has any entities other than the status record. Exception: {2}.",
                    table.Name,
                    statusEntity.PartitionKey,
                    e);
            }
        }

        // Settings related to upload of data to table storage
        private struct TableUploadSettings
        {
            internal bool Enabled;
            internal StorageAccountFactory StorageAccountFactory;
            internal string TableNamePrefix;
            internal string DeploymentId;
            internal TimeSpan EntityDeletionAge;
        }

        // Status record for a partition of the Azure table
        private class StatusEntity : TableEntity
        {
            public long Instance { get; set; }
            public bool IsDeleted { get; set; }
        }

        private class DeploymentTableEntity : TableEntity
        {
            public DateTime StartTime { get; set; }
        }

        private class CharacterReplacement
        {
            public char Original;
            public char Substitute;
        }
    }
}
