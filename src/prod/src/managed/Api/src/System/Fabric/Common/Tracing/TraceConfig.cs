// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Linq;

namespace System.Fabric.Common.Tracing
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics.CodeAnalysis;
    using System.Diagnostics.Tracing;
    using System.Fabric.Strings;

    internal static class TraceConfig
    {
        private const string SectionName = "Trace";
        private const string TraceEtwSectionName = "Trace/Etw";
        private const string TraceFileSectionName = "Trace/File";
        private const string TraceConsoleSectionName = "Trace/Console";
        private const string TraceLevelKey = "Level";
        private const string TraceSamplingKey = "Sampling";
        private const string TraceProvisionalFeatureStatus = "TraceProvisionalFeatureStatus";
        private const string FiltersKey = "Filters";
        private const string PathKey = "Path";
        private const string OptionKey = "Option";

#if DotNetCoreClrLinux
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        private const string CommonSectionName = "Common";
        private const string LinuxStructuredTracesEnabledParamName = "LinuxStructuredTracesEnabled";

        private static bool LinuxStructuredTracesEnabled = false;
#endif

        private static readonly char[] FilterValueSeparatorArray = { ',' };

        private static readonly object SyncLock = new object();
        private static readonly ConfigUpdateHandler UpdateHandler = new ConfigUpdateHandler();
        private static readonly ReadOnlyCollection<TraceSinkFilter> SinkFilters;

        private static IConfigReader configReader;

        static TraceConfig()
        {
            SinkFilters = new ReadOnlyCollection<TraceSinkFilter>(CreateSinkFilters());
        }

        // Initialize to prevent need for null check.
        public static event Action<TraceSinkType> OnFilterUpdate = sinkType => { };

#if DotNetCoreClrLinux
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        public static event Action<bool> OnLinuxStructuredTracesEnabledUpdate = (enabled) => { };
#endif

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static void InitializeFromConfigStore(bool forceUpdate = false)
        {
            lock (SyncLock)
            {
                if (configReader != null && !forceUpdate)
                {
                    return;
                }

                configReader = new ConfigReader(NativeConfigStore.FabricGetConfigStore(UpdateHandler));
            }

            InitializeFromConfigStore(configReader);
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static void InitializeFromConfigStore(IConfigReader configReaderOverride)
        {
            ReleaseAssert.IsTrue(configReaderOverride != null, "ConfigReaderOverride must be non-null value.");
            lock (SyncLock)
            {
                configReader = configReaderOverride;
            }

            var traceSections = configReader.GetSections();

            SetDefaultConfigFilters(SinkFilters);
            foreach (var section in traceSections)
            {
                InitializeTraceSink(section);
            }
#if DotNetCoreClrLinux

            // TODO - Following code will be removed once fully transitioned to structured traces in Linux
            // try to read from Common config section whether structured traces are enabled: Default is false.
            InitializeLinuxStructuredTraceEnabled();
#endif
        }

        /// <summary>
        /// Set default level for a sink type.
        /// </summary>
        /// <param name="sinkType">Type of the sink.</param>
        /// <param name="level">Default level for the sink.</param>
        public static void SetDefaultLevel(TraceSinkType sinkType, EventLevel level)
        {
            SinkFilters[(int)sinkType].DefaultLevel = level;
            OnFilterUpdate(sinkType);
        }

        /// <summary>
        /// Set default sampling ratio for a sink type.
        /// </summary>
        /// <param name="sinkType">Type of the sink.</param>
        /// <param name="samplingRatio">Default sampling ratio for the sink.</param>
        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static void SetDefaultSamplingRatio(TraceSinkType sinkType, double samplingRatio)
        {
            SinkFilters[(int)sinkType].DefaultSamplingRatio = ConvertSamplingRatio(samplingRatio);
            OnFilterUpdate(sinkType);
        }

        /// <summary>
        /// Add filter for a sink.
        /// </summary>
        /// <param name="sinkType">Type of the sink.</param>
        /// <param name="filter">The filter to be applied for the sink.
        /// Syntax: task.event@id:level
        /// </param>
        /// <returns>Whether the filter is set correctly.</returns>
        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static bool AddFilter(TraceSinkType sinkType, string filter)
        {
            if (!InternalAddFilter(sinkType, filter))
            {
                return false;
            }

            OnFilterUpdate(sinkType);
            return true;
        }

        /// <summary>
        /// Removes all filters based on source.
        /// </summary>
        /// <param name="sinkType">Type of the sink.</param>
        /// <param name="source">The source to be removed.
        /// Syntax: task.event@id:level
        /// </param>
        /// <returns>Whether the filter is removed correctly.</returns>
        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static bool RemoveFilter(TraceSinkType sinkType, string source)
        {
            if (!InternalRemoveFilter(sinkType, source))
            {
                return false;
            }

            OnFilterUpdate(sinkType);
            return true;
        }

        public static bool GetEventEnabledStatus(
            TraceSinkType sinkType, 
            EventLevel level, 
            EventTask taskId,
            string eventName)
        {
            lock (SyncLock)
            {
                int garbage;
                return SinkFilters[(int) sinkType].StaticCheck(
                    level,
                    taskId,
                    eventName,
                    out garbage);
            }
        }

        public static int GetEventSamplingRatio(
            TraceSinkType sinkType,
            EventLevel level,
            EventTask taskId,
            string eventName)
        {
            int samplingRatio;
            lock (SyncLock)
            {
                SinkFilters[(int) sinkType].StaticCheck(
                    level,
                    taskId,
                    eventName,
                    out samplingRatio);
            }

            return samplingRatio;
        }

        public static bool GetEventProvisionalFeatureStatus(TraceSinkType sinkType)
        {
            bool featureStatus;
            lock (SyncLock)
            {
                featureStatus = SinkFilters[(int)sinkType].ProvisionalFeatureStatus;
            }

            return featureStatus;
        }

#if DotNetCoreClrLinux
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        public static bool GetLinuxStructuredTracesEnabled()
        {
            return TraceConfig.LinuxStructuredTracesEnabled;
        }
#endif

        /// <summary>
        /// Add filter for a sink.
        /// </summary>
        /// <param name="sinkType">Type of the sink.</param>
        /// <param name="filter">The filter to be applied for the sink.
        /// Syntax: task.event@id:level
        /// </param>
        /// <returns>Whether the filter is set correctly.</returns>
        private static bool InternalAddFilter(TraceSinkType sinkType, string filter)
        {
            int index = filter.LastIndexOf(':');
            if (index < 0)
            {
                return false;
            }

            string source = filter.Substring(0, index).Trim();
            string levelString = filter.Substring(index + 1).Trim();

            int samplingRatio = 0;
            index = levelString.IndexOf('#');
            if (index > 0)
            {
                string ratioString = levelString.Substring(index + 1).Trim();
                levelString = levelString.Substring(0, index).Trim();

                double ratio;
                if (double.TryParse(ratioString, out ratio))
                {
                    samplingRatio = ConvertSamplingRatio(ratio);
                }
            }

            int level;
            if (!int.TryParse(levelString, out level))
            {
                return false;
            }

            string secondLevel = null;
            index = source.IndexOf('.');
            if (index >= 0)
            {
                secondLevel = source.Substring(index + 1).Trim();
                source = source.Substring(0, index).Trim();
            }

            return InternalAddFilter(sinkType, source, secondLevel, ConvertLevel(level), samplingRatio);
        }

        private static TraceSinkFilter[] CreateSinkFilters()
        {
            TraceSinkFilter[] filters = new TraceSinkFilter[(int)TraceSinkType.Max];
            for (int i = 0; i < filters.Length; i++)
            {
                TraceSinkType sink = (TraceSinkType)i;
                filters[i] = new TraceSinkFilter(sink, EventLevel.LogAlways);
            }

            SetDefaultConfigFilters(filters);

            return filters;
        }

        private static void SetDefaultConfigFilters(IReadOnlyList<TraceSinkFilter> filters)
        {
            for (int i = 0; i < filters.Count; i++)
            {
                TraceSinkType sink = (TraceSinkType)i;
                var level = sink == TraceSinkType.Console ? EventLevel.LogAlways : EventLevel.Informational;
                filters[i].DefaultLevel = level;
            } 
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        private static EventLevel ConvertLevel(int level)
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

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        private static int ConvertSamplingRatio(double ratio)
        {
            if (ratio < 1E-6)
            {
                return 0;
            }

            if (ratio > 1.0)
            {
                return 1;
            }

            return (int)((1.0 / ratio) + 0.5);
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        private static void InitializeTraceLevel(IConfigReader configStore, string configSection, TraceSinkType sinkType)
        {
            SinkFilters[(int)sinkType].DefaultLevel = configStore.ReadTraceLevelKey(configSection);
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        private static void InitializeTraceProvisionalFeatureStatus(IConfigReader configStore, string configSection, TraceSinkType sinkType)
        {
            SinkFilters[(int)sinkType].ProvisionalFeatureStatus = configStore.ReadTraceProvisionalFeatureStatus(configSection);
        }
        
        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        private static void InitializeTraceSamplingRatio(IConfigReader configStore, string configSection, TraceSinkType sinkType)
        {
            SinkFilters[(int)sinkType].DefaultSamplingRatio = configStore.ReadTraceSamplingKey(configSection);
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        private static void InitializeFilters(IConfigReader configStore, string configSection, TraceSinkType sinkType)
        {
            SinkFilters[(int)sinkType].ClearFilters();
            var filterString = configStore.ReadFiltersKey(configSection);
            if (!string.IsNullOrEmpty(filterString))
            {
                var tokens = filterString.Split(FilterValueSeparatorArray, System.StringSplitOptions.RemoveEmptyEntries);
                foreach (var token in tokens)
                {
                    InternalAddFilter(sinkType, token);
                }
            }
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        private static void InitializeTraceSink(string section)
        {
            if (string.Compare(section, TraceEtwSectionName, StringComparison.OrdinalIgnoreCase) == 0)
            {
                InitializeEtwTrace(configReader);
            }
            else if (string.Compare(section, TraceFileSectionName, StringComparison.OrdinalIgnoreCase) == 0)
            {
                InitializeTextTrace(configReader);
            }
            else if (string.Compare(section, TraceConsoleSectionName, StringComparison.OrdinalIgnoreCase) == 0)
            {
                InitializeConsoleTrace(configReader);
            }
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        private static void InitializeEtwTrace(IConfigReader configStore)
        {
            InitializeTraceLevel(configStore, TraceEtwSectionName, TraceSinkType.ETW);
            InitializeTraceSamplingRatio(configStore, TraceEtwSectionName, TraceSinkType.ETW);
            InitializeTraceProvisionalFeatureStatus(configStore, TraceEtwSectionName, TraceSinkType.ETW);
            InitializeFilters(configStore, TraceEtwSectionName, TraceSinkType.ETW);

            OnFilterUpdate(TraceSinkType.ETW);
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        private static void InitializeTextTrace(IConfigReader configStore)
        {
            var filePath = configStore.ReadPathKey();
            if (!string.IsNullOrEmpty(filePath))
            {
                TraceTextFileSink.SetPath(filePath);

                InitializeTraceLevel(configStore, TraceFileSectionName, TraceSinkType.TextFile);
                InitializeFilters(configStore, TraceFileSectionName, TraceSinkType.TextFile);

                var option = configStore.ReadOptionKey();
                if (!string.IsNullOrEmpty(option))
                {
                    TraceTextFileSink.SetOption(option);
                }

                OnFilterUpdate(TraceSinkType.TextFile);
            }
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        private static void InitializeConsoleTrace(IConfigReader configStore)
        {
            InitializeTraceLevel(configStore, TraceConsoleSectionName, TraceSinkType.Console);
            InitializeFilters(configStore, TraceConsoleSectionName, TraceSinkType.Console);

            OnFilterUpdate(TraceSinkType.Console);
        }

#if DotNetCoreClrLinux
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        // try to read from Common config section whether structured traces are enabled: Default is false.
        private static void InitializeLinuxStructuredTraceEnabled()
        {
            TraceConfig.LinuxStructuredTracesEnabled = configReader.ReadLinuxStructuredTracesEnabled();
            OnLinuxStructuredTracesEnabledUpdate(TraceConfig.LinuxStructuredTracesEnabled);
        }
#endif

        private static bool InternalRemoveFilter(TraceSinkType sinkType, string taskName)
        {
            string eventName = null;
            int index = taskName.IndexOf('.');
            if (index >= 0)
            {
                eventName = taskName.Substring(index + 1).Trim();
                taskName = taskName.Substring(0, index).Trim();
            }

            EventTask taskId;
            if (!string.IsNullOrEmpty(taskName))
            {
                taskId = FabricEvents.Events.GetEventTask(taskName);
                if (taskId == FabricEvents.Tasks.Max)
                {
                    return false;
                }

            }
            else
            {
                taskId = FabricEvents.Tasks.Max;
            }

            TraceSinkFilter filter = SinkFilters[(int)sinkType];
            lock (SyncLock)
            {
                filter.RemoveFilter(taskId, eventName);
            }

            return true;
        }

        /// <summary>
        /// Add filter for a sink.
        /// </summary>
        /// <param name="sink">Type of the sink.</param>
        /// <param name="taskName">Task name.</param>
        /// <param name="eventName">Event name.</param>
        /// <param name="level">Level for the filter.</param>
        /// <param name="samplingRatio">Sampling ratio for records below the level.</param>
        /// <returns>Whether the filter is set correctly.</returns>
        private static bool InternalAddFilter(TraceSinkType sink, string taskName, string eventName, EventLevel level, int samplingRatio)
        {
            EventTask taskId;
            if (!string.IsNullOrEmpty(taskName))
            {
                taskId = FabricEvents.Events.GetEventTask(taskName);

                if (taskId == FabricEvents.Tasks.Max)
                {
                    return false;
                }
            }
            else
            {
                taskId = FabricEvents.Tasks.Max;
            }

            TraceSinkFilter filter = SinkFilters[(int)sink];
            lock (SyncLock)
            {
                filter.AddFilter(taskId, eventName, level, samplingRatio);
            }

            return true;
        }

        private class ConfigUpdateHandler : IConfigStoreUpdateHandler
        {
            public bool OnUpdate(string sectionName, string keyName)
            {
                TraceConfig.InitializeTraceSink(sectionName);
#if DotNetCoreClrLinux
                // TODO - Following code will be removed once fully transitioned to structured traces in Linux
                // try to read from Common config section whether structured traces are enabled: Default is false.
                TraceConfig.InitializeLinuxStructuredTraceEnabled();
#endif

                return true;
            }

            public bool CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted)
            {
                throw new InvalidOperationException(StringResources.Error_InvalidOperation);
            }
        }

        public interface IConfigReader
        {
            IEnumerable<string> GetSections();

            string ReadOptionKey();

            string ReadFiltersKey(string configSection);

            int ReadTraceSamplingKey(string configSection);

            bool ReadTraceProvisionalFeatureStatus(string configSection);

            string ReadPathKey();

            EventLevel ReadTraceLevelKey(string configSection);

#if DotNetCoreClrLinux
            // TODO - Following code will be removed once fully transitioned to structured traces in Linux
            // try to read from Common config section whether structured traces are enabled: Default is false.
            bool ReadLinuxStructuredTracesEnabled();
#endif
        }

        private class ConfigReader : IConfigReader
        {
            private readonly IConfigStore configStore;

            public ConfigReader(IConfigStore configStore)
            {
                this.configStore = configStore;
            }

            public IEnumerable<string> GetSections()
            {
                return configStore.GetSections(SectionName);
            }

            public string ReadOptionKey()
            {
                return configStore.ReadUnencryptedString(TraceFileSectionName, OptionKey);
            }

            public string ReadFiltersKey(string configSection)
            {
                return configStore.ReadUnencryptedString(configSection, FiltersKey);
            }

            public int ReadTraceSamplingKey(string configSection)
            {
                var samplingString = configStore.ReadUnencryptedString(configSection, TraceSamplingKey);
                int eventSamplingRatio = 0;
                if (!string.IsNullOrEmpty(samplingString))
                {
                    double ratio;
                    if (double.TryParse(samplingString, out ratio))
                    {
                        eventSamplingRatio = ConvertSamplingRatio(ratio);
                    }
                }

                return eventSamplingRatio;
            }

            public bool ReadTraceProvisionalFeatureStatus(string configSection)
            {
                string provisionalFeatureControlFlag = configStore.ReadUnencryptedString(configSection, TraceProvisionalFeatureStatus);

                // Default Status of Feature is set to True.
                bool provisionalStatus = true;
                if (!string.IsNullOrEmpty(provisionalFeatureControlFlag))
                {
                    if (!bool.TryParse(provisionalFeatureControlFlag, out provisionalStatus))
                    {
                        provisionalStatus = true;
                    }
                }

                return provisionalStatus;
            }

            public string ReadPathKey()
            {
                return configStore.ReadUnencryptedString(TraceFileSectionName, PathKey);
            }

            public EventLevel ReadTraceLevelKey(string configSection)
            {
                var levelString = configStore.ReadUnencryptedString(configSection, TraceLevelKey);
                var eventLevel = EventLevel.Informational;
                if (!string.IsNullOrEmpty(levelString))
                {
                    int level;
                    if (int.TryParse(levelString, out level))
                    {
                        eventLevel = ConvertLevel(level);
                    }
                }

                return eventLevel;
            }

#if DotNetCoreClrLinux
            // TODO - Following code will be removed once fully transitioned to structured traces in Linux
            // try to read from Common config section whether structured traces are enabled: Default is false.
            public bool ReadLinuxStructuredTracesEnabled()
            {
                string enabledStr = configStore.ReadUnencryptedString(CommonSectionName, LinuxStructuredTracesEnabledParamName);

                bool enabled;
                bool res = bool.TryParse(enabledStr, out enabled);

                // default is false in case it doesn't find the parameter or it fails to parse its value to bool.
                return res && enabled;
            }
#endif
        }
    }
}