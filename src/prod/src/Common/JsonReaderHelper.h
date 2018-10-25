// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <utility>

namespace Common
{
    class JsonReaderVisitor
    {
        template<typename TKey, typename TValue>
        class MapItem : public IFabricJsonSerializable
        {
        public:

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"Key", key_)
                SERIALIZABLE_PROPERTY(L"Value", value_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            TKey key_;
            TValue value_;
        };

    public:
        JsonReaderVisitor(
            IJsonReader* pReader,
            JsonSerializerFlags serializerFlags = JsonSerializerFlags::Default)
            : m_pReader(pReader)
            , m_pSerializerFlags(serializerFlags)
        {
        }

        __declspec(property(get = get_Flags)) JsonSerializerFlags Flags;
        JsonSerializerFlags get_Flags() const { return m_pSerializerFlags; }

        template<class T>
        static HRESULT Set(JsonReaderVisitor* pVisitor, void* pData)
        {
            static_assert(false, "Unknown object format");
        }
        template<class T>
        static HRESULT SetInt64AsNumber(JsonReaderVisitor* pVisitor, void* pData)
        {
            static_assert(false, "Unknown object format");
        }
        template<>
        static HRESULT Set<bool>(JsonReaderVisitor* pVisitor, void* pData)
        {
            JsonTokenType tokenType = JsonTokenType_NotStarted;
            HRESULT hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            bool *pbData = (bool*)(pData);
            switch(tokenType)
            {
            case JsonTokenType_True: *pbData = true; return S_OK;
            case JsonTokenType_False: *pbData = false; return S_OK;
            default: 
                return E_FAIL; // expected bool value
            }
        }

        template<>
        static HRESULT Set<byte>(JsonReaderVisitor* pVisitor, void* pData)
        {
            double dblValue = 0.0;
            HRESULT hr = pVisitor->m_pReader->GetNumberValue(&dblValue);
            if(FAILED(hr)) { return hr; }

            if (dblValue < 0) { return E_FAIL; }
            if (dblValue > UCHAR_MAX) { return E_FAIL; }

            byte* pnData = (byte*)pData;
            *pnData = (byte)dblValue;
            return hr;
        }
                
        template<>
        static HRESULT Set<double>(JsonReaderVisitor* pVisitor, void* pData)
        {
            double dblValue = 0.0;
            HRESULT hr = pVisitor->m_pReader->GetNumberValue(&dblValue);
            if(FAILED(hr)) { return hr; }

            double* pnData = (double*)pData;
            *pnData = (double)dblValue;
            return hr;
        }

#if !defined(PLATFORM_UNIX)
        template<>
        static HRESULT Set<LONG>(JsonReaderVisitor* pVisitor, void* pData)
        {
            double dblValue = 0.0;
            HRESULT hr = pVisitor->m_pReader->GetNumberValue(&dblValue);
            if(FAILED(hr)) { return hr; }

            if (dblValue < LONG_MIN) { return E_FAIL; }
            if (dblValue > LONG_MAX) { return E_FAIL; }

            LONG* pnData = (LONG*)pData;
            *pnData = (LONG)dblValue;
            return hr;
        }

        template<>
        static HRESULT Set<ULONG>(JsonReaderVisitor* pVisitor, void* pData)
        {
            double dblValue = 0.0;
            HRESULT hr = pVisitor->m_pReader->GetNumberValue(&dblValue);
            if(FAILED(hr)) { return hr; }

            if (dblValue < 0) { return E_FAIL; }
            if (dblValue > ULONG_MAX) { return E_FAIL; }

            ULONG* pnData = (ULONG*)pData;
            *pnData = (ULONG)dblValue;
            return hr;
        }
#else
        template<>
        static HRESULT Set<INT64>(JsonReaderVisitor* pVisitor, void* pData)
        {
            std::wstring strValue;
            HRESULT hr = Set<std::wstring>(pVisitor, &strValue);
            if (FAILED(hr)) { return hr; }

            INT64 *nValue = (INT64*) pData;
            *nValue = 0;
            if (strValue.empty()) { return S_OK; }
            if (!StringUtility::TryFromWString<INT64>(strValue, *nValue)) return E_FAIL;

            return S_OK;
        }

        template<>
        static HRESULT Set<UINT64>(JsonReaderVisitor* pVisitor, void* pData)
        {
            std::wstring strValue;
            HRESULT hr = Set<std::wstring>(pVisitor, &strValue);
            if (FAILED(hr)) { return hr; }

            UINT64 *nValue = (UINT64*) pData;
            *nValue = 0;
            if (strValue.empty()) { return S_OK; }
            if (!StringUtility::TryFromWString<UINT64>(strValue, *nValue)) return E_FAIL;

            return S_OK;
        }
#endif

        template<>
        static HRESULT Set<int>(JsonReaderVisitor* pVisitor, void* pData)
        {
            double dblValue = 0.0;
            HRESULT hr = pVisitor->m_pReader->GetNumberValue(&dblValue);
            if(FAILED(hr)) { return hr; }

            if (dblValue < INT_MIN) { return E_FAIL; }
            if (dblValue > INT_MAX) { return E_FAIL; }

            int* pnData = (int*)pData;
            *pnData = (int)dblValue;
            return hr;
        }

        template<>
        static HRESULT Set<unsigned int>(JsonReaderVisitor* pVisitor, void* pData)
        {
            double dblValue = 0.0;
            HRESULT hr = pVisitor->m_pReader->GetNumberValue(&dblValue);
            if(FAILED(hr)) { return hr; }

            if (dblValue < 0) { return E_FAIL; }
            if (dblValue > UINT_MAX) { return E_FAIL; }

            unsigned int* pnData = (unsigned int*)pData;
            *pnData = (unsigned int)dblValue;
            return hr;
        }

        template<>
        static HRESULT SetInt64AsNumber<INT64>(JsonReaderVisitor* pVisitor, void* pData)
        {
            double dblValue = 0.0;
            HRESULT hr = pVisitor->m_pReader->GetNumberValue(&dblValue);
            if (FAILED(hr)) { return hr; }

            if (dblValue < LLONG_MIN) { return E_FAIL; }
            if (dblValue > LLONG_MAX) { return E_FAIL; }

            INT64* pnData = (INT64*)pData;
            *pnData = (INT64)dblValue;
            return hr;
        }

