// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceType_NTLMAuthenticationPolicyDescription("NTLMAuthenticationPolicyDescription");

NTLMAuthenticationPolicyDescription::NTLMAuthenticationPolicyDescription()
: IsEnabled(false),
PasswordSecret(L""),
IsPasswordSecretEncrypted(false),
X509StoreLocation(X509Default::StoreLocation()),
X509StoreName(X509Default::StoreName()),
X509Thumbprint(L"")
{
}

NTLMAuthenticationPolicyDescription::NTLMAuthenticationPolicyDescription(NTLMAuthenticationPolicyDescription const & other)
: IsEnabled(other.IsEnabled),
PasswordSecret(other.PasswordSecret),
IsPasswordSecretEncrypted(other.IsPasswordSecretEncrypted),
X509StoreLocation(other.X509StoreLocation),
X509StoreName(other.X509StoreName),
X509Thumbprint(other.X509Thumbprint)
{
}

NTLMAuthenticationPolicyDescription::NTLMAuthenticationPolicyDescription(NTLMAuthenticationPolicyDescription && other)
: IsEnabled(other.IsEnabled),
PasswordSecret(move(other.PasswordSecret)),
IsPasswordSecretEncrypted(other.IsPasswordSecretEncrypted),
X509StoreLocation(move(other.X509StoreLocation)),
X509StoreName(move(other.X509StoreName)),
X509Thumbprint(move(other.X509Thumbprint))
{
}

NTLMAuthenticationPolicyDescription const & NTLMAuthenticationPolicyDescription::operator = (NTLMAuthenticationPolicyDescription const & other)
{
    if(this != &other)
    {
        this->IsEnabled = other.IsEnabled;
        this->PasswordSecret = other.PasswordSecret;
        this->IsPasswordSecretEncrypted = other.IsPasswordSecretEncrypted;
        this->X509StoreLocation = other.X509StoreLocation;
        this->X509StoreName = other.X509StoreName;
        this->X509Thumbprint = other.X509Thumbprint;
    }

    return *this;
}

NTLMAuthenticationPolicyDescription const & NTLMAuthenticationPolicyDescription::operator = (NTLMAuthenticationPolicyDescription && other)
{
    if(this != &other)
    {
        this->IsEnabled = other.IsEnabled;
        this->PasswordSecret = move(other.PasswordSecret);
        this->IsPasswordSecretEncrypted = other.IsPasswordSecretEncrypted;
        this->X509StoreLocation = move(other.X509StoreLocation);
        this->X509StoreName = move(other.X509StoreName);
        this->X509Thumbprint = move(other.X509Thumbprint);
    }

    return *this;
}

bool NTLMAuthenticationPolicyDescription::operator == (NTLMAuthenticationPolicyDescription const & other) const
{
    bool equals = true;

    equals = this->IsEnabled == other.IsEnabled;
    if(!equals) { return equals; }

    equals = this->PasswordSecret == other.PasswordSecret;
    if(!equals) { return equals; }

    equals = this->IsPasswordSecretEncrypted == other.IsPasswordSecretEncrypted;
    if(!equals) { return equals; }

    equals = this->X509StoreLocation == other.X509StoreLocation;
    if(!equals) { return equals; }

    equals = this->X509StoreName == other.X509StoreName;
    if(!equals) { return equals; }

    equals = this->X509Thumbprint == other.X509Thumbprint;
    if(!equals) { return equals; }

    return equals;
}

bool NTLMAuthenticationPolicyDescription::operator != (NTLMAuthenticationPolicyDescription const & other) const
{
    return !(*this == other);
}

void NTLMAuthenticationPolicyDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("NTLMAuthenticationPolicyDescription { ");
    w.Write("IsEnabled = {0}, ", IsEnabled);
    w.Write("PasswordSecret = {0}, ", PasswordSecret);
    w.Write("IsPasswordSecretEncrypted = {0}", IsPasswordSecretEncrypted);
    w.Write("X509StoreName = {0}", X509StoreLocation);
    w.Write("X509StoreName = {0}", X509StoreName);
    w.Write("X509Thumbprint = {0}", X509Thumbprint);
    w.Write("}");
}

void NTLMAuthenticationPolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader,
    bool const isPasswordSecretOptional)
{
    // <NTLMAuthenticationPolicy IsEnabled="" PasswordSecret="" IsPasswordSecretEncrypted="", X509StoreName="", X509Thumbprint="" />
    xmlReader->StartElement(
        *SchemaNames::Element_NTLMAuthenticationPolicy,
        *SchemaNames::Namespace);

    if(xmlReader->HasAttribute(
        *SchemaNames::Attribute_IsEnabled))
    {
        wstring isEnabledString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsEnabled);
        bool success = StringUtility::TryFromWString<bool>(isEnabledString, this->IsEnabled);
        ASSERT_IFNOT(success, "FromString: Invalid argument {0}", isEnabledString);
    }
    else
    {
        this->IsEnabled = true;
    }

    if(isPasswordSecretOptional)
    {
        if(xmlReader->HasAttribute(*SchemaNames::Attribute_PasswordSecret))
        {
            this->PasswordSecret = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_PasswordSecret);
        }
    }
    else
    {
        this->PasswordSecret = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_PasswordSecret);
    }
    
    if(xmlReader->HasAttribute(
        *SchemaNames::Attribute_PasswordSecretEncrypted))
    {
        wstring isPasswordSecretEncryptedString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_PasswordSecretEncrypted);
        bool success = StringUtility::TryFromWString<bool>(isPasswordSecretEncryptedString, this->IsPasswordSecretEncrypted);
        ASSERT_IFNOT(success, "FromString: Invalid argument {0}", isPasswordSecretEncryptedString);
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_X509StoreLocation))
    {
        auto x509StoreLocation = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_X509StoreLocation);
        auto error = X509StoreLocation::Parse(x509StoreLocation, this->X509StoreLocation);
        ASSERT_IFNOT(error.IsSuccess(), "ReadFromXml: Invalid argument {0}", x509StoreLocation);
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_X509StoreName))
    {
        this->X509StoreName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_X509StoreName);
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_X509Thumbprint))
    {
        this->X509Thumbprint = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_X509Thumbprint);
    }

    xmlReader->ReadElement();
}


ErrorCode NTLMAuthenticationPolicyDescription::WriteToXml(XmlWriterUPtr const & xmlWriter, bool const isPasswordSecretOptional)
{
	//<NTLM...>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_NTLMAuthenticationPolicy, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}

	er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_IsEnabled, this->IsEnabled);
	if (!er.IsSuccess())
	{
		return er;
	}

	if (!isPasswordSecretOptional)
	{
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_PasswordSecret, this->PasswordSecret);
		if (!er.IsSuccess())
		{
			return er;
		}

		er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_PasswordSecretEncrypted,
			this->IsPasswordSecretEncrypted);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

    if(this->X509StoreLocation != X509Default::StoreLocation())
    {
        wstring x509StoreLocation = wformatString("{0}", this->X509StoreLocation);
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_X509StoreLocation, x509StoreLocation);
        if(!er.IsSuccess())
        {
            return er;
        }
    }

    if(this->X509StoreName != X509Default::StoreName()) 
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_X509StoreName, this->X509StoreName);
        if(!er.IsSuccess())
        {
            return er;
        }
    }    

    if(!this->X509Thumbprint.empty())
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_X509Thumbprint, this->X509Thumbprint);
        if(!er.IsSuccess())
        {
            return er;
        }
    }

	return xmlWriter->WriteEndElement();
}



void NTLMAuthenticationPolicyDescription::clear()
{
    this->IsEnabled = false;
    this->PasswordSecret.clear();
    this->IsPasswordSecretEncrypted = false;
    this->X509StoreLocation = X509Default::StoreLocation();
    this->X509StoreName = X509Default::StoreName(); 
    this->X509Thumbprint.clear();
}
