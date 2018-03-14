// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

EndpointDescription::EndpointDescription() 
    : Name(),
    Protocol(ProtocolType::Tcp),
    Type(EndpointType::Internal),    
    CertificateRef(),
    Port(0),
    ExplicitPortSpecified(false),
    UriScheme(),
    PathSuffix(),
    CodePackageRef(),
    IpAddressOrFqdn()
{
}

bool EndpointDescription::operator == (EndpointDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = (this->Protocol == other.Protocol);
    if (!equals) { return equals; }

    equals = (this->Type == other.Type);
    if (!equals) { return equals; }    

    equals = StringUtility::AreEqualCaseInsensitive(this->CertificateRef, other.CertificateRef);
    if (!equals) { return equals; }

    equals = (this->Port == other.Port);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->UriScheme, other.UriScheme);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->PathSuffix, other.PathSuffix);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->CodePackageRef, other.CodePackageRef);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->IpAddressOrFqdn, other.IpAddressOrFqdn);
    if (!equals) { return equals; }

    return equals;
}

bool EndpointDescription::operator != (EndpointDescription const & other) const
{
    return !(*this == other);
}

void EndpointDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("EndpointDescription { ");
    w.Write("Name = {0}, Protocol = {1}, Type = {2}, Certificate = {3}, Port = {4}, UriScheme={5}, PathSuffix={6}, CodePackageRef={7}, IpAddressOrFqdn={8}.", 
        Name, 
        Protocol,
        Type,
        CertificateRef,
        Port,
        UriScheme,
        PathSuffix,
        CodePackageRef,
        IpAddressOrFqdn);
    w.Write("}");
}

void EndpointDescription::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in std::vector<EndpointDescription> const & endpoints, 
    __out  FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST & publicEndpoints)
{
    auto endpointsCount = endpoints.size();
    publicEndpoints.Count = static_cast<ULONG>(endpointsCount);

    if (endpointsCount > 0)
    {
        auto items = heap.AddArray<FABRIC_ENDPOINT_RESOURCE_DESCRIPTION>(endpointsCount);

        for (size_t i = 0; i < items.GetCount(); ++i)
        {
            endpoints[i].ToPublicApi(heap, items[i]);
        }

        publicEndpoints.Items = items.GetRawArray();
    }
    else
    {
        publicEndpoints.Items = NULL;
    }
}

void EndpointDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_ENDPOINT_RESOURCE_DESCRIPTION & publicDescription) const
{
    auto description = heap.AddItem<FABRIC_ENDPOINT_RESOURCE_DESCRIPTION>();

    publicDescription.Name = heap.AddString(this->Name);

    wstring protocolString;
    StringWriter(protocolString).Write(this->Protocol);
    publicDescription.Protocol = heap.AddString(protocolString);
    
    wstring typeString;
    StringWriter(typeString).Write(this->Type);
    publicDescription.Type = heap.AddString(typeString);
    
    publicDescription.Port = this->Port;    

    publicDescription.CertificateName = heap.AddString(this->CertificateRef);

    auto endpointDescriptionEx1Ptr = heap.AddItem<FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX1>();
    publicDescription.Reserved = endpointDescriptionEx1Ptr.GetRawPointer();
    
    FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX1 & endpointDescriptionEx1 = *endpointDescriptionEx1Ptr;
    endpointDescriptionEx1.UriScheme = heap.AddString(this->UriScheme);
    endpointDescriptionEx1.PathSuffix = heap.AddString(this->PathSuffix); 

    auto endpointDescriptionEx2Ptr = heap.AddItem<FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX2>();
    endpointDescriptionEx1.Reserved = endpointDescriptionEx2Ptr.GetRawPointer();

    auto & endpointDescriptionEx2 = *endpointDescriptionEx2Ptr;
    endpointDescriptionEx2.CodePackageName = heap.AddString(this->CodePackageRef);
    endpointDescriptionEx2.IpAddressOrFqdn = heap.AddString(this->IpAddressOrFqdn);
}

void EndpointDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_Endpoint, 
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Protocol))
    {
        wstring protocol = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Protocol);
        this->Protocol = ProtocolType::GetProtocolType(protocol);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Type))
    {
        wstring type = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Type);
        this->Type = EndpointType::GetEndpointType(type);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_CodePackageRef))
    {
        this->CodePackageRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CodePackageRef);
    }
    
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_CertificateRef))
    {
        this->CertificateRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CertificateRef);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Port))
    {
        this->Port = xmlReader->ReadAttributeValueAs<ULONG>(*SchemaNames::Attribute_Port);
        this->ExplicitPortSpecified = true;
    }    
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_UriScheme))
    {
        this->UriScheme = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UriScheme);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_PathSuffix))
    {
        this->PathSuffix = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_PathSuffix);
    }
    xmlReader->ReadElement();
}

void EndpointDescription::clear()
{
    this->Name.clear();
    this->CertificateRef.clear();
    this->Port = 0;    
    this->Type = EndpointType::Internal;
    this->Protocol = ProtocolType::Tcp;
    this->UriScheme.clear();
    this->PathSuffix.clear();
    this->CodePackageRef.clear();
    this->IpAddressOrFqdn.clear();
}