        template<>
        static HRESULT SetInt64AsNumber<UINT64>(JsonReaderVisitor* pVisitor, void* pData)
        {
            double dblValue = 0.0;
            HRESULT hr = pVisitor->m_pReader->GetNumberValue(&dblValue);
            if (FAILED(hr)) { return hr; }

            if (dblValue < 0) { return E_FAIL; }
            if (dblValue > ULLONG_MAX) { return E_FAIL; }

            UINT64* pnData = (UINT64*)pData;
            *pnData = (UINT64)dblValue;
            return hr;
        }

        template<>
        static HRESULT Set<Guid>(JsonReaderVisitor* pVisitor, void* pData)
        {
            ULONG nLength = 0;
            HRESULT hr = pVisitor->m_pReader->GetStringLength(&nLength);
            if(FAILED(hr)) { return hr; }

            ULONG bufferSize;
            hr = ULongAdd(nLength, 1, &bufferSize);
            if (FAILED(hr)) return hr;

            std::vector<WCHAR> buffer(bufferSize);
            hr = pVisitor->m_pReader->GetStringValue(buffer.data(), bufferSize);
            if(FAILED(hr)) return hr;

            Guid* pGuid = (Guid*)pData;
            *pGuid = Guid(buffer.data());

            return S_OK;
        }

        template<>
        static HRESULT Set<DateTime>(JsonReaderVisitor* pVisitor, void* pData)
        {
            std::wstring strValue;
            HRESULT hr = Set<std::wstring>(pVisitor, &strValue);
            if (FAILED(hr)) { return hr; }

            DateTime *pDateTime = (DateTime *)pData;
            if (strValue.empty()) 
            {
                *pDateTime = DateTime(0); 
                return S_OK;
            }

            if (!DateTime::TryParse(strValue, *pDateTime))
            {
                __int64 ticks = 0;
                if (!StringUtility::TryFromWString<__int64>(strValue, ticks)) return E_FAIL;
                *pDateTime = DateTime(ticks);
            }

            return S_OK;
        }
        
        template<>
        static HRESULT Set<TimeSpan>(JsonReaderVisitor* pVisitor, void* pData)
        {
            std::wstring strValue;
            HRESULT hr = Set<std::wstring>(pVisitor, &strValue);
            if (FAILED(hr)) { return hr; }

            TimeSpan* pTimeSpan = (TimeSpan *)pData;
            if (strValue.empty()) { return S_OK; }
            if (!TimeSpan::TryFromIsoString(strValue, *pTimeSpan))
            {
                int64 millis = 0;
                if (!StringUtility::TryFromWString<int64>(strValue, millis)) return E_FAIL;

                *pTimeSpan = TimeSpan::FromMilliseconds(static_cast<double>(millis));
            }

            return S_OK;
        }

        template<>
        static HRESULT Set<std::wstring>(JsonReaderVisitor* pVisitor, void* pData)
        {
            //
            // This code to read numbers as wstring exists only to support deserializing enum's given as
            // string. This can be improved.
            //
            JsonTokenType tokenType;
            HRESULT hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            std::wstring* pstrData = (std::wstring *)pData;
            if (tokenType == JsonTokenType_String)
            {
                ULONG nLength = 0;
                hr = pVisitor->m_pReader->GetStringLength(&nLength);
                if(FAILED(hr)) { return hr; }

                ULONG bufferSize;
                hr = ULongAdd(nLength, 1, &bufferSize);
                if (FAILED(hr)) return hr;

                std::vector<WCHAR> buffer(bufferSize);
                hr = pVisitor->m_pReader->GetStringValue(buffer.data(), bufferSize);
                if (FAILED(hr)) return hr;

                *pstrData = std::wstring(buffer.data());

                return hr;
            }
            else if (tokenType == JsonTokenType_Number)
            {
                DOUBLE number;
                hr = pVisitor->m_pReader->GetNumberValue(&number);
                if (FAILED(hr)) { return hr; }
                *pstrData = StringUtility::ToWString<DOUBLE>(number);

                return S_OK;
            }

            return E_FAIL;
        }

        template<>
        static HRESULT Set<LargeInteger>(JsonReaderVisitor* pVisitor, void *pData)
        {
            ULONG nLength = 0;
            HRESULT hr = pVisitor->m_pReader->GetStringLength(&nLength);
            if(FAILED(hr)) { return hr; }

            ULONG bufferSize;
            hr = ULongAdd(nLength, 1, &bufferSize);
            if (FAILED(hr)) return hr;

            std::vector<WCHAR> buffer(bufferSize);
            hr = pVisitor->m_pReader->GetStringValue(buffer.data(), bufferSize);
            if(FAILED(hr)) return hr;

            LargeInteger* plargeInt = (LargeInteger *)pData;
            if (!LargeInteger::TryParse(buffer.data(), *plargeInt)) return E_FAIL;

            return S_OK;
        }

        template<>
        static HRESULT Set<Uri>(JsonReaderVisitor *pVisitor, void *pData)
        {
            ULONG nLength = 0;
            HRESULT hr = pVisitor->m_pReader->GetStringLength(&nLength);
            if(FAILED(hr)) { return hr; }

            ULONG bufferSize;
            hr = ULongAdd(nLength, 1, &bufferSize);
            if (FAILED(hr)) return hr;

            std::vector<WCHAR> buffer(bufferSize);
            hr = pVisitor->m_pReader->GetStringValue(buffer.data(), bufferSize);
            if(FAILED(hr)) return hr;

            Uri *uri = (Uri *)pData;
            if (!Uri::TryParse(buffer.data(), *uri)) return E_FAIL;

            return S_OK;
        }

