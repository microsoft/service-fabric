// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Diagnostics.Tracing.Config
{
    using System;
    using System.Diagnostics.Tracing;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Threading;
    using Filter;

    internal class TraceConfig
    {
        private const string TraceEventsSectionPrefix = "Tracing/";
        private const EventLevel DefaultTracingLevel = EventLevel.Informational;
        private const EventKeywords DefaultKeyword = (EventKeywords)(-1L);
        private readonly char[] filterValueSeparatorArray = { ',' };
        private readonly string traceEventsSectionName;
        private readonly string packageName;
        private TraceSinkFilter sinkFilter = null;

        internal TraceConfig(string eventSourceName, string packageName)
        {
            this.traceEventsSectionName = string.Concat(TraceEventsSectionPrefix, eventSourceName);
            this.packageName = packageName;
            ConfigurationSettings configurationSettings = null;
            CodePackageActivationContext context = null;

            try
            {
                context = FabricRuntime.GetActivationContext();
            }
            catch (Exception)
            {
                // We want to handle the standalone case
            }

            if (context != null)
            {
                // Handle scenario for System Servcies as they dont have config package.
                // System Services like Fault Analysis Service & Update Orchestration Service depends on Microsoft.ServiceFabric.Services.dll which depends on
                // Microsoft.ServiceFabric.Diagnostics.dll
                if (context.GetConfigurationPackageNames().Contains(packageName))
                {
                    var configPackage = context.GetConfigurationPackageObject(packageName);
                    context.ConfigurationPackageModifiedEvent += this.OnConfigurationPackageModified;

                    if (configPackage != null)
                    {
                        configurationSettings = configPackage.Settings;
                    }
                }
            }

            this.UpdateSettings(configurationSettings, true);
        }

        // Initialize to prevent need for null check.
        internal event Action OnFilterUpdate = () => { };

        internal bool GetEventEnabledStatus(
            EventLevel level,
            EventKeywords keywords,
            string eventName)
        {
            return this.sinkFilter.CheckEnabledStatus(
                level,
                keywords,
                eventName);
        }

        private void UpdateSettings(ConfigurationSettings configurationSettings, bool firstTimeInitialization = false)
        {
            ConfigurationSection configSection = null;
            if (configurationSettings != null && configurationSettings.Sections.Contains(this.traceEventsSectionName))
            {
                configSection = configurationSettings.Sections[this.traceEventsSectionName];
            }

            this.InitializeEventTrace(configSection, firstTimeInitialization);
        }

        private void OnConfigurationPackageModified(object sender, PackageModifiedEventArgs<ConfigurationPackage> e)
        {
            if (e.NewPackage.Description.Name.Equals(this.packageName))
            {
                this.UpdateSettings(e.NewPackage.Settings);
            }
        }

        private EventLevel ConvertLevel(int level)
        {
            if (level > (int)EventLevel.Verbose)
            {
                return EventLevel.Verbose;
            }

            if (level < (int)EventLevel.LogAlways)
            {
                return EventLevel.LogAlways;
            }

            return (EventLevel)level;
        }

        private void InitializeEventTrace(ConfigurationSection configSection, bool firstTimeInitialization)
        {
            SettingsConfigReader configReader = new SettingsConfigReader(configSection);
            string level = configReader.ReadTraceLevelKey();
            string keywords = configReader.ReadKeywordsKey();
            string includeEventList = configReader.ReadIncludeEventKey();
            string excludeEventList = configReader.ReadExcludeEventKey();

            TraceSinkFilter traceSinkFilter = new TraceSinkFilter(DefaultTracingLevel, DefaultKeyword);

            traceSinkFilter.Level = string.IsNullOrEmpty(level) ? DefaultTracingLevel : this.ConvertLevel(int.Parse(level));
            traceSinkFilter.Keywords = string.IsNullOrEmpty(keywords) ? DefaultKeyword : (EventKeywords)Convert.ToUInt64(keywords, 16);

            if (!string.IsNullOrEmpty(includeEventList))
            {
                this.ProcessEventList(traceSinkFilter, includeEventList, TraceSinkFilter.FilterType.Include);
            }

            if (!string.IsNullOrEmpty(excludeEventList))
            {
                this.ProcessEventList(traceSinkFilter, excludeEventList, TraceSinkFilter.FilterType.Exclude);
            }

            if (firstTimeInitialization)
            {
                // Assign sinkFilter for first time initialization only if any OnConfigurationPackageModified callbacks haven't modified it already
                Interlocked.CompareExchange(ref this.sinkFilter, traceSinkFilter, null);
            }
            else
            {
                this.sinkFilter = traceSinkFilter;
            }

            this.OnFilterUpdate();
        }

        private void ProcessEventList(TraceSinkFilter traceSinkFilter, string eventList, TraceSinkFilter.FilterType filterType)
        {
            var events = eventList.Split(this.filterValueSeparatorArray, System.StringSplitOptions.RemoveEmptyEntries);

            foreach (var e in events)
            {
                traceSinkFilter.AddFilter(e, filterType);
            }
        }
    }
}