//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceFabricRuntimeAccessDescription::ServiceFabricRuntimeAccessDescription()
    : RemoveServiceFabricRuntimeAccess(false),
    UseServiceFabricReplicatedStore(false)
{
}

ServiceFabricRuntimeAccessDescription::ServiceFabricRuntimeAccessDescription(ServiceFabricRuntimeAccessDescription && other)
    : RemoveServiceFabricRuntimeAccess(other.RemoveServiceFabricRuntimeAccess),
    UseServiceFabricReplicatedStore(other.UseServiceFabricReplicatedStore)
{
}

ServiceFabricRuntimeAccessDescription::ServiceFabricRuntimeAccessDescription(ServiceFabricRuntimeAccessDescription const& other)
    : RemoveServiceFabricRuntimeAccess(other.RemoveServiceFabricRuntimeAccess),
    UseServiceFabricReplicatedStore(other.UseServiceFabricReplicatedStore)
{
}

ServiceFabricRuntimeAccessDescription const & ServiceFabricRuntimeAccessDescription::operator=(
    ServiceFabricRuntimeAccessDescription && other)
{
    if (this != &other)
    {
        RemoveServiceFabricRuntimeAccess = other.RemoveServiceFabricRuntimeAccess;
        UseServiceFabricReplicatedStore = other.UseServiceFabricReplicatedStore;
    }
    return *this;
}

ServiceFabricRuntimeAccessDescription const & ServiceFabricRuntimeAccessDescription::operator=(
    ServiceFabricRuntimeAccessDescription const & other)
{
    if (this != &other)
    {
        RemoveServiceFabricRuntimeAccess = other.RemoveServiceFabricRuntimeAccess;
        UseServiceFabricReplicatedStore = other.UseServiceFabricReplicatedStore;
    }
    return *this;
}

bool ServiceFabricRuntimeAccessDescription::operator==(ServiceFabricRuntimeAccessDescription const & other) const
{
    return RemoveServiceFabricRuntimeAccess == other.RemoveServiceFabricRuntimeAccess &&
        UseServiceFabricReplicatedStore == other.UseServiceFabricReplicatedStore;

}

bool ServiceFabricRuntimeAccessDescription::operator!=(ServiceFabricRuntimeAccessDescription const & other) const
{
    return !(*this == other);
}

void ServiceFabricRuntimeAccessDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(wformatString(
        "ServiceFabricRuntimeAccessDescription (RemoveServiceFabricRuntimeAccess = {0}, UseServiceFabricReplicatedStore = {1})",
        RemoveServiceFabricRuntimeAccess,
        UseServiceFabricReplicatedStore));
}

void ServiceFabricRuntimeAccessDescription::ReadFromXml(Common::XmlReaderUPtr const & xmlReader)
{
    // <ServiceFabricRuntimeAccessPolicy RemoveServiceFabricRuntimeAccess=true>
    xmlReader->StartElement(
        *SchemaNames::Element_ServiceFabricRuntimeAccessPolicy,
        *SchemaNames::Namespace);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_RemoveServiceFabricRuntimeAccess))
    {
        auto removeServiceFabricRuntimeAccessString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RemoveServiceFabricRuntimeAccess);
        if (!StringUtility::TryFromWString<bool>(
            removeServiceFabricRuntimeAccessString,
            this->RemoveServiceFabricRuntimeAccess))
        {
            Parser::ThrowInvalidContent(xmlReader, L"RemoveServiceFabricRuntimeAccess", removeServiceFabricRuntimeAccessString);
        }
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_UseServiceFabricReplicatedStore))
    {
        auto useSFReplicatedStore = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UseServiceFabricReplicatedStore);
        if (!StringUtility::TryFromWString<bool>(
            useSFReplicatedStore,
            this->UseServiceFabricReplicatedStore))
        {
            Parser::ThrowInvalidContent(xmlReader, L"UseServiceFabricReplicatedStore", useSFReplicatedStore);
        }
    }
    // Read the rest of the empty element
    xmlReader->ReadElement();
}
