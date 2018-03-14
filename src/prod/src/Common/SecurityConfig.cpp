// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

DEFINE_SINGLETON_COMPONENT_CONFIG(SecurityConfig)

static const StringLiteral TraceComponent("SecurityConfig");
static const StringLiteral TraceX509NameMap("X509NameMap");

SecurityConfig::IssuerStoreKeyValueMap SecurityConfig::IssuerStoreKeyValueMap::Parse(StringMap const & entries)
{
    IssuerStoreKeyValueMap result;

    for (auto const & entry : entries)
    {
        vector<wstring> storeNamesVector;
        StringUtility::Split<wstring>(entry.second, storeNamesVector, L",", true);
        set<wstring, X509IssuerStoreNameIsLess> storeNamesSet(storeNamesVector.begin(), storeNamesVector.end());
        result.Add(entry.first, storeNamesSet);
    }

    return result;
}

void SecurityConfig::IssuerStoreKeyValueMap::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    for (auto const & entry : *this)
    {
        bool isFirstStore = true;
        w.Write("('{0}', Store = ", entry.first);
        for (auto const & store : entry.second)
        {
            if (isFirstStore)
            {
                w.Write("{0}", store);
                isFirstStore = false;
            }
            else
            {
                w.Write(",{0}", store);
            }
        }
        w.Write(")");
    }
}

bool SecurityConfig::IssuerStoreKeyValueMap::operator == (IssuerStoreKeyValueMap const & rhs) const
{
    if (this == &rhs)
    {
        return true;
    }

    for (auto const & entry : *this)
    {
        auto iter = rhs.find(entry.first);
        if (iter == rhs.cend())
        {
            return false;
        }

        if (entry.second != iter->second)
        {
            return false;
        }
    }

    for (auto const & entry : rhs)
    {
        auto iter = this->find(entry.first);
        if (iter == this->cend())
        {
            return false;
        }

        if (entry.second != iter->second)
        {
            return false;
        }
    }

    return true;
}

bool SecurityConfig::IssuerStoreKeyValueMap::operator != (IssuerStoreKeyValueMap const & rhs) const
{
    return !(rhs == *this);
}

void SecurityConfig::IssuerStoreKeyValueMap::Add(std::wstring const & name, std::set<std::wstring, X509IssuerStoreNameIsLess> const & issuerStores)
{
    auto iter = find(name);
    if (iter == cend())
    {
        emplace(pair<std::wstring, std::set<wstring, X509IssuerStoreNameIsLess>>(name, issuerStores));
    }
    else
    {
        iter->second.insert(issuerStores.cbegin(), issuerStores.cend());
    }

}

void SecurityConfig::IssuerStoreKeyValueMap::Add(std::wstring const & name, std::wstring const & issuerStore)
{
    auto iter = find(name);
    if (iter == cend())
    {
        std::set<wstring, X509IssuerStoreNameIsLess> issuerStores;
        issuerStores.insert(issuerStore);
        emplace(pair<std::wstring, std::set<wstring, X509IssuerStoreNameIsLess>>(name, issuerStores));
    }
    else
    {
        iter->second.insert(issuerStore);
    }

    return;
}

void SecurityConfig::IssuerStoreKeyValueMap::AddIssuerEntries(PCFABRIC_X509_ISSUER_NAME x509Issuers, ULONG count)
{
    for (ULONG i = 0; i < count; ++i)
    {
        for (ULONG j = 0; j < x509Issuers[i].IssuerStoreCount; ++j)
        {
            if (x509Issuers[i].Name == NULL)
            {
                Add(L"", x509Issuers[i].IssuerStores[j]);
            }
            else
            {
                Add(x509Issuers[i].Name, x509Issuers[i].IssuerStores[j]);
            }
        }
    }
}

