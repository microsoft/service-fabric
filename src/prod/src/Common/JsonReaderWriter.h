// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <utility>

//
// Specialcasing NodeId since vftable cannot be added to nodeid without breaking compat.
// TODO move nodeid to common.
//
#include "ServiceModel/federation/NodeId.h"

namespace Common
{
    //
    // This is the mapping between the primitive types of the REST gateway and the Json serialized type.
    // bool  - true/false
    // short - Json Number
    // LONG  - Json Number
    // byte[] - Array of Json Number
    // all other types - Json string.
    //
    class JsonWriterVisitor
    {
    public:
        JsonWriterVisitor(
            IJsonWriter* pWriter, 
            JsonSerializerFlags serializerFlags = JsonSerializerFlags::Default) 
            : m_pWriter(pWriter)
            , m_pSerializerFlags(serializerFlags)
        {}

        __declspec(property(get = get_Flags)) JsonSerializerFlags Flags;
        JsonSerializerFlags get_Flags() const { return m_pSerializerFlags; }

#define EVALUATE_CONDITION() if (condition == false) { return S_OK; }

#define WRITE_PROPERTY_NAME()                           \
    if (pszName != nullptr)                             \
        {                                               \
        HRESULT hr = m_pWriter->PropertyName(pszName);  \
        if(FAILED(hr)) { return hr; }                   \
        }                                               

#define WRITE_PROPERTY_VALUE(ValueType, Data)           \
    return m_pWriter->ValueType(Data)                   \

        HRESULT Visit(LPCWSTR pszName, bool bValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(BoolValue, bValue);
        }

        HRESULT Visit(LPCWSTR pszName, __int64 nValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()

            std::wstring strValue = StringUtility::ToWString<__int64>(nValue);

            WRITE_PROPERTY_VALUE(StringValue, strValue.data());
        }

        HRESULT Visit(LPCWSTR pszName, unsigned __int64 nValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()

            std::wstring strValue = StringUtility::ToWString<unsigned __int64>(nValue);

            WRITE_PROPERTY_VALUE(StringValue, strValue.data());
        }

        HRESULT Visit_Int64AsNum(LPCWSTR pszName, unsigned __int64 nValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(UIntValue, nValue);
        }

        HRESULT Visit_Int64AsNum(LPCWSTR pszName, __int64 nValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(IntValue, nValue);
        }

#if !defined(PLATFORM_UNIX)
        HRESULT Visit(LPCWSTR pszName, ULONG nValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(IntValue, nValue);
        }

        HRESULT Visit(LPCWSTR pszName, LONG nValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(IntValue, nValue);
        }
#endif

        HRESULT Visit(LPCWSTR pszName, unsigned int nValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(IntValue, nValue);
        }

        HRESULT Visit(LPCWSTR pszName, int nValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(IntValue, nValue);
        }

        HRESULT Visit(LPCWSTR pszName, byte value, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(IntValue, value);
        }
                
        HRESULT Visit(LPCWSTR pszName, double value, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(NumberValue, value);
        }

        HRESULT Visit(LPCWSTR pszName, LPCWSTR& strValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(StringValue, strValue);
        }

        HRESULT Visit(LPCWSTR pszName, std::wstring& strValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(StringValue, strValue.data());
        }

        HRESULT Visit(LPCWSTR pszName, LPCWSTR strValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(StringValue, strValue);
        }

        HRESULT Visit(LPCWSTR pszName, Uri& uriValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(StringValue, uriValue.ToString().data());
        }

        HRESULT Visit(LPCWSTR pszName, LargeInteger& value, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(StringValue, value.ToString().data());
        }

        HRESULT Visit(LPCWSTR pszName, Guid& guidValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()
            WRITE_PROPERTY_VALUE(StringValue, guidValue.ToString().data());
        }
        
        HRESULT Visit(LPCWSTR pszName, DateTime& dateTimeValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()

            std::wstring strValue;
            if (m_pSerializerFlags & JsonSerializerFlags::DateTimeInIsoFormat)
            {
                strValue = dateTimeValue.ToIsoString();
            }
            else
            {
                strValue = StringUtility::ToWString<__int64>(dateTimeValue.get_Ticks());
            }

            WRITE_PROPERTY_VALUE(StringValue, strValue.data());
        }

