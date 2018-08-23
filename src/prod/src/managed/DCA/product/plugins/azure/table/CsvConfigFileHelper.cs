// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using System.Reflection;
    using System.Text.RegularExpressions;

    // This is a utility class for Azure table uploaders. 
    internal class CsvConfigFileHelper
    {
        private const char eventRegexSplitter = ':';
        private const char taskNameTypeSplitter = '.';
        private const string TableInfoDelimiter = "_";
        private const char EventInfoDelimiter = '_';

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        private readonly string effectiveDeploymentId;
        private readonly string tableNamePrefix;

        public CsvConfigFileHelper(
            string configFile,
            string logSourceId,
            FabricEvents.ExtensionsEvents traceSource,
            string effectiveDeploymentId,
            string tableNamePrefix)
        {
            this.logSourceId = logSourceId;
            this.traceSource = traceSource;
            this.TableShortNames = new HashSet<string>();
            this.tableNames = new HashSet<string>();
            this.TableConfig = new Dictionary<string, EventInfo>();
            this.effectiveDeploymentId = effectiveDeploymentId;
            this.tableNamePrefix = tableNamePrefix;

            this.ParseConfigFile(configFile);
        }

        public EventInfo GetEventInfo(string eventType)
        {
            try
            {
                return this.TableConfig[eventType];
            }
            catch (Exception)
            {
                return null;
            }
        }

        private string GetTableNamesFromShortNames(string tableShortName)
        {
            // Compute the full table name
            string tableFullName = String.Concat(
                this.tableNamePrefix,
                this.effectiveDeploymentId,
                tableShortName);

            // Check if the table name is in the correct format
            if (false == regEx.IsMatch(tableFullName))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "{0} is an invalid Azure table name.",
                    tableFullName);
                throw new ArgumentException();
            }

            return tableFullName;
        }

        private void ParseConfigLine(string configLine, string configFile)
        {
            // Sample config line
            // FM._Partitions_DEL_PartitionDeleted:^Partition\s?.* is deleted$

            // Parse the Regex
            var regexSplitterIndex = configLine.IndexOf(eventRegexSplitter);
            var eventDetail = configLine.Substring(0, regexSplitterIndex);
            var regex = configLine.Substring(regexSplitterIndex + 1);

            // Parse the TaskInfo
            var taskSplitterIndex = configLine.IndexOf(taskNameTypeSplitter);
            //var taskInfo = eventDetail.Substring(0, taskSplitterIndex);

            // Parse TableName, Is Delete event and Event Type 
            var details = eventDetail.Substring(taskSplitterIndex+1).Split(EventInfoDelimiter);
            var tableInfo = details[1];
            var isDeleteEvent = false;
            if (details[1].CompareTo("DEL") == 0)
            {
                isDeleteEvent = true;
            }

            var eventInfo = new EventInfo();
            eventInfo.Regex = new Regex(regex);
            eventInfo.IsDeleteEvent = isDeleteEvent;
            eventInfo.TableInfo = this.GetTableNamesFromShortNames(tableInfo);
            eventInfo.ShortTableInfo = tableInfo;


            if (!this.tableNames.Contains(eventInfo.TableInfo))
            {
                this.tableNames.Add(eventInfo.TableInfo);
            }

            try
            {
                this.TableConfig.Add(eventDetail, eventInfo);
            }
            catch (ArgumentException e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Duplicate entries exist in table config file {0} for event {1].",
                    configFile,
                    eventDetail);
                throw e;
            }
        }

        private void ParseConfigFile(string eventConfigFileName)
        {
            // Get the directory where the config file is located
            string assemblyLocation = typeof(AzureTableQueryableCsvUploader).GetTypeInfo().Assembly.Location;
            string configFileDirectory = Path.GetDirectoryName(assemblyLocation);
            string configFileName = Path.Combine(configFileDirectory, eventConfigFileName);

            // Get the table names from config file
            try
            {
                StreamReader file = new StreamReader(new FileStream(configFileName, FileMode.Open, FileAccess.Read));

                string configLine;
                while ((configLine = file.ReadLine()) != null)
                {
                    ParseConfigLine(configLine, configFileName);
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to determine table names from config file {0}. Exception: {1}.",
                    configFileName,
                    e);

                throw e;
            }
        }
        
        // The constituent parts of event name
        private enum TableInfoParts
        {
            Table,
            DeletionEvent
        }

        // List of short names for the tables we'll be uploading events to
        public HashSet<string> TableShortNames { get; private set; }

        // List of tables we'll be uploading events to
        public HashSet<string> tableNames { get; private set; }

        // TaskName to (Event Type to Event Details)
        private Dictionary<string, EventInfo> TableConfig;
        
        // Regular expression describing valid table names
        public readonly Regex regEx = new Regex(AzureConstants.AzureTableNameRegularExpression);

        public class EventInfo
        {
            public string TableInfo;
            public string ShortTableInfo;
            public Regex Regex;
            public bool IsDeleteEvent;
        }
    }
}