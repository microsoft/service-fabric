// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Transport;

Global<wstring> const ClaimsRetrievalMetadata::InvalidMetadataValue = make_global<wstring>(L"");

Global<wstring> const ClaimsRetrievalMetadata::MetadataType_AAD = make_global<wstring>(L"aad");
Global<wstring> const ClaimsRetrievalMetadata::AAD_LoginUri = make_global<wstring>(L"login");
Global<wstring> const ClaimsRetrievalMetadata::AAD_AuthorityUri = make_global<wstring>(L"authority");
Global<wstring> const ClaimsRetrievalMetadata::AAD_TenantId = make_global<wstring>(L"tenant");
Global<wstring> const ClaimsRetrievalMetadata::AAD_ClusterApplication = make_global<wstring>(L"cluster");
Global<wstring> const ClaimsRetrievalMetadata::AAD_ClientApplication = make_global<wstring>(L"client");
Global<wstring> const ClaimsRetrievalMetadata::AAD_ClientRedirectUri = make_global<wstring>(L"redirect");

ClaimsRetrievalMetadata::ClaimsRetrievalMetadata()
    : type_()
    , metadata_()
{
}

ClaimsRetrievalMetadata::ClaimsRetrievalMetadata(wstring const & type)
    : type_(type)
    , metadata_()
{
}

ClaimsRetrievalMetadata::ClaimsRetrievalMetadata(ClaimsRetrievalMetadata && other)
    : type_(move(other.type_))
    , metadata_(move(other.metadata_))
{
}

ClaimsRetrievalMetadata ClaimsRetrievalMetadata::CreateForAAD()
{
    ClaimsRetrievalMetadata obj(*MetadataType_AAD);

    auto const & securityConfig = SecurityConfig::GetConfig();

    auto login = securityConfig.AADLoginEndpoint;
    auto authority = wformatString(securityConfig.AADTokenEndpointFormat, securityConfig.AADTenantId);
    auto tenant = securityConfig.AADTenantId;
    auto cluster = securityConfig.AADClusterApplication;
    auto client = securityConfig.AADClientApplication;
    auto redirect = securityConfig.AADClientRedirectUri;

    obj.SetMetadataValue(*AAD_LoginUri, login);
    obj.SetMetadataValue(*AAD_AuthorityUri, authority);
    obj.SetMetadataValue(*AAD_TenantId, tenant);
    obj.SetMetadataValue(*AAD_ClusterApplication, cluster);
    obj.SetMetadataValue(*AAD_ClientApplication, client);
    obj.SetMetadataValue(*AAD_ClientRedirectUri, redirect);

    return obj;
}

bool ClaimsRetrievalMetadata::get_IsValid() const
{
    return this->IsAAD;
}

bool ClaimsRetrievalMetadata::get_IsAAD() const
{
    return (type_ == *MetadataType_AAD);
}

wstring const & ClaimsRetrievalMetadata::get_AADLogin() const
{
    return GetMetadataValue(*AAD_LoginUri);
}

wstring const & ClaimsRetrievalMetadata::get_AADAuthority() const
{
    return GetMetadataValue(*AAD_AuthorityUri);
}

wstring const & ClaimsRetrievalMetadata::get_AADTenant() const
{
    return GetMetadataValue(*AAD_TenantId);
}

wstring const & ClaimsRetrievalMetadata::get_AADCluster() const
{
    return GetMetadataValue(*AAD_ClusterApplication);
}

wstring const & ClaimsRetrievalMetadata::get_AADClient() const
{
    return GetMetadataValue(*AAD_ClientApplication);
}

wstring const & ClaimsRetrievalMetadata::get_AADRedirect() const
{
    return GetMetadataValue(*AAD_ClientRedirectUri);
}

void ClaimsRetrievalMetadata::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w << L"type=" << type_;

    for (auto const & metadata : metadata_)
    {
        w << L",";
        w << metadata.first << L"=" << metadata.second;
    }
}

void ClaimsRetrievalMetadata::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_CLAIMS_RETRIEVAL_METADATA & metadata) const
{
    if (this->IsAAD)
    {
        metadata.Kind = FABRIC_CLAIMS_RETRIEVAL_METADATA_KIND_AAD;

        auto typed = heap.AddItem<FABRIC_AAD_CLAIMS_RETRIEVAL_METADATA>();

        typed->Authority = heap.AddString(this->AAD_Authority);
        typed->TenantId = heap.AddString(this->AAD_Tenant);
        typed->ClusterApplication = heap.AddString(this->AAD_Cluster);
        typed->ClientApplication = heap.AddString(this->AAD_Client);
        typed->ClientRedirect = heap.AddString(this->AAD_Redirect);

        auto ex1 = heap.AddItem<FABRIC_AAD_CLAIMS_RETRIEVAL_METADATA_EX1>();
        ex1->LoginEndpoint = heap.AddString(this->AAD_Login);

        typed->Reserved = ex1.GetRawPointer();

        metadata.Value = typed.GetRawPointer();
    }
    else
    {
        metadata.Kind = FABRIC_CLAIMS_RETRIEVAL_METADATA_KIND_NONE;
        metadata.Value = nullptr;
    }
}

void ClaimsRetrievalMetadata::SetMetadataValue(std::wstring const & key, std::wstring const & value)
{
    metadata_.insert(make_pair(key, value));
}

wstring const & ClaimsRetrievalMetadata::GetMetadataValue(wstring const & key) const
{
    auto it = metadata_.find(key);
    if (it != metadata_.end())
    {
        return it->second;
    }
    else
    {
        return *InvalidMetadataValue;
    }
}
