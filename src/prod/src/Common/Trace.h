// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// Trace pointer in text tracing
#define TextTracePtrAs(ptr, type) static_cast<void*>(const_cast<type *>(static_cast<type const*>(ptr)))
#define TextTracePtr(ptr) TextTracePtrAs(ptr, void)
#define TextTraceThisAs(type) TextTracePtrAs(this, type)
#define TextTraceThis TextTracePtr(this)

// Trace pointer in structured tracing
#define TracePtrAs(ptr, type) reinterpret_cast<uint64>(static_cast<type const*>(ptr))
#define TracePtr(ptr) TracePtrAs(ptr, void)
#define TraceThisAs(type) TracePtrAs(this, type)
#define TraceThis TracePtr(this)

namespace Common
{
    class Config;

    class TextTraceWriter;

    class TraceProvider : IConfigUpdateSink
    {
        DENY_COPY(TraceProvider);
    public:
        TraceProvider(std::string const & name, std::string const & message, std::string const & symbol, Guid guid);

        static TraceProvider* StaticInit_Singleton();

        static Guid GetTraceGuid();

        static TraceProvider* GetSingleton()
        {
            return Singleton;
        }

        TraceEvent & CreateEvent(TraceTaskCodes::Enum taskId, USHORT eventId, StringLiteral eventName, LogLevel::Enum level, TraceChannelType::Enum channel, TraceKeywords::Enum keywords, StringLiteral format);

        static StringLiteral GetKeywordString(ULONGLONG keyword);

        static void SetDefaultLevel(TraceSinkType::Enum sinkType, LogLevel::Enum level)
        {
            StaticInit_Singleton()->InternalSetDefaultLevel(sinkType, level);
        }

        static void SetDefaultSamplingRatio(TraceSinkType::Enum sinkType, double samplingRatio)
        {
            StaticInit_Singleton()->InternalSetDefaultSamplingRatio(sinkType, samplingRatio);
        }

        static void LoadConfiguration(Config & config);        
        static void Test_ReloadConfiguration(Config & config);

        static void GenerateManifest(std::wstring const & path, std::wstring const & targetFile);

        virtual bool OnUpdate(std::wstring const & section, std::wstring const & key);
        virtual bool CheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted);

        std::wstring const & GetTraceId() const
        {
            return traceId_;
        }

        TraceEvent & TraceProvider::GetTextEvent(TraceTaskCodes::Enum taskId, LogLevel::Enum level);

        TextTraceWriter GetTextEventWriter(TraceTaskCodes::Enum taskId, LogLevel::Enum level);

        void AddEventSource(TraceTaskCodes::Enum taskId, StringLiteral taskName);

        void AddMapValue(StringLiteral mapName, uint key, std::string const & value);
        
        void AddBitMapValue(StringLiteral mapName, uint key, std::string const & value);

        bool AddFilter(TraceSinkType::Enum sinkType, std::wstring const & filter);

        void WriteManifest(TraceManifestGenerator & manifest) const;

        void InitializeEtwHandle();

    private:
        class EventSource;
        class ValueMapsEntry;
        class BitMapsEntry;
        typedef std::shared_ptr<ValueMapsEntry> ValueMapsEntrySPtr;
        typedef std::pair<std::string, ValueMapsEntrySPtr> ValueMapsPair;
        typedef std::map<std::string, ValueMapsEntrySPtr> ValueMaps;
        typedef std::shared_ptr<BitMapsEntry> BitMapsEntrySPtr;
        typedef std::pair<std::string, BitMapsEntrySPtr> BitMapsPair;
        typedef std::map<std::string, BitMapsEntrySPtr> BitMaps;

        static TraceProvider * Create();

        static void PinCurrentModule();

#if defined(PLATFORM_UNIX)
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        void SetLinuxStructuredTraceEnabled(bool linuxStructuredTraceEnabled);
#endif

        void LoadConfiguration(Config & config, std::wstring const & section, bool isUpdate);

        void Initialize();

        void AddEventSources();

        TraceTaskCodes::Enum GetTaskId(std::string const & taskName) const;

#if defined(PLATFORM_UNIX)
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        void RefreshEnableLinuxStructuredTraces(bool enabled);
#endif

        void RefreshFilterStates(TraceEvent & traceEvent);

        void RefreshFilterStates(TraceSinkType::Enum sinkType);

        void InternalSetDefaultLevel(TraceSinkType::Enum sinkType, LogLevel::Enum level);

        void InternalSetDefaultSamplingRatio(TraceSinkType::Enum sinkType, double samplingRatio);

        bool InternalAddFilter(TraceSinkType::Enum TraceSinkType, std::wstring const & filter);

        bool InternalAddFilter(TraceSinkType::Enum sinkType, std::string const & taskName, std::string const & eventName, LogLevel::Enum level, int samplingRatio);


        static int ConvertSamplingRatio(double ratio);

        static LogLevel::Enum ConvertLevel(int64 level);

        static TraceProvider* Singleton;

        std::string name_;
        std::wstring traceId_;
        std::string message_;
        std::string symbol_;
        Guid guid_;
        ::REGHANDLE etwHandle_;
        ULONGLONG keywords_;

        TraceSinkFilter filters_[3];

        std::unique_ptr<EventSource> sources_[TraceTaskCodes::MaxTask];
        ValueMaps maps_;
        BitMaps bitmaps_;

#if defined(PLATFORM_UNIX)
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        bool isLinuxstructuredTracesEnabled_;
#endif

        mutable RwLock lock_;
    };
}
