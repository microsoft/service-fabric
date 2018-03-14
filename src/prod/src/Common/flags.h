// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
#define BEGIN_DECLARE_T_FLAGS_CLASS( T, ClassName ) \
    class ClassName : public Common::FlagsBase<T> \
    { \
    private: \
    typedef T FlagType; \
    class StructuredTraceInitializer { public: StructuredTraceInitializer(); }; \
    static Common::Global<StructuredTraceInitializer> GlobalStructuredTraceInitializer; \
    public: \
    ClassName(); \
    ClassName(T flags); \
    static std::string FlagToString(T bit); \
    static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name); \
    void WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const&) const; \
    ClassName & operator |= (T bits); \
    ClassName & operator &= (T bits); \

#define DECLARE_FLAG_BIT( Name, Bit ) \
    public: \
    static const FlagType Name = (1 << Bit); \
    bool Is##Name##Set() const { return (**this & Name) != 0; } \

#define IDL_T_FLAG_TYPE_CONVERSIONS( T, PublicType ) \
    void FromPublicApi( PublicType flags) \
    { \
        flags_ = 0; \
        T temp = static_cast<T>(flags); \
        for (auto bit=0; temp!=0; ++bit, temp=(temp>>1)) \
        { \
            flags_ |= ((temp & 0x1) << bit); \
        } \
    } \
    PublicType ToPublicApi() \
    { \
        T temp = flags_; \
        T flags = 0; \
        for (auto bit=0; temp!=0; ++bit, temp=(temp>>1)) \
        { \
            flags |= ((temp & 0x1) << bit); \
        } \
        return static_cast<PublicType>(flags); \
    } \

#define END_DECLARE_FLAGS_CLASS() };

#define BEGIN_DEFINE_T_FLAG_CLASS( T, ClassName, FirstFlag, LastFlag ) \
    Common::Global<ClassName::StructuredTraceInitializer> ClassName::GlobalStructuredTraceInitializer = Common::make_global<ClassName::StructuredTraceInitializer>(); \
    ClassName::ClassName() : FlagsBase() { } \
    ClassName::ClassName(T flags) : FlagsBase(flags) { } \
    std::string ClassName::AddField(Common::TraceEvent & traceEvent, std::string const & name) \
    { \
        return traceEvent.AddMapField(name, #ClassName); \
    } \
    void ClassName::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const&) const \
    { \
        if (this->IsEmptyFlags()) { writer << L"-"; return; } \
        bool isFirstMatch = true; \
        for (T f = FirstFlag; f <= LastFlag; f = (f << 1)) \
        { \
            if ((**this & f) != 0) \
            { \
                if (!isFirstMatch) { writer << L"|"; } \
                writer << FlagToString( f ); \
                isFirstMatch = false; \
            } \
        } \
    } \
    ClassName & ClassName::operator |= (T bits) \
    { \
        flags_ |= bits; \
        return *this; \
    } \
    ClassName & ClassName::operator &= (T bits) \
    { \
        flags_ &= bits; \
        return *this; \
    } \
    ClassName::StructuredTraceInitializer::StructuredTraceInitializer() \
    { \
        for (T f = ClassName::FirstFlag; f <= ClassName::LastFlag; f = (f << 1)) \
        { \
            Common::TraceProvider::StaticInit_Singleton()->AddBitMapValue(#ClassName, f, ClassName::FlagToString( f )); \
        } \
    }; \
    std::string ClassName::FlagToString(T flag) \
    { \
        switch (flag) \
        { \

#define DEFINE_FLAG_VALUE( Name, String ) \
        case Name: return #String; \

#define DEFINE_FLAG( Name ) \
        DEFINE_FLAG_VALUE( Name, Name )

#define END_DEFINE_FLAG_CLASS( ) \
        default: return Common::formatString("{0:x}", flag); \
        } \
    } \

#define BEGIN_DECLARE_FLAGS_CLASS( ClassName ) \
    BEGIN_DECLARE_T_FLAGS_CLASS( uint, ClassName ) \

#define BEGIN_DEFINE_FLAG_CLASS( ClassName, FirstFlag, LastFlag ) \
    BEGIN_DEFINE_T_FLAG_CLASS( uint, ClassName, FirstFlag, LastFlag) \

#define IDL_FLAG_TYPE_CONVERSIONS( PublicType ) \
    IDL_T_FLAG_TYPE_CONVERSIONS( uint, PublicType ) \

    template <typename T>
    class FlagsBase : public Serialization::FabricSerializable
    {
    public:
        static const T None = 0;
        bool IsEmptyFlags() const { return (flags_ == None); }

    public:
        FlagsBase() : flags_(None) { }
        FlagsBase(T flags) : flags_(flags) { }

        T & operator*() { return flags_; }
        T const & operator*() const { return flags_; }

        virtual void WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const&) const = 0;

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.WriteCopy<T>(flags_);
        }

        FABRIC_FIELDS_01(flags_);

    protected:
        T flags_;
    };
}