        HRESULT Visit(LPCWSTR pszName, TimeSpan& timeSpanValue, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()

            std::wstring strValue;
            if (m_pSerializerFlags & JsonSerializerFlags::DateTimeInIsoFormat)
            {
                strValue = timeSpanValue.ToIsoString();
            }
            else
            {
                strValue = StringUtility::ToWString<int64>(timeSpanValue.TotalMilliseconds());
            }

            WRITE_PROPERTY_VALUE(StringValue, strValue.data());
        }

        HRESULT Visit(LPCWSTR pszName, Federation::NodeId &nodeId, bool condition = true)
        {           
            HRESULT hr = StartObject(pszName, condition);
            if (FAILED(hr)) return hr;

            hr = Visit(L"Id", const_cast<LargeInteger&>(nodeId.IdValue), condition);
            if (FAILED(hr)) return hr;

            return EndObject(condition);
        }

        HRESULT Visit(LPCWSTR pszName, Common::FabricConfigVersion &configVersion, bool condition = true)
        {           
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()

            std::wstring strValue = configVersion.ToString();

            WRITE_PROPERTY_VALUE(StringValue, strValue.data());
        }

        template<typename T>
        HRESULT Visit(LPCWSTR pszName, std::vector<T> &objectArray, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()

            HRESULT hr = m_pWriter->ArrayStart();
            if(FAILED(hr)) { return hr; }

            for (size_t i = 0; i < objectArray.size(); i++)
            {
                __if_exists(IMPLEMENTS(T, IsSerializable))
                {
                    hr = objectArray[i].ToJson(*this, condition);
                    if (FAILED(hr)) { return hr; }
                }
                __if_not_exists(IMPLEMENTS(T, IsSerializable))
                {
                    hr = Visit(nullptr, objectArray[i], condition);
                    if (FAILED(hr)) { return hr; }
                }
            }

            return m_pWriter->ArrayEnd();
        }

        template<typename TKey, typename TValue>
        HRESULT Visit(LPCWSTR pszName, std::map<TKey, TValue> &map, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()

            HRESULT hr = m_pWriter->ArrayStart();
            if(FAILED(hr)) { return hr; }

            for (typename std::map<TKey, TValue>::iterator item = map.begin(); item != map.end(); ++item)
            {
                hr = StartObject(nullptr, condition);
                if (FAILED(hr)) { return hr; }

                TKey key = item->first;
                hr = Dispatch(*this, L"Key", key, condition);
                if (FAILED(hr)) { return hr; }

                TValue value = item->second;
                hr = Dispatch(*this, L"Value", value, condition);
                if (FAILED(hr)) { return hr; }

                hr = EndObject(condition);
                if (FAILED(hr)) { return hr; }
            }

            return m_pWriter->ArrayEnd();
        }

        // Output SimpleMapType
        //
        // Some older classes are already writing map<wstring, V> members as non-simple maps, so do not
        // override Visit<wstring, TValue>. Make this opt-in via SERIALIZABLE_PROPERTY_SIMPLE_MAP for
        // new objects only.
        //
        template<typename TValue>
        HRESULT VisitSimpleMap(LPCWSTR pszName, std::map<std::wstring, TValue> &map, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()

            auto hr = StartObject(nullptr, condition);
            if (FAILED(hr)) { return hr; }

            for (auto & item : map)
            {
                auto const & key = item.first;
                hr = Dispatch(*this, WStringLiteral(key.c_str(), key.c_str()+key.size()), item.second, condition);
                if (FAILED(hr)) { return hr; }
            }

            return EndObject(condition);
        }

        //
        // Byte array is serialized as array of Json numbers.
        //
        HRESULT Visit(LPCWSTR pszName, std::vector<BYTE> &byteArray, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME()

            HRESULT hr = m_pWriter->ArrayStart();
            if(FAILED(hr)) { return hr; }

            for (size_t i = 0; i < byteArray.size(); i++)
            {
                LONG value = byteArray[i];
                hr = Visit(nullptr, value, condition);
                if (FAILED(hr)) { return hr; }
            }

            return m_pWriter->ArrayEnd();
        }

