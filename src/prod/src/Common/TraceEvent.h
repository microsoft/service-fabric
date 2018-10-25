// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TraceEvent
    {
        DENY_COPY(TraceEvent);

    public:
        static const int TextEvents = 4;
        static const int PerTaskEventBits = 8;
        static const int PerTaskEvents = (1 << PerTaskEventBits);
        static const size_t MaxFieldsPerEvent = 32; // ETW has a limitation of 128.

        static uint16 GenerateContextSequenceId(size_t count = 1)
        {
            uint16 id = ++ContextSequenceId;
            return (id << 8) + static_cast<uint16>(count);
        }

        TraceEvent(
            TraceTaskCodes::Enum taskId,
            StringLiteral taskName,
            USHORT eventId,
            StringLiteral eventName,
            LogLevel::Enum level,
            TraceChannelType::Enum channel,
            TraceKeywords::Enum keywords,
            StringLiteral format,
            ::REGHANDLE etwHandle,
            TraceSinkFilter & consoleFilter);

        static StringLiteral GetIdFieldName()
        {
            return "id";
        }

        static StringLiteral GetTypeFieldName()
        {
            return "type";
        }

        static StringLiteral GetTextFieldName()
        {
            return "text";
        }

        static StringLiteral GetContextSequenceIdFieldName()
        {
            return "contextSequenceId";
        }

        USHORT GetGlobalId() const
        {
            return descriptor_.Id;
        }

        USHORT GetLocalId() const
        {
            return (GetGlobalId() & (TraceEvent::PerTaskEvents - 1));
        }

        StringLiteral GetName() const
        {
            return eventName_;
        }

        StringLiteral GetTaskName() const
        {
            return taskName_;
        }

        size_t GetFieldCount() const
        {
            return fields_.size();
        }

        bool IsChildEvent() const
        {
            return isChildEvent_;
        }

        bool HasIdField() const
        {
            return hasId_;
        }

        StringLiteral GetFormat() const
        {
            return format_;
        }

        bool IsETWSinkEnabled() const
        {
            return (((filterStates_[TraceSinkType::ETW]) ||
                     ((samplingRatio_ > 0) && (++SamplingCount % samplingRatio_ == 0))) &&
                    EventEnabled(etwHandle_, &descriptor_));
        }

        bool IsFileSinkEnabled() const
        {
            return (filterStates_[TraceSinkType::TextFile] && TraceTextFileSink::IsEnabled());
        }

        bool IsConsoleSinkEnabled() const
        {
            return filterStates_[TraceSinkType::Console];
        }
#if defined(PLATFORM_UNIX)
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        bool IsLinuxStructuredTracesEnabled() const
        {
            return enableLinuxStructuredTrace_;
        }
#endif

        void Write(PEVENT_DATA_DESCRIPTOR data);

        template<class T>
        void WriteToTextSink(T const & a0, std::wstring const & data, bool useConsole, bool useFile, bool useETW)
        {
            WriteToTextSinkInternal(HasIdField() ? wformatString(a0) : std::wstring(), data, useConsole, useFile, useETW);
        }

        template<>
        void WriteToTextSink<std::wstring>(std::wstring const & a0, std::wstring const & data, bool useConsole, bool useFile, bool useETW)
        {
            WriteToTextSinkInternal(HasIdField() ? a0 : std::wstring(), data, useConsole, useFile, useETW);
        }

        template<>
        void WriteToTextSink<Guid>(Guid const & a0, std::wstring const & data, bool useConsole, bool useFile, bool useETW)
        {
            WriteToTextSinkInternal(HasIdField() ? a0.ToString() : std::wstring(), data, useConsole, useFile, useETW);
        }

        void WriteTextEvent(StringLiteral type, std::wstring const & id, std::wstring const & text, bool useETW, bool useFile, bool useConsole);

        template<class T>
        void AddEventField(std::string & format, std::string const & name, size_t & index)
        {
            UNREFERENCED_PARAMETER(format);
            __if_exists (IMPLEMENTS(T, AddField))
            {
                size_t oldIndex = index;
                size_t oldCount = fields_.size();
                std::string innerFormat = T::AddField(*this, name);
                ExpandArgument(format, innerFormat, index);
                ASSERT_IF(index - oldIndex != fields_.size() - oldCount,
                    "Invalid number of arguments added {0}/{1} for {2}",
                    index - oldIndex, fields_.size() - oldCount, eventName_);
            }
            __if_not_exists (IMPLEMENTS(T, AddField))
            {
                AddField<T>(name);
                index++;
            }
        }

        template<class T>
        void AddEventField(StringLiteral name, size_t & index)
        {
            std::string nameString(name.begin());
            AddEventField<T>(etwFormat_, nameString, index);
        }

        template<class T>
        void AddEventMetadataFields(std::string & format, size_t & index)
        {
            ASSERT_IF(fields_.size() != 0, "Metadata fields should be added first");
            std::string fieldsFormat = T::AddMetadataFields(*this, index);

            size_t currentCount = CountArguments(format);
            for (int i = static_cast<int>(currentCount) - 1; i >= 0; i--)
            {
                UpdateArgument(format, i, i + index);
            }

            format.insert(0, fieldsFormat);
        }

        template<class T>
        void AddEventMetadataFields(size_t & index)
        {
            AddEventMetadataFields<T>(etwFormat_, index);
        }

        void AddContextSequenceField(std::string const & name);

        void ConvertEtwFormatString();
#if defined(PLATFORM_UNIX)
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        void RefreshEnableLinuxStructuredTraces(bool enabled);
#endif

        void RefreshFilterStates(TraceSinkType::Enum sinkType, TraceSinkFilter const & filter);

        void WriteManifest(TraceManifestGenerator & manifest);

#pragma region( AddField )
        template<class T>
        void AddField(std::string const & name)
        {
            AddFieldTraits<T>::AddField(*this, name);
        }

        std::string AddMapField(std::string const & name, StringLiteral mapName)
        {
            ASSERT(mapName.size() > 0);
            this->AddFieldDescription(name, "win:UInt32", "xs:unsignedInt", mapName);

            return std::string();
        }

        template<class T>
        struct AddFieldTraits
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
#if !defined(PLATFORM_UNIX)
                __if_not_exists (T::WriteToEtw)
                {
                    // Check if there is an overload for AddField() below for the type that is being traced
                    static_assert(false, "WriteToEtw not implemented");
                }
#endif

                traceEvent.AddContextSequenceField(name);
            }
        };

        template<class ChildT>
        struct AddFieldTraits<std::vector<ChildT>>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
