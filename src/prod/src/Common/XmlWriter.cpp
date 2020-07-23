// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace std;
using namespace Common;

StringLiteral const TraceType_XmlWriter = "XmlWriter";

XmlWriter::XmlWriter(ComProxyXmlLiteWriterUPtr & xmlLiteWriter)
    : liteWriter_(move(xmlLiteWriter))
{
}

XmlWriter::~XmlWriter()
{
}

ErrorCode XmlWriter::Create(wstring const & fileName, __out XmlWriterUPtr & writer, bool writeByteOrderMark, bool indent)
{
    ComProxyXmlLiteWriterUPtr liteWriter;

    auto uncPath = Path::ConvertToNtPath(fileName);

    auto error = ComProxyXmlLiteWriter::Create(uncPath, liteWriter, writeByteOrderMark, indent);
    if (!error.IsSuccess()) { return error; }

    writer = move(make_unique<XmlWriter>(liteWriter));

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode XmlWriter::Create(ComPointer<IUnknown> const& output, __out XmlWriterUPtr & writer, bool writeByteOrderMark, bool indent)
{
    ComProxyXmlLiteWriterUPtr liteWriter;

    auto error = ComProxyXmlLiteWriter::Create(output, liteWriter, writeByteOrderMark, indent);
    if (!error.IsSuccess()) { return error; }

    writer = move(make_unique<XmlWriter>(liteWriter));
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode XmlWriter::WriteNode(XmlReader & creader, bool writeDefaultAttributes)
{
#if !defined(PLATFORM_UNIX)
    auto error = liteWriter_->WriteNode(creader.LiteReader->XmlLiteReader, writeDefaultAttributes);
    return error;
#else
    XmlReader &reader = creader;
    UINT startDepth = reader.GetDepth();
    UINT endDepth = -1;

    ::XmlNodeType nodeType;
    wstring elemName, elemContent;

    do {
        nodeType = reader.GetNodeType();
        switch (nodeType) {
        case XmlNodeType_Element:
        {
            elemName = reader.GetLocalName();
            liteWriter_->WriteStartElement(elemName);

            bool attrFound = reader.MoveToFirstAttribute();
            bool doMoveToElement = attrFound;
            while (attrFound)
            {
                liteWriter_->WriteAttribute(reader.GetLocalName(), reader.GetValue(), reader.GetPrefix());
                attrFound = reader.MoveToNextAttribute();
            }
            if (doMoveToElement)
            {
                reader.MoveToElement();
            }
            if(reader.IsEmptyElement())
            {
                liteWriter_->WriteEndElement();
            }
            break;
        }
        case XmlNodeType_Text:
        {
            elemContent = reader.GetValue();
            liteWriter_->WriteString(elemContent);
            break;
        }
        case XmlNodeType_EndElement:
            liteWriter_->WriteEndElement();
            endDepth = reader.GetDepth();
            break;

        case XmlNodeType_Comment:
            liteWriter_->WriteComment(reader.GetValue());
            break;

        }

        reader.Read(nodeType);
    } while (startDepth != endDepth);

    liteWriter_->Flush();
    return ErrorCode(ErrorCodeValue::Success);
#endif
}

ErrorCode XmlWriter::Flush()
{
    auto error = liteWriter_->Flush();
    return error;
}

ErrorCode XmlWriter::WriteStartDocument(XmlStandalone value)
{
    return liteWriter_->WriteStartDocument(value);
}

ErrorCode XmlWriter::WriteAttribute(std::wstring const & attrName, std::wstring const & value,
    std::wstring const & prefix, std::wstring const & nameSpace )
{
    return liteWriter_->WriteAttribute(attrName, value, prefix, nameSpace);
}



ErrorCode XmlWriter::WriteBooleanAttribute(std::wstring const & attrName, bool & value,
	std::wstring const & prefix, std::wstring const & nameSpace)
{
	wstring attrValue = value ? L"true" : L"false";
	return liteWriter_->WriteAttribute(attrName, attrValue, prefix, nameSpace);
}


ErrorCode XmlWriter::WriteComment(std::wstring const & comment)
{
    return liteWriter_->WriteComment(comment);
}

ErrorCode XmlWriter::WriteDocType(std::wstring const & name, std::wstring const & pubId, std::wstring const & sysid,
    std::wstring const & subset)
{
    return liteWriter_->WriteDocType(name, pubId, sysid, subset);
}



ErrorCode XmlWriter::WriteString(std::wstring const & content)
{
    return liteWriter_->WriteString(content);
}


ErrorCode XmlWriter::WriteStartElement(std::wstring const & name, std::wstring const & prefix,
    std::wstring const & nameSpace)
{
    return liteWriter_->WriteStartElement(name, prefix, nameSpace);
}

ErrorCode XmlWriter::WriteElementWithContent(std::wstring const & name, std::wstring const content,
    std::wstring const & prefix, std::wstring const & nameSpace)
{
    return liteWriter_->WriteElementWithContent(name, content, prefix, nameSpace);
}

ErrorCode XmlWriter::WriteEndDocument()
{
    return liteWriter_->WriteEndDocument();
}

ErrorCode XmlWriter::WriteEndElement()
{
    return liteWriter_->WriteEndElement();
}

ErrorCode XmlWriter::Close()
{
    return liteWriter_->Close();
}



void XmlWriter::ThrowIfNotSuccess(ErrorCode const & error)
{
    if (!error.IsSuccess())
    {
        throw XmlException(error);
    }
}

