// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    TraceProvider* TraceProvider::Singleton = TraceProvider::StaticInit_Singleton();

    TextTraceComponent<TraceTaskCodes::General> Trace;
    
    GlobalWString EtwTraceSection = make_global<wstring>(L"Trace/Etw");
    GlobalWString FileTraceSection = make_global<wstring>(L"Trace/File");
    GlobalWString ConsoleTraceSection = make_global<wstring>(L"Trace/Console");

#if defined(PLATFORM_UNIX)
    // TODO - Following code will be removed once fully transitioned to structured traces in Linux
    GlobalWString CommonSection = make_global<wstring>(L"Common");
    GlobalWString LinuxStructuredTracesEnabled = make_global<wstring>(L"LinuxStructuredTracesEnabled");
#endif

    class TraceProvider::EventSource
    {
        DENY_COPY(EventSource);

    public:
        EventSource(TraceTaskCodes::Enum taskId, StringLiteral taskName, TraceProvider & provider)
            :   taskName_(taskName)
        {
            for (int i = 0; i < TraceEvent::TextEvents; i++)
            {
                LogLevel::Enum level = static_cast<LogLevel::Enum>(LogLevel::Error + i);

                unique_ptr<TraceEvent> textEvent = make_unique<TraceEvent>(
                    taskId,
                    taskName,
                    GetTextEventId(level),
                    GetTextEventName(level),
                    level,
                    (level <= LogLevel::Enum::Warning) ? TraceChannelType::Admin : TraceChannelType::Debug,
                    TraceKeywords::Default,
                    "{2}",
                    provider.etwHandle_,
                    provider.filters_[TraceSinkType::Console]);

                provider.RefreshFilterStates(*textEvent);

                RegisterEvent(move(textEvent));
            }
        }

        StringLiteral GetTaskName() const
        {
            return taskName_;
        }

        static StringLiteral GetTextEventName(LogLevel::Enum level)
        {
            switch (level)
            {
            case LogLevel::Error:
                return "ErrorText";
            case LogLevel::Warning:
                return "WarningText";
            case LogLevel::Info:
                return "InfoText";
            case LogLevel::Noise:
                return "NoiseText";
            default:
                throw runtime_error("Invalid text trace level");
            }
        }

        static USHORT GetTextEventId(LogLevel::Enum level)
        {
            return static_cast<USHORT>(level - LogLevel::Error);
        }

        TraceEvent & GetTextEvent(LogLevel::Enum level)
        {
            USHORT index = GetTextEventId(level);
            if (!events_[index])
            {
                throw runtime_error("Event not defined!");
            }

            return *events_[index];
        }

        TraceEvent & RegisterEvent(unique_ptr<TraceEvent> && traceEvent)
        {
            USHORT index = traceEvent->GetLocalId();
            if (events_[index])
            {
                if (traceEvent->GetName() != events_[index]->GetName())
                {
                    throw runtime_error("A different event already defined");
                }
            }
            else
            {
                events_[index] = move(traceEvent);
            }

            return *events_[index];
        }

        void RefreshFilterStates(TraceSinkType::Enum sinkType, TraceSinkFilter const & filter)
        {
            for (int i = 0; i < TraceEvent::PerTaskEvents; i++)
            {
                if (events_[i])
                {
                    events_[i]->RefreshFilterStates(sinkType, filter);
                }
            }
        }

#if defined(PLATFORM_UNIX)
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        void RefreshEnableLinuxStructuredTraces(bool enabled)
        {
            for (int i = 0; i < TraceEvent::PerTaskEvents; i++)
            {
                if (events_[i])
                {
                    events_[i]->RefreshEnableLinuxStructuredTraces(enabled);
                }
            }
        }
#endif

        void WriteManifest(TraceManifestGenerator & manifest, USHORT taskId)
        {
            StringWriterA & taskWriter = manifest.GetTaskWriter();

            taskWriter << "          <task" << "\r\n";
            taskWriter << "              message=\"" << manifest.StringResource(taskName_) << "\"" << "\r\n";
            taskWriter << "              name=\"" << taskName_ << "\"" << "\r\n";
            taskWriter << "              value=\"" << taskId << "\"" << "\r\n";
            taskWriter << "              />\r\n";

            for (int i = 0; i < TraceEvent::PerTaskEvents; i++)
            {
                if (events_[i])
                {
                    events_[i]->WriteManifest(manifest);
                }
            }
        }

    private:
        StringLiteral taskName_;
        std::unique_ptr<TraceEvent> events_[TraceEvent::PerTaskEvents];
    };

    class TraceProvider::ValueMapsEntry
    {
        DENY_COPY(ValueMapsEntry);

    public:
        ValueMapsEntry(std::string const & mapName) 
            : mapName_(mapName)
            , mapValues_()
        { 
        }

        void AddMapValue(uint key, std::string const & value)
        {
            auto it = mapValues_.find(key);
            if (it != mapValues_.end())
            {
                throw runtime_error("Map key/value already defined");
            }
            mapValues_.insert(make_pair(key, value));
        }

        void WriteManifest(TraceManifestGenerator & manifest)
        {
            StringWriterA & mapWriter = manifest.GetMapWriter();

            mapWriter << "          <valueMap name=\"" << mapName_ << "\">\r\n";

            for (auto it = mapValues_.begin(); it != mapValues_.end(); ++it)
            {
                mapWriter << "              <map" << "\r\n";
                mapWriter << "                  value=\"" << it->first << "\"\r\n";
                mapWriter << "                  message=\"" << manifest.StringResource(it->second) << "\"\r\n";
                mapWriter << "                  />\r\n";
            }

            mapWriter << "          </valueMap>\r\n";
        }

    private:
        std::string mapName_;
        std::map<uint, std::string> mapValues_;
    };

    class TraceProvider::BitMapsEntry
    {
        DENY_COPY(BitMapsEntry);

    public:
        BitMapsEntry(std::string const & mapName) 
            : mapName_(mapName)
            , mapValues_()
        { 
        }

        void AddMapValue(uint key, std::string const & value)
        {
            auto it = mapValues_.find(key);
            if (it != mapValues_.end())
            {
                throw runtime_error("Map key/value already defined");
            }
            mapValues_.insert(make_pair(key, value));
        }

        void WriteManifest(TraceManifestGenerator & manifest)
        {
            StringWriterA & mapWriter = manifest.GetMapWriter();

            mapWriter << "          <bitMap name=\"" << mapName_ << "\">\r\n";

            for (auto it = mapValues_.begin(); it != mapValues_.end(); ++it)
            {
                mapWriter << "              <map" << "\r\n";
                mapWriter << "                  value=\"";
                mapWriter.Write("0x{0:X}", it->first);
                mapWriter << "\"\r\n";
                mapWriter << "                  message=\"" << manifest.StringResource(it->second) << "\"\r\n";
                mapWriter << "                  />\r\n";
            }

            mapWriter << "          </bitMap>\r\n";
        }

    private:
        std::string mapName_;
        std::map<uint, std::string> mapValues_;
    };

    Guid TraceProvider::GetTraceGuid()
    {
        return Guid(L"cbd93bc2-71e5-4566-b3a7-595d8eeca6e8");
    }

    TraceProvider* TraceProvider::StaticInit_Singleton()
    {
        static TraceProvider* provider = Create();

        PinCurrentModule();

        return provider;
    }

    void TraceProvider::PinCurrentModule()
    {
#if !defined(PLATFORM_UNIX)
        // Oftentimes, our code does not wait for threadpool callback completion, this means our DLLs, such
        // as FabricRuntime.dll, FabricClient.dll and FabricCommon.dll, cannot be unloaded. Also, we have 
        // many TAEF CITs, those DLL cannot be unloaded either. So we need to pin our module always, no
        // matter whether we know we are in a DLL. Otherwise we will have to do this for each product DLL
        // and each TAEF CIT DLL individually.

        // See http://msdn.microsoft.com/en-us/library/ms683200(v=VS.85).aspx
        // The second parameter of GetModuleHandleEx can be a pointer to any
        // address mapped to the module.  We define a static to use as that
        // pointer.
        static int staticInThisModule = 0;

        HMODULE currentModule = NULL;
        BOOL success = ::GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCTSTR>(&staticInThisModule),
            &currentModule);

        ASSERT_IF(success == 0, "GetModuleHandleEx");