#if !defined(PLATFORM_UNIX)
                __if_not_exists (ChildT::WriteToEtw)
                {
					// Check if there is an overload for AddField() below for the type that is being traced
                    static_assert(false, "WriteToEtw not implemented");
                }
#endif

                traceEvent.AddContextSequenceField(name);
            }
        };

        template<class KeyT, class ValueT>
        struct AddFieldTraits<std::map<KeyT, ValueT>>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
#if !defined(PLATFORM_UNIX)
                __if_not_exists (ValueT::WriteToEtw)
                {
					// Check if there is an overload for AddField() below for the type that is being traced
                    static_assert(false, "WriteToEtw not implemented");
                }
#endif

                traceEvent.AddContextSequenceField(name);
            }
        };

        template<class ChildT>
        struct AddFieldTraits<std::vector<std::unique_ptr<ChildT>>>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
#if !defined(PLATFORM_UNIX)
                __if_not_exists (ChildT::WriteToEtw)
                {
					// Check if there is an overload for AddField() below for the type that is being traced
                    static_assert(false, "WriteToEtw not implemented");
                }
#endif

                traceEvent.AddContextSequenceField(name);
            }
        };

        template<class ChildT>
        struct AddFieldTraits<std::vector<std::shared_ptr<ChildT>>>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
#if !defined(PLATFORM_UNIX)
                __if_not_exists (ChildT::WriteToEtw)
                {
					// Check if there is an overload for AddField() below for the type that is being traced
                    static_assert(false, "WriteToEtw not implemented");
                }
