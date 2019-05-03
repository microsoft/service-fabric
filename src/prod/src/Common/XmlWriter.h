// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    class XmlWriter;
    typedef std::unique_ptr<XmlWriter> XmlWriterUPtr;

    // XmlWriter built on top of XmlLite based writer that provides methods 
    // similar to .NET XmlReader to allow easier de-serialization of XML.
    // 
    // All methods of this type return an error code detailing success or failure
    // of the method call
    class XmlWriter :
        TextTraceComponent<TraceTaskCodes::Common>
    {
        DENY_COPY(XmlWriter)

    public:
        XmlWriter(ComProxyXmlLiteWriterUPtr & xmlLiteWriter);
        ~XmlWriter();
        static ErrorCode Create(std::wstring const & fileName, __out XmlWriterUPtr & writer, bool writeByteOrderMark = true, bool indent = true);

        static ErrorCode Create(
                Common::ComPointer<IUnknown> const& output, 
                __out XmlWriterUPtr & writer, 
                bool writeByteOrderMark = true, 
                bool indent = true);

        // copies everything from the reader to the writer and moves the reader to the start of the next sibling
        ErrorCode WriteNode(XmlReader & reader, bool writeDefaultAttributes = false);

        ErrorCode Flush();

        //Writes XML version and if the document is  standalone or otherwise
        //@param XmlStandalone - if the given document is a standalone XML document
        //       or if it belongs to another document, default value is Omit this part
        //       of this delcaration
        ErrorCode WriteStartDocument(XmlStandalone value = XmlStandalone_Omit);

        //Gives the the current element the given attribute name, value pair.
        //@param std::wstring & attrName, the name of the attribute
        //@param std::wstring & attrValue, the value of the attribute
        //@param std::wstring & prefix, the prefix of the attribute (default is empty string)
        //@param std::wstring & nameSpace, the namespace for the attribute (default is empty string)
        ErrorCode WriteAttribute(std::wstring const & attrName, std::wstring const & value,
            std::wstring const & prefix = L"", std::wstring const & nameSpace = L"");

		//Writes a numerical attribute value with the given attribute name
		//to the current element
		//@param Type - the type of the attribute value
		//@param std::wstring & attrName, the name of the attribute
		//@param Type & attrValue, the value of the attribute
		//@param std::wstring & prefix, the prefix of the attribute (default is empty string)
		//@param std::wstring & nameSpace, the namespace for the attribute (default is empty string)
		template<typename Type>
		ErrorCode WriteNumericAttribute(std::wstring const & attrName, Type & value,
			std::wstring const & prefix = L"", std::wstring const & nameSpace = L"")
		{
			wostringstream stream;
			stream << value;
			wstring attrValue = stream.str();
			return  liteWriter_->WriteAttribute(attrName, attrValue, prefix, nameSpace);
		}

		//Writes a numerical attribute value with the given attribute name
		//to the current element
		//@param Type - the type of the attribute value
		//@param std::wstring & attrName, the name of the attribute
		//@param bool & attrValue, the value of the attribute
		//@param std::wstring & prefix, the prefix of the attribute (default is empty string)
		//@param std::wstring & nameSpace, the namespace for the attribute (default is empty string)
		ErrorCode WriteBooleanAttribute(std::wstring const & attrName, bool & value,
			std::wstring const & prefix = L"", std::wstring const & nameSpace = L"");

        //Writes a valid comment at the current head of the writer 
        //@param std::wstring & comment - the comment to be written
        ErrorCode WriteComment(std::wstring const & comment);

        //Writes the document type with the describing parameters
        ErrorCode WriteDocType(std::wstring const & name, std::wstring const & pubId, std::wstring const & sysid,
            std::wstring const & subset);

        //Writes the given wchar as content to the current element
        //@param std::wstring content - the content to write to the string
        ErrorCode WriteString(std::wstring const & content);

        //Creates a new element as a child of the current element with the given parameters
        ErrorCode WriteStartElement(std::wstring const & name, std::wstring const & prefix = L"",
            std::wstring const & nameSpace = L"");

        //Creats a new element with the given parameters with the given string as content
        ErrorCode WriteElementWithContent(std::wstring const & name, std::wstring const content,
            std::wstring const & prefix = L"", std::wstring const & nameSpace = L"");

        //Ends all the current tags up to and including the root element
        ErrorCode WriteEndDocument();

        //Writes the end tag for the current element
        ErrorCode WriteEndElement();

        //Ends the document and flushes
        ErrorCode Close();

        __declspec (property(get=get_FileName)) std::wstring const & FileName;
        std::wstring const & get_FileName() const { return this->liteWriter_->OutputName; }

    private:
        ComProxyXmlLiteWriterUPtr liteWriter_;

        void ThrowIfNotSuccess(ErrorCode const & error);
    };
}
