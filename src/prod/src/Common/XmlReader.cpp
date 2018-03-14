// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral const TraceType_XmlReader = "XmlReader";

ErrorCode XmlReader::Create(wstring const & fileName, __out XmlReaderUPtr & reader)
{
    ComProxyXmlLiteReaderUPtr liteReader;

    auto uncPath = Path::ConvertToNtPath(fileName);

    auto error = ComProxyXmlLiteReader::Create(uncPath, liteReader);
    if (!error.IsSuccess()) { return error; } 

    reader = move(make_unique<XmlReader>(liteReader));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode XmlReader::ReadXmlFile(
    wstring const & fileName, 
    wstring const & rootElementName,  
    wstring const & namespaceUri, 
    __out wstring & xmlContent)
{
    auto error = ErrorCode(ErrorCodeValue::Success);

    try
    {
        XmlReaderUPtr xmlReader;
        error = XmlReader::Create(fileName, xmlReader);
        if (error.IsSuccess())
        {
            xmlReader->StartElement(rootElementName, namespaceUri, false);
            xmlContent = xmlReader->ReadOuterXml();
        }
    }
    catch(XmlException e)
    {
        error = e.Error;
    }

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_XmlReader,
            "ReadXmlFile Failed: ErrorCode={0}, FileName={1}, RootElement={2}, NamespaceUri={3}",
            error,
            fileName,
            rootElementName,
            namespaceUri);
    }

    return error;
}

XmlReader::XmlReader(ComProxyXmlLiteReaderUPtr & xmlLiteReader)
    : liteReader_(move(xmlLiteReader)),
    attrValues_()
{
}

XmlReader::~XmlReader()
{
}


bool XmlReader::IsStartElement(wstring const & localName, wstring const & namespaceUri, bool readAttrs)
{
    XmlNodeType nodeType = MoveToContent();
    bool success;

    if (nodeType == ::XmlNodeType_Element)
    {
        success = ((GetLocalName() == localName) && (GetNamespaceUri() == namespaceUri));
    }
    else
    {
        success = false;
    }

    if (success && readAttrs)
    {
        ReadAttributes();
    }

    return success;
}

void XmlReader::StartElement(wstring const & localName, wstring const & namespaceUri, bool readAttrs)
{
    XmlNodeType nodeType = MoveToContent();

    if (nodeType != ::XmlNodeType_Element)
    {
        ThrowInvalidContent(::XmlNodeType_Element, nodeType);
    }

    wstring nodeLocalName = GetLocalName();
    wstring nodeNamespaceUri = GetNamespaceUri();

    if ((nodeLocalName != localName) || (nodeNamespaceUri != namespaceUri))
    {
        ThrowInvalidContent(
            GetFullyQualifiedName(localName, namespaceUri),
            GetFullyQualifiedName());
    }

    if (readAttrs)
    {
        ReadAttributes();
    }
}

void XmlReader::ReadStartElement()
{
    ReadContent(::XmlNodeType_Element);
}

void XmlReader::ReadEndElement()
{
    ReadContent(::XmlNodeType_EndElement);
}

void XmlReader::ReadElement()
{
    ::XmlNodeType nodeType = MoveToContent();
    if (nodeType != ::XmlNodeType_Element)
    {
        ThrowInvalidContent(::XmlNodeType_Element, nodeType);
    }

    if (IsEmptyElement())
    {
        Read();
    }
    else
    {
        Read();
        ReadEndElement();
    }
}

void XmlReader::MoveToNextElement()
{
    ::XmlNodeType nodeType = GetNodeType();
     Read(nodeType);
}

