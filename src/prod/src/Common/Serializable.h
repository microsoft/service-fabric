// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class JsonReaderVisitor;
    class JsonWriterVisitor;

    enum JsonSerializerFlags
    {
        Default = 0,
        DateTimeInIsoFormat = 1 << 1,
        EnumInStringFormat = 1 << 2
    };

    class IFabricJsonSerializable
    {
    public:
        virtual ~IFabricJsonSerializable() {};
        virtual HRESULT FromJson(JsonReaderVisitor &reader, bool newObject = true, bool conditional = true) = 0;
        virtual HRESULT ToJson(JsonWriterVisitor &writer, bool newObject = true, bool conditional = true) = 0;
    };

    // A const array of these is created for each enum. A sentinel value is used for the end.
    // The old way used two maps, which is really way overkill for a small number of items.
    // Instead, we just build an array of these and search. Looking up a value is ~ an order of
    // magnitude faster than a map. Looking up by string is ~20% faster than using a map.
    // A large number of entries is needed before an associative lookup is worthwhile.
    template <typename Enum>
    struct StringEnumPair
    {
        wchar_t const* string;
        Enum value;
    };

    template <typename Enum>
    inline Common::ErrorCode EnumToWStringHelper(Enum val, __out std::wstring &strVal, const StringEnumPair<Enum>* pairs)
    {
        for ( ;pairs->string != nullptr; ++pairs)
        {
            if (pairs->value == val)
            {
                strVal = pairs->string;
                return Common::ErrorCodeValue::Success;
            }
        }
        return Common::ErrorCodeValue::InvalidArgument;
    }

    template <typename Enum>
    inline Common::ErrorCode WStringToEnumHelper(std::wstring const &strVal, __out Enum &val, const StringEnumPair<Enum>* pairs)
    {
        for ( ;pairs->string != nullptr; ++pairs)
        {
            if (wcscmp(pairs->string, strVal.c_str()) == 0)
            {
                val = pairs->value;
                return Common::ErrorCodeValue::Success;
            }
        }
        return Common::ErrorCodeValue::InvalidArgument;
    }

    //
    // We have 2 types of enums definitions in our native product code.
    // 1. Defined in our IDL files
    // 2. Defined in our header files corresponding to our cpp files - typically inside a separate namespace.
    // Both of them are handled separately and the type inference logic below is for distinguishing that.
    //
    template <bool B, class T>
    struct IdlOrServiceModelEnum
    {   
        // Implementation is not needed since a bool is specialized for true and false
    };

    template <class T>
    struct IdlOrServiceModelEnum<true, T>
    {
        static ErrorCode WStringToEnum(std::wstring const &enumString, __out T &enumVal)
        {
            return WStringToEnumHelper(enumString, enumVal, Common::EnumJsonSerializer<T>::Get());
        }

        static ErrorCode EnumToWString(T &enumVal, __out std::wstring &enumString)
        {
            return EnumToWStringHelper(enumVal, enumString, Common::EnumJsonSerializer<T>::Get());
        }
    };

    // This specialization is for non-enums and the type must have a setter
    template <class T>
    struct IdlOrServiceModelEnum<false, T>
    {
        static ErrorCode WStringToEnum(std::wstring const &enumString, __out T &enumVal)
        {
            return EnumFromString(enumString, enumVal);
        }

        static ErrorCode EnumToWString(T &enumVal, __out std::wstring &enumString)
        {
            return EnumToString(enumVal, enumString);
        }
    };

    // Enum Helpers
    template<typename T>
    static ErrorCode EnumToWString(T &enumVal, __out std::wstring &enumString)
    {
        const bool isIdlEnum = EnumJsonSerializer<T>::Marker;

        return IdlOrServiceModelEnum<isIdlEnum, T>::EnumToWString(enumVal, enumString);
    }

    template<typename T>
    static ErrorCode WStringToEnum(std::wstring const &enumString, __out T &enumVal)
    {
        const bool isIdlEnum = EnumJsonSerializer<T>::Marker;

        return IdlOrServiceModelEnum<isIdlEnum, T>::WStringToEnum(enumString, enumVal);
    }
}

//
// Macros that define the helper method for serializing/deserializing an object 
//

