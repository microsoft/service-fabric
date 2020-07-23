// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TestCommon.h"

using namespace AccessControl;
using namespace Transport;
using namespace Common;
using namespace std;

namespace TransportUnitTest
{
    BOOST_AUTO_TEST_SUITE2(AccessControlTests)

    BOOST_AUTO_TEST_CASE(SecurityPrincipalX509Test)
    {
        ENTER;

        SecurityPrincipal::SPtr principalImpl;
        auto error = SecurityPrincipal::FromPublic(nullptr, principalImpl);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ArgumentNull));

        FABRIC_SECURITY_PRINCIPAL_IDENTIFIER invalidInput = {};
        invalidInput.Kind = FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_INVALID;
        error = SecurityPrincipal::FromPublic(&invalidInput, principalImpl);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

        FABRIC_SECURITY_X509_PRINCIPAL_IDENTIFIER x509Principal = {};
        x509Principal.CommonName = L"TestCommonName";

        FABRIC_SECURITY_PRINCIPAL_IDENTIFIER principalInput = {
            FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_X509,
            &x509Principal
        };

        error = SecurityPrincipal::FromPublic(&principalInput, principalImpl);
        Trace.WriteInfo(TraceType, "FromPublic={0}", principalImpl);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(principalImpl->Type() == SecurityPrincipalType::X509);
        VERIFY_IS_TRUE(principalImpl->CheckCommonName(x509Principal.CommonName));

        ScopedHeap heap;
        FABRIC_SECURITY_PRINCIPAL_IDENTIFIER principalOutput;
        principalImpl->ToPublic(heap, principalOutput);
        VERIFY_IS_TRUE(principalOutput.Kind == FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_X509);
        VERIFY_IS_TRUE(
            StringUtility::AreEqualCaseInsensitive(
                ((FABRIC_SECURITY_X509_PRINCIPAL_IDENTIFIER*)principalOutput.Value)->CommonName,
                x509Principal.CommonName));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SecurityPrincipalClaimTest)
    {
        ENTER;

        const wstring claimType = L"ClaimType";
        const wstring claimValue = L"ClaimValue";
        const wstring claim = claimType + L"=" + claimValue;
        FABRIC_SECURITY_CLAIM_PRINCIPAL_IDENTIFIER claimPrincipal = {};
        claimPrincipal.Claim = claim.c_str();

        FABRIC_SECURITY_PRINCIPAL_IDENTIFIER principalInput = {
            FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_CLAIM,
            &claimPrincipal
        };

        SecurityPrincipal::SPtr principalImpl;
        auto error = SecurityPrincipal::FromPublic(&principalInput, principalImpl);
        Trace.WriteInfo(TraceType, "FromPublic={0}", principalImpl);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(principalImpl->Type() == SecurityPrincipalType::Claim);
        SecuritySettings::RoleClaims claims;
        claims.AddClaim(claimType, claimValue);
        VERIFY_IS_TRUE(principalImpl->CheckClaim(claims));

        ScopedHeap heap;
        FABRIC_SECURITY_PRINCIPAL_IDENTIFIER principalOutput;
        principalImpl->ToPublic(heap, principalOutput);
        VERIFY_IS_TRUE(principalOutput.Kind == FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_CLAIM);
        VERIFY_IS_TRUE(
            StringUtility::AreEqualCaseInsensitive(
            ((FABRIC_SECURITY_CLAIM_PRINCIPAL_IDENTIFIER*)principalOutput.Value)->Claim,
            claimPrincipal.Claim));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SecurityPrincipalWindowsTest)
    {
        ENTER;

        wstring thisAccount = TransportSecurity().LocalWindowsIdentity();
        FABRIC_SECURITY_WINDOWS_PRINCIPAL_IDENTIFIER windowsPrincipal = {};
        windowsPrincipal.AccountName = thisAccount.c_str();

        FABRIC_SECURITY_PRINCIPAL_IDENTIFIER principalInput =
        {
            FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_WINDOWS,
            &windowsPrincipal
        };

        SecurityPrincipal::SPtr principalImpl;
        auto error = SecurityPrincipal::FromPublic(&principalInput, principalImpl);
        Trace.WriteInfo(TraceType, "FromPublic={0}", principalImpl);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(principalImpl->Type() == SecurityPrincipalType::Windows);

        //AccessTokenSPtr processToken;
        //error = AccessToken::CreateProcessToken(
        //    ::GetCurrentProcess(),
        //    TOKEN_ALL_ACCESS | TOKEN_READ | TOKEN_WRITE | TOKEN_EXECUTE,
        //    processToken);

        //VERIFY_IS_TRUE(error.IsSuccess());

        //AccessTokenSPtr duplicatedToken;
        //error = processToken->DuplicateToken(TOKEN_IMPERSONATE, duplicatedToken);
        //VERIFY_IS_TRUE(error.IsSuccess());

        //ImpersonationContextUPtr impersonationContext;
        //error = duplicatedToken->Impersonate(impersonationContext);
        //VERIFY_IS_TRUE(error.IsSuccess());

        //VERIFY_IS_TRUE(principalImpl->CheckTokenMembership(duplicatedToken->TokenHandle->Value));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AclTest)
    {
        ENTER;

        FABRIC_SECURITY_X509_PRINCIPAL_IDENTIFIER x509Principal = {};
        x509Principal.CommonName = L"TestCommonName";

        FABRIC_SECURITY_PRINCIPAL_IDENTIFIER principal1= {
            FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_X509,
            &x509Principal
        };

        const wstring claimType = L"ClaimType";
        const wstring claimValue = L"ClaimValue";
        const wstring claim = claimType + L"=" + claimValue;
        FABRIC_SECURITY_CLAIM_PRINCIPAL_IDENTIFIER claimPrincipal = {};
        claimPrincipal.Claim = claim.c_str();

        FABRIC_SECURITY_PRINCIPAL_IDENTIFIER principal2 = {
            FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_CLAIM,
            &claimPrincipal
        };

        FABRIC_SECURITY_ACE aceList[] =
        {
            {
                FABRIC_SECURITY_ACE_TYPE_ALLOWED,
                &principal1,
                0xbabe,
                nullptr,
            },
            {
                FABRIC_SECURITY_ACE_TYPE_ALLOWED,
                &principal2,
                0xf00d,
                nullptr,
            }
        };

        FABRIC_SECURITY_ACL acl = { 2, aceList, nullptr };

        FabricAcl aclImpl;
        auto error = FabricAcl::FromPublic(&acl, aclImpl);
        VERIFY_IS_TRUE(error.IsSuccess());

        vector<byte> serializedAcl;
        error = FabricSerializer::Serialize(&aclImpl, serializedAcl);
        VERIFY_IS_TRUE(error.IsSuccess());

        FabricAcl deserializedAcl;
        error = FabricSerializer::Deserialize(deserializedAcl, serializedAcl);
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(deserializedAcl.AccessCheckX509(x509Principal.CommonName, aceList[0].AccessMask));
        VERIFY_IS_FALSE(deserializedAcl.AccessCheckX509(x509Principal.CommonName, aceList[1].AccessMask));
        VERIFY_IS_TRUE(deserializedAcl.AccessCheckX509(x509Principal.CommonName, aceList[0].AccessMask & aceList[1].AccessMask));
        VERIFY_IS_FALSE(deserializedAcl.AccessCheckX509(x509Principal.CommonName, aceList[0].AccessMask | aceList[1].AccessMask));

        SecuritySettings::RoleClaims claims;
        claims.AddClaim(claimType, claimValue);
        VERIFY_IS_TRUE(deserializedAcl.AccessCheckClaim(claims, aceList[1].AccessMask));
        VERIFY_IS_FALSE(deserializedAcl.AccessCheckClaim(claims, aceList[0].AccessMask));
        VERIFY_IS_TRUE(deserializedAcl.AccessCheckClaim(claims, aceList[0].AccessMask & aceList[1].AccessMask));
        VERIFY_IS_FALSE(deserializedAcl.AccessCheckClaim(claims, aceList[0].AccessMask | aceList[1].AccessMask));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ResourceIdentifierTest)
    {
        ENTER;

        {
            ResourceIdentifier::SPtr impl;
            auto error = ResourceIdentifier::FromPublic(nullptr, impl);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ArgumentNull));

            FABRIC_SECURITY_RESOURCE_IDENTIFIER ri = {};
            ri.Kind = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_INVALID;
            error = ResourceIdentifier::FromPublic(&ri, impl);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));
        }

        {
            FABRIC_SECURITY_IMAGESTORE_PATH_RESOURCE_IDENTIFIER path = {};
            path.Path = L"Test/xyz/abc";
            FABRIC_SECURITY_RESOURCE_IDENTIFIER ri =
            {
                FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_PATH_IN_FABRIC_IMAGESTORE,
                &path
            };

            ResourceIdentifier::SPtr impl;
            auto error = ResourceIdentifier::FromPublic(&ri, impl);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(StringUtility::EndsWith<wstring>(impl->FabricUri().Name, path.Path));

            ScopedHeap heap;
            FABRIC_SECURITY_RESOURCE_IDENTIFIER output;
            impl->ToPublic(heap, output);
            VERIFY_IS_TRUE(output.Kind == FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_PATH_IN_FABRIC_IMAGESTORE);
            VERIFY_IS_TRUE(StringUtility::Compare(((FABRIC_SECURITY_IMAGESTORE_PATH_RESOURCE_IDENTIFIER*)output.Value)->Path, path.Path) == 0);
        }

        {
            FABRIC_SECURITY_APPLICATION_TYPE_RESOURCE_IDENTIFIER applicationType = {};
            applicationType.ApplicationTypeName = L"TestApplicationType";
            FABRIC_SECURITY_RESOURCE_IDENTIFIER ri =
            {
                FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION_TYPE,
                &applicationType
            };

            ResourceIdentifier::SPtr impl;
            auto error = ResourceIdentifier::FromPublic(&ri, impl);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(StringUtility::EndsWith<wstring>(impl->FabricUri().Name, applicationType.ApplicationTypeName));

            ScopedHeap heap;
            FABRIC_SECURITY_RESOURCE_IDENTIFIER output;
            impl->ToPublic(heap, output);
            VERIFY_IS_TRUE(output.Kind == FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION_TYPE);
            VERIFY_IS_TRUE(StringUtility::Compare(((FABRIC_SECURITY_APPLICATION_TYPE_RESOURCE_IDENTIFIER*)output.Value)->ApplicationTypeName, applicationType.ApplicationTypeName) == 0);
        }

        {
            FABRIC_SECURITY_APPLICATION_RESOURCE_IDENTIFIER application = {};
            application.ApplicationName = L"TestApplication";
            FABRIC_SECURITY_RESOURCE_IDENTIFIER ri =
            {
                FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION,
                &application
            };

            ResourceIdentifier::SPtr impl;
            auto error = ResourceIdentifier::FromPublic(&ri, impl);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(StringUtility::EndsWith<wstring>(impl->FabricUri().Name, application.ApplicationName));

            ScopedHeap heap;
            FABRIC_SECURITY_RESOURCE_IDENTIFIER output;
            impl->ToPublic(heap, output);
            VERIFY_IS_TRUE(output.Kind == FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION);
            VERIFY_IS_TRUE(StringUtility::Compare(((FABRIC_SECURITY_APPLICATION_RESOURCE_IDENTIFIER*)output.Value)->ApplicationName, application.ApplicationName) == 0);
        }

        {
            FABRIC_SECURITY_SERVICE_RESOURCE_IDENTIFIER service = {};
            service.ServiceName = L"TestService";
            FABRIC_SECURITY_RESOURCE_IDENTIFIER ri =
            {
                FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_SERVICE,
                &service
            };

            ResourceIdentifier::SPtr impl;
            auto error = ResourceIdentifier::FromPublic(&ri, impl);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(StringUtility::EndsWith<wstring>(impl->FabricUri().Name, service.ServiceName));

            ScopedHeap heap;
            FABRIC_SECURITY_RESOURCE_IDENTIFIER output;
            impl->ToPublic(heap, output);
            VERIFY_IS_TRUE(output.Kind == FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_SERVICE);
            VERIFY_IS_TRUE(StringUtility::Compare(((FABRIC_SECURITY_SERVICE_RESOURCE_IDENTIFIER*)output.Value)->ServiceName, service.ServiceName) == 0);
        }

        {
            FABRIC_SECURITY_NAME_RESOURCE_IDENTIFIER name = {};
            name.Name = L"fabric:/abc/cde/xyz";
            FABRIC_SECURITY_RESOURCE_IDENTIFIER ri =
            {
                FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_NAME,
                &name
            };

            ResourceIdentifier::SPtr impl;
            auto error = ResourceIdentifier::FromPublic(&ri, impl);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(impl->FabricUri().Name == name.Name);

            ScopedHeap heap;
            FABRIC_SECURITY_RESOURCE_IDENTIFIER output;
            impl->ToPublic(heap, output);
            VERIFY_IS_TRUE(output.Kind == FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_NAME);
            VERIFY_IS_TRUE(StringUtility::Compare(((FABRIC_SECURITY_NAME_RESOURCE_IDENTIFIER*)output.Value)->Name, name.Name) == 0);
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()
}