void XmlReader::SkipElement(wstring const & localName, wstring const & namespaceUri, bool isOptional)
{
    if (isOptional && !IsStartElement(localName, namespaceUri, false)) return;

    StartElement(localName, namespaceUri, false);
    if (IsEmptyElement())
    {
        Read();
    }
    else
    {
        bool foundMatchingEndElement = false;
        UINT startDepth = GetDepth();
        UINT endDepth;
        ::XmlNodeType nodeType;

        while(Read(nodeType))
        {
            if (nodeType == ::XmlNodeType_EndElement)
            {
                endDepth = GetDepth();
#if !defined(PLATFORM_UNIX)
                if (endDepth == (startDepth + 1))
#else
                if (endDepth == startDepth)
#endif
                {
                    Read();
                    foundMatchingEndElement = true;
                    break;
                }
            }
        }

        if (!foundMatchingEndElement)
        {
            ThrowUnexpectedEOF(L"SkipElement");
        }
    }
}

bool XmlReader::HasAttribute(wstring const & attrName, wstring const & namespaceUri)
{
    auto iter = attrValues_.find(GetFullyQualifiedName(namespaceUri, attrName));
    return (iter != attrValues_.end());
}

wstring XmlReader::ReadAttributeValue(wstring const & attrName, wstring const & namespaceUri, bool trim)
{
    wstring key = GetFullyQualifiedName(namespaceUri, attrName);

    auto iter = attrValues_.find(key);
    if (iter == attrValues_.end())
    {
        ThrowInvalidContent(L"Attribute: " + key, L"none");
    }

    wstring retval = iter->second;
    if (trim)
    {
        StringUtility::TrimWhitespaces(retval);
    }

    return retval;
}

wstring XmlReader::ReadElementValue(bool trim)
{
    wstring retval;
    ::XmlNodeType nodeType = GetNodeType();
    bool done = false;
    while(!done)
    {
        switch(nodeType)
        {
        case XmlNodeType_Whitespace:
        case XmlNodeType_CDATA:
        case XmlNodeType_Text:
            {
                retval.append(GetValue());
                break;
            }
        case XmlNodeType_Element:
        case XmlNodeType_EndElement:
            {
                done = true;
                break;
            }
        };

        if (!done)
        {
            done = !Read(nodeType);
        }
    };

    if (trim)
    {
        StringUtility::TrimWhitespaces(retval);
    }

    return retval;
}

wstring XmlReader::ReadOuterXml()
{
    ::XmlNodeType nodeType = MoveToContent();
    if (nodeType != ::XmlNodeType_Element)
    {
        ThrowInvalidContent(::XmlNodeType_Element, nodeType);
    }

    ComPointer<IStream> memoryStream;
    memoryStream.SetNoAddRef(::SHCreateMemStream(NULL, 0));
    if (!memoryStream)
    {
        ThrowIfNotSuccess(E_FAIL, L"::SHCreateMemStream");
    }

    ComPointer<IXmlWriterOutput> writerOutput;
    auto hr = ::CreateXmlWriterOutputWithEncodingName(memoryStream.GetRawPointer(), NULL, L"utf-16", writerOutput.InitializationAddress());
    ThrowIfNotSuccess(hr, L"CreateXmlWriterOutputWithEncodingName");
    
    ComPointer<IUnknown> comPointer;
    hr = writerOutput->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(comPointer.InitializationAddress()));
    ThrowIfNotSuccess(hr, L"writerOutput->QueryInterface");

    XmlWriterUPtr xmlWriterUPtr;
    auto errorCode = XmlWriter::Create(comPointer, xmlWriterUPtr, false);
    ThrowIfNotSuccess(errorCode, L"XmlWriter::Create");

    xmlWriterUPtr->WriteNode(*this, false);
    xmlWriterUPtr->Flush();

    STATSTG streamData = {0};
    hr = memoryStream->Stat(&streamData, STATFLAG_NONAME);
    ThrowIfNotSuccess(hr, L"memoryStream->Stat");
    ULONG size = streamData.cbSize.LowPart;

    LPWSTR content = (WCHAR*) new BYTE[size + sizeof(WCHAR)];
    LARGE_INTEGER position;
    position.QuadPart = 0;

    hr = memoryStream->Seek(position, STREAM_SEEK_SET, NULL);
    if (hr != S_OK)  { delete[] content; }
    ThrowIfNotSuccess(hr, L"memoryStream->Seek");

    ULONG read;
    hr = memoryStream->Read(content, size, &read);
    if (hr != S_OK)  { delete[] content; }
    ThrowIfNotSuccess(hr, L"memoryStream->Read");

    content[size / sizeof(WCHAR)] = L'\0';
    wstring result(content);
    delete[] content;
    
    return result;
}