#endif
    }

    TraceProvider::TraceProvider(std::string const & name, std::string const & message, std::string const & symbol, Guid guid)
        :   name_(name),
        traceId_(wformatString(name)),
        message_(message),
        symbol_(symbol),
        guid_(guid),
        etwHandle_(NULL),
        keywords_(0)
    {
        filters_[TraceSinkType::ETW].SetDefaultLevel(LogLevel::Info);
        filters_[TraceSinkType::TextFile].SetDefaultLevel(LogLevel::Info);
        filters_[TraceSinkType::Console].SetDefaultLevel(LogLevel::Silent);

#if defined(PLATFORM_UNIX)
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        isLinuxstructuredTracesEnabled_ = false;
#endif

    }

    void TraceProvider::AddEventSource(TraceTaskCodes::Enum taskId, StringLiteral taskName)
    {
        AcquireWriteLock lock(lock_);
        {
            if (sources_[taskId])
            {
                throw runtime_error("Task already defined");
            }

            sources_[taskId] = make_unique<EventSource>(taskId, taskName, *this);
        }
    }

    void TraceProvider::AddMapValue(StringLiteral nameLiteral, uint key, std::string const & value)
    {
        AcquireWriteLock lock(lock_);

        std::string mapName(nameLiteral.begin());

        auto it = maps_.find(mapName);
        if (it == maps_.end())
        {
            it = maps_.insert(ValueMapsPair(mapName, make_shared<ValueMapsEntry>(mapName))).first;
        }

        it->second->AddMapValue(key, value);
    }

    void TraceProvider::AddBitMapValue(StringLiteral nameLiteral, uint key, std::string const & value)
    {
        AcquireWriteLock lock(lock_);

        std::string mapName(nameLiteral.begin());

        auto it = bitmaps_.find(mapName);
        if (it == bitmaps_.end())
        {
            it = bitmaps_.insert(BitMapsPair(mapName, make_shared<BitMapsEntry>(mapName))).first;
        }

        it->second->AddMapValue(key, value);
    }

    TraceEvent & TraceProvider::GetTextEvent(TraceTaskCodes::Enum taskId, LogLevel::Enum level)
    {
        AcquireReadLock lock(lock_);
        {
            if (!sources_[taskId])
            {
                throw runtime_error("Source not defined");
            }

            return sources_[taskId]->GetTextEvent(level);
        }
    }

    TextTraceWriter TraceProvider::GetTextEventWriter(TraceTaskCodes::Enum taskId, LogLevel::Enum level)
    {
        return TextTraceWriter(GetTextEvent(taskId, level));
    }

    TraceTaskCodes::Enum TraceProvider::GetTaskId(std::string const & taskName) const
    {
        for (USHORT i = 0; i < TraceTaskCodes::MaxTask; i++)
        {
            if (sources_[i] &&  taskName == sources_[i]->GetTaskName())
            {
                return static_cast<TraceTaskCodes::Enum>(i);
            }
        }

        return TraceTaskCodes::MaxTask;
    }

    void TraceProvider::RefreshFilterStates(TraceEvent & traceEvent)
    {
        traceEvent.RefreshFilterStates(TraceSinkType::ETW, filters_[TraceSinkType::ETW]);
        traceEvent.RefreshFilterStates(TraceSinkType::TextFile, filters_[TraceSinkType::TextFile]);
        traceEvent.RefreshFilterStates(TraceSinkType::Console, filters_[TraceSinkType::Console]);
    }

    void TraceProvider::RefreshFilterStates(TraceSinkType::Enum sinkType)
    {
        for (USHORT i = 0; i < TraceTaskCodes::MaxTask; i++)
        {
            if (sources_[i])
            {
                sources_[i]->RefreshFilterStates(sinkType, filters_[sinkType]);
            }
        }
    }