        template<>
        static HRESULT Set<NamingUri>(JsonReaderVisitor *pVisitor, void *pData)
        {
            ULONG nLength = 0;
            HRESULT hr = pVisitor->m_pReader->GetStringLength(&nLength);
            if(FAILED(hr)) { return hr; }

            ULONG bufferSize;
            hr = ULongAdd(nLength, 1, &bufferSize);
            if (FAILED(hr)) return hr;

            std::vector<WCHAR> buffer(bufferSize);
            hr = pVisitor->m_pReader->GetStringValue(buffer.data(), bufferSize);
            if(FAILED(hr)) return hr;

            NamingUri *nuri = (NamingUri *)pData;
            if (!NamingUri::TryParse(buffer.data(), *nuri)) return E_FAIL;

            return S_OK;
        }

        template<>
        static HRESULT Set<FabricConfigVersion>(JsonReaderVisitor *pVisitor, void *pData)
        {
            ULONG nLength = 0;
            HRESULT hr = pVisitor->m_pReader->GetStringLength(&nLength);
            if(FAILED(hr)) { return hr; }

            ULONG bufferSize;
            hr = ULongAdd(nLength, 1, &bufferSize);
            if (FAILED(hr)) return hr;

            std::vector<WCHAR> buffer(bufferSize);
            hr = pVisitor->m_pReader->GetStringValue(buffer.data(), bufferSize);
            if(FAILED(hr)) return hr;

            FabricConfigVersion *pVersion = (FabricConfigVersion *)pData;
            auto error = FabricConfigVersion::FromString(buffer.data(), *pVersion);

            return error.ToHResult();
        }

#if !defined(PLATFORM_UNIX) 
        template<>
        static HRESULT Set<__int64>(JsonReaderVisitor *pVisitor, void *pData)
        {
            std::wstring strValue;
            HRESULT hr = Set<std::wstring>(pVisitor, &strValue);
            if (FAILED(hr)) { return hr; }

            __int64 *nValue = (__int64*) pData;
            *nValue = 0;
            if (strValue.empty()) { return S_OK; }
            if (!StringUtility::TryFromWString<__int64>(strValue, *nValue)) return E_FAIL;

            return S_OK;
        }

        template<>
        static HRESULT Set<unsigned __int64>(JsonReaderVisitor *pVisitor, void *pData)
        {
            std::wstring strValue;
            HRESULT hr = Set<std::wstring>(pVisitor, &strValue);
            if (FAILED(hr)) { return hr; }

            unsigned __int64 *nValue = (unsigned __int64*) pData;
            *nValue = 0;
            if (strValue.empty()) { return S_OK; }
            if (!StringUtility::TryFromWString<unsigned __int64>(strValue, *nValue)) return E_FAIL;

            return S_OK;
        }
#endif

        // Specialcasing NodeId since vftable cannot be added to nodeid without breaking compat.
        template<>
        static HRESULT Set<Federation::NodeId>(JsonReaderVisitor *pVisitor, void *pData)
        {
            InternalNodeId nodeIdInternal;
            
            // The current token should already be begin object.
            JsonTokenType tokenType;
            HRESULT hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            if(tokenType != JsonTokenType_BeginObject) { return E_FAIL; }

            hr = nodeIdInternal.FromJson(*pVisitor);
            if (FAILED(hr)) { return hr; }

            hr = pVisitor->ParseJsonObject();

            Federation::NodeId *nodeId = (Federation::NodeId*)pData;
            *nodeId = Federation::NodeId(nodeIdInternal.id_);

            return S_OK;
        }

        template<class T>
        static HRESULT SetEnum(JsonReaderVisitor *pVisitor, void *pData)
        {
            T * enumVal = (T*)pData;

            JsonTokenType tokenType;
            HRESULT hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            if (tokenType == JsonTokenType_Number)
            {
                hr = Set<int>(pVisitor, pData);
                if (FAILED(hr)) { return hr; }
            }
            else
            {
                std::wstring strValue;
                hr = Set<std::wstring>(pVisitor, &strValue);
                if (FAILED(hr)) { return hr; }

                Common::ErrorCode error = Common::WStringToEnum(strValue, *enumVal);
                if (error.IsSuccess()) { return S_OK; }

                if (!StringUtility::TryFromWString<int>(strValue, (int &)*enumVal)) { return E_FAIL; }
            }

            return S_OK;
        }

        //
        // When the key is wstring, Map<wstring, value> can be serialized as
        //
        // [{ "Key" : "<wstring_key>", "Value" : "<Actual value> }, { "Key" : "<wstring_key1>", "Value" : "<Actual value>"}]
        //
        // or
        //
        // { "<wstring_key>" : "Actual value", "<wstring_key1>" : "Actual value" } -- indicated by SimpleMapType in this code.
        //
        //
        template<typename TKey, typename TValue>
        static HRESULT SetMap(JsonReaderVisitor *pVisitor, void *pData)
        {
            JsonTokenType tokenType;

            //
            // Read start token
            //
            HRESULT hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            if (tokenType == JsonTokenType_BeginArray) 
            { 
                std::map<TKey, TValue> *map = static_cast<std::map<TKey, TValue> *>(pData);

                for ( ; ; )
                {
                    MapItem<TKey, TValue> item;
                    hr = pVisitor->Deserialize(item);

                    if (FAILED(hr)) 
                    {
                        hr = pVisitor->m_pReader->GetTokenType(&tokenType);
                        if(FAILED(hr)) { return hr; }

                        if (tokenType != JsonTokenType_EndArray)
                        {
                            return E_FAIL;
                        }

                        //
                        // Remove the empty object the failing Dispatch 
                        // created.
                        //
                        pVisitor->objectStack_.pop_back();
                        pVisitor->currentObject_ = pVisitor->objectStack_.back();
                        break;
                    }

                    map->insert(std::make_pair(std::move(item.key_), std::move(item.value_)));
                }

                return S_OK;
            }
            else if (tokenType == JsonTokenType_BeginObject)
            {
                return SetSimpleMap<TValue>(pVisitor, pData);
            }
            else
            {
                return E_FAIL;
            }
        }