void XmlReader::Read()
{
    ::XmlNodeType nodeType;
    Read(nodeType);
}

::XmlNodeType XmlReader::MoveToContent()
{
    ::XmlNodeType nodeType = GetNodeType();

    bool done = false;
    while(!done)
    {
        switch(nodeType)
        {
        case ::XmlNodeType_Text:
        case ::XmlNodeType_CDATA:
        case ::XmlNodeType_Element:
        case ::XmlNodeType_EndElement:
            {
                done = true;
                break;
            }

        case ::XmlNodeType_Attribute:
            {
                done = true;
                MoveToElement();
                nodeType = ::XmlNodeType_Element;
                break;
            }

        default:
            {
                done = !Read(nodeType);
                if (done)
                {
                    ThrowUnexpectedEOF(L"MoveToContent"); 
                }
                break;
            }
        }
    };

    return nodeType;
}

void XmlReader::ReadContent(::XmlNodeType contentType)
{
    ::XmlNodeType nodeType = MoveToContent();
    if (nodeType != contentType)
    {
        ThrowInvalidContent(contentType, nodeType);
    }

    Read();
}

void XmlReader::ReadAttributes()
{
    attrValues_.clear();

    bool attrFound = MoveToFirstAttribute();
    bool doMoveToElement = attrFound;
    while(attrFound)
    {
        attrValues_.insert(
            make_pair(
            GetFullyQualifiedName(), 
            GetValue()));
        attrFound = MoveToNextAttribute();
    }

    if (doMoveToElement)
    {
        MoveToElement();
    }
}

wstring XmlReader::GetFullyQualifiedName()
{
    return GetFullyQualifiedName(GetNamespaceUri(), GetLocalName());
}

wstring XmlReader::GetFullyQualifiedName(wstring const & namespaceUri, wstring const & attrName)
{
    return wstring(namespaceUri + L":" + attrName);
}

UINT XmlReader::GetAttributeCount()
{
    UINT attributeCount;
    auto error = liteReader_->GetAttributeCount(attributeCount);
    ThrowIfNotSuccess(error);

    return attributeCount;
}

UINT XmlReader::GetDepth()
{
    UINT depth;
    auto error = liteReader_->GetDepth(depth);
    ThrowIfNotSuccess(error);

    return depth;
}

UINT XmlReader::GetLineNumber()
{
    UINT lineNumber;
    auto error = liteReader_->GetLineNumber(lineNumber);
    ThrowIfNotSuccess(error);

    return lineNumber;
}

UINT XmlReader::GetLinePosition()
{
    UINT linePosition;
    auto error = liteReader_->GetLinePosition(linePosition);
    ThrowIfNotSuccess(error);

    return linePosition;
}

wstring XmlReader::GetLocalName()
{
    wstring localName;
    auto error = liteReader_->GetLocalName(localName);
    ThrowIfNotSuccess(error);

    return localName;
}

wstring XmlReader::GetNamespaceUri()
{
    wstring namespaceUri;
    auto error = liteReader_->GetNamespaceUri(namespaceUri);
    ThrowIfNotSuccess(error);

    return namespaceUri;
}

::XmlNodeType XmlReader::GetNodeType()
{
    ::XmlNodeType nodeType;

    auto error = liteReader_->GetNodeType(nodeType);
    ThrowIfNotSuccess(error);

    return nodeType;
}