void SecurityConfig::IssuerStoreKeyValueMap::ToPublicApi(Common::ScopedHeap & heap, FABRIC_X509_CREDENTIALS_EX3 & x509Ex3) const
{
    auto remoteCertIssuerArray = heap.AddArray<FABRIC_X509_ISSUER_NAME>(size());
    x509Ex3.RemoteCertIssuerCount = (ULONG)size();
    int i =0, j = 0;
    for (auto const & entry : *this)
    {
        remoteCertIssuerArray[i].Name = heap.AddString(entry.first, true);
        auto remoteCertIssuerStoresArray = heap.AddArray<LPCWSTR>(entry.second.size());
        j = 0;
        for (auto const & issuerStore : entry.second)
        {
            remoteCertIssuerStoresArray[j++] = heap.AddString(issuerStore);
        }
        remoteCertIssuerArray[i].IssuerStoreCount = (ULONG)entry.second.size();
        remoteCertIssuerArray[i].IssuerStores = remoteCertIssuerStoresArray.GetRawArray();
        i++;
    }
    x509Ex3.RemoteCertIssuers = remoteCertIssuerArray.GetRawArray();
}

ErrorCode SecurityConfig::IssuerStoreKeyValueMap::GetPublicKeys(X509IdentitySet & publicKeys) const
{
    for (auto const & entry : *this)
    {
        std::shared_ptr<Common::X509FindValue> findValue;
        if (!entry.first.empty())
        {
            wstring commonName(entry.first);
            StringUtility::TrimWhitespaces(commonName);
            X509FindValue::Create(X509FindType::FindBySubjectName, commonName, findValue);
        }

        for (wstring storeName : entry.second)
        {
            StringUtility::TrimWhitespaces(storeName);
            CertContexts certs;

            ErrorCode error(ErrorCodeValue::Success);
            error = CryptoUtility::GetCertificate(
                X509StoreLocation::LocalMachine,
                storeName,
                findValue,
                certs);

            if (error.IsSuccess())
            {
                for (auto const & cert : certs)
                {
                    publicKeys.AddPublicKey(cert.get());
                }
            }
            else if (error.ReadValue() == ErrorCodeValue::CertificateNotFound)
            {
                Trace.WriteInfo(
                    TraceX509NameMap,
                    "GetPublicKeys for CN {0} was not found in store {1}. Error {2}. Continuing..",
                    entry.first,
                    storeName,
                    error);
                 continue;
            }
            else
            {
                 Trace.WriteError(
                    TraceX509NameMap,
                    "GetPublicKeys for CN {0} returned error {1}",
                    entry.first,
                    error);
                return error;
            }
        }
    }

    return ErrorCodeValue::Success;
}

SecurityConfig::X509NameMap::X509NameMap() : parseError_(ErrorCodeValue::Success)
{
}

SecurityConfig::X509NameMap SecurityConfig::X509NameMap::Parse(StringMap const & entries)
{
    X509NameMap result;

    for (auto const & entry : entries)
    {
        X509IdentitySet certIssuers;
        result.parseError_ = certIssuers.SetToThumbprints(entry.second).ReadValue();
        if (result.parseError_ != ErrorCodeValue::Success) return result;

        result.emplace(pair<wstring, X509IdentitySet>(entry.first, move(certIssuers)));
    }

    return result;
}

ErrorCode SecurityConfig::X509NameMap::AddNames(PCFABRIC_X509_NAME x509Names, ULONG count)
{
    for (ULONG i = 0; i < count; ++i)
    {
        if (x509Names[i].Name == nullptr)
            return parseError_ = ErrorCodeValue::InvalidX509NameList;

        wstring name(x509Names[i].Name);
        if (name.empty())
            return parseError_ = ErrorCodeValue::InvalidX509NameList;

        if (StringUtility::StartsWith<wstring>(name, L" ") || StringUtility::EndsWith<wstring>(name, L" "))
        {
            Trace.WriteWarning(TraceX509NameMap, "X509Name '{0}' has leading/trailing space, probably unintended", name); 
        }

        X509IdentitySet issuers;
        if (x509Names[i].IssuerCertThumbprint)
        {
            auto error = issuers.AddThumbprint(x509Names[i].IssuerCertThumbprint);
            if (!error.IsSuccess())
                return parseError_ = error.ReadValue();
        }

        auto iter = find(x509Names[i].Name);
        if (iter == cend())
        {
            emplace(pair<wstring, X509IdentitySet>(move(name), move(issuers)));
            continue;
        }

        // The same name appeared twice, once with an empty issuer list
        if (iter->second.IsEmpty() || issuers.IsEmpty())
            return parseError_ = ErrorCodeValue::InvalidX509NameList;

        iter->second.Add(issuers);
    }

    return parseError_;
}