        template<typename TValue>
        static HRESULT SetSimpleMap(JsonReaderVisitor *pVisitor, void *pData)
        {
            JsonTokenType tokenType;

            HRESULT hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            ASSERT_IF(tokenType != JsonTokenType_BeginObject, "Object not in valid SimpleMap format");

            pVisitor->m_pReader->SaveContext();

            hr = pVisitor->m_pReader->Read();
            if(FAILED(hr)) { return hr; }
            hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if (FAILED(hr)) { return hr; }

            std::map<std::wstring, TValue> *map = static_cast<std::map<std::wstring, TValue> *>(pData);

            //
            // Simple Map type
            //
            while (tokenType != JsonTokenType_EndObject)
            {
                MapItem<std::wstring, TValue> item;

                if (tokenType != JsonTokenType_FieldName) { return E_FAIL; }

                //
                // For Simple Map Items the key is the fieldName
                //
                hr = pVisitor->GetFieldName(item.key_);
                if (FAILED(hr)) { return hr; }

                hr = pVisitor->m_pReader->Read();
                if(FAILED(hr)) { return hr; }

                //
                // Deserialize the value item
                //
                __if_exists(IMPLEMENTS(TValue, VisitSerializable))
                {
                    hr = item.value_.FromJson(*pVisitor);
                    if (FAILED(hr)) { return hr; }

                    hr = pVisitor->ParseJsonObject();
                    if (FAILED(hr)) { return hr; }
                }

                __if_not_exists(IMPLEMENTS(TValue, VisitSerializable))
                {
                    hr = pVisitor->m_pReader->GetTokenType(&tokenType);
                    if (FAILED(hr)){ return hr; }

                    if (tokenType == JsonTokenType_BeginArray)
                    {
                        hr = E_NOTIMPL;
                    }
                    else
                    {
                        hr = Set<TValue>(pVisitor, &(item.value_));
                    }
                    if (FAILED(hr)) { return hr; }
                }

                hr = pVisitor->m_pReader->Read();
                if(FAILED(hr)) { return hr; }

                hr = pVisitor->m_pReader->GetTokenType(&tokenType);
                if (FAILED(hr)) { return hr; }

                map->insert(std::make_pair(std::move(item.key_), std::move(item.value_)));
            }

            pVisitor->m_pReader->RestoreContext();
            return pVisitor->m_pReader->SkipObject();
        }

        template <class T>
        struct is_std_vector { static const bool value = false; };

        template <class T>
        struct is_std_vector<std::vector<T> > { static const bool value = true; };

        //only support vector as value in the map
        template<typename TCollection, typename = std::enable_if<is_std_vector<TCollection>::value, int>>
        static HRESULT SetSimpleMap1(JsonReaderVisitor *pVisitor, void *pData)
        {
            JsonTokenType tokenType;

            HRESULT hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if (FAILED(hr)) { return hr; }

            ASSERT_IF(tokenType != JsonTokenType_BeginObject, "Object not in valid SimpleMap format");

            pVisitor->m_pReader->SaveContext();

            hr = pVisitor->m_pReader->Read();
            if (FAILED(hr)) { return hr; }
            hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if (FAILED(hr)) { return hr; }

            std::map<std::wstring, TCollection> *map = static_cast<std::map<std::wstring, TCollection> *>(pData);

            //
            // Simple Map type
            //
            while (tokenType != JsonTokenType_EndObject)
            {
                MapItem<std::wstring, TCollection> item;

                if (tokenType != JsonTokenType_FieldName) { return E_FAIL; }

                //
                // For Simple Map Items the key is the fieldName
                //
                hr = pVisitor->GetFieldName(item.key_);
                if (FAILED(hr)) { return hr; }

                hr = pVisitor->m_pReader->Read();
                if (FAILED(hr)) { return hr; }

                hr = pVisitor->m_pReader->GetTokenType(&tokenType);
                if (FAILED(hr)) { return hr; }

                //this is only for maps of vectors
                if (tokenType != JsonTokenType_BeginArray)
                {
                    hr = E_NOTIMPL;
                }
                else
                {
                    hr = SetArray<typename TCollection::value_type>(pVisitor, &(item.value_));
                }
                if (FAILED(hr)) { return hr; }


                hr = pVisitor->m_pReader->Read();
                if (FAILED(hr)) { return hr; }

                hr = pVisitor->m_pReader->GetTokenType(&tokenType);
                if (FAILED(hr)) { return hr; }

                map->insert(std::make_pair(std::move(item.key_), std::move(item.value_)));
            }

            pVisitor->m_pReader->RestoreContext();
            return pVisitor->m_pReader->SkipObject();
        }

        template<typename TKey, typename TValue>
        static HRESULT SetSimpleMap(JsonReaderVisitor *pVisitor, void *pData)
        {
            static_assert(false, "Only 'wstring' keys are supported for SimpleMap types");
            return E_FAIL;
        }

        template<typename T>
        static HRESULT SetSharedPointerArray(JsonReaderVisitor *pVisitor, void *pData)
        {
            JsonTokenType tokenType;

            //
            // Read array start token
            //       
            HRESULT hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            if(tokenType != JsonTokenType_BeginArray) { return E_FAIL; }

            std::vector<std::shared_ptr<T>> *objectArray = static_cast<std::vector<std::shared_ptr<T>> *>(pData);
            pVisitor->m_pReader->Read();
            hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            while (tokenType != JsonTokenType_EndArray)
            {
                std::shared_ptr<T> item;
                hr = SetSharedPointer<T>(pVisitor, (void*)&item);
                if (FAILED(hr)) { return hr; }

                objectArray->push_back(std::move(item));
                pVisitor->m_pReader->Read();
                hr = pVisitor->m_pReader->GetTokenType(&tokenType);
                if (FAILED(hr)) { return hr; }
            }           

            return S_OK;
        }

