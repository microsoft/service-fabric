// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace AccessControl
{
    class ResourceIdentifier
        : public Serialization::FabricSerializable
        , public Common::TextTraceComponent<Common::TraceTaskCodes::AccessControl>
    {
    public:
        typedef std::shared_ptr<ResourceIdentifier> SPtr;

        static Common::ErrorCode FromPublic(const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource, _Out_ SPtr & result);
        virtual void ToPublic(Common::ScopedHeap & heap, _Out_ FABRIC_SECURITY_RESOURCE_IDENTIFIER & resource) const = 0;

        Common::ErrorCode Status() const;

        Common::NamingUri const & FabricUri() const;

        virtual ResourceType::Enum Type() const;

        static Serialization::IFabricSerializable * FabricSerializerActivator(Serialization::FabricTypeInformation typeInformation);

        FABRIC_FIELDS_01(fabricUri_)

    protected:
        std::wstring StripAclNamespace(std::wstring const & aclNamespace) const;

        Common::ErrorCode error_;
        Common::NamingUri fabricUri_;

    private:
        const static ResourceType::Enum type_ = ResourceType::Invalid;
    };

#define DECLARE_RESOURCE_IDENTIFIER_TYPE(TypeEnum) \
public: \
    void ToPublic( \
        Common::ScopedHeap & heap, \
        _Out_ FABRIC_SECURITY_RESOURCE_IDENTIFIER & resource) const override; \
    ResourceType::Enum Type() const override; \
    NTSTATUS GetTypeInformation(_Out_ Serialization::FabricTypeInformation & typeInformation) const override; \
private: \
    const static ResourceType::Enum type_ = TypeEnum;

#define DEFINE_RESOURCE_IDENTIFIER_TYPE(TypeName) \
    ResourceType::Enum TypeName::Type() const { return type_; } \
    _Use_decl_annotations_ NTSTATUS TypeName::GetTypeInformation( \
        Serialization::FabricTypeInformation & typeInformation) const \
    { \
        typeInformation.buffer = reinterpret_cast<UCHAR const *>(&type_); \
        typeInformation.length = sizeof(type_); \
        return STATUS_SUCCESS; \
    }

    class ResourceIdentifierPathInImageStore : public ResourceIdentifier
    {
    public:
        ResourceIdentifierPathInImageStore();
        ResourceIdentifierPathInImageStore(std::wstring const & path);

        DECLARE_RESOURCE_IDENTIFIER_TYPE(ResourceType::PathInImageStore)
    };

    class ResourceIdentifierApplicationType : public ResourceIdentifier
    {
    public:
        ResourceIdentifierApplicationType();
        ResourceIdentifierApplicationType(std::wstring const & applicationType);

        DECLARE_RESOURCE_IDENTIFIER_TYPE(ResourceType::ApplicationType)
    };

    class ResourceIdentifierApplication : public ResourceIdentifier
    {
    public:
        ResourceIdentifierApplication();
        ResourceIdentifierApplication(std::wstring const & application);

        DECLARE_RESOURCE_IDENTIFIER_TYPE(ResourceType::Application)
    };

    class ResourceIdentifierService : public ResourceIdentifier
    {
    public:
        ResourceIdentifierService();
        ResourceIdentifierService(std::wstring const & service);

        DECLARE_RESOURCE_IDENTIFIER_TYPE(ResourceType::Service)
    };

    class ResourceIdentifierName : public ResourceIdentifier
    {
    public:
        ResourceIdentifierName();
        ResourceIdentifierName(std::wstring const & name);

        DECLARE_RESOURCE_IDENTIFIER_TYPE(ResourceType::Name)
    };
}