        template<typename T>
        HRESULT Visit(LPCWSTR pszName, std::shared_ptr<T> &objectSPtr, bool condition = true)
        {
            EVALUATE_CONDITION()

            if (objectSPtr)
            {
                __if_exists(IMPLEMENTS(T, IsSerializable))
                {
                    size_t length = 0;
                    if (pszName != nullptr)
                    {
                        HRESULT hr = StringCchLength(pszName, ParameterValidator::MaxStringSize, &length);
                        if (FAILED(hr)) { return hr; }
                    }

                    return Dispatch(*this, WStringLiteral(pszName, pszName+length), *objectSPtr); 
                }
                __if_not_exists(IMPLEMENTS(T, IsSerializable))
                {
                    return Visit(pszName, *objectSPtr);
                }
            }

            return S_OK;
        }

#if defined(PLATFORM_UNIX)
#define EXPLICIT_INTRINSIC_VISIT(T) \
        template<>\
        HRESULT Visit<T>(LPCWSTR pszName, std::shared_ptr<T> &objectSPtr, bool condition)\
        {\
            EVALUATE_CONDITION()\
            if (objectSPtr)\
            {\
                return Visit(pszName, *objectSPtr);\
            }\
            return S_OK;\
        }

        EXPLICIT_INTRINSIC_VISIT(LONG)
        EXPLICIT_INTRINSIC_VISIT(bool)
#undef EXPLICIT_INTRINSIC_VISIT
#endif

        template<typename T>
        HRESULT Visit(LPCWSTR pszName, std::unique_ptr<T> &objectSPtr, bool condition = true)
        {
            EVALUATE_CONDITION()

            if (objectSPtr)
            {
                __if_exists(IMPLEMENTS(T, IsSerializable))
                {
                    size_t length = 0;
                    if (pszName != nullptr)
                    {
                        HRESULT hr = StringCchLength(pszName, ParameterValidator::MaxStringSize, &length);
                        if (FAILED(hr)) { return hr; }
                    }

                    return Dispatch(*this, WStringLiteral(pszName, pszName + length), *objectSPtr);
                }
                __if_not_exists(IMPLEMENTS(T, IsSerializable))
                {
                    return Visit(pszName, *objectSPtr);
                }
            }

            return S_OK;
        }

#if defined(PLATFORM_UNIX)
#define EXPLICIT_INTRINSIC_VISIT(T) \
    template<>\
    HRESULT Visit<T>(LPCWSTR pszName, std::unique_ptr<T> &objectSPtr, bool condition)\
        {\
        EVALUATE_CONDITION()\
        if (objectSPtr)\
        {\
        return Visit(pszName, *objectSPtr); \
        }\
        return S_OK; \
        }

        EXPLICIT_INTRINSIC_VISIT(LONG)
            EXPLICIT_INTRINSIC_VISIT(bool)
#undef EXPLICIT_INTRINSIC_VISIT
#endif

        HRESULT StartObject(LPCWSTR pszName, bool condition = true)
        {
            EVALUATE_CONDITION()
            WRITE_PROPERTY_NAME();

            return m_pWriter->ObjectStart();
        }

        HRESULT EndObject(bool condition = true)
        {
            EVALUATE_CONDITION()

            return m_pWriter->ObjectEnd();
        }

        template<typename T>
        static __forceinline HRESULT Dispatch(JsonWriterVisitor& writer, Common::WStringLiteral pszName, T const &value, bool conditional = true)
        {
            __if_exists(IMPLEMENTS(T, IsSerializable))
            {
                return Dispatch(writer, pszName, (Common::IFabricJsonSerializable&)value, conditional);
            }

            __if_not_exists(IMPLEMENTS(T, IsSerializable))
            {
                return writer.Visit(pszName.begin(), value, conditional);
            }
        }

        template<typename T>
        static __forceinline HRESULT Dispatch(JsonWriterVisitor& writer, Common::WStringLiteral pszName, T &value, bool conditional = true)
        {
            __if_exists(IMPLEMENTS(T, IsSerializable))
            {
                return Dispatch(writer, pszName, (Common::IFabricJsonSerializable&)value, conditional);
            }

            __if_not_exists(IMPLEMENTS(T, IsSerializable))
            {
                return writer.Visit(pszName.begin(), value, conditional);
            }
        }

        template<typename T>
        static __forceinline HRESULT Dispatch_Int64AsNum(JsonWriterVisitor& writer, Common::WStringLiteral pszName, T &value, bool conditional = true)
        {
            __if_not_exists(IMPLEMENTS(T, IsSerializable))
            {
                return writer.Visit_Int64AsNum(pszName.begin(), value, conditional);
            }
        }

#if defined(PLATFORM_UNIX)
#define INTRINSIC_SPECIALIZATION(type) \
        template<> \
        static __forceinline HRESULT Dispatch<type>(JsonWriterVisitor& writer, Common::WStringLiteral pszName, type &value, bool conditional) \
        { \
            return writer.Visit(pszName.begin(), value, conditional); \
        }