        template<typename T>
        static HRESULT SetArray(JsonReaderVisitor *pVisitor, void *pData)
        {
            JsonTokenType tokenType;

            //
            // Read array start token
            //       
            HRESULT hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            if(tokenType != JsonTokenType_BeginArray) { return E_FAIL; }

            std::vector<T> *objectArray = static_cast<std::vector<T> *>(pData);

            //
            // Read the values.
            //
            __if_not_exists(IMPLEMENTS(T, VisitSerializable))
            {
                pVisitor->m_pReader->Read();
                hr = pVisitor->m_pReader->GetTokenType(&tokenType);
                if(FAILED(hr)) { return hr; }

                while (tokenType != JsonTokenType_EndArray)
                {
                    T item;
                    // Read the primitive type
                    hr = Set<T>(pVisitor, (void*)&item);
                    if (FAILED(hr)) { return hr; }

                    objectArray->push_back(std::move(item));
                    pVisitor->m_pReader->Read();
                    hr = pVisitor->m_pReader->GetTokenType(&tokenType);
                    if (FAILED(hr)) { return hr; }
                }           
            }

            __if_exists(IMPLEMENTS(T, VisitSerializable))
            {
                for ( ; ; )
                {
                    T item;

                    hr = pVisitor->Deserialize(item);
                    if (FAILED(hr)) 
                    {
                        hr = pVisitor->m_pReader->GetTokenType(&tokenType);
                        if(FAILED(hr)) { return hr; }

                        if (tokenType != JsonTokenType_EndArray)
                        {
                            return E_FAIL;
                        }

                        //
                        // Remove the empty object the failing Dispatch 
                        // created.
                        //
                        pVisitor->objectStack_.pop_back();
                        pVisitor->currentObject_ = pVisitor->objectStack_.back();
                        break;
                    }
                    objectArray->push_back(std::move(item));
                }
            }

            return S_OK;
        }

        //
        // Byte array is serialized as an array of JSON Numbers, so we need to specialize the 
        // array deserialization for it.
        //
        template<>
        static HRESULT SetArray<BYTE>(JsonReaderVisitor *pVisitor, void *pData)
        {
            JsonTokenType tokenType;

            //
            // Read array start token
            //       
            HRESULT hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            if(tokenType != JsonTokenType_BeginArray) { return E_FAIL; }

            std::vector<BYTE> *objectArray = static_cast<std::vector<BYTE> *>(pData);

            pVisitor->m_pReader->Read();
            hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            while (tokenType != JsonTokenType_EndArray)
            {
                BYTE byte;
                LONG value;

                hr = Set<LONG>(pVisitor, (void*)&value);
                if (FAILED(hr)) { return hr; }

                byte = (BYTE)value;
                objectArray->push_back(byte);
                pVisitor->m_pReader->Read();
                hr = pVisitor->m_pReader->GetTokenType(&tokenType);
                if (FAILED(hr)) { return hr; }
            }  

            return S_OK;
        }

        template<typename T>
        static HRESULT ActivateSharedPointer(JsonReaderVisitor & pVisitor, std::shared_ptr<T> & sharedPtr)
        {
            //
            // Call the type activation method if the object contains a KIND field
            //
            __if_exists(IMPLEMENTS(T, JsonTypeActivator))
            {
                T baseObject;
                if (GetBaseTypeIfDynamicType(pVisitor, baseObject))
                {
                    sharedPtr = std::dynamic_pointer_cast<T>(T::JsonTypeActivator(baseObject.GetActivatorDiscriminatorValue()));
                    if (!sharedPtr) { return E_FAIL; }
                }
                else
                {
                    sharedPtr = std::make_shared<T>();
                }
            }

            __if_not_exists(IMPLEMENTS(T, JsonTypeActivator))
            {
                UNREFERENCED_PARAMETER(pVisitor);
                sharedPtr = std::make_shared<T>();
            }

            return S_OK;
        }

        template<typename T>
        static HRESULT SetSharedPointer(JsonReaderVisitor *pVisitor, void *pData)
        {
            std::shared_ptr<T> *sharedPtr = static_cast<std::shared_ptr<T>*>(pData);

            __if_exists(IMPLEMENTS(T, VisitSerializable))
            {
                HRESULT hr = ActivateSharedPointer(*pVisitor, *sharedPtr);
                if (FAILED(hr)) { return hr; }

                //
                // Set* is called after reading the initial token,
                // Gather the object information, and start parsing the object
                //
                hr = (*sharedPtr)->FromJson(*pVisitor);
                if (FAILED(hr)) { return hr; }

                //
                // Deserialize the object.
                //
                return pVisitor->ParseJsonObject();
            }

            __if_not_exists(IMPLEMENTS(T, VisitSerializable))
            {
                *sharedPtr = std::make_shared<T>();
                return Set<T>(pVisitor, (*sharedPtr).get());
            }
        }

#if defined(PLATFORM_UNIX)
#define EXPLICIT_INTRINSIC_SETSHAREDPOINTER(T) \
        template<>\
        static HRESULT SetSharedPointer<T>(JsonReaderVisitor *pVisitor, void *pData)\
        {\
            std::shared_ptr<T> *sharedPtr = static_cast<std::shared_ptr<T>*>(pData);\
            *sharedPtr = std::make_shared<T>();\
            return Set<T>(pVisitor, (*sharedPtr).get());\
        }\

        EXPLICIT_INTRINSIC_SETSHAREDPOINTER(LONG)
        EXPLICIT_INTRINSIC_SETSHAREDPOINTER(bool)
#undef EXPLICIT_INTRINSIC_SETSHAREDPOINTER^M
#endif

        template<typename T>
        static HRESULT SetUniquePointer(JsonReaderVisitor *pVisitor, void *pData)
        {
            std::unique_ptr<T> *uniquePtr = static_cast<std::unique_ptr<T>*>(pData);

            __if_exists(IMPLEMENTS(T, VisitSerializable))
            {
                *uniquePtr = make_unique<T>();

                //
                // Set* is called after reading the initial token,
                // Gather the object information, and start parsing the object
                //
                auto hr = (*uniquePtr)->FromJson(*pVisitor);
                if (FAILED(hr)) { return hr; }

                //
                // Deserialize the object.
                //
                return pVisitor->ParseJsonObject();
            }

            __if_not_exists(IMPLEMENTS(T, VisitSerializable))
            {
                *uniquePtr = make_unique<T>();
                return Set<T>(pVisitor, (*uniquePtr).get());
            }
        }

#if defined(PLATFORM_UNIX)
#define EXPLICIT_INTRINSIC_SETUNIQUEPOINTER(T) \
    template<>\
    static HRESULT SetUniquePointer<T>(JsonReaderVisitor *pVisitor, void *pData)\
        {\
        std::unique_ptr<T> *uniquePtr = static_cast<std::unique_ptr<T>*>(pData); \
        *uniquePtr = make_unique<T>(); \
        return Set<T>(pVisitor, (*uniquePtr).get()); \
        }\

