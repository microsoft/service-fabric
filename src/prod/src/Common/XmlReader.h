// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    class XmlReader;
    typedef std::unique_ptr<XmlReader> XmlReaderUPtr;

    // XmlReader built on top of XmlLite based reader that provides methods 
    // similar to .NET XmlReader to allow easier de-serialization of XML.
    // 
    // All instance methods of this type throws XmlException if reader 
    // fails. Use of exception allows writing straightforward deserialization 
    // code. For example, see Parser.cpp in ServiceModel.
    class XmlReader :
        TextTraceComponent<TraceTaskCodes::Common>
    {
        DENY_COPY(XmlReader)

    public:
        static ErrorCode Create(std::wstring const & fileName, __out XmlReaderUPtr & reader);
        static Common::ErrorCode ReadXmlFile(std::wstring const & fileName, std::wstring const & rootElementName,  std::wstring const & namespaceUri, __out std::wstring & xmlContent);

        XmlReader(ComProxyXmlLiteReaderUPtr & xmlLiteReader);
        ~XmlReader();

        // skips non-content nodes. checks if the first content node is an element with the specified name and namespace
        // use this method to parse optional elements
        // positions the reader on the start element tag that allows reading through the attributes
        bool IsStartElement(std::wstring const & localName, std::wstring const & namespaceUri, bool readAttrs = true);

        // skips non-content nodes and ensures that the first content node is an element with the specified name and namespace
        // use this method to ensure required elements are present in the XML
        // positions the reader on the start element tag that allows reading through the attributes
        void StartElement(std::wstring const & localName, std::wstring const & namespaceUri, bool readAttrs = true);

        // reads the start element tag including all of the attributes and positions the reader on the next node
        // the reader must be positioned on the start element. This method will skip all of the attributes. To read
        // the attribute use StartElement method and then call attribute methods
        void ReadStartElement();

        // moves the reader to first non-content node and ensures that it is an end element tag and consumes it.
        // positions the reader on the node after the end element tag
        void ReadEndElement();

        // ensures that the reader is positioned on the start element tag and then reads both start and end element tags
        // use thid method to read through the elements that have no content (takes care of the element without end tags)
        // positions the reader on the node after the end element tag
        void ReadElement();


        // ensures that the reader is positioned on the start element tag and then reads start and 
        // positions the reader on the node after the element tag (child element)
        void MoveToNextElement();

        // moves the reader to the next non-content node and ensures that it is an element node with the specified
        // name and namespace (if not optional). reads through the entire XML under that element and the end element tag. 
        // positions the reader on the next node after the end element tag
        // use this method to skip through the elements that you do not care in the de-serialization
        void SkipElement(std::wstring const & localName, std::wstring const & namespaceUri, bool isOptional = false);
        
        // returns true if the reader is positioned on the start element and the element is empty (no end tag)
        bool IsEmptyElement();

        // return true if the reader has specified attribute
        // call this method after positioning the reader on the start element using StartElement or successful IsStartElement method
        bool HasAttribute(std::wstring const & attrName, std::wstring const & namespaceUri = L"");

        // returns the value of the specified attribute, throws an error if the attribute is not found
        // call this method after positioning the reader on the start element using StartElement or successful IsStartElement method
        std::wstring ReadAttributeValue(std::wstring const & attrName, std::wstring const & namespaceUri = L"", bool trim = false);

        // reads the content of the element in a string
        // reads through the content of the element and advances reader to the end element tag or next element tag
        std::wstring ReadElementValue(bool trim = false);

        // reads the content, including markup, representing this node and all its children.
        std::wstring ReadOuterXml();

        // returns the value of the specified attribute as the T, throws an error if the attribute is not found
        // call this method after positioning the reader on the start element using StartElement or successful IsStartElement method
        template <typename T>
        T ReadAttributeValueAs(std::wstring const & attrName, std::wstring const & namespaceUri = L"")
        {
            T parsed;
            std::wstring attrValue = ReadAttributeValue(attrName, namespaceUri);
            if (!StringUtility::TryFromWString(attrValue, parsed))
            {
                ThrowInvalidContent(
                    StringUtility::ToWString(std::string(typeid(parsed).name())), 
                    attrValue);
            }

            return parsed;
        }

        UINT GetLineNumber();
        UINT GetLinePosition();

        __declspec (property(get=get_FileName)) std::wstring const & FileName;
        std::wstring const & get_FileName() const { return this->liteReader_->InputName; }

        __declspec (property(get=get_LiteReader)) ComProxyXmlLiteReaderUPtr const& LiteReader;
        ComProxyXmlLiteReaderUPtr const& get_LiteReader() const { return this->liteReader_; }

        ::XmlNodeType MoveToContent();

#if !defined(PLATFORM_UNIX)
    private:
#else
    public:
#endif
        void ReadContent(::XmlNodeType contentType);
        void ReadAttributes();
        std::wstring GetFullyQualifiedName();
        static std::wstring GetFullyQualifiedName(std::wstring const & namespaceUri, std::wstring const & localName);

        // wrappers for XmlLite Readers that throws
        UINT GetAttributeCount();
        UINT GetDepth();
        std::wstring GetLocalName();
        std::wstring GetNamespaceUri();
        ::XmlNodeType GetNodeType();
        std::wstring GetPrefix();
        std::wstring GetQualifiedName();
        std::wstring GetValue();
        void MoveToElement();
        bool MoveToFirstAttribute();
        bool MoveToNextAttribute();
        void Read();
        bool Read(__out ::XmlNodeType & nodeType);

        // throw utility methods
        void ThrowUnexpectedEOF(std::wstring const & operationName);
        void ThrowInvalidContent(XmlNodeType expectedContentType, XmlNodeType actualContentType);
        void ThrowInvalidContent(std::wstring const & expectedContent, std::wstring const & actualContent);
        void ThrowIfNotSuccess(ErrorCode const & error);
        void ThrowIfNotSuccess(ErrorCode const & error, std::wstring const& operationNAme);
        void ThrowIfNotSuccess(HRESULT hr, std::wstring const& operationNAme);
    private:
        ComProxyXmlLiteReaderUPtr liteReader_;
        std::map<std::wstring, std::wstring> attrValues_;
    };
}
