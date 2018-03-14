// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace AccessControl;
using namespace Common;
using namespace Serialization;
using namespace std;

static GlobalWString AccessControlNamespace = Common::make_global<std::wstring>(L"/system/AccessControl/");

const ResourceType::Enum ResourceIdentifier::type_;

const ResourceType::Enum ResourceIdentifierPathInImageStore::type_;
DEFINE_RESOURCE_IDENTIFIER_TYPE(ResourceIdentifierPathInImageStore)

const ResourceType::Enum ResourceIdentifierApplicationType::type_;
DEFINE_RESOURCE_IDENTIFIER_TYPE(ResourceIdentifierApplicationType)

const ResourceType::Enum ResourceIdentifierApplication::type_;
DEFINE_RESOURCE_IDENTIFIER_TYPE(ResourceIdentifierApplication)

const ResourceType::Enum ResourceIdentifierService::type_;
DEFINE_RESOURCE_IDENTIFIER_TYPE(ResourceIdentifierService)

const ResourceType::Enum ResourceIdentifierName::type_;
DEFINE_RESOURCE_IDENTIFIER_TYPE(ResourceIdentifierName)

_Use_decl_annotations_
ErrorCode ResourceIdentifier::FromPublic(const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource, SPtr & result)
{
    if (!resource)
    {
        result = nullptr;
        return ErrorCodeValue::ArgumentNull;
    }

    switch (resource->Kind)
    {
    case ResourceType::PathInImageStore:
        result = make_shared<ResourceIdentifierPathInImageStore>(((FABRIC_SECURITY_IMAGESTORE_PATH_RESOURCE_IDENTIFIER*)resource->Value)->Path);
        return result->Status();

    case ResourceType::ApplicationType:
        result = make_shared<ResourceIdentifierApplicationType>(((FABRIC_SECURITY_APPLICATION_TYPE_RESOURCE_IDENTIFIER*)resource->Value)->ApplicationTypeName);
        return result->Status();

    case ResourceType::Application:
        result = make_shared<ResourceIdentifierApplication>(((FABRIC_SECURITY_APPLICATION_RESOURCE_IDENTIFIER*)resource->Value)->ApplicationName);
        return result->Status();

    case ResourceType::Service:
        result = make_shared<ResourceIdentifierService>(((FABRIC_SECURITY_SERVICE_RESOURCE_IDENTIFIER*)resource->Value)->ServiceName);
        return result->Status();

    case ResourceType::Name:
        result = make_shared<ResourceIdentifierName>(((FABRIC_SECURITY_NAME_RESOURCE_IDENTIFIER*)resource->Value)->Name);
        return result->Status();

    default:
        return ErrorCodeValue::InvalidArgument;
    }
}

ErrorCode ResourceIdentifier::Status() const
{
    return error_;
}
NamingUri const & ResourceIdentifier::FabricUri() const
{
    return fabricUri_;
}

ResourceType::Enum ResourceIdentifier::Type() const
{
    return type_;
}

IFabricSerializable * ResourceIdentifier::FabricSerializerActivator(FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr || typeInformation.length != sizeof(ResourceType::Enum))
    {
        return nullptr;
    }

    switch (*((ResourceType::Enum*)typeInformation.buffer))
    {
    case ResourceType::PathInImageStore:
        return new ResourceIdentifierPathInImageStore();

    case ResourceType::ApplicationType:
        return new ResourceIdentifierApplicationType();

    case ResourceType::Application:
        return new ResourceIdentifierApplication();

    case ResourceType::Service:
        return new ResourceIdentifierService();

    case ResourceType::Name:
        return new ResourceIdentifierName();

    default:
        return nullptr;
    }
}

wstring ResourceIdentifier::StripAclNamespace(wstring const & aclNamespace) const
{
    if (StringUtility::StartsWith(fabricUri_.Path, aclNamespace))
    {
        return fabricUri_.Path.substr(aclNamespace.size(), fabricUri_.Path.size() - aclNamespace.size());
    }

    Assert::TestAssert(
        "Invalid ResourceIdentifier URI {0}: must start with {1} if generated",
        fabricUri_,
        aclNamespace);

    return fabricUri_.Path;
}

ResourceIdentifierPathInImageStore::ResourceIdentifierPathInImageStore()
{
}

static GlobalWString PathInImageStoreAclNamespace = Common::make_global<std::wstring>(
    *AccessControlNamespace + L"PathInImageStore/");