wstring XmlReader::GetPrefix()
{
    wstring prefix;
    auto error = liteReader_->GetPrefix(prefix);
    ThrowIfNotSuccess(error);

    return prefix;
}

wstring XmlReader::GetQualifiedName()
{
    wstring qualifiedName;
    auto error = liteReader_->GetQualifiedName(qualifiedName);
    ThrowIfNotSuccess(error);

    return qualifiedName;
}

wstring XmlReader::GetValue()
{
    wstring value;
    auto error = liteReader_->GetValue(value);
    ThrowIfNotSuccess(error);

    return value;
}

void XmlReader::MoveToElement()
{
    auto error = liteReader_->MoveToElement();
    ThrowIfNotSuccess(error);
}

bool XmlReader::MoveToFirstAttribute()
{
    bool success;
    auto error = liteReader_->MoveToFirstAttribute(success);
    ThrowIfNotSuccess(error);

    return success;
}

bool XmlReader::MoveToNextAttribute()
{
    bool success;
    auto error = liteReader_->MoveToNextAttribute(success);
    ThrowIfNotSuccess(error);

    return success;
}

bool XmlReader::Read(__out ::XmlNodeType & nodeType)
{
    bool isEOF;
    auto error = liteReader_->Read(nodeType, isEOF);
    ThrowIfNotSuccess(error);

    // any movement of the reader will clear the current attribute map
    // to read attributes, reader must be positioned on the element node
    if (attrValues_.size() > 0) { attrValues_.clear(); }

    return !isEOF;
}

bool XmlReader::IsEmptyElement()
{
    return liteReader_->IsEmptyElement();
}

void XmlReader::ThrowUnexpectedEOF(wstring const & operationName)
{
    WriteError(
        TraceType_XmlReader,
        "Unexpected EOF in {0}. InputName={1}",
        operationName,
        liteReader_->InputName);

    throw XmlException(ErrorCode(ErrorCodeValue::XmlUnexpectedEndOfFile));
}

void XmlReader::ThrowInvalidContent(::XmlNodeType expectedContentType, ::XmlNodeType actualContentType)
{
    UINT lineNo = GetLineNumber();
    UINT linePos = GetLinePosition();

    WriteError(
        TraceType_XmlReader,
        "Invalid content type, expected {0}, found {1}. Input={2}, Line={3}, Position={4}",
        expectedContentType,
        actualContentType,
        liteReader_->InputName,
        lineNo,
        linePos);

    throw XmlException(ErrorCode(ErrorCodeValue::XmlInvalidContent));
}

void XmlReader::ThrowInvalidContent(wstring const & expectedContent, wstring const & actualContent)
{
    UINT lineNo = GetLineNumber();
    UINT linePos = GetLinePosition();

    WriteError(
        TraceType_XmlReader,
        "Invalid content, expected {0}, found {1}. Input={2}, Line={3}, Position={4}",
        expectedContent,
        actualContent,
        liteReader_->InputName,
        lineNo,
        linePos);

    throw XmlException(ErrorCode(ErrorCodeValue::XmlInvalidContent));
}

void XmlReader::ThrowIfNotSuccess(HRESULT hr, wstring const& operationNAme)
{
    if (hr != S_OK)
    {
        WriteError(
            TraceType_XmlReader,
            "{0} failed. HRESULT={1}",
            operationNAme,
            hr);
        ThrowIfNotSuccess(ErrorCode::FromHResult(hr));
    }
}

void XmlReader::ThrowIfNotSuccess(ErrorCode const & error, wstring const& operationNAme)
{
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType_XmlReader,
            "{0} failed. ErrorCode={1}",
            operationNAme,
            error);
        throw XmlException(error);
    }
}

void XmlReader::ThrowIfNotSuccess(ErrorCode const & error)
{
    if (!error.IsSuccess())
    {
        throw XmlException(error);
    }
}