#endif

                traceEvent.AddContextSequenceField(name);
            }
        };

        template<>
        struct AddFieldTraits<std::string>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:AnsiString", "xs:string");
            }
        };

        template<>
        struct AddFieldTraits<StringLiteral>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:AnsiString", "xs:string");
            }
        };

        template<>
        struct AddFieldTraits<std::wstring>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:UnicodeString", "xs:string");
            }
        };

        template<>
        struct AddFieldTraits<LPCWSTR>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:UnicodeString", "xs:string");
            }
        };

        template<>
        struct AddFieldTraits<WStringLiteral>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:UnicodeString", "xs:string");
            }
        };

        template<>
        struct AddFieldTraits<char>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:Int8", "xs:byte");
            }
        };

        template<>
        struct AddFieldTraits<UCHAR>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:UInt8", "xs:unsignedByte");
            }
        };

        template<>
        struct AddFieldTraits<int16>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:Int16", "xs:short");
            }
        };

        template<>
        struct AddFieldTraits<uint16>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:UInt16", "xs:unsignedShort");
            }
        };

        template<>
        struct AddFieldTraits<int32>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:Int32", "xs:int");
            }
        };

        template<>
        struct AddFieldTraits<uint32>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:UInt32", "xs:unsignedInt");
            }
        };

        template<>
        struct AddFieldTraits<int64>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:Int64", "xs:long");
            }
        };

        template<>
        struct AddFieldTraits<uint64>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:UInt64", "xs:unsignedLong");
            }
        };

        template<>
        struct AddFieldTraits<float>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:Float", "xs:float");
            }
        };

        template<>
        struct AddFieldTraits<double>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:Double", "xs:double");
            }
        };

        template<>
        struct AddFieldTraits<bool>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:Boolean", "xs:boolean");
            }
        };

        template<>
        struct AddFieldTraits<Guid>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:GUID", "xs:GUID");
            }
        };

        template<>
        struct AddFieldTraits<DateTime>
        {
            static void AddField(TraceEvent & traceEvent, std::string const & name)
            {
                traceEvent.AddFieldDescription(name, "win:FILETIME", "xs:dateTime");
            }
        };
 
#pragma endregion

    private:
        struct FieldDescription
        {
            FieldDescription(std::string const & name, StringLiteral inType, StringLiteral outType, StringLiteral mapName)
                :   Name(name),
                    InType(inType),
                    OutType(outType),
                    MapName(mapName)
            {
            }

            void SetHexFormat();

            std::string Name;
            StringLiteral InType;
            StringLiteral OutType;
            StringLiteral MapName;
        };

        void WriteToTextSinkInternal(std::wstring const & id, std::wstring const & data, bool useConsole, bool useFile, bool useETW);

        static size_t CountArguments(std::string const & format);

        static void UpdateArgument(std::string & format, size_t oldIndex, size_t newIndex);

        void ExpandArgument(std::string & format, std::string const & innerFormat, size_t & index);

        void AddFieldDescription(std::string const & name, StringLiteral inType, StringLiteral outType, StringLiteral mapName = "");

        StringLiteral GetLevelString() const;

        EVENT_DESCRIPTOR descriptor_;
        std::vector<FieldDescription> fields_;

        StringLiteral taskName_;
        StringLiteral eventName_;
        StringLiteral format_;
        std::string etwFormat_;

        LogLevel::Enum level_;
        bool hasId_;
        bool isText_;
        bool isChildEvent_;

        bool filterStates_[3];
        int samplingRatio_;
        ULONGLONG testKeyword_;

        static int SamplingCount;
        static uint16 ContextSequenceId;
        static ULONGLONG AdminChannelKeywordMask;
        static ULONGLONG DebugChannelKeywordMask;
        static ULONGLONG AnalyticChannelKeywordMask;
        static ULONGLONG OperationalChannelKeywordMask;

        static ULONGLONG InitializeTestKeyword();

        ::REGHANDLE etwHandle_;
        TraceSinkFilter & consoleFilter_;

#if defined(PLATFORM_UNIX)
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        bool enableLinuxStructuredTrace_;
#endif
    };
}
