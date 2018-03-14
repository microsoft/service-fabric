// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    MIDL_INTERFACE("DBDE8EC5-11BC-46CB-A5D3-46C065CF884F")
IJsonWriter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ObjectStart() = 0;
        virtual HRESULT STDMETHODCALLTYPE ObjectEnd() = 0;
        virtual HRESULT STDMETHODCALLTYPE ArrayStart() = 0;
        virtual HRESULT STDMETHODCALLTYPE ArrayEnd() = 0;
        virtual HRESULT STDMETHODCALLTYPE PropertyName(LPCWSTR pszName) = 0;
        virtual HRESULT STDMETHODCALLTYPE StringValue(LPCWSTR pszValue) = 0;
        virtual HRESULT STDMETHODCALLTYPE FragmentValue(LPCSTR pszFragment) = 0;
        virtual HRESULT STDMETHODCALLTYPE IntValue(__int64 nValue) = 0;
        virtual HRESULT STDMETHODCALLTYPE UIntValue(unsigned __int64 nValue) = 0;
        virtual HRESULT STDMETHODCALLTYPE NumberValue(double dblValue) = 0;
        virtual HRESULT STDMETHODCALLTYPE BoolValue(bool bValue) = 0;
        virtual HRESULT STDMETHODCALLTYPE NullValue() = 0;
    };
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    enum JsonTokenType
    {
        JsonTokenType_NotStarted = 0,
        JsonTokenType_BeginArray = 1,
        JsonTokenType_EndArray = 2,
        JsonTokenType_BeginObject = 3,
        JsonTokenType_EndObject = 4,
        JsonTokenType_String = 5,
        JsonTokenType_Number = 6,
        JsonTokenType_True = 7,
        JsonTokenType_False = 8,
        JsonTokenType_Null = 9,
        JsonTokenType_FieldName = 10,
        JsonTokenType_NameSeparator = 11,
        JsonTokenType_ValueSeparator = 12
    };
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    MIDL_INTERFACE("C826A977-7B03-4B52-BB5C-A922DBB227EB")
IJsonReader : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Read() = 0;
        virtual HRESULT STDMETHODCALLTYPE GetTokenType(JsonTokenType* pTokenType) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetNumberValue(DOUBLE* pdblValue) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetFieldNameLength(ULONG* pnLength) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetStringLength(ULONG* pnLength) = 0; 
        virtual HRESULT STDMETHODCALLTYPE GetFieldName(__in_ecount(nLength) LPWSTR pszBuffer, __in ULONG nLength) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetStringValue(__in_ecount(nLength) LPWSTR pszBuffer, __in ULONG nLength) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetDepth(ULONG* pnDepth) = 0;
        virtual HRESULT STDMETHODCALLTYPE SkipObject() = 0;
        virtual HRESULT STDMETHODCALLTYPE SaveContext() = 0;
        virtual HRESULT STDMETHODCALLTYPE RestoreContext() = 0;
    };
    //-------------------------------------------------------------------------------------------------------------------------------------------
}
