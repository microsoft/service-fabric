// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{

#define DECLARE_STRUCTURED_TRACE(traceName, ...) Common::TraceEventWriter<__VA_ARGS__> traceName;

#define DECLARE_OPERATIONAL_TRACE(traceName, ...) \
    DECLARE_STRUCTURED_TRACE(traceName, Common::Guid, __VA_ARGS__)

#define DECLARE_CLUSTER_OPERATIONAL_TRACE(traceName, ...) \
    DECLARE_OPERATIONAL_TRACE(traceName, __VA_ARGS__)

#define DECLARE_NODES_OPERATIONAL_TRACE(traceName, ...) \
    DECLARE_OPERATIONAL_TRACE(traceName, std::wstring, __VA_ARGS__)

#define DECLARE_APPLICATIONS_OPERATIONAL_TRACE(traceName, ...) \
    DECLARE_OPERATIONAL_TRACE(traceName, std::wstring, __VA_ARGS__)

#define DECLARE_SERVICES_OPERATIONAL_TRACE(traceName, ...) \
    DECLARE_OPERATIONAL_TRACE(traceName, std::wstring, __VA_ARGS__)

#define DECLARE_PARTITIONS_OPERATIONAL_TRACE(traceName, ...) \
    DECLARE_OPERATIONAL_TRACE(traceName, Common::Guid, __VA_ARGS__)

#define DECLARE_REPLICAS_OPERATIONAL_TRACE(traceName, ...) \
    DECLARE_OPERATIONAL_TRACE(traceName, Common::Guid, FABRIC_REPLICA_ID, __VA_ARGS__)

#define BEGIN_STRUCTURED_TRACES( Component ) \
    public: \
    Component() : \

#define END_STRUCTURED_TRACES \
    dummyMember() \
    { } \
    bool dummyMember; \

