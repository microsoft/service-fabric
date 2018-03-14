// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class ClaimsRetrievalMetadata : public Serialization::FabricSerializable, public Common::IFabricJsonSerializable
    {
        DENY_COPY(ClaimsRetrievalMetadata)

    public:

        static Common::Global<std::wstring> const InvalidMetadataValue;

        static Common::Global<std::wstring> const MetadataType_AAD;
        static Common::Global<std::wstring> const AAD_LoginUri;
        static Common::Global<std::wstring> const AAD_AuthorityUri;
        static Common::Global<std::wstring> const AAD_TenantId;
        static Common::Global<std::wstring> const AAD_ClusterApplication;
        static Common::Global<std::wstring> const AAD_ClientApplication;
        static Common::Global<std::wstring> const AAD_ClientRedirectUri;

    private:

        ClaimsRetrievalMetadata(std::wstring const & type);

    public:

        ClaimsRetrievalMetadata();

        ClaimsRetrievalMetadata(ClaimsRetrievalMetadata &&);

        virtual ~ClaimsRetrievalMetadata() { }

        static ClaimsRetrievalMetadata CreateForAAD();

        __declspec (property(get=get_IsValid)) bool IsValid;

        __declspec (property(get=get_IsAAD)) bool IsAAD;
        __declspec (property(get=get_AADLogin)) std::wstring const & AAD_Login;
        __declspec (property(get=get_AADAuthority)) std::wstring const & AAD_Authority;
        __declspec (property(get=get_AADTenant)) std::wstring const & AAD_Tenant;
        __declspec (property(get=get_AADCluster)) std::wstring const & AAD_Cluster;
        __declspec (property(get=get_AADClient)) std::wstring const & AAD_Client;
        __declspec (property(get=get_AADRedirect)) std::wstring const & AAD_Redirect;

        bool get_IsValid() const;
        bool get_IsAAD() const;
        std::wstring const & get_AADLogin() const;
        std::wstring const & get_AADAuthority() const;
        std::wstring const & get_AADTenant() const;
        std::wstring const & get_AADCluster() const;
        std::wstring const & get_AADClient() const;
        std::wstring const & get_AADRedirect() const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const&) const;

        void ToPublicApi(
            __in Common::ScopedHeap &,
            __out FABRIC_CLAIMS_RETRIEVAL_METADATA &) const;

        FABRIC_FIELDS_02(type_, metadata_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"type", type_)
            SERIALIZABLE_PROPERTY_SIMPLE_MAP(L"metadata", metadata_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:

        void SetMetadataValue(std::wstring const & key, std::wstring const & value);
        std::wstring const & GetMetadataValue(std::wstring const & key) const;

        std::wstring type_;
        std::map<std::wstring, std::wstring> metadata_;
    };

    typedef std::shared_ptr<ClaimsRetrievalMetadata> ClaimsRetrievalMetadataSPtr;
}