        EXPLICIT_INTRINSIC_SETUNIQUEPOINTER(LONG)
            EXPLICIT_INTRINSIC_SETUNIQUEPOINTER(bool)
#undef EXPLICIT_INTRINSIC_SETUNIQUEPOINTER^M
#endif
        
        template<typename T>
        HRESULT Visit(LPCWSTR pszName, T& value, bool conditional = true)
        {
            UNREFERENCED_PARAMETER(conditional);

            void* pData = &value;
            std::wstring fieldName = pszName;
            Setter setter = nullptr;
            const bool isEnum = std::is_enum<T>::value;

            setter = SetterLookup<isEnum, T>::GetSetter();

            ASSERT_IF(setter == nullptr, "Type doesnt have a setter");

            ValueFiller valueFiller(std::make_pair(setter, pData));

            ASSERT_IF(
                currentObject_->map.count(fieldName) != 0,
                "Object having multiple values with the same fieldname");
            currentObject_->map[fieldName] = valueFiller;
            return S_OK;
        }

        template<typename T>
        HRESULT VisitInt64AsNumber(LPCWSTR pszName, T& value, bool conditional = true)
        {
            UNREFERENCED_PARAMETER(conditional);

            void* pData = &value;
            std::wstring fieldName = pszName;
            Setter setter = nullptr;
            const bool isEnum = std::is_enum<T>::value;

            setter = SetterLookup<isEnum, T>::GetInt64AsNumberSetter();

            ASSERT_IF(setter == nullptr, "Type doesnt have a setter");

            ValueFiller valueFiller(std::make_pair(setter, pData));

            ASSERT_IF(
                currentObject_->map.count(fieldName) != 0,
                "Object having multiple values with the same fieldname");
            currentObject_->map[fieldName] = valueFiller;
            return S_OK;
        }

        template<typename T>
        HRESULT Visit(LPCWSTR pszName, std::vector<T> &objectArray, bool conditional = true)
        {
            UNREFERENCED_PARAMETER(conditional);

            Setter setter = &SetArray<T>;
            void* pData = &objectArray;
            ValueFiller valueFiller(std::make_pair(setter, pData));
            std::wstring fieldName = pszName;

            ASSERT_IF(
                currentObject_->map.count(fieldName) != 0,
                "Object having multiple values with the same fieldname");
            currentObject_->map[fieldName] = valueFiller;

            return S_OK;
        }

        template<typename T>
        HRESULT Visit(LPCWSTR pszName, std::vector<std::shared_ptr<T>> &objectArray, bool conditional = true)
        {
            UNREFERENCED_PARAMETER(conditional);

            Setter setter = &SetSharedPointerArray<T>;
            void* pData = &objectArray;
            ValueFiller valueFiller(std::make_pair(setter, pData));
            std::wstring fieldName = pszName;

            ASSERT_IF(
                currentObject_->map.count(fieldName) != 0,
                "Object having multiple values with the same fieldname");
            currentObject_->map[fieldName] = valueFiller;

            return S_OK;
        }

        template<typename TKey, typename TValue>
        HRESULT Visit(LPCWSTR pszName, std::map<TKey, TValue> &map, bool conditional = true)
        {
            UNREFERENCED_PARAMETER(conditional);

            Setter setter = &SetMap<TKey, TValue>;
            void* pData = &map;
            ValueFiller valueFiller(std::make_pair(setter, pData));
            std::wstring fieldName = pszName;

            ASSERT_IF(
                objectStack_.back()->map.count(fieldName) != 0, 
                "Object having multiple values with the same fieldname");
            currentObject_->map[fieldName] = valueFiller;

            return S_OK;
        }

        template<typename T>
        HRESULT Visit(LPCWSTR pszName, std::shared_ptr<T> &sptr, bool conditional = true)
        {
            UNREFERENCED_PARAMETER(conditional);

            Setter setter = &SetSharedPointer<T>;
            void* pData = &sptr;
            ValueFiller valueFiller(std::make_pair(setter, pData));
            std::wstring fieldName = pszName;

            ASSERT_IF(
                objectStack_.back()->map.count(fieldName) != 0, 
                "Object having multiple values with the same fieldname");
            currentObject_->map[fieldName] = valueFiller;

            return S_OK;
        }

        template<typename TKey, typename TValue>
        HRESULT Visit(LPCWSTR pszName, std::map<TKey, std::vector<TValue>> &map, bool conditional = true)
        {
            UNREFERENCED_PARAMETER(conditional);
            Setter setter = &SetSimpleMap1<std::vector<TValue>>;
            void* pData = &map;
            ValueFiller valueFiller(std::make_pair(setter, pData));
            
            std::wstring fieldName = pszName;

            ASSERT_IF(
                objectStack_.back()->map.count(fieldName) != 0,
                "Object having multiple values with the same fieldname");
            currentObject_->map[fieldName] = valueFiller;
            return S_OK;
        }

        template<typename T>
        HRESULT Visit(LPCWSTR pszName, std::unique_ptr<T> &sptr, bool conditional = true)
        {
            UNREFERENCED_PARAMETER(conditional);

            Setter setter = &SetUniquePointer<T>;
            void* pData = &sptr;
            ValueFiller valueFiller(std::make_pair(setter, pData));
            std::wstring fieldName = pszName;

            ASSERT_IF(
                objectStack_.back()->map.count(fieldName) != 0,
                "Object having multiple values with the same fieldname");
            currentObject_->map[fieldName] = valueFiller;

            return S_OK;
        }

