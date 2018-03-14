// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TraceEventContext
    {
        DENY_COPY(TraceEventContext);

    public:
        static const size_t BufferSize = 4096;
        static const size_t MaxStringSize = 60000;
        static const size_t MaxUnicodeStringSize = (MaxStringSize / 2);

        TraceEventContext(size_t eventCount)
        :   eventCount_(eventCount),
            eventIndex_(0),
            externalUsed_(0),
            externalAvailable_(0),
            externalBuffer_(nullptr)
        {
            events_ = static_cast<EVENT_DATA_DESCRIPTOR*>(static_cast<void*>(buffer_));
            size_t used = sizeof(EVENT_DATA_DESCRIPTOR) * eventCount;
            internalAvailable_ = BufferSize - used;
            internalBuffer_ = buffer_ + used;
        }

        ~TraceEventContext()
        {
            if (externalBuffer_)
            {
                delete[] externalBuffer_;
            }
        }

        EVENT_DATA_DESCRIPTOR* GetEvents() const
        {
            return events_;
        }

        void* GetBuffer(size_t size)
        {
            if (size <= internalAvailable_)
            {
                void* result = internalBuffer_;
                internalBuffer_ += size;
                internalAvailable_ -= size;

                return result;
            }
            else
            {
                return GetExternalBuffer(size);
            }
        }

        void SetEventData(void const* data, size_t size)
        {
            if (eventIndex_ >= eventCount_)
            {
                throw std::runtime_error("Too many fields");
            }

            EventDataDescCreate(events_ + eventIndex_, data, static_cast<ULONG>(size));
            eventIndex_++;
        }

#pragma region( Write )
        template <class T>
        void Write(T const & t)
        {
            WriteTraits<T>::Write(*this, t);
        }

        template <class T>
        struct WriteTraits
        {
            static void Write(TraceEventContext & context, T const & t)
            {
                __if_exists (IMPLEMENTS(T, FillEventData))
                {
                    t.FillEventData(context);
                }
                __if_not_exists (IMPLEMENTS(T, FillEventData))
                {
                    __if_exists (IMPLEMENTS(T, WriteToEtw))
                    {
                        uint16 id = TraceEvent::GenerateContextSequenceId(1);
                        t.WriteToEtw(id);
                        context.WriteCopy(id);
                    }
                    __if_not_exists (IMPLEMENTS(T, WriteToEtw))
                    {
                        static_assert(is_blittable<T>::value, "Event field type not supported");
                        context.SetEventData(&t, sizeof(T));
                    }
                }
            }
        };

        template <class ChildT>
        struct WriteTraits<std::vector<ChildT>>
        {
            static void Write(TraceEventContext & context, std::vector<ChildT> const & t)
            {
                uint16 id = TraceEvent::GenerateContextSequenceId(t.size());

                for (ChildT const & child : t)
                {
                    __if_exists (ChildT::WriteToEtw)
                    {
                        child.WriteToEtw(id);
                    }
                    __if_not_exists (ChildT::WriteToEtw)
                    {
                        child->WriteToEtw(id);
                    }
                }

                context.WriteCopy(id);
            }
        };

        // For maps we support only tracing out the values
        template  <class keyT, class valueT>
        struct WriteTraits<std::map<keyT, valueT>>
        {
            static void Write(TraceEventContext & context, std::map<keyT, valueT> const & t)
            {
                uint16 id = TraceEvent::GenerateContextSequenceId(t.size());

                for(auto it = t.cbegin(); it != t.cend(); ++it)
                {
                    __if_exists (valueT::WriteToEtw)
                    {
                        it->second.WriteToEtw(id);
                    }
                    __if_not_exists (valueT::WriteToEtw)
                    {
                        it->second->WriteToEtw(id);
                    }
                }

                context.WriteCopy(id);
            }
        };

        template <>
        struct WriteTraits<std::string>
        {
            static void Write(TraceEventContext & context, std::string const & t)
            {
                context.SetEventData(t.c_str(), t.length() + 1);
            }
        };

        template <>
        struct WriteTraits<StringLiteral>
        {
            static void Write(TraceEventContext & context, StringLiteral const & t)
            {
                context.SetEventData(t.begin(), t.size() + 1);
            }
        };

        template <>
        struct WriteTraits<std::wstring>
        {
            static void Write(TraceEventContext & context, std::wstring const & t)
            {
                context.SetEventData(t.c_str(), (t.length() + 1) << 1);
            }
        };

        template <>
        struct WriteTraits<LPCWSTR>
        {
            static void Write(TraceEventContext & context, LPCWSTR t)
            {
                size_t cb = 0;
                HRESULT hr = ::StringCbLength(t, STRSAFE_MAX_CCH * sizeof(WCHAR), &cb);
                if (SUCCEEDED(hr)) // can't assert since assert itself writes
                {
                    context.SetEventData(t, cb + sizeof(WCHAR));
                }
            }
        };

        template <>
        struct WriteTraits<WStringLiteral>
        {
            static void Write(TraceEventContext & context, WStringLiteral const & t)
            {
                context.SetEventData(t.begin(), (t.size() + 1) << 1);
            }
        };

        template <>
        struct WriteTraits<bool>
        {
            static void Write(TraceEventContext & context, bool const & t)
            {
                static int32 trueValue = 1;
                static int32 falseValue = 0;
                context.SetEventData(t ? &trueValue : &falseValue, sizeof(int32));
            }
        };
#pragma endregion

#pragma region( WriteCopy )
        template <class T>
        void WriteCopy(T const & t)
        {
            static_assert(is_blittable<T>::value, "Event field can not be copied");

            void* buffer = GetBuffer(sizeof(T));
            memcpy_s(buffer, sizeof(T), &t, sizeof(T));
            SetEventData(buffer, sizeof(T));
        }

        template <>
        void WriteCopy<Common::TimeSpan>(Common::TimeSpan const & timespan)
        {
            int64 elapsed = timespan.TotalMilliseconds();
            WriteCopy<int64>(elapsed);
        }

        template <>
        void WriteCopy<std::string>(std::string const & t)
        {
            size_t length = t.length();
            if (length > MaxStringSize)
            {
                length = MaxStringSize;
            }
            size_t size = length + 1;
            char* buffer = static_cast<char*>(GetBuffer(size));
            memcpy_s(buffer, size, t.c_str(), length);
            buffer[length] = 0;
            SetEventData(buffer, size);
        }

        template <>
        void WriteCopy<std::wstring>(std::wstring const & t)
        {
            size_t length = t.length();
            if (length > MaxUnicodeStringSize)
            {
                length = MaxUnicodeStringSize;
            }
            size_t size = (length + 1) << 1;
            wchar_t* buffer = static_cast<wchar_t*>(GetBuffer(size));
            memcpy_s(buffer, size, t.c_str(), size - 2);
            buffer[length] = 0;
            SetEventData(buffer, size);
        }
#pragma endregion

    private:
        void* GetExternalBuffer(size_t size);

        size_t eventCount_;
        size_t eventIndex_;
        size_t internalAvailable_;
        size_t externalUsed_;
        size_t externalAvailable_;
        EVENT_DATA_DESCRIPTOR* events_;
        char* internalBuffer_;
        char* externalBuffer_;
        char buffer_[BufferSize];
    };
}