void SecurityConfig::X509NameMap::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    for (auto const & entry : *this)
    {
        w.Write("('{0}',issuer={1})", entry.first, entry.second);
    }
}

ErrorCode SecurityConfig::X509NameMap::ParseError() const
{
    return parseError_;
}

bool SecurityConfig::X509NameMap::operator == (X509NameMap const & rhs) const
{
    if (this == &rhs) return true;

    for (auto const & entry : *this)
    {
        auto iter = rhs.find(entry.first);
        if (iter == rhs.cend()) return false;

        if (entry.second != iter->second) return false;
    }

    for (auto const & entry : rhs)
    {
        auto iter = this->find(entry.first);
        if (iter == this->cend()) return false;

        if (entry.second != iter->second) return false;
    }

    return true;
}

bool SecurityConfig::X509NameMap::operator != (X509NameMap const & rhs) const
{
    return !(rhs == *this);
}

void SecurityConfig::X509NameMap::Add(std::wstring const & name, X509IdentitySet const & certIssuers, bool overwrite)
{
    if (StringUtility::StartsWith<wstring>(name, L" ") || StringUtility::EndsWith<wstring>(name, L" "))
    {
        Trace.WriteWarning(TraceX509NameMap, "X509Name '{0}' has leading/trailing space, probably unintended", name); 
    }

    auto iter = find(name);
    if (iter == cend())
    {
        emplace(pair<wstring, X509IdentitySet>(name, certIssuers));
        return;
    }

    if (overwrite)
    {
        Trace.WriteInfo(TraceX509NameMap, "Overwriting existing X509Name '{0}' with new issuer {1}", name, certIssuers);
        iter->second = certIssuers;
        return;
    }

    if (iter->second.IsEmpty())
    {
        if (certIssuers.IsEmpty())
        {
            Trace.WriteInfo(TraceX509NameMap, "X509Name '{0}' already added, ignoring new addition", name);
            return;
        }

        Trace.WriteWarning(
            TraceX509NameMap,
            "X509Name '{0}' already added without issuer pinning, ignoring new addition with issuer restricted to {1}",
            name,
            certIssuers);

        return;
    }

    if (certIssuers.IsEmpty())
    {
        Trace.WriteWarning(
            TraceX509NameMap,
            "X509Name '{0}' already added with issuer pinned to {1}, new addition without issuer pinning will disable issuer pinning",
            name,
            iter->second);

        iter->second = certIssuers;
        return;
    }

    // merge issuer set
    iter->second.Add(certIssuers);
}

ErrorCode SecurityConfig::X509NameMap::Add(wstring const & name, wstring const & issuerCertThumbprints)
{
    X509IdentitySet issuers;
    auto error = issuers.SetToThumbprints(issuerCertThumbprints);
    if (!error.IsSuccess()) return error;

    Add(name, issuers);
    return error;
}

void SecurityConfig::X509NameMap::Add(std::wstring const & name, X509Identity::SPtr const & issuerId)
{
    X509IdentitySet issuers;
    issuers.Add(issuerId);
    Add(name, issuers);
}

void SecurityConfig::X509NameMap::Add(X509NameMap const & rhs)
{
    for (auto const & name : rhs)
    {
        Add(name.first, name.second);
    }
}

ErrorCode SecurityConfig::X509NameMap::AddNames(wstring const & names, wstring const & issuerCertThumbprints)
{
    vector<wstring> nameList;
    StringUtility::Split<wstring>(names, nameList, L",", true);
    ErrorCode error;
    for (auto const & name : nameList)
    {
        error = Add(name, issuerCertThumbprints);
        if (!error.IsSuccess()) return error;
    }

    return error;
}

bool SecurityConfig::X509NameMap::MatchIssuer(
    X509Identity::SPtr const & issuerId1,
    X509Identity::SPtr const & issuerId2,
    X509NameMapBase::const_iterator toMatch)
{
    return
        toMatch->second.IsEmpty() ||
        toMatch->second.Contains(issuerId1) ||
        toMatch->second.Contains(issuerId2);
}