        HRESULT StartObject(LPCWSTR pszName, bool conditional = true)
        {
            UNREFERENCED_PARAMETER(conditional);

            if(pszName != nullptr) 
            { 
                ObjectSPtr object = std::make_shared<Object>();
                object->isNested = true;
                ValueFiller valueFiller(object);
                std::wstring fieldName = pszName;

                ASSERT_IF(
                    currentObject_->map.count(fieldName) != 0,
                    "Object having multiple values with the same fieldname");

                currentObject_->map.insert(std::make_pair(fieldName, valueFiller));

                //
                // Update the current object
                //
                if (currentObject_->isNested)
                {
                    objectStack_.push_back(currentObject_);
                }
                currentObject_ = object;
            }
            else
            {
                ObjectSPtr object = std::make_shared<Object>();
                objectStack_.push_back(object);
                currentObject_ = object;
            }

            return S_OK;
        }

        HRESULT EndObject(bool conditional = true)
        {
            UNREFERENCED_PARAMETER(conditional);   

            //
            // If the current object is a nested object, set the currentObject to the 
            // parent object here.
            //
            if (!objectStack_.empty() && currentObject_->isNested)
            {
                currentObject_ = objectStack_.back();
                if (currentObject_->isNested)
                {
                    //
                    // This object is already a part of another object, so we can remove it from the stack.
                    //
                    objectStack_.pop_back();
                }
            }

            return S_OK;
        }

        template<typename T>
        HRESULT Deserialize(std::shared_ptr<T>& sharedPtr)
        {
            HRESULT hr = ActivateSharedPointer(*this, sharedPtr);
            if (SUCCEEDED(hr))
            {
                hr = Deserialize(*sharedPtr);
            }
            return hr;
        }

        template<typename T>
        HRESULT Deserialize(std::unique_ptr<T>& sharedPtr)
        {
            *uniquePtr = make_unique<T>();
            HRESULT hr = Deserialize(*uniquePtr);
            return hr;
        }

        HRESULT Deserialize(IFabricJsonSerializable &object)
        {
            //
            // Gather the object information.
            //
            HRESULT hr = object.FromJson(*this);
            if (FAILED(hr)) { return hr; }

            //
            // Deserialize the bytestream.
            //
            hr = Expect(JsonTokenType_BeginObject);
            if (FAILED(hr)) { return hr; }

            return ParseJsonObject();
        }

        static HRESULT Dispatch(JsonReaderVisitor &reader, Common::WStringLiteral pszName, IFabricJsonSerializable &value, bool conditional = true)
        {
            HRESULT hr = reader.StartObject(pszName.begin(), conditional);
            if(FAILED(hr)) return hr;

            hr = value.FromJson(reader, false, conditional);
            if(FAILED(hr)) return hr;

            return reader.EndObject(conditional);
        }

        template<typename T>
        static __forceinline HRESULT Dispatch(JsonReaderVisitor& reader, Common::WStringLiteral pszName, T &value, bool conditional = true)
        {
            __if_exists(IMPLEMENTS(T, IsSerializable))
            {
                return Dispatch(reader, pszName, (Common::IFabricJsonSerializable&) value, conditional);
            }

            __if_not_exists(IMPLEMENTS(T, IsSerializable))
            {
                return reader.Visit(pszName.begin(), value, conditional);
            }
        }

        template<typename T>
        static __forceinline HRESULT Dispatch_Int64AsNum(JsonReaderVisitor& reader, Common::WStringLiteral pszName, T &value, bool conditional = true)
        {
            return reader.VisitInt64AsNumber(pszName.begin(), value, conditional);
        }

#if defined(PLATFORM_UNIX)
#define INTRINSIC_SPECIALIZATION(type) \
        template<> \
        static __forceinline HRESULT Dispatch<type>(JsonReaderVisitor& reader, Common::WStringLiteral pszName, type &value, bool conditional) \
        { \
            return reader.Visit(pszName.begin(), value, conditional); \
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

    private:

        //
        // This routine tries to populate the object on the top of the stack using the Json stream 
        //
        HRESULT ParseJsonObject()
        {
            if (objectStack_.empty()) 
            { 
                return E_FAIL;
            }

            for ( ; ; )
            {
                HRESULT hr = m_pReader->Read();
                if(FAILED(hr)) { return hr; }

                JsonTokenType tokenType = JsonTokenType_NotStarted;
                hr = m_pReader->GetTokenType(&tokenType);
                if(FAILED(hr)) return hr;

                if(tokenType == JsonTokenType_EndObject) 
                {
                    objectStack_.pop_back();
                    break;
                }
                if(tokenType != JsonTokenType_FieldName) { return E_FAIL; }

                std::wstring fieldName;
                hr = GetFieldName(fieldName);
                if(FAILED(hr)) { return hr; }

                hr = m_pReader->Read();
                if(FAILED(hr)) { return hr; }

                if(objectStack_.back()->map.count(fieldName) != 0)
                {
                    hr = m_pReader->GetTokenType(&tokenType);
                    if (FAILED(hr)) { return hr; }
                    //null is fine here - it means the value is undefined
                    if (tokenType == JsonTokenType_Null)
                    {
                        continue;
                    }
                    ValueFiller &filler = objectStack_.back()->map.find(fieldName)->second;

                    //
                    // The value can be a member object - handled by if clause or 
                    // a native type or a special case like SPTR(because we need to allocate memory) - handled by else clause. 
                    //
                    if (filler.object_ != nullptr)
                    {
                        ASSERT_IF(filler.setter_.first != nullptr, "Both object and setter cannot be non null");
                        ASSERT_IF(filler.object_->map.empty(), "Object without members cannot be deserialized");
                        if (tokenType != JsonTokenType_BeginObject) { return E_FAIL; }
                        
                        objectStack_.push_back(filler.object_);
                        hr = ParseJsonObject();
                        if (FAILED(hr)) { return hr; }
                    }
                    else
                    {
                        std::pair<Setter, void*> entry = filler.setter_;
                        hr = entry.first(this, entry.second);
                        if (FAILED(hr)) { return hr; }
                    }
                }
                else
                {
                    // We dont recognize this member, so skip this member.
                    tokenType = JsonTokenType_NotStarted;
                    hr = m_pReader->GetTokenType(&tokenType);
                    if (tokenType == JsonTokenType_BeginArray ||
                        tokenType == JsonTokenType_BeginObject)
                    {
                        hr = m_pReader->SkipObject();
                        if (FAILED(hr)) { return hr; }
                    }
                    else
                    {
                        // The read in the outer loop is sufficient to skip past the primitive type value 
                        // to the next token.
                    }
                }
            }

            return S_OK;
        }

        HRESULT GetFieldName(std::wstring &fieldName)
        {
            JsonTokenType tokenType = JsonTokenType_NotStarted;
            HRESULT hr = m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) return hr;

            if(tokenType == JsonTokenType_EndObject) { return hr; }

            if(tokenType != JsonTokenType_FieldName) { return E_FAIL; }

            ULONG nLength = 0;
            hr = m_pReader->GetFieldNameLength(&nLength);
            if(FAILED(hr)) { return hr; }

            fieldName.resize(nLength);
            hr = m_pReader->GetFieldName(const_cast<wchar_t*>(fieldName.c_str()), nLength+1);
            return hr;
        }

