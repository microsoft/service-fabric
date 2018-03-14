// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral const TraceType_XmlLiteReader = "XmlLiteReader";

ComProxyXmlLiteReader::ComProxyXmlLiteReader(ComPointer<IXmlReader> reader)
    : reader_(reader), 
    inputName_()
{
}

ComProxyXmlLiteReader::~ComProxyXmlLiteReader()
{
}
ErrorCode ComProxyXmlLiteReader::Create(
    __in std::wstring const & inputName,
    __in IStream *inputStream,
    __out ComProxyXmlLiteReaderUPtr & xmlLiteReader)
{
    if (inputStream == nullptr)
    {
        return ErrorCode(ErrorCodeValue::ArgumentNull);
    }

    ComPointer<IXmlReader> reader;
    auto hr = ::CreateXmlReader(
        ::IID_IXmlReader,
#if defined(PLATFORM_UNIX)
        inputName.c_str(),
#endif
        reader.VoidInitializationAddress(),
        NULL);

    if (hr != S_OK)
    {
#if !defined(PLATFORM_UNIX)
        WriteError(
            TraceType_XmlLiteReader,
            "CreateXmlReader(stream) failed. HRESULT={0}",
            hr);
#else
        WriteError(
            TraceType_XmlLiteReader,
            "CreateXmlReader failed(stream). Name: {0}, HRESULT={1}",
            inputName, hr);
#endif
        return ErrorCode::FromHResult(hr);
    }

    xmlLiteReader = move(make_unique<ComProxyXmlLiteReader>(reader));

#if !defined(PLATFORM_UNIX)
    ComPointer<IStream> xmlMemoryStream;
    xmlMemoryStream.SetNoAddRef(inputStream);

    auto error = xmlLiteReader->SetInput(inputName, xmlMemoryStream);
    if (!error.IsSuccess()) { return error; }
#endif

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ComProxyXmlLiteReader::Create(
__in std::wstring const & inputName,
__out ComProxyXmlLiteReaderUPtr & xmlLiteReader)
{
    ComPointer<IXmlReader> reader;
    auto hr = ::CreateXmlReader(
        ::IID_IXmlReader, 
#if defined(PLATFORM_UNIX)
        inputName.c_str(),
#endif
        reader.VoidInitializationAddress(),
        NULL);
    if (hr != S_OK)
    {
#if !defined(PLATFORM_UNIX)
        WriteError(
            TraceType_XmlLiteReader,
            "CreateXmlReader failed. HRESULT={0}",
            hr);
#else
        WriteError(
                TraceType_XmlLiteReader,
                "CreateXmlReader failed. File: {0}, HRESULT={1}",
                inputName, hr);
#endif
        return ErrorCode::FromHResult(hr);
    }

    xmlLiteReader = move(make_unique<ComProxyXmlLiteReader>(reader));

#if !defined(PLATFORM_UNIX)
    ComPointer<IStream> xmlFileStream;
    auto error = ComXmlFileStream::Open(inputName, false, xmlFileStream);
    if (!error.IsSuccess()) { return error; }

    error = xmlLiteReader->SetInput(inputName, xmlFileStream);
    if (!error.IsSuccess()) { return error; }
#endif

    return ErrorCode(ErrorCodeValue::Success);
}

#if defined(PLATFORM_UNIX)
ErrorCode ComProxyXmlLiteReader::Create(
    __in const char *xmlString,
    __out ComProxyXmlLiteReaderUPtr & xmlLiteReader)
{
    ComPointer<IXmlReader> xmlReader;
    auto hr = XmlLiteReader::CreateXmlReader(xmlString, xmlReader.VoidInitializationAddress());
    xmlLiteReader = move(make_unique<ComProxyXmlLiteReader>(xmlReader));
    return ErrorCode::FromHResult(hr);
}
#endif

ErrorCode ComProxyXmlLiteReader::SetInput(wstring const & inputName, ComPointer<IStream> const & stream)
{
    auto hr = reader_->SetInput(stream.GetRawPointer());
    if (hr != S_OK)
    {
        WriteError(
            TraceType_XmlLiteReader,
            "SetInput failed. HRESULT={0}, Input={1}",
            hr,
            inputName_);
        return ErrorCode::FromHResult(hr);
    }

    inputName_ = inputName;
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ComProxyXmlLiteReader::GetAttributeCount(__out UINT & attributeCount)
{
    return ToErrorCode(
        reader_->GetAttributeCount(&attributeCount),
        L"GetAttributeCount");
}

ErrorCode ComProxyXmlLiteReader::GetDepth(__out UINT & depth)
{
    return ToErrorCode(
        reader_->GetDepth(&depth),
        L"GetDepth");
}

ErrorCode ComProxyXmlLiteReader::GetLineNumber(__out UINT & lineNumber)
{
    return ToErrorCode(
        reader_->GetLineNumber(&lineNumber),
        L"GetLineNumber");
}

ErrorCode ComProxyXmlLiteReader::GetLinePosition(__out UINT & linePosition)
{
    return ToErrorCode(
        reader_->GetLinePosition(&linePosition),
        L"GetLinePosition");
}

ErrorCode ComProxyXmlLiteReader::GetLocalName(__out wstring & localName)
{
    
    LPCWSTR outString;
    auto hr = reader_->GetLocalName(&outString, NULL);
    if (hr != S_OK) { return OnReaderError(hr, L"GetLocalName"); }

    localName.assign(outString);
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ComProxyXmlLiteReader::GetNamespaceUri(__out wstring & namespaceUri)
{
    LPCWSTR outString;
    auto hr = reader_->GetNamespaceUri(&outString, NULL);
    if (hr != S_OK) { return OnReaderError(hr, L"GetNamespaceUri"); }

    namespaceUri.assign(outString);
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ComProxyXmlLiteReader::GetNodeType(__out ::XmlNodeType & nodeType)
{
    return ToErrorCode(
        reader_->GetNodeType(&nodeType),
        L"GetNodeType");
}

ErrorCode ComProxyXmlLiteReader::GetPrefix(__out wstring & prefix)
{
    LPCWSTR outString;
    auto hr = reader_->GetPrefix(&outString, NULL);
    if (hr != S_OK) { return OnReaderError(hr, L"GetPrefix"); }

    prefix.assign(outString);
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ComProxyXmlLiteReader::GetQualifiedName(__out wstring & qualifiedName)
{
    LPCWSTR outString;
    auto hr = reader_->GetQualifiedName(&outString, NULL);
    if (hr != S_OK) { return OnReaderError(hr, L"GetQualifiedName"); }

    qualifiedName.assign(outString);
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ComProxyXmlLiteReader::GetValue(__out wstring & value)
{
    LPCWSTR outString;
    auto hr = reader_->GetValue(&outString, NULL);
    if (hr != S_OK) { return OnReaderError(hr, L"GetValue"); }

    value.assign(outString);
    return ErrorCode(ErrorCodeValue::Success);
}

bool ComProxyXmlLiteReader::IsEmptyElement()
{
    return (reader_->IsEmptyElement() != 0);
}

bool ComProxyXmlLiteReader::IsEOF()
{
    return (reader_->IsEOF() != 0);
}

ErrorCode ComProxyXmlLiteReader::MoveToAttributeByName(wstring const & attrName, wstring const & namespaceUri, __out bool & attrFound)
{
    HRESULT hr;
    if (namespaceUri.length() > 0)
    {
        hr = reader_->MoveToAttributeByName(
            attrName.c_str(),
            namespaceUri.c_str());
    }
    else
    {
        hr = reader_->MoveToAttributeByName(
            attrName.c_str(),
            NULL);
    }

    if (hr == S_OK || hr == S_FALSE)
    {
        attrFound = (hr == S_OK);
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return OnReaderError(
            hr, 
            wstring(L"MoveToAttributeByName(" + attrName + L"," + namespaceUri + L")"));
    }
}

ErrorCode ComProxyXmlLiteReader::MoveToElement()
{
    auto hr = reader_->MoveToElement();
    if (hr != S_OK) { return OnReaderError(hr, L"MoveToElement"); }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ComProxyXmlLiteReader::MoveToFirstAttribute(__out bool & success)
{
    auto hr = reader_->MoveToFirstAttribute();
    if (hr == S_FALSE)
    {
        success = false;
        return ErrorCode(ErrorCodeValue::Success);
    }

    if (hr != S_OK)
    {
        return OnReaderError(hr, L"MoveToNextAttribute"); 
    }
    else
    {
        success = true;
        return ErrorCode(ErrorCodeValue::Success);
    }
}

ErrorCode ComProxyXmlLiteReader::MoveToNextAttribute(__out bool & success)
{
    auto hr = reader_->MoveToNextAttribute();
    if (hr == S_FALSE)
    {
        success = false;
        return ErrorCode(ErrorCodeValue::Success);
    }

    if (hr != S_OK)
    {
        return OnReaderError(hr, L"MoveToNextAttribute"); 
    }
    else
    {
        success = true;
        return ErrorCode(ErrorCodeValue::Success);
    }
}

ErrorCode ComProxyXmlLiteReader::Read(__out ::XmlNodeType & nodeType, __out bool & isEOF)
{
    ::XmlNodeType outNodeType;

    auto hr = reader_->Read(&outNodeType);
    if (hr == S_FALSE) 
    {
        isEOF = true;
        return ErrorCode(ErrorCodeValue::Success);
    }

    if (hr != S_OK)
    {
        return OnReaderError(hr, L"Read"); 
    }
    else
    {
        isEOF = false;
        nodeType = outNodeType;
        return ErrorCode(ErrorCodeValue::Success);
    }
}

ErrorCode ComProxyXmlLiteReader::ToErrorCode(
    HRESULT hr, 
    wstring const & operationName)
{
    if (hr != S_OK)
    {
        return OnReaderError(hr, operationName);
    }
    else
    {
        return ErrorCode(ErrorCodeValue::Success);
    }
}

ErrorCode ComProxyXmlLiteReader::OnReaderError(
    HRESULT hr, 
    wstring const & operationName)
{
   
    UINT lineNumber = 0;
    UINT linePosition = 0;
    TryGetLineNumber(lineNumber);
    TryGetLinePosition(linePosition);

    WriteError(
        TraceType_XmlLiteReader,
        "{0} failed. HRESULT={1}, Input={2}, LineNumber={3}, LinePosition={4}",
        operationName,
        hr,
        inputName_,
        lineNumber,
        linePosition);

    return ErrorCode::FromHResult(hr);
}

bool ComProxyXmlLiteReader::TryGetLineNumber(__out UINT & lineNumber)
{
    UINT outUInt;
    auto hr = reader_->GetLineNumber(&outUInt);
    if (hr != S_OK) { return false; }

    lineNumber = outUInt;
    return true;
}

bool  ComProxyXmlLiteReader::TryGetLinePosition(__out UINT & linePosition)
{
    UINT outUInt;
    auto hr = reader_->GetLinePosition(&outUInt);
    if (hr != S_OK) { return false; }

    linePosition = outUInt;
    return true;
}