ErrorCode EndpointDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Endpoint, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Protocol, ProtocolType::EnumToString(this->Protocol));
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Type, EndpointType::EnumToString(this->Type));
    if (!er.IsSuccess())
    {
        return er;
    }

    if (!(this->CodePackageRef.empty()))
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_CodePackageRef, this->CodePackageRef);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    if (!(this->CertificateRef.empty()))
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_CertificateRef, this->CertificateRef);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    if (this->ExplicitPortSpecified)
    {
        er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_Port, this->Port);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    if (!this->UriScheme.empty())
    {
        er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_UriScheme, this->UriScheme);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    if (!this->PathSuffix.empty())
    {
        er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_PathSuffix, this->PathSuffix);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    return xmlWriter->WriteEndElement();
}

bool EndpointDescription::WriteToFile(wstring const& fileName, vector<EndpointDescription> const& endpointDescriptions)
{
    try
    {
        FileWriter fileWriter;
        auto error = fileWriter.TryOpen(fileName);
        if (!error.IsSuccess())
        {
            Trace.WriteError(
                "EndpointDescription.WriteToFile", 
                "FileName={0}. Failed to open with error={1}.",
                fileName,
                error);
            return false;
        }

        wstring text;
        auto iter = endpointDescriptions.begin();
        while(iter != endpointDescriptions.end())
        {
            EndpointDescription const& endpointDescription = *iter;

            ASSERT_IF(endpointDescription.Name.empty(), "Endpoint Name is empty");

            text.append(wformatString(
                "{0};{1};{2};{3};{4};{5};{6};{7};{8}",
                endpointDescription.Name,
                endpointDescription.Protocol, 
                endpointDescription.Type,                
                endpointDescription.Port,
                endpointDescription.CertificateRef,
                endpointDescription.UriScheme,
                endpointDescription.PathSuffix,
                endpointDescription.CodePackageRef,
                endpointDescription.IpAddressOrFqdn));

            ++iter;
            if(iter != endpointDescriptions.end())
            {
                text.append(L",");
            }
        }

        std::string result;
        StringUtility::UnicodeToAnsi(text, result);
        fileWriter << result;

        Trace.WriteNoise("EndpointDescription.WriteToFile", "FileName={0}. Text Written={1}.", fileName, text);
    }
    catch (std::exception const& e)
    {
        Trace.WriteError("EndpointDescription.WriteToFile", "FileName={0}. Failed with error={1}.", fileName, e.what());
        return false;
    }

    return true;
}

bool EndpointDescription::ReadFromFile(wstring const& fileName, vector<EndpointDescription> & endpointDescriptions)
{
    wstring textW;
    try
    {
        File fileReader;
        auto error = fileReader.TryOpen(fileName, Common::FileMode::Open, Common::FileAccess::Read, Common::FileShare::Read);
        if (!error.IsSuccess())
        {
            Trace.WriteError("EndpointDescription.ReadFromFile", "Failed to open '{0}' with error={1}.", fileName, error);
            return false;
        }

        int64 fileSize = fileReader.size();
        size_t size = static_cast<size_t>(fileSize);

        string text;
        text.resize(size);
        fileReader.Read(&text[0], static_cast<int>(size));
        fileReader.Close();

        StringWriter(textW).Write(text);
    }
    catch (std::exception const& e)
    {
        Trace.WriteError("EndpointDescription.ReadFromFile", "FileName={0}. Failed with error={1}.", fileName, e.what());
        return false;
    }

    Trace.WriteNoise("EndpointDescription.ReadFromFile", "FileName={0}. Text Read: {1}.", fileName, textW);

    StringCollection endpointDescriptionStrings;
    StringUtility::Split<wstring>(textW, endpointDescriptionStrings, L",");
    for (auto iter = endpointDescriptionStrings.begin(); iter != endpointDescriptionStrings.end(); ++iter)
    {
        wstring const& endpointDescriptionString = *iter;
        StringCollection endpointDescriptionValues;
        StringUtility::Split<wstring>(endpointDescriptionString, endpointDescriptionValues, L";", false /* skip empty tokens */);

        ASSERT_IFNOT(endpointDescriptionValues.size() == 9, "Invalid format for {0}", endpointDescriptionStrings);

        EndpointDescription endpointDescription;
        endpointDescription.Name = endpointDescriptionValues[0];
        endpointDescription.Protocol = ProtocolType::GetProtocolType(endpointDescriptionValues[1]);
        endpointDescription.Type = EndpointType::GetEndpointType(endpointDescriptionValues[2]); 

        ULONG port;
        ASSERT_IFNOT(StringUtility::TryFromWString<ULONG>(endpointDescriptionValues[3], port), "Could not parse port in EndpointDescription {0}", endpointDescriptionString);
        
        endpointDescription.Port = port;
        endpointDescription.CertificateRef = endpointDescriptionValues[4];
        endpointDescription.UriScheme = endpointDescriptionValues[5];
        endpointDescription.PathSuffix = endpointDescriptionValues[6];
        endpointDescription.CodePackageRef = endpointDescriptionValues[7];
        endpointDescription.IpAddressOrFqdn = endpointDescriptionValues[8];

        endpointDescriptions.push_back(move(endpointDescription));
    }

    return true;
}