        HRESULT Expect(JsonTokenType expected)
        {
            HRESULT hr = m_pReader->Read();
            if(FAILED(hr)) { return hr; }

            JsonTokenType tokenType = JsonTokenType_NotStarted;
            hr = m_pReader->GetTokenType(&tokenType);
            if(FAILED(hr)) { return hr; }

            if(tokenType != expected) { return E_FAIL; }

            return hr;
        }

        // 
        // The reader can be in 2 states.
        // 1. The reader hasnt been initialized. This happens when we need to activate an object and then
        // parse the json string as that object.
        // 2. The reader is at the start of the object when this function is invoked. This happens when
        // we encounter a member that needs activation inside an object that is already being parsed.
        //
        template<typename T>
        static bool GetBaseTypeIfDynamicType(JsonReaderVisitor &pVisitor, T &baseobject)
        {
            JsonTokenType tokenType = JsonTokenType_NotStarted;
            HRESULT hr = pVisitor.m_pReader->GetTokenType(&tokenType);
            if (FAILED(hr)) { return false; }

            // Is Dynamic Type
            {
                pVisitor.m_pReader->SaveContext();
                // case 1. 
                if (tokenType == JsonTokenType_NotStarted)
                {
                    // For case 1, initialize the reader stream
                    hr = pVisitor.Expect(JsonTokenType_BeginObject);
                }

                if (FAILED(hr) || !IsDynamicType(&pVisitor))
                {
                    pVisitor.m_pReader->RestoreContext();
                    return false;
                }
                pVisitor.m_pReader->RestoreContext();
            }

            // GetBasetype
            {
                pVisitor.m_pReader->SaveContext();
                // case 1. 
                if (tokenType == JsonTokenType_NotStarted)
                {
                    // For case 1, initialize the reader stream
                    hr = pVisitor.Expect(JsonTokenType_BeginObject);
                }

                if (!FAILED(hr))
                {
                    hr = baseobject.FromJson(pVisitor);
                    if (!FAILED(hr)) { hr = pVisitor.ParseJsonObject(); }
                }
                pVisitor.m_pReader->RestoreContext();
            }

            return !FAILED(hr);
        }

        //
        // Returns true if the object starts with a field name 'Kind' or 'kind'. 
        //
        static bool IsDynamicType(JsonReaderVisitor* pVisitor)
        {
            // Skip past the begin object token
            JsonTokenType tokenType = JsonTokenType_NotStarted;
            HRESULT hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if (FAILED(hr)) { return false; }
        
            if (tokenType != JsonTokenType_BeginObject) { return false; }

            hr = pVisitor->m_pReader->Read();
            if(FAILED(hr)) { return false; }
            hr = pVisitor->m_pReader->GetTokenType(&tokenType);
            if (FAILED(hr)) { return false; }

            if (tokenType != JsonTokenType_FieldName) { return false; }

            std::wstring fieldName;
            hr = pVisitor->GetFieldName(fieldName);
            if (FAILED(hr)) { return false; }

            if ((fieldName == L"Kind") || (fieldName == L"kind"))
            {
                return true;
            }

            return false;
        }

        class ValueFiller;
        class Object;
        typedef HRESULT (*Setter)(JsonReaderVisitor* pVisitor, void* pData);
        typedef std::shared_ptr<Object> ObjectSPtr;

        class Object
        {
        public:
            Object()
                :isNested(false) 
            {}

            bool isNested;
            std::map<std::wstring, ValueFiller> map;
        };

        class ValueFiller
        {
        public:

            ValueFiller() {};

            ValueFiller(std::pair<Setter, void*> setter)
                : setter_(setter)
                , object_()
            {}

            ValueFiller(ObjectSPtr &object)
                : object_(object)
                , setter_(std::make_pair(nullptr, nullptr))
            {}

            std::pair<Setter, void*> setter_;
            ObjectSPtr object_;
        };

        //
        // NodeId needs to be special cased since it cannot inherit from
        // IFabricJsonSerializable
        //
        class InternalNodeId : public IFabricJsonSerializable
        {
        public:
            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"Id", id_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            LargeInteger id_;
        };

        //
        // This represents the object currently being created.
        //
        ObjectSPtr currentObject_;

        //
        // This stack tracks the objects being deserialized. The top of the stack
        // represents the object being currently deserialized.
        //
        std::vector<ObjectSPtr> objectStack_;
        IJsonReader* m_pReader;
        JsonSerializerFlags m_pSerializerFlags;

        template <bool B, class T>
        struct SetterLookup
        {   
            // Implementation is not needed since a bool is specialized for true and false
        };

        // We will de-serialize enums given as string or ULONG
        template <class T>
        struct SetterLookup<true, T>
        {
            static Setter GetSetter()
            {
                return &SetEnum<T>;
            }
        };

        // This specialization is for non-enums and the type must have a setter
        template <class T>
        struct SetterLookup<false, T>
        {
            static Setter GetSetter()
            {
                return &Set<T>;
            }
            static Setter GetInt64AsNumberSetter()
            {
                return &SetInt64AsNumber<T>;
            }
        };

    };
}
