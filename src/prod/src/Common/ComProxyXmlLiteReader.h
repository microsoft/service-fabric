// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    class ComProxyXmlLiteReader;
    typedef std::unique_ptr<ComProxyXmlLiteReader> ComProxyXmlLiteReaderUPtr;

    // wraps methods of XmlLite IXmlReader
    class ComProxyXmlLiteReader :
        TextTraceComponent<TraceTaskCodes::Common>
    {
        DENY_COPY(ComProxyXmlLiteReader)

    public:
        ComProxyXmlLiteReader(ComPointer<IXmlReader> reader);
        ~ComProxyXmlLiteReader();

        static ErrorCode Create(
            __in std::wstring const & inputName,
            __out ComProxyXmlLiteReaderUPtr & xmlLiteReader);

        static ErrorCode Create(
            __in std::wstring const & inputName,
            __in IStream *inputStream,
            __out ComProxyXmlLiteReaderUPtr & xmlLiteReader);

#if defined(PLATFORM_UNIX)
        static ErrorCode Create(
            __in const char *xmlString,
            __out ComProxyXmlLiteReaderUPtr & xmlLiteReader);
#endif

        ErrorCode GetAttributeCount(__out UINT & attributeCount);
        ErrorCode GetDepth(__out UINT & depth);
        ErrorCode GetLineNumber(__out UINT & lineNumber);
        ErrorCode GetLinePosition(__out UINT & linePosition);
        ErrorCode GetLocalName(__out std::wstring & localName);
        ErrorCode GetNamespaceUri(__out std::wstring & namespaceUri);
        ErrorCode GetNodeType(__out ::XmlNodeType & nodeType);
        ErrorCode GetPrefix(__out std::wstring & prefix);
        ErrorCode GetQualifiedName(__out std::wstring & qualifiedName);
        ErrorCode GetValue(__out std::wstring & value);
        bool IsEmptyElement();
        bool IsEOF();
        ErrorCode MoveToAttributeByName(std::wstring const & attrName, std::wstring const & namespaceUri, __out bool & attrFound);
        ErrorCode MoveToElement();
        ErrorCode MoveToFirstAttribute(__out bool & success);
        ErrorCode MoveToNextAttribute(__out bool & success);
        ErrorCode Read(__out ::XmlNodeType & nodeType, __out bool & isEOF);

        __declspec (property(get=get_InputName)) std::wstring const & InputName;
        std::wstring const & get_InputName() const { return this->inputName_; }

        __declspec (property(get=get_XmlLiteReader)) ComPointer<IXmlReader> const & XmlLiteReader;
        ComPointer<IXmlReader> const & get_XmlLiteReader() const { return this->reader_; }

    private:
        ErrorCode SetInput(std::wstring const & inputName, ComPointer<IStream> const & stream);
        bool TryGetLineNumber(__out UINT & lineNumber);
        bool TryGetLinePosition(__out UINT & linePosition);
        ErrorCode ToErrorCode(HRESULT hr, std::wstring const & operationName);
        ErrorCode OnReaderError(HRESULT hr, std::wstring const & operationName);

    private:
        ComPointer<IXmlReader> reader_;
        std::wstring inputName_;
    };
}