ResourceIdentifierPathInImageStore::ResourceIdentifierPathInImageStore(wstring const & path)
{
    fabricUri_ = NamingUri(*PathInImageStoreAclNamespace + path);
}

_Use_decl_annotations_
void ResourceIdentifierPathInImageStore::ToPublic(ScopedHeap & heap, FABRIC_SECURITY_RESOURCE_IDENTIFIER & resource) const
{
    resource.Kind = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_PATH_IN_FABRIC_IMAGESTORE;
    auto refPtr = heap.AddItem<FABRIC_SECURITY_IMAGESTORE_PATH_RESOURCE_IDENTIFIER>();
    resource.Value = refPtr.GetRawPointer();
    refPtr->Path = heap.AddString(StripAclNamespace(*PathInImageStoreAclNamespace));
}

ResourceIdentifierApplicationType::ResourceIdentifierApplicationType()
{
}

static GlobalWString ApplicationTypeAclNamespace = Common::make_global<std::wstring>(
    *AccessControlNamespace + L"ApplicationType/");

ResourceIdentifierApplicationType::ResourceIdentifierApplicationType(wstring const & applicationType)
{
    fabricUri_ = NamingUri(*ApplicationTypeAclNamespace + applicationType);
}

_Use_decl_annotations_
void ResourceIdentifierApplicationType::ToPublic(ScopedHeap & heap, FABRIC_SECURITY_RESOURCE_IDENTIFIER & resource) const
{
    resource.Kind = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION_TYPE;
    auto refPtr = heap.AddItem<FABRIC_SECURITY_APPLICATION_TYPE_RESOURCE_IDENTIFIER>();
    resource.Value = refPtr.GetRawPointer();
    refPtr->ApplicationTypeName = heap.AddString(StripAclNamespace(*ApplicationTypeAclNamespace));
}

ResourceIdentifierApplication::ResourceIdentifierApplication()
{
}

static GlobalWString ApplicationAclNamespace = Common::make_global<std::wstring>(*AccessControlNamespace + L"Application/");

ResourceIdentifierApplication::ResourceIdentifierApplication(wstring const & application)
{
    fabricUri_ = NamingUri(*ApplicationAclNamespace + application);
}

_Use_decl_annotations_
void ResourceIdentifierApplication::ToPublic(ScopedHeap & heap, FABRIC_SECURITY_RESOURCE_IDENTIFIER & resource) const
{
    resource.Kind = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION;
    auto refPtr = heap.AddItem<FABRIC_SECURITY_APPLICATION_RESOURCE_IDENTIFIER>();
    resource.Value = refPtr.GetRawPointer();
    refPtr->ApplicationName = heap.AddString(StripAclNamespace(*ApplicationAclNamespace));
}

ResourceIdentifierService::ResourceIdentifierService()
{
}

static GlobalWString ServiceAclNamespace = Common::make_global<std::wstring>(*AccessControlNamespace + L"Service/");

ResourceIdentifierService::ResourceIdentifierService(wstring const & service)
{
    fabricUri_ = NamingUri(*ServiceAclNamespace + service);
}

_Use_decl_annotations_
void ResourceIdentifierService::ToPublic(ScopedHeap & heap, FABRIC_SECURITY_RESOURCE_IDENTIFIER & resource) const
{
    resource.Kind = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_SERVICE;
    auto refPtr = heap.AddItem<FABRIC_SECURITY_SERVICE_RESOURCE_IDENTIFIER>();
    resource.Value = refPtr.GetRawPointer();
    refPtr->ServiceName = heap.AddString(StripAclNamespace(*ServiceAclNamespace));
}

ResourceIdentifierName::ResourceIdentifierName()
{
}

ResourceIdentifierName::ResourceIdentifierName(wstring const & name)
{
    if (!NamingUri::TryParse(name, fabricUri_))
    {
        error_ = ErrorCodeValue::InvalidNameUri;
    }
}

_Use_decl_annotations_
void ResourceIdentifierName::ToPublic(ScopedHeap & heap, FABRIC_SECURITY_RESOURCE_IDENTIFIER & resource) const
{
    resource.Kind = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_NAME;
    auto refPtr = heap.AddItem<FABRIC_SECURITY_NAME_RESOURCE_IDENTIFIER>();
    resource.Value = refPtr.GetRawPointer();
    refPtr->Name = heap.AddString(fabricUri_.Name);
}
