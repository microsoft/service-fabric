// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <codecvt>
#include <pdhmsg.h>
#include <regex>

using namespace std;
using namespace Common;

// UTF-8 to UTF-16 Conversion. TODO: wchar_t seems still 4-bytes in to_bytes()
static wstring utf8to16(const char *str)
{
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> conv;
    u16string u16str = conv.from_bytes(str);
    return wstring((wchar_t *) u16str.c_str());
}

static string utf16to8(const wchar_t *wstr)
{
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> conv;
    return conv.to_bytes((const char16_t *) wstr);
}

static HRESULT xmlResultConv(int xmlResult)
{
    return (xmlResult == 1 ? S_OK : (xmlResult == 0 ? S_FALSE : E_FAIL));
}

STDAPI CreateXmlReader(_In_ REFIID riid, _In_ LPCWSTR ppwszFileName, _Outptr_ void **ppvObject, _In_opt_ IMalloc *pMalloc)
{
    string filename = utf16to8(ppwszFileName);
    xmlTextReaderPtr xmlReader = xmlReaderForFile(filename.c_str(), NULL, 0);
    if (xmlReader != NULL)
    {
        *ppvObject = new XmlLiteReader(xmlReader);
        xmlTextReaderRead(xmlReader);
        ((XmlLiteReader*)(*ppvObject))->AddRef();
    }
    return (xmlReader != NULL ? S_OK : E_FAIL);
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE XmlLiteReader::AddRef(void)
{
    return InterlockedIncrement(&refCount_);
}

ULONG STDMETHODCALLTYPE XmlLiteReader::Release(void)
{
    LONG result = InterlockedDecrement(&refCount_);
    ASSERT(result >= 0);

    if (result == 0)
    {
        if (reader_)
        {
            xmlTextReaderClose(reader_);
        }
        delete this;
    }
    return result;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::SetInput(_In_opt_ IUnknown *pInput)
{
    // libxml2 does not use IStream - just return success.
    return 0;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetProperty(_In_ UINT nProperty, _Out_ LONG_PTR *ppValue)
{
    // property of XmlReader - not implemented. 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::SetProperty(_In_ UINT nProperty, _In_opt_ LONG_PTR pValue)
{
    // property of XmlReader - not implemented. 
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::Read(_Out_opt_ XmlNodeType *pNodeType)
{
    int result = xmlTextReaderRead(reader_);
    if (result == 1 && pNodeType)
    {
       // safe to cast: exact mapping of nodetype between libxml2 and xmllite
       *pNodeType = (XmlNodeType) xmlTextReaderNodeType(reader_);
    }
    return xmlResultConv(result);
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetNodeType(_Out_ XmlNodeType *pNodeType)
{
    *pNodeType = (XmlNodeType) xmlTextReaderNodeType(reader_);
    return (*pNodeType == -1 ? E_FAIL : S_OK);
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::MoveToFirstAttribute(void)
{
    int result = xmlTextReaderMoveToFirstAttribute(reader_);
    return xmlResultConv(result); 
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::MoveToNextAttribute(void)
{
    int result = xmlTextReaderMoveToNextAttribute(reader_);
    return xmlResultConv(result);
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::MoveToAttributeByName(_In_ LPCWSTR pwszLocalName, _In_opt_ LPCWSTR pwszNamespaceUri)
{
    int result = 0;
    string localname = utf16to8(pwszLocalName);
    if (pwszNamespaceUri == NULL)
    {
        result = xmlTextReaderMoveToAttribute(reader_, (const xmlChar *) localname.c_str());
    }
    else
    {
        string ns = utf16to8(pwszNamespaceUri);
        result = xmlTextReaderMoveToAttributeNs(reader_, (const xmlChar *) localname.c_str(), (const xmlChar *) ns.c_str());
    }
    return xmlResultConv(result);
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::MoveToElement(void)
{
    int result = xmlTextReaderMoveToElement(reader_);
    return xmlResultConv(result);
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetQualifiedName( 
    _Outptr_result_buffer_maybenull_(*pcwchQualifiedName+1)  LPCWSTR *ppwszQualifiedName,
    _Out_opt_  UINT *pcwchQualifiedName)
{
    const char *name = (const char*) xmlTextReaderConstName(reader_);

    wstring wstr = (name == NULL ? L"" : utf8to16(name));
    wstrbuf_.push_back(wstr);
    *ppwszQualifiedName = wstrbuf_.back().c_str();
    if(pcwchQualifiedName) *pcwchQualifiedName = wstrbuf_.back().length();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetNamespaceUri( 
    _Outptr_result_buffer_maybenull_(*pcwchNamespaceUri+1) LPCWSTR *ppwszNamespaceUri,
    _Out_opt_ UINT *pcwchNamespaceUri)
{
    const char *ns = (const char*) xmlTextReaderConstNamespaceUri(reader_);

    wstring wstr = (ns == NULL ? L"" : utf8to16(ns));
    wstrbuf_.push_back(wstr);
    *ppwszNamespaceUri = wstrbuf_.back().c_str();
    if(pcwchNamespaceUri) *pcwchNamespaceUri = wstrbuf_.back().length();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetLocalName( 
    _Outptr_result_buffer_maybenull_(*pcwchLocalName+1) LPCWSTR *ppwszLocalName,
    _Out_opt_ UINT *pcwchLocalName)
{
    const char *name = (const char*) xmlTextReaderConstLocalName(reader_);

    wstring wstr = (name == NULL ? L"" : utf8to16(name));
    wstrbuf_.push_back(wstr);
    *ppwszLocalName = wstrbuf_.back().c_str();
    if(pcwchLocalName) *pcwchLocalName = wstrbuf_.back().length();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetPrefix( 
    _Outptr_result_buffer_maybenull_(*pcwchPrefix+1) LPCWSTR *ppwszPrefix,
    _Out_opt_ UINT *pcwchPrefix)
{
    const char *prefix = (const char*) xmlTextReaderConstPrefix(reader_);

    wstring wstr = (prefix == NULL ? L"" : utf8to16(prefix));
    wstrbuf_.push_back(wstr);
    *ppwszPrefix = wstrbuf_.back().c_str();
    if(pcwchPrefix) *pcwchPrefix = wstrbuf_.back().length();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetValue( 
    _Outptr_result_buffer_maybenull_(*pcwchValue+1) LPCWSTR *ppwszValue,
    _Out_opt_ UINT *pcwchValue)
{
    const char *value = (const char*) xmlTextReaderConstValue(reader_);

    wstring wstr = (value == NULL ? L"" : utf8to16(value));
    wstrbuf_.push_back(wstr);
    *ppwszValue = wstrbuf_.back().c_str();
    if(pcwchValue) *pcwchValue = wstrbuf_.back().length();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetBaseUri(
    _Outptr_result_buffer_maybenull_(*pcwchBaseUri + 1) LPCWSTR *ppwszBaseUri,
    _Out_opt_ UINT *pcwchBaseUri)
{
    const char *uri = (const char*)xmlTextReaderBaseUri(reader_);

    wstring wstr = (uri == NULL ? L"" : utf8to16(uri));
    wstrbuf_.push_back(wstr);
    *ppwszBaseUri = wstrbuf_.back().c_str();
    if (pcwchBaseUri) *pcwchBaseUri = wstrbuf_.back().length();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::ReadValueChunk(
    _Out_writes_to_(cwchChunkSize, *pcwchRead) WCHAR *pwchBuffer,
    _In_ UINT cwchChunkSize,
    _Inout_ UINT *pcwchRead)
{
    memset(pwchBuffer, 0, cwchChunkSize);
    const char *value = (const char*) xmlTextReaderConstValue(reader_);
    
    if (value != NULL)
    {
        wstring wstr = utf8to16(value);
        size_t sz = min((size_t)cwchChunkSize, wstrbuf_.back().length() * sizeof(wchar_t));
        memcpy(pwchBuffer, wstr.c_str(), sz);
        *pcwchRead = sz;
    }
    return S_OK;
}

BOOL STDMETHODCALLTYPE XmlLiteReader::IsDefault(void)
{
    return xmlTextReaderIsDefault(reader_);
}

BOOL STDMETHODCALLTYPE XmlLiteReader::IsEmptyElement(void)
{
    return xmlTextReaderIsEmptyElement(reader_);
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetLineNumber(_Out_ UINT *pnLineNumber)
{
    *pnLineNumber = xmlTextReaderGetParserLineNumber(reader_);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetLinePosition(_Out_ UINT *pnLinePosition)
{
    *pnLinePosition = xmlTextReaderGetParserColumnNumber(reader_);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetAttributeCount(_Out_ UINT *pnAttributeCount)
{
    *pnAttributeCount = xmlTextReaderAttributeCount(reader_);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE XmlLiteReader::GetDepth(_Out_ UINT *pnDepth)
{
    *pnDepth = xmlTextReaderDepth(reader_);
    return S_OK;
}

BOOL STDMETHODCALLTYPE XmlLiteReader::IsEOF(void)
{
    return xmlTextReaderReadState(reader_) == XML_TEXTREADER_MODE_EOF;
}

HRESULT XmlLiteReader::CreateXmlReader(const char *xmlString, _Outptr_ void ** ppvObject)
{
    int len = strlen(xmlString);
    xmlTextReaderPtr xmlReader = xmlReaderForMemory(xmlString, len, NULL, NULL, 0);
    if (xmlReader != NULL)
    {
        *ppvObject = new XmlLiteReader(xmlReader);
        ((XmlLiteReader*)(*ppvObject))->AddRef();
    }
    return (xmlReader != NULL ? S_OK : E_FAIL);
}