#if defined(PLATFORM_UNIX)
    // TODO - Following code will be removed once fully transitioned to structured traces in Linux
    void TraceProvider::RefreshEnableLinuxStructuredTraces(bool enabled)
    {
        for (USHORT i = 0; i < TraceTaskCodes::MaxTask; i++)
        {
            if (sources_[i])
            {
                sources_[i]->RefreshEnableLinuxStructuredTraces(enabled);
            }
        }
    }
#endif

    TraceEvent & TraceProvider::CreateEvent(
        TraceTaskCodes::Enum taskId,
        USHORT eventId,
        StringLiteral eventName,
        LogLevel::Enum level,
        TraceChannelType::Enum channel,
        TraceKeywords::Enum keywords,
        StringLiteral format)
    {
        AcquireWriteLock lock(lock_);
        {
            unique_ptr<EventSource> const & source = sources_[taskId];
            if (!source)
            {
                throw runtime_error("Source not defined");
            }

            unique_ptr<TraceEvent> traceEvent = make_unique<TraceEvent>(taskId, source->GetTaskName(), eventId, eventName, level, channel, keywords, format, etwHandle_, filters_[TraceSinkType::Console]);

            RefreshFilterStates(*traceEvent);
            keywords_ |= keywords;

            return source->RegisterEvent(move(traceEvent));
        }
    }

    void TraceProvider::InternalSetDefaultLevel(TraceSinkType::Enum sinkType, LogLevel::Enum level)
    {
        AcquireWriteLock lock(lock_);
        {
            filters_[sinkType].SetDefaultLevel(level);
            RefreshFilterStates(sinkType);
        }
    }

    void TraceProvider::InternalSetDefaultSamplingRatio(TraceSinkType::Enum sinkType, double samplingRatio)
    {
        AcquireWriteLock lock(lock_);
        {
            filters_[sinkType].SetDefaultSamplingRatio(ConvertSamplingRatio(samplingRatio));
            RefreshFilterStates(sinkType);
        }
    }

    bool TraceProvider::AddFilter(TraceSinkType::Enum sinkType, std::wstring const & filter)
    {
        AcquireWriteLock lock(lock_);
        {
            if (!InternalAddFilter(sinkType, filter))
            {
                return false;
            }

            RefreshFilterStates(sinkType);
        }

        return true;
    }

    bool TraceProvider::InternalAddFilter(TraceSinkType::Enum sinkType, wstring const & filter)
    {
        size_t index = filter.find(':');
        if (index == wstring::npos)
        {
            return false;
        }

        wstring source = filter.substr(0, index);
        wstring levelString = filter.substr(index + 1);
        StringUtility::TrimWhitespaces(levelString);

        int samplingRatio = 0;
        index = levelString.find('#');
        if (index != wstring::npos)
        {
            wstring ratioString = levelString.substr(index + 1);
            StringUtility::TrimWhitespaces(ratioString);

            samplingRatio = ConvertSamplingRatio(Double_Parse(ratioString));
            levelString = levelString.substr(0, index);
            StringUtility::TrimWhitespaces(levelString);
        }

        int64 level;
        if (!TryParseInt64(levelString, level))
        {
            return false;
        }

        wstring secondLevel;

        index = source.find('.');
        if (index != wstring::npos)
        {
            secondLevel = source.substr(index + 1);
            StringUtility::TrimWhitespaces(secondLevel);

            source = source.substr(0, index);
        }

        StringUtility::TrimWhitespaces(source);

        std::string taskName;
        StringUtility::UnicodeToAnsi(source, taskName);

        std::string eventName;
        StringUtility::UnicodeToAnsi(secondLevel, eventName);

        InternalAddFilter(sinkType, taskName, eventName, ConvertLevel(level), samplingRatio);

        return true;
    }

    bool TraceProvider::InternalAddFilter(TraceSinkType::Enum sinkType, std::string const & taskName, std::string const & eventName, LogLevel::Enum level, int samplingRatio)
    {
        TraceTaskCodes::Enum taskId = Singleton->GetTaskId(taskName);
        if (taskId == TraceTaskCodes::MaxTask)
        {
            return false;
        }

        filters_[sinkType].AddFilter(taskId, eventName, level, samplingRatio);

        return true;
    }

    int TraceProvider::ConvertSamplingRatio(double ratio)
    {
        if (ratio < 1E-6)
        {
            return 0;
        }

        if (ratio > 1.0)
        {
            return 1;
        }

        return static_cast<int>(1.0 / ratio + 0.5);
    }

    bool TraceProvider::OnUpdate(std::wstring const & section, std::wstring const &)
    {
        Config config;
        LoadConfiguration(config, section, true);

        return true;
    }

    bool TraceProvider::CheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted)
    {
        UNREFERENCED_PARAMETER(section);
        UNREFERENCED_PARAMETER(key);
        UNREFERENCED_PARAMETER(value);
        UNREFERENCED_PARAMETER(isEncrypted);

        return true;
    }

    void TraceProvider::LoadConfiguration(Config & config)
    {
        static volatile LONG loaded = 0;

        if (InterlockedIncrement(&loaded) == 1)
        {
            config.RegisterForUpdate(*EtwTraceSection, StaticInit_Singleton());
            config.RegisterForUpdate(*FileTraceSection, StaticInit_Singleton());
            config.RegisterForUpdate(*ConsoleTraceSection, StaticInit_Singleton());

            StringCollection sections;
            config.GetSections(sections, L"Trace");
            for (size_t i = 0; i < sections.size(); ++i)
            {
                StaticInit_Singleton()->LoadConfiguration(config, sections[i], false);
            }

#if defined(PLATFORM_UNIX)
            // TODO - Following code will be removed once fully transitioned to structured traces in Linux
            // This is needed to allow reading from manifest whether structured traces are enabled..
            config.RegisterForUpdate(*CommonSection, StaticInit_Singleton());

            StaticInit_Singleton()->LoadConfiguration(config, *CommonSection, false);
#endif
        }
    }