        INTRINSIC_SPECIALIZATION(unsigned char)
        INTRINSIC_SPECIALIZATION(INT64)
        INTRINSIC_SPECIALIZATION(unsigned int)
        INTRINSIC_SPECIALIZATION(bool)
        INTRINSIC_SPECIALIZATION(UINT64)
        INTRINSIC_SPECIALIZATION(double)
        INTRINSIC_SPECIALIZATION(int)
#undef INTRINSIC_SPECIALIZATION
#endif

        static inline HRESULT Dispatch(JsonWriterVisitor& writer, Common::WStringLiteral pszName, IFabricJsonSerializable &value, bool conditional)
        {
            HRESULT hr = writer.StartObject(pszName.begin(), conditional);
            if (FAILED(hr)) return hr;

            hr = value.ToJson(writer, false, conditional);
            if (FAILED(hr)) return hr;

            return writer.EndObject(conditional);
        }
        
    private:
        IJsonWriter* m_pWriter;
        JsonSerializerFlags m_pSerializerFlags;
    };

    class JsonHelper
    {
    public:
        template<typename T>
        static ErrorCode Serialize(T &object, __out_opt ByteBufferUPtr &bytesUPtr, JsonSerializerFlags serializerFlags = JsonSerializerFlags::Default)
        {
            ComPointer<JsonWriter> jsonWriter = make_com<JsonWriter>();
            ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
            JsonWriterVisitor visitor(jsonWriter.GetRawPointer(), serializerFlags);

            HRESULT hr = object.ToJson(visitor);
            if (FAILED(hr))
            {
                return ErrorCodeValue::SerializationError;
            }

            hr = jsonWriter->GetBytes(*bufferUPtr.get());
            if (FAILED(hr))
            {
                return ErrorCodeValue::SerializationError;
            }

            bytesUPtr = std::move(bufferUPtr);
            return ErrorCode::Success();
        }

        template<typename T>
        static ErrorCode Serialize(std::vector<T> &objectArray, __out_opt ByteBufferUPtr &bytesUPtr, JsonSerializerFlags serializerFlags = JsonSerializerFlags::Default)
        {
            ComPointer<JsonWriter> jsonWriter = make_com<JsonWriter>();
            ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
            JsonWriterVisitor visitor(jsonWriter.GetRawPointer(), serializerFlags);
            HRESULT hr;

            jsonWriter->ArrayStart();
            for (typename std::vector<T>::iterator itr = objectArray.begin(); itr != objectArray.end(); ++itr)
            {
                hr = itr->ToJson(visitor);
                if (FAILED(hr))
                {
                    return ErrorCodeValue::SerializationError;
                }
            }
            jsonWriter->ArrayEnd();

            hr = jsonWriter->GetBytes(*bufferUPtr.get());
            if (FAILED(hr))
            {
                return ErrorCodeValue::SerializationError;
            }

            bytesUPtr = std::move(bufferUPtr);
            return ErrorCode::Success();
        }

        template<typename T>
        static ErrorCode Serialize(T &object, __out_opt std::wstring &jsonString, JsonSerializerFlags serializerFlags = JsonSerializerFlags::Default)
        {
            ComPointer<JsonWriter> jsonWriter = make_com<JsonWriter>();
            ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
            JsonWriterVisitor visitor(jsonWriter.GetRawPointer(), serializerFlags);

            HRESULT hr = object.ToJson(visitor);
            if (FAILED(hr))
            {
                return ErrorCodeValue::SerializationError;
            }

            hr = jsonWriter->GetBytes(*bufferUPtr.get());
            if (FAILED(hr))
            {
                return ErrorCodeValue::SerializationError;
            }

            int ret = MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                (char*)(bufferUPtr->data()),
                (int)bufferUPtr->size(),
                nullptr,
                0);

            if (ret == 0)
            {
                return ErrorCodeValue::SerializationError;
            }

            jsonString.resize(ret, '\0');

            ret = MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                (char*)(bufferUPtr->data()),
                (int)bufferUPtr->size(),
                const_cast<WCHAR *>(jsonString.c_str()),
                ret);

            if (ret == 0)
            {
                return ErrorCodeValue::SerializationError;
            }