bool SecurityConfig::X509NameMap::MatchIssuer(
    X509Identity::SPtr const & issuerId,
    X509NameMapBase::const_iterator toMatch)
{
    return
        toMatch->second.IsEmpty() ||
        toMatch->second.Contains(issuerId);
}


_Use_decl_annotations_ bool SecurityConfig::X509NameMap::Match(
    wstring const & name,
    X509Identity::SPtr const & issuerId,
    X509NameMapBase::const_iterator & matched) const
{
    matched = find(name);
    return (matched != cend()) && MatchIssuer(issuerId, matched);
}

_Use_decl_annotations_ bool SecurityConfig::X509NameMap::Match(
    wstring const & name,
    X509Identity::SPtr const & issuerId1,
    X509Identity::SPtr const & issuerId2,
    X509NameMap::const_iterator & matched) const
{
    matched = find(name);
    return (matched != cend()) && MatchIssuer(issuerId1, issuerId2, matched);
}

ErrorCode SecurityConfig::X509NameMap::OverwriteIssuers(X509IdentitySet const & certIssuers)
{
    for (auto & entry : *this)
    {
        Add(entry.first, certIssuers, true);
    }

    return ErrorCodeValue::Success;
}

X509IdentitySet SecurityConfig::X509NameMap::GetAllIssuers() const
{
    X509IdentitySet allIssuers;
    for (auto & entry : *this)
    {
        allIssuers.Add(entry.second);
    }

    return allIssuers;
}

ErrorCode SecurityConfig::X509NameMap::SetDefaultIssuers(X509IdentitySet const & certIssuers)
{
    for (auto & name : *this)
    {
        if (!name.second.IsEmpty())
        {
            // Default issuers cannot be used when some name already has issuers set
            return ErrorCodeValue::InvalidX509NameList;
        }

        name.second = certIssuers;
    }

    return ErrorCodeValue::Success;
}

vector<pair<wstring, wstring>> SecurityConfig::X509NameMap::ToVector() const
{
    vector<pair<wstring, wstring>> nameList;

    for (auto const & name : *this)
    {
        if (name.second.IsEmpty())
        {
            nameList.emplace_back(pair<wstring, wstring>(name.first, L""));
            continue;
        }

        vector<wstring> thumbprints = name.second.ToStrings();
        if (thumbprints.empty())
        {
            nameList.emplace_back(pair<wstring, wstring>(name.first, L""));
        }
        else
        {
            for (auto const & thumbprint : thumbprints)
            {
                nameList.emplace_back(pair<wstring, wstring>(name.first, thumbprint));
            }
        }
    }

    return nameList;
}

#define ADD_CONFIG_MAP( ConfigName, ConfigValue ) \
    configs[GetConfig().ConfigName##Entry.Key] = ConfigValue; \

#define ADD_CONFIG_MAP_ENTRY( ConfigName ) \
    ADD_CONFIG_MAP(ConfigName, GetConfig().ConfigName) \

map<wstring, wstring> SecurityConfig::GetDefaultAzureActiveDirectoryConfigurations()
{
    map<wstring, wstring> configs;

    ADD_CONFIG_MAP_ENTRY( AADTenantId )
    ADD_CONFIG_MAP_ENTRY( AADClusterApplication )
    ADD_CONFIG_MAP_ENTRY( AADClientApplication )
    ADD_CONFIG_MAP_ENTRY( AADClientRedirectUri )
    ADD_CONFIG_MAP_ENTRY( AADLoginEndpoint )
    ADD_CONFIG_MAP_ENTRY( AADTokenIssuerFormat )
    ADD_CONFIG_MAP_ENTRY( AADCertEndpointFormat )
    ADD_CONFIG_MAP_ENTRY( AADRoleClaimKey )
    ADD_CONFIG_MAP_ENTRY( AADAdminRoleClaimValue )
    ADD_CONFIG_MAP_ENTRY( AADUserRoleClaimValue )
    ADD_CONFIG_MAP( AADSigningCertRolloverCheckInterval, wformatString("{0}", GetConfig().AADSigningCertRolloverCheckInterval.TotalSeconds()) )

    for (auto const & config : configs)
    {
        Trace.WriteInfo(TraceComponent, "AAD config: {0}={1}", config.first, config.second);
    }

    return configs;
}