#define STRUCTURED_TRACE(traceName, taskCode, traceId, traceLevel, ...) \
    traceName(Common::TraceTaskCodes::taskCode, traceId, #traceName, Common::LogLevel::traceLevel, __VA_ARGS__)

#define STRUCTURED_COMPLETE_TRACE(traceName, taskCode, traceId, traceLevel, traceChannelType, traceKeywords, ...) \
	traceName(Common::TraceTaskCodes::taskCode, traceId, #traceName, Common::LogLevel::traceLevel, Common::TraceChannelType::traceChannelType, Common::TraceKeywords::traceKeywords, __VA_ARGS__)

#define STRUCTURED_DEL_QUERY_TRACE(tableName, traceName, taskCode, traceId, traceLevel, ...) \
    STRUCTURED_QUERY_TRACE(tableName##_DEL, traceName, taskCode, traceId, traceLevel, __VA_ARGS__)

#define STRUCTURED_QUERY_TRACE(tableName, traceName, taskCode, traceId, traceLevel, ...) \
    STRUCTURED_QUERY_TRACE_HELPER(traceName, _##tableName##_##traceName, taskCode, traceId, traceLevel, __VA_ARGS__)

    // Note: To be removed after all operational traces is changed to use the new macros.
#define STRUCTURED_OPERATIONAL_TRACE(tablePrefix, traceName, taskCode, traceId, traceLevel, ...) \
    STRUCTURED_QUERY_TRACE_HELPER(traceName, _##tablePrefix##Ops_##traceName, taskCode, traceId, traceLevel, Common::TraceChannelType::Operational, __VA_ARGS__)

#define OPERATIONAL_TRACE(traceName, tablePrefix, publicEventName, category, taskCode, traceId, traceLevel, formatString, ...) \
    STRUCTURED_OPERATIONAL_TRACE_HELPER(traceName, _##tablePrefix##Ops_##traceName, publicEventName, category, taskCode, traceId, traceLevel, Common::TraceChannelType::Operational, formatString, "eventInstanceId", __VA_ARGS__)

#define CLUSTER_OPERATIONAL_TRACE(traceName, publicEventName, category, taskCode, traceId, traceLevel, formatString, ...) \
    OPERATIONAL_TRACE(traceName, Cluster, publicEventName, category, taskCode, traceId, traceLevel, formatString, __VA_ARGS__)

#define NODES_OPERATIONAL_TRACE(traceName, publicEventName, category, taskCode, traceId, traceLevel, formatString, ...) \
    OPERATIONAL_TRACE(traceName, Nodes, publicEventName, category, taskCode, traceId, traceLevel, formatString, "nodeName", __VA_ARGS__)

#define APPLICATIONS_OPERATIONAL_TRACE(traceName, publicEventName, category, taskCode, traceId, traceLevel, formatString, ...) \
    OPERATIONAL_TRACE(traceName, Applications, publicEventName, category, taskCode, traceId, traceLevel, formatString, "applicationName", __VA_ARGS__)

#define SERVICES_OPERATIONAL_TRACE(traceName, publicEventName, category, taskCode, traceId, traceLevel, formatString, ...) \
    OPERATIONAL_TRACE(traceName, Services, publicEventName, category, taskCode, traceId, traceLevel, formatString, "serviceName", __VA_ARGS__)

#define PARTITIONS_OPERATIONAL_TRACE(traceName, publicEventName, category, taskCode, traceId, traceLevel, formatString, ...) \
    OPERATIONAL_TRACE(traceName, Partitions, publicEventName, category, taskCode, traceId, traceLevel, formatString, "partitionId", __VA_ARGS__)

#define REPLICAS_OPERATIONAL_TRACE(traceName, publicEventName, category, taskCode, traceId, traceLevel, formatString, ...) \
    OPERATIONAL_TRACE(traceName, Replicas, publicEventName, category, taskCode, traceId, traceLevel, formatString, "partitionId", "replicaId", __VA_ARGS__)

#define STRUCTURED_QUERY_TRACE_HELPER(traceName, queryTraceName, taskCode, traceId, traceLevel, ...) \
    traceName(Common::TraceTaskCodes::taskCode, traceId, #queryTraceName, Common::LogLevel::traceLevel, __VA_ARGS__)

#define STRUCTURED_OPERATIONAL_TRACE_HELPER(traceName, queryTraceName, publicEventName, category, taskCode, traceId, traceLevel, ...) \
    traceName(ExtendedEventMetadata::Create(publicEventName, category), Common::TraceTaskCodes::taskCode, traceId, #queryTraceName, Common::LogLevel::traceLevel, __VA_ARGS__)

#define OperationalStateTransitionCategory L"StateTransition"
#define OperationalLifeCycleCategory L"LifeCycle"
#define OperationalUpgradeCategory L"Upgrade"
#define OperationalHealthCategory L"Health"

#define DECLARE_ENUM_STRUCTURED_TRACE( enumClassName ) \
    class Trace \
    { \
    public: \
        Trace(Enum e) : enum_(e) { } \
        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const; \
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name); \
        void FillEventData(Common::TraceEventContext & context) const; \
    private: \
        class Initializer \
        { \
        public: \
            Initializer(); \
        }; \
        static Common::Global<Initializer> GlobalInitializer; \
        Enum enum_; \
    }; \

#define BEGIN_ENUM_STRUCTURED_TRACE( enumClassName ) \
    void Trace::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const \
    { \
        w << enum_; \
    } \
    std::string Trace::AddField(Common::TraceEvent & traceEvent, std::string const & name) \
    { \
        return traceEvent.AddMapField(name, #enumClassName); \
    } \
    void Trace::FillEventData(Common::TraceEventContext & context) const \
    { \
        static_assert(sizeof(enumClassName::Enum) <= sizeof(uint), "enum size larger than uint"); \
        context.WriteCopy<uint>(static_cast<uint>(enum_)); \
    } \
    Common::Global<Trace::Initializer> Trace::GlobalInitializer = Common::make_global<Trace::Initializer>(); \
    Trace::Initializer::Initializer() \
    { \

#define END_ENUM_STRUCTURED_TRACE( enumClassName ) \
    } \

#define ADD_ENUM_MAP_VALUE( enumClassName, enumValue ) \
    { \
        std::string tmp; \
        Common::StringWriterA(tmp).Write("{0}", (enumValue)); \
        static_assert(sizeof(enumClassName::Enum) <= sizeof(uint), "enum size larger than uint"); \
        Common::TraceProvider::StaticInit_Singleton()->AddMapValue(#enumClassName, static_cast<uint>(enumValue), tmp); \
    } \

#define ADD_FLAGS_MAP_VALUE( enumClassName, enumValue ) \
    { \
        std::string tmp; \
        Common::StringWriterA(tmp).Write("{0}", (enumValue)); \
        static_assert(sizeof(enumClassName::Enum) <= sizeof(uint), "enum size larger than uint"); \
        Common::TraceProvider::StaticInit_Singleton()->AddBitMapValue(#enumClassName, static_cast<uint>(enumValue), tmp); \
    } \

#define ADD_ENUM_MAP_VALUE_RANGE( enumClassName, enumStart, enumEnd ) \
    static_assert(sizeof(enumClassName::Enum) <= sizeof(uint), "enum size larger than uint"); \
    for (enumClassName::Enum e = (enumStart); e <= (enumEnd); e = static_cast<enumClassName::Enum>(static_cast<uint>(e)+1)) \
    ADD_ENUM_MAP_VALUE( enumClassName, e ) \

#define ADD_CASTED_ENUM_MAP_VALUE( enumClassName, enumValue ) \
    ADD_ENUM_MAP_VALUE( enumClassName, static_cast<enumClassName::Enum>(enumValue) ) \

#define ADD_CASTED_ENUM_MAP_VALUE_RANGE( enumClassName, enumStart, enumEnd ) \
    ADD_ENUM_MAP_VALUE_RANGE( enumClassName, static_cast<enumClassName::Enum>(enumStart), static_cast<enumClassName::Enum>(enumEnd) ) \

#define ENUM_STRUCTURED_TRACE( enumClassName, enumStart, enumEnd) \
    BEGIN_ENUM_STRUCTURED_TRACE( enumClassName ) \
    ADD_ENUM_MAP_VALUE_RANGE( enumClassName, enumStart, enumEnd ) \
    END_ENUM_STRUCTURED_TRACE( enumClassName ) \

#define ADD_FLAGS_MAP_VALUE_RANGE( enumClassName, enumZeroValue, enumStart, enumEnd ) \
    ADD_FLAGS_MAP_VALUE( enumClassName, enumZeroValue ) \
    static_assert(sizeof(enumClassName::Enum) <= sizeof(uint), "enum size larger than uint"); \
    static_assert(enumStart > 0 && static_cast<uint>(enumEnd) <= (1U << 31), "Invalid flags enum start/end values"); \
    for (enumClassName::Enum e = (enumStart); e <= (enumEnd); e = static_cast<enumClassName::Enum>(static_cast<uint>(e) << 1)) \
    ADD_FLAGS_MAP_VALUE( enumClassName, e ) \

#define FLAGS_STRUCTURED_TRACE( enumClassName, enumZeroValue, enumStart, enumEnd ) \
    BEGIN_ENUM_STRUCTURED_TRACE( enumClassName ) \
    ADD_FLAGS_MAP_VALUE_RANGE( enumClassName, enumZeroValue, enumStart, enumEnd ) \
    END_ENUM_STRUCTURED_TRACE( enumClassName ) \

#define BEGIN_ENUM_TO_TEXT( enumClassName ) \
    void WriteToTextWriter(Common::TextWriter & w, Enum const & e) \
    { \
        switch (e) \
        { \

#define ADD_ENUM_TO_TEXT_VALUE( enumValue ) \
        case enumValue: w << #enumValue; return; \

#define END_ENUM_TO_TEXT( enumClassName ) \
        }; \
        static_assert(sizeof(enumClassName::Enum) <= sizeof(uint), "enum size larger than uint"); \
        w.Write("0x{0:x}", static_cast<uint>(e)); \
    } \

#define BEGIN_FLAGS_TO_TEXT( enumClassName, noneValueName ) \
    void WriteToTextWriter(Common::TextWriter & w, Enum const & e) \
    { \
        if (e == noneValueName) { w << #noneValueName; return; } \
        Enum flags = e; \
        bool isAnyFlagSet = false; \

#define ADD_FLAGS_TO_TEXT_VALUE( enumValue ) \
        if (flags & enumValue) { \
            if (isAnyFlagSet) { w << '|'; } \
            w << #enumValue; \
            flags &= ~enumValue; \
            isAnyFlagSet = true; \
        } \

#define END_FLAGS_TO_TEXT( enumClassName ) \
    static_assert(sizeof(enumClassName::Enum) <= sizeof(uint), "enum size larger than uint"); \
        if (flags != 0) { \
            if (isAnyFlagSet) { w << '|'; } \
            w.Write("0x{0:x}", static_cast<uint>(flags)); \
        } \
    } \

    template <typename _Arg0 = detail::NilType, typename _Arg1 = detail::NilType, typename _Arg2 = detail::NilType, typename _Arg3 = detail::NilType, typename _Arg4 = detail::NilType, typename _Arg5 = detail::NilType, typename _Arg6 = detail::NilType, typename _Arg7 = detail::NilType, typename _Arg8 = detail::NilType, typename _Arg9 = detail::NilType, typename _Arg10 = detail::NilType, typename _Arg11 = detail::NilType, typename _Arg12 = detail::NilType, typename _Arg13 = detail::NilType>
    class TraceEventWriter
    {
        DENY_COPY(TraceEventWriter);

    public:
        TraceEventWriter(
            TraceTaskCodes::Enum taskId,
            USHORT eventId,
            StringLiteral eventName,
            LogLevel::Enum level,
            StringLiteral format,
            StringLiteral argName0 = StringLiteral(),
            StringLiteral argName1 = StringLiteral(),
            StringLiteral argName2 = StringLiteral(),
            StringLiteral argName3 = StringLiteral(),
            StringLiteral argName4 = StringLiteral(),
            StringLiteral argName5 = StringLiteral(),
            StringLiteral argName6 = StringLiteral(),
            StringLiteral argName7 = StringLiteral(),
            StringLiteral argName8 = StringLiteral(),
            StringLiteral argName9 = StringLiteral(),
            StringLiteral argName10 = StringLiteral(),
            StringLiteral argName11 = StringLiteral(),
            StringLiteral argName12 = StringLiteral(),
            StringLiteral argName13 = StringLiteral())
            : event_(TraceProvider::StaticInit_Singleton()->CreateEvent(taskId, eventId, eventName, level, (level <= LogLevel::Enum::Warning) ? TraceChannelType::Admin : TraceChannelType::Debug, TraceKeywords::Default, format))
            , metadataFieldsCount_(0)
        {
            Initialization(
                argName0,
                argName1,
                argName2,
                argName3,
                argName4,
                argName5,
                argName6,
                argName7,
                argName8,
                argName9,
                argName10,
                argName11,
                argName12,
                argName13);
        }

        TraceEventWriter(
            TraceTaskCodes::Enum taskId,
            USHORT eventId,
            StringLiteral eventName,
            LogLevel::Enum level,
            TraceChannelType::Enum channel,
            StringLiteral format,
            StringLiteral argName0 = StringLiteral(),
            StringLiteral argName1 = StringLiteral(),
            StringLiteral argName2 = StringLiteral(),
            StringLiteral argName3 = StringLiteral(),
            StringLiteral argName4 = StringLiteral(),
            StringLiteral argName5 = StringLiteral(),
            StringLiteral argName6 = StringLiteral(),
            StringLiteral argName7 = StringLiteral(),
            StringLiteral argName8 = StringLiteral(),
            StringLiteral argName9 = StringLiteral(),
            StringLiteral argName10 = StringLiteral(),
            StringLiteral argName11 = StringLiteral(),
            StringLiteral argName12 = StringLiteral(),
            StringLiteral argName13 = StringLiteral())
            : event_(TraceProvider::StaticInit_Singleton()->CreateEvent(taskId, eventId, eventName, level, channel, TraceKeywords::Default, format))
            , metadataFieldsCount_(0)
        {
            Initialization(
                argName0,
                argName1,
                argName2,
                argName3,
                argName4,
                argName5,
                argName6,
                argName7,
                argName8,
                argName9,
                argName10,
                argName11,
                argName12,
                argName13);
        }

        TraceEventWriter(
            std::unique_ptr<ExtendedEventMetadata> extendedMetadata,
            TraceTaskCodes::Enum taskId,
            USHORT eventId,
            StringLiteral eventName,
            LogLevel::Enum level,
            TraceChannelType::Enum channel,
            StringLiteral format,
            StringLiteral argName0 = StringLiteral(),
            StringLiteral argName1 = StringLiteral(),
            StringLiteral argName2 = StringLiteral(),
            StringLiteral argName3 = StringLiteral(),
            StringLiteral argName4 = StringLiteral(),
            StringLiteral argName5 = StringLiteral(),
            StringLiteral argName6 = StringLiteral(),
            StringLiteral argName7 = StringLiteral(),
            StringLiteral argName8 = StringLiteral(),
            StringLiteral argName9 = StringLiteral(),
            StringLiteral argName10 = StringLiteral(),
            StringLiteral argName11 = StringLiteral(),
            StringLiteral argName12 = StringLiteral(),
            StringLiteral argName13 = StringLiteral())
            : event_(TraceProvider::StaticInit_Singleton()->CreateEvent(taskId, eventId, eventName, level, channel, TraceKeywords::Default, format))
            , metadataFieldsCount_(0)
            , extendedMetadata_(std::move(extendedMetadata))
        {
            if (event_.GetFieldCount() == 0 && extendedMetadata_)
            {
                event_.AddEventMetadataFields<ExtendedEventMetadata>(metadataFieldsCount_);
            }

            Initialization(
                argName0,
                argName1,
                argName2,
                argName3,
                argName4,
                argName5,
                argName6,
                argName7,
                argName8,
                argName9,
                argName10,
                argName11,
                argName12,
                argName13);
        }

        TraceEventWriter(
            TraceTaskCodes::Enum taskId,
            USHORT eventId,
            StringLiteral eventName,
            LogLevel::Enum level,
            TraceChannelType::Enum channel,
            TraceKeywords::Enum keywords,
            StringLiteral format,
            StringLiteral argName0 = StringLiteral(),
            StringLiteral argName1 = StringLiteral(),
            StringLiteral argName2 = StringLiteral(),
            StringLiteral argName3 = StringLiteral(),
            StringLiteral argName4 = StringLiteral(),
            StringLiteral argName5 = StringLiteral(),
            StringLiteral argName6 = StringLiteral(),
            StringLiteral argName7 = StringLiteral(),
            StringLiteral argName8 = StringLiteral(),
            StringLiteral argName9 = StringLiteral(),
            StringLiteral argName10 = StringLiteral(),
            StringLiteral argName11 = StringLiteral(),
            StringLiteral argName12 = StringLiteral(),
            StringLiteral argName13 = StringLiteral())
            : event_(TraceProvider::StaticInit_Singleton()->CreateEvent(taskId, eventId, eventName, level, channel, keywords, format))
            , metadataFieldsCount_(0)
        {
            Initialization(
                argName0,
                argName1,
                argName2,
                argName3,
                argName4,
                argName5,
                argName6,
                argName7,
                argName8,
                argName9,
                argName10,
                argName11,
                argName12,
                argName13);
        }

        TraceEventWriter(
            std::unique_ptr<ExtendedEventMetadata> extendedMetadata,
            TraceTaskCodes::Enum taskId,
            USHORT eventId,
            StringLiteral eventName,
            LogLevel::Enum level,
            TraceChannelType::Enum channel,
            TraceKeywords::Enum keywords,
            StringLiteral format,
            StringLiteral argName0 = StringLiteral(),
            StringLiteral argName1 = StringLiteral(),
            StringLiteral argName2 = StringLiteral(),
            StringLiteral argName3 = StringLiteral(),
            StringLiteral argName4 = StringLiteral(),
            StringLiteral argName5 = StringLiteral(),
            StringLiteral argName6 = StringLiteral(),
            StringLiteral argName7 = StringLiteral(),
            StringLiteral argName8 = StringLiteral(),
            StringLiteral argName9 = StringLiteral(),
            StringLiteral argName10 = StringLiteral(),
            StringLiteral argName11 = StringLiteral(),
            StringLiteral argName12 = StringLiteral(),
            StringLiteral argName13 = StringLiteral())
            : event_(TraceProvider::StaticInit_Singleton()->CreateEvent(taskId, eventId, eventName, level, channel, keywords, format))
            , metadataFieldsCount_(0)
            , extendedMetadata_(std::move(extendedMetadata))
        {
            if (event_.GetFieldCount() == 0 && extendedMetadata_)
            {
                event_.AddEventMetadataFields<ExtendedEventMetadata>(metadataFieldsCount_);
            }

            Initialization(
                argName0,
                argName1,
                argName2,
                argName3,
                argName4,
                argName5,
                argName6,
                argName7,
                argName8,
                argName9,
                argName10,
                argName11,
                argName12,
                argName13);
        }

        void Initialization(
            StringLiteral argName0,
            StringLiteral argName1,
            StringLiteral argName2,
            StringLiteral argName3,
            StringLiteral argName4,
            StringLiteral argName5,
            StringLiteral argName6,
            StringLiteral argName7,
            StringLiteral argName8,
            StringLiteral argName9,
            StringLiteral argName10,
            StringLiteral argName11,
            StringLiteral argName12,
            StringLiteral argName13)
        {
            UNREFERENCED_PARAMETER(argName0);
            UNREFERENCED_PARAMETER(argName1);
            UNREFERENCED_PARAMETER(argName2);
            UNREFERENCED_PARAMETER(argName3);
            UNREFERENCED_PARAMETER(argName4);
            UNREFERENCED_PARAMETER(argName5);
            UNREFERENCED_PARAMETER(argName6);
            UNREFERENCED_PARAMETER(argName7);
            UNREFERENCED_PARAMETER(argName8);
            UNREFERENCED_PARAMETER(argName9);
            UNREFERENCED_PARAMETER(argName10);
            UNREFERENCED_PARAMETER(argName11);
            UNREFERENCED_PARAMETER(argName12);
            UNREFERENCED_PARAMETER(argName13);

            if (event_.GetFieldCount() == metadataFieldsCount_)
            {
                size_t index = metadataFieldsCount_;
                UNREFERENCED_PARAMETER(index);

                __if_not_exists(detail::IsNilType<_Arg0>::True) { event_.AddEventField<_Arg0>(argName0, index); }
                __if_not_exists(detail::IsNilType<_Arg1>::True) { event_.AddEventField<_Arg1>(argName1, index); }
                __if_not_exists(detail::IsNilType<_Arg2>::True) { event_.AddEventField<_Arg2>(argName2, index); }
                __if_not_exists(detail::IsNilType<_Arg3>::True) { event_.AddEventField<_Arg3>(argName3, index); }
                __if_not_exists(detail::IsNilType<_Arg4>::True) { event_.AddEventField<_Arg4>(argName4, index); }
                __if_not_exists(detail::IsNilType<_Arg5>::True) { event_.AddEventField<_Arg5>(argName5, index); }
                __if_not_exists(detail::IsNilType<_Arg6>::True) { event_.AddEventField<_Arg6>(argName6, index); }
                __if_not_exists(detail::IsNilType<_Arg7>::True) { event_.AddEventField<_Arg7>(argName7, index); }
                __if_not_exists(detail::IsNilType<_Arg8>::True) { event_.AddEventField<_Arg8>(argName8, index); }
                __if_not_exists(detail::IsNilType<_Arg9>::True) { event_.AddEventField<_Arg9>(argName9, index); }
                __if_not_exists(detail::IsNilType<_Arg10>::True) { event_.AddEventField<_Arg10>(argName10, index); }
                __if_not_exists(detail::IsNilType<_Arg11>::True) { event_.AddEventField<_Arg11>(argName11, index); }
                __if_not_exists(detail::IsNilType<_Arg12>::True) { event_.AddEventField<_Arg12>(argName12, index); }
                __if_not_exists(detail::IsNilType<_Arg13>::True) { event_.AddEventField<_Arg13>(argName13, index); }
            }

            event_.ConvertEtwFormatString();
        }

        StringLiteral GetTaskName() const
        {
            return event_.GetTaskName();
        }

        StringLiteral GetEventName() const
        {
            return event_.GetName();
        }

        void operator() () const
        {
            static_assert(detail::IsNilType<_Arg0>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                if (extendedMetadata_)
                {
                    TraceEventContext context(event_.GetFieldCount());
                    extendedMetadata_->AppendFields(context);

                    event_.Write(context.GetEvents());
                }
                else
                {
                    event_.Write(NULL);
                }
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat());

                event_.WriteToTextSink(std::wstring(), formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0) const
        {
            static_assert(detail::IsNilType<_Arg1>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1) const
        {
            static_assert(detail::IsNilType<_Arg2>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2) const
        {
            static_assert(detail::IsNilType<_Arg3>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2, _Arg3 const & a3) const
        {
            static_assert(detail::IsNilType<_Arg4>::value, "Invalid number of arguments");

           if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);
                context.Write(a3);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2, a3);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2, _Arg3 const & a3, _Arg4 const & a4) const
        {
            static_assert(detail::IsNilType<_Arg5>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);
                context.Write(a3);
                context.Write(a4);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2, a3, a4);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2, _Arg3 const & a3, _Arg4 const & a4, _Arg5 const & a5) const
        {
            static_assert(detail::IsNilType<_Arg6>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);
                context.Write(a3);
                context.Write(a4);
                context.Write(a5);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2, a3, a4, a5);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2, _Arg3 const & a3, _Arg4 const & a4, _Arg5 const & a5, _Arg6 const & a6) const
        {
            static_assert(detail::IsNilType<_Arg7>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);
                context.Write(a3);
                context.Write(a4);
                context.Write(a5);
                context.Write(a6);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2, a3, a4, a5, a6);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2, _Arg3 const & a3, _Arg4 const & a4, _Arg5 const & a5, _Arg6 const & a6, _Arg7 const & a7) const
        {
            static_assert(detail::IsNilType<_Arg8>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);
                context.Write(a3);
                context.Write(a4);
                context.Write(a5);
                context.Write(a6);
                context.Write(a7);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2, a3, a4, a5, a6, a7);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2, _Arg3 const & a3, _Arg4 const & a4, _Arg5 const & a5, _Arg6 const & a6, _Arg7 const & a7, _Arg8 const & a8) const
        {
            static_assert(detail::IsNilType<_Arg9>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);
                context.Write(a3);
                context.Write(a4);
                context.Write(a5);
                context.Write(a6);
                context.Write(a7);
                context.Write(a8);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2, a3, a4, a5, a6, a7, a8);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2, _Arg3 const & a3, _Arg4 const & a4, _Arg5 const & a5, _Arg6 const & a6, _Arg7 const & a7, _Arg8 const & a8, _Arg9 const & a9) const
        {
            static_assert(detail::IsNilType<_Arg10>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);
                context.Write(a3);
                context.Write(a4);
                context.Write(a5);
                context.Write(a6);
                context.Write(a7);
                context.Write(a8);
                context.Write(a9);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2, _Arg3 const & a3, _Arg4 const & a4, _Arg5 const & a5, _Arg6 const & a6, _Arg7 const & a7, _Arg8 const & a8, _Arg9 const & a9, _Arg10 const & a10) const
        {
            static_assert(detail::IsNilType<_Arg11>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);
                context.Write(a3);
                context.Write(a4);
                context.Write(a5);
                context.Write(a6);
                context.Write(a7);
                context.Write(a8);
                context.Write(a9);
                context.Write(a10);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2, _Arg3 const & a3, _Arg4 const & a4, _Arg5 const & a5, _Arg6 const & a6, _Arg7 const & a7, _Arg8 const & a8, _Arg9 const & a9, _Arg10 const & a10, _Arg11 const & a11) const
        {
            static_assert(detail::IsNilType<_Arg12>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);
                context.Write(a3);
                context.Write(a4);
                context.Write(a5);
                context.Write(a6);
                context.Write(a7);
                context.Write(a8);
                context.Write(a9);
                context.Write(a10);
                context.Write(a11);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2, _Arg3 const & a3, _Arg4 const & a4, _Arg5 const & a5, _Arg6 const & a6, _Arg7 const & a7, _Arg8 const & a8, _Arg9 const & a9, _Arg10 const & a10, _Arg11 const & a11, _Arg12 const & a12) const
        {
            static_assert(detail::IsNilType<_Arg13>::value, "Invalid number of arguments");

            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);
                context.Write(a3);
                context.Write(a4);
                context.Write(a5);
                context.Write(a6);
                context.Write(a7);
                context.Write(a8);
                context.Write(a9);
                context.Write(a10);
                context.Write(a11);
                context.Write(a12);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

        void operator() (_Arg0 const & a0, _Arg1 const & a1, _Arg2 const & a2, _Arg3 const & a3, _Arg4 const & a4, _Arg5 const & a5, _Arg6 const & a6, _Arg7 const & a7, _Arg8 const & a8, _Arg9 const & a9, _Arg10 const & a10, _Arg11 const & a11, _Arg12 const & a12, _Arg13 const & a13) const
        {
            if (event_.IsETWSinkEnabled()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                TraceEventContext context(event_.GetFieldCount());

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(context);
                }

                context.Write(a0);
                context.Write(a1);
                context.Write(a2);
                context.Write(a3);
                context.Write(a4);
                context.Write(a5);
                context.Write(a6);
                context.Write(a7);
                context.Write(a8);
                context.Write(a9);
                context.Write(a10);
                context.Write(a11);
                context.Write(a12);
                context.Write(a13);

                event_.Write(context.GetEvents());
            }

            if (event_.IsChildEvent()
#if defined(PLATFORM_UNIX)
                    && event_.IsLinuxStructuredTracesEnabled()
#endif
                    )
            {
                return;
            }

            bool useFile = event_.IsFileSinkEnabled();
            bool useConsole = event_.IsConsoleSinkEnabled();
            bool useETW = false;
#if defined(PLATFORM_UNIX)
            useETW = event_.IsETWSinkEnabled() && !event_.IsLinuxStructuredTracesEnabled();
#endif
            if (useConsole || useFile || useETW)
            {
                std::wstring formattedString;
                StringWriter w(formattedString);

                if (extendedMetadata_)
                {
                    extendedMetadata_->AppendFields(w);
                }

                w.Write(event_.GetFormat(), a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);

                event_.WriteToTextSink(a0, formattedString, useConsole, useFile, useETW);
            }
        }

    private:
        TraceEvent & event_;
        size_t metadataFieldsCount_;
        std::unique_ptr<ExtendedEventMetadata> extendedMetadata_;
    };
}