#if defined(PLATFORM_UNIX)
    // TODO - Following code will be removed once fully transitioned to structured traces in Linux
    void TraceProvider::SetLinuxStructuredTraceEnabled(bool linuxStructuredTracesEnabled)
    {
        // change to isLinuxStructuredTraces_ (not doing now to not starting build from scratch)
        isLinuxstructuredTracesEnabled_ = linuxStructuredTracesEnabled;
    }
#endif

    void TraceProvider::Test_ReloadConfiguration(Config & config)
    {
        StringCollection sections;
        config.GetSections(sections, L"Trace");
        for (size_t i = 0; i < sections.size(); ++i)
        {
            StaticInit_Singleton()->LoadConfiguration(config, sections[i], true);
        }
    }

    void TraceProvider::LoadConfiguration(Config & config, wstring const & section, bool isUpdate)
    {

#if defined(PLATFORM_UNIX)
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        if (section == CommonSection)
        {
            std::wstring structuredTracesEnabled;
            std::wstring defaultValue = L"false";
            config.ReadUnencryptedConfig(section, *LinuxStructuredTracesEnabled, structuredTracesEnabled, defaultValue);
            bool enableLinuxStructuredTraces = structuredTracesEnabled == L"true" || structuredTracesEnabled == L"True";
            RefreshEnableLinuxStructuredTraces(enableLinuxStructuredTraces);

            return;
        }
#endif

        StringCollection filters;
        config.ReadUnencryptedConfig<StringCollection>(section, L"Filters", filters, filters);

        TraceSinkType::Enum sinkType;
        if (section == FileTraceSection)
        {
            sinkType = TraceSinkType::TextFile;

            std::wstring filePath;
            config.ReadUnencryptedConfig<wstring>(section, L"Path", filePath, L"fabric.trace");

            std::wstring option;
            config.ReadUnencryptedConfig<wstring>(section, L"Option", option, L"");

            if (isUpdate || TraceTextFileSink::GetPath().empty())
            {
                TraceTextFileSink::SetPath(filePath);
            }

            if (!option.empty())
            {
                TraceTextFileSink::SetOption(option);
            }
        }
        else if (section == ConsoleTraceSection)
        {
            sinkType = TraceSinkType::Console;
        }
        else if (section == EtwTraceSection)
        {
            sinkType = TraceSinkType::ETW;
        }
        else
        {
            return;
        }

        AcquireWriteLock lock(lock_);
        {
            int sinkLevel;
            config.ReadUnencryptedConfig<int>(section, L"Level", sinkLevel, LogLevel::Info);
            filters_[sinkType].SetDefaultLevel(ConvertLevel(sinkLevel));

            if (sinkType == TraceSinkType::ETW)
            {
                wstring sampling;
                config.ReadUnencryptedConfig<wstring>(section, L"Sampling", sampling, L"0");
                filters_[sinkType].SetDefaultSamplingRatio(ConvertSamplingRatio(Double_Parse(sampling)));
            }

            filters_[sinkType].ClearFilters();

            for (wstring const & filter : filters)
            {
                InternalAddFilter(sinkType, filter);
            }

            RefreshFilterStates(sinkType);
        }
    }

    StringLiteral TraceProvider::GetKeywordString(ULONGLONG keyword)
    {
        switch (keyword)
        {
        case TraceKeywords::Default:
            return "Default";
        case TraceKeywords::AppInstance:
            return "AppInstance";
        case TraceKeywords::ForQuery:
            return "ForQuery";
        case TraceKeywords::CustomerInfo:
            return "CustomerInfo";
        case TraceKeywords::DataMessaging:
            return "DataMessaging";
        case TraceKeywords::DataMessagingAll:
            return "DataMessagingAll";
        default:
            throw runtime_error("Invalid keyword");
        }
    }

    void TraceProvider::WriteManifest(TraceManifestGenerator & manifest) const
    {
        AcquireReadLock lock(lock_);
        {
            StringWriterA & keywordWriter = manifest.GetKeywordWriter();
            ULONGLONG keyword = 1;
            do
            {
                if ((keywords_ & keyword) == keyword)
                {
                    StringLiteral keywordString = GetKeywordString(keyword);
                    keywordWriter << "          <keyword" << "\r\n";
                    keywordWriter << "              mask=\"0x" << formatString("{0:x}", keyword) << "\"" << "\r\n";
                    keywordWriter << "              message=\"" << manifest.StringResource(keywordString) << "\"" << "\r\n";
                    keywordWriter << "              name=\"" << keywordString << "\"" << "\r\n";
                    keywordWriter << "              />\r\n";
                }

                keyword <<= 1;
            }
            while (keyword > 1);

            for (USHORT i = 0; i < TraceTaskCodes::MaxTask; i++)
            {
                if (sources_[i])
                {
                    sources_[i]->WriteManifest(manifest, i);
                }
            }

            for (auto it = maps_.begin(); it != maps_.end(); ++it)
            {
                it->second->WriteManifest(manifest);
            }

            for (auto it = bitmaps_.begin(); it != bitmaps_.end(); ++it)
            {
                it->second->WriteManifest(manifest);
            }
        }
    }

    void TraceProvider::GenerateManifest(std::wstring const & path, std::wstring const & targetFile)
    {
        TraceManifestGenerator manifest;
        Singleton->WriteManifest(manifest);

        std::string asciiTargetFile;
        StringUtility::UnicodeToAnsi(targetFile, asciiTargetFile);

        FileWriter writer;
        auto error = writer.TryOpen(path);
        if (!error.IsSuccess())
        {
            Assert::CodingError("Failed to open {0} : error={1}", path, error);
        }

        writer.WriteUnicodeBOM();
        manifest.Write(writer, Singleton->guid_, Singleton->name_, Singleton->message_, Singleton->symbol_, asciiTargetFile);
        writer.Close();
    }

    LogLevel::Enum TraceProvider::ConvertLevel(int64 level)
    {
        if (level > static_cast<int64>(LogLevel::Noise))
        {
            return LogLevel::Noise;
        }

        if (level <= static_cast<int64>(LogLevel::Silent))
        {
            return LogLevel::Silent;
        }

        return static_cast<LogLevel::Enum>(level);
    }

    TraceProvider* TraceProvider::Create()
    {
        TraceProvider * provider = new TraceProvider(
            "Microsoft-ServiceFabric",
            "Microsoft-Service Fabric",
            "Microsoft_ServiceFabric",
            TraceProvider::GetTraceGuid());

        provider->Initialize();

        return provider;
    }

    void TraceProvider::InitializeEtwHandle()
    {
        ULONG ulResult = EventRegister(&guid_.AsGUID(), NULL, NULL, &etwHandle_);
        if (ulResult != ERROR_SUCCESS)
        {
            throw runtime_error("Event Source Registration Failed");
        }
    }

    void TraceProvider::Initialize()
    {
        InitializeEtwHandle();

        AddEventSources();
    }

    void TraceProvider::AddEventSources()
    {
        AddEventSource(TraceTaskCodes::General, "General");
        AddEventSource(TraceTaskCodes::Java, "Java");
        AddEventSource(TraceTaskCodes::Managed, "Managed");
        AddEventSource(TraceTaskCodes::Common, "Common");
        AddEventSource(TraceTaskCodes::Config, "Config");
        AddEventSource(TraceTaskCodes::Timer, "Timer");
        AddEventSource(TraceTaskCodes::AsyncExclusiveLock, "AsyncExclusiveLock");
        AddEventSource(TraceTaskCodes::PerfMonitor, "PerfMonitor");

        AddEventSource(TraceTaskCodes::LeaseAgent, "LeaseAgent");
        AddEventSource(TraceTaskCodes::Transport, "Transport");
        AddEventSource(TraceTaskCodes::IPC, "IPC");
        AddEventSource(TraceTaskCodes::UnreliableTransport, "UnreliableTransport");

        AddEventSource(TraceTaskCodes::P2P, "P2P");
        AddEventSource(TraceTaskCodes::RoutingTable, "RoutingTable");
        AddEventSource(TraceTaskCodes::Token, "Token");
        AddEventSource(TraceTaskCodes::VoteProxy, "VoteProxy");
        AddEventSource(TraceTaskCodes::VoteManager, "VoteManager");
        AddEventSource(TraceTaskCodes::Bootstrap, "Bootstrap");
        AddEventSource(TraceTaskCodes::Join, "Join");
        AddEventSource(TraceTaskCodes::JoinLock, "JoinLock");
        AddEventSource(TraceTaskCodes::Gap, "Gap");
        AddEventSource(TraceTaskCodes::Lease, "Lease");
        AddEventSource(TraceTaskCodes::Arbitration, "Arbitration");
        AddEventSource(TraceTaskCodes::Routing, "Routing");
        AddEventSource(TraceTaskCodes::Broadcast, "Broadcast");
        AddEventSource(TraceTaskCodes::Multicast, "Multicast");
        AddEventSource(TraceTaskCodes::SiteNode, "SiteNode");

        AddEventSource(TraceTaskCodes::Reliability, "Reliability");
        AddEventSource(TraceTaskCodes::Replication, "RE");
        AddEventSource(TraceTaskCodes::OperationQueue, "OQ");
        AddEventSource(TraceTaskCodes::Api, "Api");
        AddEventSource(TraceTaskCodes::CRM, "CRM");
        AddEventSource(TraceTaskCodes::MCRM, "MasterCRM");
        AddEventSource(TraceTaskCodes::FM, "FM");
        AddEventSource(TraceTaskCodes::RA, "RA");
        AddEventSource(TraceTaskCodes::RAP, "RAP");
        AddEventSource(TraceTaskCodes::FailoverUnit, "FT");
        AddEventSource(TraceTaskCodes::FMM, "FMM");
        AddEventSource(TraceTaskCodes::PLB, "PLB");
        AddEventSource(TraceTaskCodes::PLBM, "PLBM");

        AddEventSource(TraceTaskCodes::EseStore, "EseStore");
        AddEventSource(TraceTaskCodes::RocksDbStore, "RocksDbStore");
        AddEventSource(TraceTaskCodes::ReplicatedStore, "REStore");
        AddEventSource(TraceTaskCodes::ReplicatedStoreStateMachine, "REStoreSM");
        AddEventSource(TraceTaskCodes::SqlStore, "SqlStore");
        AddEventSource(TraceTaskCodes::KeyValueStore, "KeyValueStore");

        AddEventSource(TraceTaskCodes::NM, "NM");
        AddEventSource(TraceTaskCodes::NamingStoreService, "NamingStoreService");
        AddEventSource(TraceTaskCodes::NamingGateway, "NamingGateway");
        AddEventSource(TraceTaskCodes::NamingCommon, "NamingCommon");
        AddEventSource(TraceTaskCodes::HealthClient, "HealthClient");

        AddEventSource(TraceTaskCodes::Hosting, "Hosting");
        AddEventSource(TraceTaskCodes::FabricWorkerProcess, "FabricWorkerProcess");
        AddEventSource(TraceTaskCodes::ClrRuntimeHost, "ClrRuntimeHost");
        AddEventSource(TraceTaskCodes::FabricTypeHost, "FabricTypeHost");

        AddEventSource(TraceTaskCodes::ServiceGroupCommon, "SG");
        AddEventSource(TraceTaskCodes::ServiceGroupStateful, "SGSF");
        AddEventSource(TraceTaskCodes::ServiceGroupStateless, "SGSL");
        AddEventSource(TraceTaskCodes::ServiceGroupStatefulMember, "SGSFM");
        AddEventSource(TraceTaskCodes::ServiceGroupStatelessMember, "SGSLM");
        AddEventSource(TraceTaskCodes::ServiceGroupReplication, "SGRE");

        AddEventSource(TraceTaskCodes::BwTree, "BWT");
        AddEventSource(TraceTaskCodes::BwTreeVolatileStorage, "BWTV");
        AddEventSource(TraceTaskCodes::BwTreeStableStorage, "BWTP");

        AddEventSource(TraceTaskCodes::FabricNode, "FabricNode");
        AddEventSource(TraceTaskCodes::ClusterManagerTransport, "ClusterManagerTransport");
        AddEventSource(TraceTaskCodes::ImageStore, "ImageStore");
        AddEventSource(TraceTaskCodes::NativeImageStoreClient, "NativeImageStoreClient");
        AddEventSource(TraceTaskCodes::ClusterManager, "CM");
        AddEventSource(TraceTaskCodes::InfrastructureService, "IS");
        AddEventSource(TraceTaskCodes::FaultAnalysisService, "FAS");
        AddEventSource(TraceTaskCodes::UpgradeOrchestrationService, "UOS");
        AddEventSource(TraceTaskCodes::CentralSecretService, "CentralSecretService");
        AddEventSource(TraceTaskCodes::LocalSecretService, "LocalSecretService");
        AddEventSource(TraceTaskCodes::ResourceManager, "ResourceManager");
        AddEventSource(TraceTaskCodes::RepairService, "RepairService");
        AddEventSource(TraceTaskCodes::RepairManager, "RM");

        AddEventSource(TraceTaskCodes::ServiceModel, "ServiceModel");

        AddEventSource(TraceTaskCodes::LeaseLayer, "LeaseLayer");
        
        AddEventSource(TraceTaskCodes::DNS, "DNS");

        AddEventSource(TraceTaskCodes::FabricSetup, "FabricSetup");

        AddEventSource(TraceTaskCodes::TestSession, "TestSession");

        AddEventSource(TraceTaskCodes::HttpApplicationGateway, "ReverseProxy");

        AddEventSource(TraceTaskCodes::Query, "Query");
        
        AddEventSource(TraceTaskCodes::HealthManagerCommon, "HMCommon");

        AddEventSource(TraceTaskCodes::HealthManager, "HM");

        AddEventSource(TraceTaskCodes::SystemServices, "SystemServices");

        AddEventSource(TraceTaskCodes::HttpGateway, "HttpGateway");

        AddEventSource(TraceTaskCodes::FabricActivator, "FabricActivator");

        AddEventSource(TraceTaskCodes::Client, "Client");

        AddEventSource(TraceTaskCodes::ReliableMessagingSession, "ReliableSession");

        AddEventSource(TraceTaskCodes::ReliableMessagingStream, "ReliableStream");

        AddEventSource(TraceTaskCodes::FileStoreService, "FileStoreService");

        AddEventSource(TraceTaskCodes::TokenValidationService, "TokenValidationService");

        AddEventSource(TraceTaskCodes::AccessControl, "AccessControl");

        AddEventSource(TraceTaskCodes::RepairPolicyEngineService, "RepairPolicyEngineService");

        AddEventSource(TraceTaskCodes::FileTransfer, "FileTransfer");

        AddEventSource(TraceTaskCodes::FabricInstallerService, "FabricInstallerService");

        AddEventSource(TraceTaskCodes::EntreeServiceProxy, "EntreeServiceProxy");

        AddEventSource(TraceTaskCodes::FabricGateway, "FabricGateway");

        AddEventSource(TraceTaskCodes::FabricTransport, "FabricTransport");

        AddEventSource(TraceTaskCodes::TestabilityComponent, "TestabilityComponent");

        AddEventSource(TraceTaskCodes::LR, "LR");
        AddEventSource(TraceTaskCodes::SM, "SM");
        AddEventSource(TraceTaskCodes::RStore, "RStore");
        AddEventSource(TraceTaskCodes::TR, "TR");
        AddEventSource(TraceTaskCodes::LogicalLog, "LogicalLog");
        AddEventSource(TraceTaskCodes::Ktl, "Ktl");
        AddEventSource(TraceTaskCodes::KtlLoggerNode, "KtlLoggerNode"); 
        AddEventSource(TraceTaskCodes::BA, "BA");
        AddEventSource(TraceTaskCodes::BAP, "BAP");

        AddEventSource(TraceTaskCodes::ResourceMonitor, "ResourceMonitor");

        AddEventSource(TraceTaskCodes::RCR, "RCR");

        AddEventSource(TraceTaskCodes::GatewayResourceManager, "GRM");
    }
}