#define BEGIN_JSON_SERIALIZABLE_PROPERTIES()                                                                                                        \
    struct IsSerializable { };                                                                                                                      \
    HRESULT VisitSerializable(Common::JsonWriterVisitor *writer, Common::JsonReaderVisitor *reader, bool newObject = true, bool conditional = true) \
    {                                                                                                                                               \
        HRESULT hr = S_OK;                                                                                                                          \
        if(newObject) {                                                                                                                             \
            hr = (writer != nullptr) ? writer->StartObject(nullptr, conditional) : reader->StartObject(nullptr, conditional) ;                      \
            if(FAILED(hr)) return hr;                                                                                                               \
        }

#define SERIALIZABLE_PROPERTY_CHAIN()                                                                           \
        hr = __super::VisitSerializable(writer, reader, false, conditional);                                    \
        if(FAILED(hr)) return hr; 

#define SERIALIZABLE_PROPERTY_READ(Name, Member)                                                                \
        else                                                                                                    \
        {                                                                                                       \
            hr = Common::JsonReaderVisitor::Dispatch(*reader, Name, (Member), conditional);                     \
        }                                                                                                       \
        if(FAILED(hr)) return hr;

#define SERIALIZABLE_PROPERTY(Name, Member)                                                                     \
        if (writer != nullptr)                                                                                  \
        {                                                                                                       \
            hr = Common::JsonWriterVisitor::Dispatch(*writer, Name, (Member), conditional);                     \
        }                                                                                                       \
        SERIALIZABLE_PROPERTY_READ(Name, Member)                                                                \

#define SERIALIZABLE_PROPERTY_SIMPLE_MAP(Name, Member)                                                          \
        if (writer != nullptr)                                                                                  \
        {                                                                                                       \
            hr = writer->VisitSimpleMap(Common::WStringLiteral(Name).begin(), (Member), conditional);           \
        }                                                                                                       \
        SERIALIZABLE_PROPERTY_READ(Name, Member)                                                                \

#define SERIALIZABLE_PROPERTY_SIMPLE_MAP_IF(Name, Member, Conditional)                                                                  \
    if (writer != nullptr)                                                                                                              \
    {                                                                                                                                   \
        hr = writer->VisitSimpleMap(Common::WStringLiteral(Name).begin(), (Member), conditional ? Conditional : conditional);           \
    }                                                                                                                                   \
    SERIALIZABLE_PROPERTY_READ(Name, Member)                                                                                            \


#define SERIALIZE_CONSTANT_IF(Name, ConstantValue, Conditional)                                                                \
        if (writer != nullptr)                                                                                                 \
        {                                                                                                                      \
            hr = Common::JsonWriterVisitor::Dispatch(*writer, Name, ConstantValue, conditional ? Conditional : conditional);   \
        }                                                                                                                      \
        if(FAILED(hr)) return hr;


#define SERIALIZABLE_PROPERTY_IF(Name, Member, Conditional)                                                             \
        if (writer != nullptr)                                                                                          \
        {                                                                                                               \
            hr = Common::JsonWriterVisitor::Dispatch(*writer, Name, (Member), conditional ? Conditional : conditional); \
        }                                                                                                               \
        else                                                                                                            \
        {                                                                                                               \
            hr = Common::JsonReaderVisitor::Dispatch(*reader, Name, (Member), conditional ? Conditional : conditional); \
        }                                                                                                               \
        if(FAILED(hr)) return hr;

#define SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(Name, Member, Conditional)                                                               \
        if (writer != nullptr)                                                                                                         \
        {                                                                                                                              \
            hr = Common::JsonWriterVisitor::Dispatch_Int64AsNum(*writer, Name, (Member), conditional ? Conditional : conditional);     \
        }                                                                                                                              \
        else                                                                                                                           \
        {                                                                                                                              \
            hr = Common::JsonReaderVisitor::Dispatch_Int64AsNum(*reader, Name, (Member), conditional ? Conditional : conditional);     \
        }                                                                                                                              \
        if(FAILED(hr)) return hr;

#define SERIALIZABLE_PROPERTY_ENUM(Name, Member)                                                                \
        SERIALIZABLE_PROPERTY_ENUM_IF(Name, Member, true)

#define SERIALIZABLE_PROPERTY_ENUM_IF(Name, Member, Conditional)                                                \
        if (writer != nullptr)                                                                                  \
        {                                                                                                       \
            if (writer->Flags & Common::JsonSerializerFlags::EnumInStringFormat)                                \
            {                                                                                                   \
                std::wstring tmp;                                                                               \
                auto error = Common::EnumToWString(Member, tmp);                                                \
                if (!error.IsSuccess()) { return error.ToHResult(); }                                           \
                hr = Common::JsonWriterVisitor::Dispatch(*writer, Name, tmp, conditional ? Conditional : conditional); \
            }                                                                                                   \
            else                                                                                                \
            {                                                                                                   \
                hr = Common::JsonWriterVisitor::Dispatch(*writer, Name, (LONG&)(Member), conditional ? Conditional : conditional); \
            }                                                                                                   \
        }                                                                                                       \
        else                                                                                                    \
        { /* This can deserialize both enum's sent as number and string */                                      \
            hr = Common::JsonReaderVisitor::Dispatch(*reader, Name, Member, conditional ? Conditional : conditional); \
        }                                                                                                       \
        if (FAILED(hr)) return hr;

#define SERIALIZABLE_PROPERTY_ENUMP(Name, Member)                                                               \
        SERIALIZABLE_PROPERTY(Name, (std::shared_ptr<LONG>&)Member)                                             \

#define END_JSON_SERIALIZABLE_PROPERTIES()                                                                      \
        if (!newObject) { return hr; }                                                                          \
        return (writer != nullptr) ? writer->EndObject(conditional) : reader->EndObject(conditional) ;          \
    }                                                                                                           \
                                                                                                                \
    virtual HRESULT FromJson(Common::JsonReaderVisitor &reader, bool newObject = true, bool conditional = true) \
    {                                                                                                           \
        return VisitSerializable(nullptr, &reader, newObject, conditional);                                     \
    }                                                                                                           \
                                                                                                                \
    virtual HRESULT ToJson(Common::JsonWriterVisitor &writer, bool newObject = true, bool conditional = true)   \
    {                                                                                                           \
        return VisitSerializable(&writer, nullptr, newObject, conditional);                                     \
    }