            return ErrorCode::Success();
        }

        //
        // Deserialize UTF-8 encoded character buffer.
        //
        template<typename T>
        static ErrorCode Deserialize(T &object, __in ByteBufferUPtr const &bytesUPtr, JsonSerializerFlags serializerFlags = JsonSerializerFlags::Default)
        {
            JsonReader::InitParams initParams((char*)(bytesUPtr->data()), static_cast<ULONG>(bytesUPtr->size()));
            ComPointer<JsonReader> jsonReader = make_com<JsonReader>(initParams);
            JsonReaderVisitor readerVisitor(jsonReader.GetRawPointer(), serializerFlags);

            HRESULT hr = readerVisitor.Deserialize(object);
            if (FAILED(hr))
            {
                return ErrorCodeValue::SerializationError;
            }

            return ErrorCode::Success();
        }

        //
        // Deserialize WCHAR buffer
        //
        template<typename T>
        static ErrorCode Deserialize(T &object, __in std::wstring const &jsonString, JsonSerializerFlags serializerFlags = JsonSerializerFlags::Default)
        {
            ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
#if !defined(PLATFORM_UNIX)
            DWORD flags = WC_ERR_INVALID_CHARS;
#else
            DWORD flags = 0; // CLR PAL does not support WC_ERR_INVALID_CHARS.
#endif                        

            int ret = WideCharToMultiByte(
                CP_UTF8,
                flags,
                jsonString.c_str(),
                (int)jsonString.length(),
                nullptr,
                0,
                NULL,
                NULL);

            if (ret == 0)
            {
                return ErrorCodeValue::SerializationError;
            }

            bufferUPtr->resize(ret);

            ret = WideCharToMultiByte(
                CP_UTF8,
                flags,
                jsonString.c_str(),
                (int)jsonString.length(),
                (char*)(bufferUPtr->data()),
                ret,
                NULL,
                NULL);

            if (ret == 0)
            {
                return ErrorCodeValue::SerializationError;
            }

            JsonReader::InitParams initParams((char*)(bufferUPtr->data()), static_cast<ULONG>(bufferUPtr->size()));
            ComPointer<JsonReader> jsonReader = make_com<JsonReader>(initParams);
            JsonReaderVisitor readerVisitor(jsonReader.GetRawPointer(), serializerFlags);

            HRESULT hr = readerVisitor.Deserialize(object);
            if (FAILED(hr))
            {
                return ErrorCodeValue::SerializationError;
            }

            return ErrorCode::Success();
        }

        template<typename T>
        static ErrorCode Deserialize(
            std::vector<T> &object,
            __in ByteBufferUPtr const &bytesUPtr,
            JsonSerializerFlags serializerFlags = JsonSerializerFlags::Default)
        {
            JsonReader::InitParams initParams((char*)(bytesUPtr->data()), static_cast<ULONG>(bytesUPtr->size()));
            ComPointer<JsonReader> jsonReader = make_com<JsonReader>(initParams);
            JsonReaderVisitor readerVisitor(jsonReader.GetRawPointer(), serializerFlags);

            HRESULT hr = jsonReader->Read();
            if (FAILED(hr))
            {
                return ErrorCodeValue::SerializationError;
            }

            hr = readerVisitor.StartObject(nullptr);
            if (FAILED(hr))
            {
                return ErrorCodeValue::SerializationError;
            }

            hr = JsonReaderVisitor::SetArray<T>(&readerVisitor, &object);
            if (FAILED(hr))
            {
                return ErrorCodeValue::SerializationError;
            }

            return ErrorCodeValue::Success;
        }

        // Deserialize vector of objects from json string
        template<typename T>
        static ErrorCode Deserialize(
            std::vector<T> &object,
            __in std::wstring const &jsonString,
            JsonSerializerFlags serializerFlags = JsonSerializerFlags::Default)
        {
            auto bufferUPtr = make_unique<ByteBuffer>();
#if !defined(PLATFORM_UNIX)
            DWORD flags = WC_ERR_INVALID_CHARS;
#else
            DWORD flags = 0; // CLR PAL does not support WC_ERR_INVALID_CHARS.
#endif                        

            int ret = WideCharToMultiByte(
                CP_UTF8,
                flags,
                jsonString.c_str(),
                (int)jsonString.length(),
                nullptr,
                0,
                NULL,
                NULL);

            if (ret == 0)
            {
                return ErrorCodeValue::SerializationError;
            }

            bufferUPtr->resize(ret);

            ret = WideCharToMultiByte(
                CP_UTF8,
                flags,
                jsonString.c_str(),
                (int)jsonString.length(),
                (char*)(bufferUPtr->data()),
                ret,
                NULL,
                NULL);

            if (ret == 0)
            {
                return ErrorCodeValue::SerializationError;
            }

            Deserialize(object, bufferUPtr, serializerFlags);
            return ErrorCodeValue::Success;
        }
    };
}