//
// Macro for registering the type activation method. The method should be of the signature below. The type
// information is identified by the enum field given below.
// static std::shared_ptr<BASE_TYPE> ACTIVATOR_METHOD(ENUM_TYPE)
//

#define JSON_TYPE_ACTIVATOR_METHOD(BASE_TYPE, ENUM_TYPE, DESCRIMINATOR_MEMBER_NAME, ACTIVATOR_METHOD)            \
    static std::shared_ptr<BASE_TYPE> JsonTypeActivator(int Kind)                                                \
    {                                                                                                            \
        return ACTIVATOR_METHOD((ENUM_TYPE)Kind);                                                                \
    }                                                                                                            \
    int GetActivatorDiscriminatorValue()                                                                         \
    {                                                                                                            \
        return static_cast<int>(this->DESCRIMINATOR_MEMBER_NAME);                                                \
    }

//
// Macros to define a class that can serialize/deserialize enums to/from string.
//
#define BEGIN_DECLARE_ENUM_JSON_SERIALIZER(EnumName)    \
struct EnumToStringSerializer                           \
{                                                                   \
    static const Common::StringEnumPair<EnumName>* Get()            \
    {                                                               \
        static const Common::StringEnumPair<EnumName> pairs[] = {

#define ADD_ENUM_VALUE(EnumValue)                                   \
            {COMMON_WSTRINGIFY_I(EnumValue), EnumValue},

#define END_DECLARE_ENUM_SERIALIZER()                               \
            {nullptr}                                               \
        };                                                          \
        return pairs;                                               \
    }                                                               \
};                                                                  \
                                                                    \
/* Having these as inline functions helps us define these without explicitly \
 * getting the namespace name as input                              \
 */                                                                 \
inline Common::ErrorCode EnumToString(Enum &enumVal, __out std::wstring &enumString)   \
{                                                                       \
    return Common::EnumToWStringHelper(enumVal, enumString, EnumToStringSerializer::Get());       \
}                                                                       \
inline Common::ErrorCode EnumFromString(std::wstring const &enumString, __out Enum &enumVal)\
{                                                                                           \
    return Common::WStringToEnumHelper(enumString, enumVal, EnumToStringSerializer::Get());        \
}

#define DEFINE_ENUM_JSON_SERIALIZER(EnumName)

#define BEGIN_DEFINE_IDL_ENUM_JSON_SERIALIZER(EnumName)                 \
    template<>                                                          \
    struct EnumJsonSerializer<EnumName>                                 \
    {                                                                   \
        static const bool Marker = true;                                \
                                                                        \
        static const StringEnumPair<EnumName>* Get()                    \
        {                                                               \
            static const StringEnumPair<EnumName> pairs[] = {

#define ADD_IDL_ENUM_VALUE(EnumValue, EnumString)              \
            {EnumString, EnumValue},

#define END_DEFINE_IDL_ENUM_SERIALIZER()                               \
            {nullptr}                                                  \
            };                                                         \
        return pairs;                                                  \
        }                                                              \
    };

// Mandatory members for a base class that can be both binary
// and JSON deserialized into derived classes
//
#define KIND_ACTIVATOR_BASE_CLASS(BASE_CLASS_NAME, KIND_ENUM_NAME) \
    public: \
        BASE_CLASS_NAME() : kind_(KIND_ENUM_NAME::Invalid) { } \
        BASE_CLASS_NAME(KIND_ENUM_NAME::Enum kind) : kind_(kind) { } \
    public: \
        FABRIC_FIELDS_01(kind_) \
        virtual NTSTATUS GetTypeInformation(__out Serialization::FabricTypeInformation &) const; \
        static Serialization::IFabricSerializable * FabricSerializerActivator(Serialization::FabricTypeInformation); \
    public: \
    BEGIN_JSON_SERIALIZABLE_PROPERTIES() \
        SERIALIZABLE_PROPERTY(ServiceModel::Constants::Kind, kind_) \
    END_JSON_SERIALIZABLE_PROPERTIES() \
    public: \
        __declspec (property(get=get_Kind)) KIND_ENUM_NAME::Enum Kind; \
        KIND_ENUM_NAME::Enum get_Kind() const { return kind_; } \
        JSON_TYPE_ACTIVATOR_METHOD(BASE_CLASS_NAME, KIND_ENUM_NAME::Enum, kind_, CreateSPtr) \
        static std::shared_ptr<BASE_CLASS_NAME> CreateSPtr(KIND_ENUM_NAME::Enum); \
    protected: \
        KIND_ENUM_NAME::Enum kind_; \
