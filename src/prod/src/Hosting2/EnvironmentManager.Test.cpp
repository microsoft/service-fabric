// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "FabricRuntime.h"
#include "Common/Common.h"
#include "FabricNode/fabricnode.h"
#include "Hosting2/FabricNodeHost.Test.h"


using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Transport;
using namespace Fabric;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;
using namespace Management::FileStoreService;

const StringLiteral TraceType("EnvironmentManagerTest");

GlobalWString DefaultRunAsUserName = make_global<wstring>(L"DefaultRunAsUser");
GlobalWString BuiltinDomainName = make_global<wstring>(L"BUILTIN\\");
GlobalWString LocalDomainName = make_global<wstring>(Environment::GetMachineName());

const ULONG AppPackageCounter = 0x17;

class EnvironmentManagerTest
{
    // This test should be run with elevated Admin access
    // Setup checks that the process runs elevated; otherwise, it fails;
protected:
    EnvironmentManagerTest() : fabricNodeHost_(make_shared<TestFabricNodeHost>()){ BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup);
    ~EnvironmentManagerTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);

    EnvironmentManager & GetEnvironmentManager();
    class DummyRoot : public Common::ComponentRoot
    {
    public:
        ~DummyRoot() {}
    };

#if !defined(PLATFORM_UNIX)
    typedef map<wstring, vector<wstring>> MembershipCollection;

    static void CheckPrincipals(
        wstring const & applicationId,
        wstring const & nodeId,
        SecurityPrincipalsProviderUPtr const & context,
        PrincipalsDescription const & principals,
        vector<wstring> & actualGroupNames,
        vector<wstring> & actualUserNames);

    static void CheckGroupInfo(
        SecurityGroupSPtr const & group,
        SecurityGroupDescription const & groupDesc,
        vector<wstring> const& actualUserNames);

    static void CheckUserInfo(
        SecurityUserSPtr const & user,
        SecurityUserDescription const & userDesc,
        bool addWindowsFabricAllowedUsersGroup);

    static ErrorCode GetUserProfile(
        std::wstring const& accountName,
        TokenHandleSPtr const& tokenHandle,
        __out wstring & directoryProfile);

    static void CheckSid(wstring const & accountName, __in PSID sid);

    static void CheckStringCollection(__in vector<wstring> & v1, __in vector<wstring> & v2);

    static void CheckPrincipalsAreDeleted(
        vector<wstring> const& actualGroupNames,
        vector<wstring> const& actualUserNames);

    static void VerifyPrincipalInformation(
        vector<SecurityPrincipalInformationSPtr> const & principalInformation,
        vector<wstring> const & expectedUsersAndGroups);
#endif
    static void CheckPathsExist(
        std::map <std::wstring, std::wstring> pfxPaths,
        std::map <std::wstring, std::wstring> passwordPaths);

    shared_ptr<TestFabricNodeHost> fabricNodeHost_;
};

BOOST_FIXTURE_TEST_SUITE(EnvironmentManagerTestSuite,EnvironmentManagerTest)

#if !defined(PLATFORM_UNIX)
BOOST_AUTO_TEST_CASE(ApplicationPrincipalsForFSSTest)
{
    Trace.WriteInfo(TraceType, "Start ApplicationPrincipalsForFSSTest");

    wstring nodeId(L"ApplicationPrincipalsForFSSTest-NodeId");
    wstring appId(L"ApplicationPrincipalsForFSSTest-AppId");

    ApplicationPrincipals::CleanupEnvironment(nodeId, appId);

    // Install 2 certificates, but uninstall 1 to trigger create user error
    wstring cn1 = wformatString("CN{0}", Guid::NewGuid());
    InstallTestCertInScope cert1(L"CN=" + cn1);
    wstring t1 = wformatString(cert1.Thumbprint());

    wstring cn2 = wformatString("CN{0}", Guid::NewGuid());
    InstallTestCertInScope cert2(L"CN=" + cn2);
    wstring t2 = wformatString(cert2.Thumbprint());
    cert2.Uninstall(false /*deleteKeyContainer*/);

    HostingConfig::GetConfig().Test_Reset();
    HostingConfig::GetConfig().FileStoreServiceUserCreationRetryTimeout = TimeSpan::FromSeconds(1);

    FileStoreServiceConfig::GetConfig().Test_Reset();

    // Change FSS config to enable primary account
    FileStoreServiceConfig::GetConfig().PrimaryAccountType = wformatString(SecurityPrincipalAccountType::Enum::LocalUser);
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreLocation = wformatString(cert1.StoreLocation());
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreName = cert1.StoreName();
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509Thumbprint = t1;

    FileStoreServiceConfig::GetConfig().SecondaryAccountType = wformatString(SecurityPrincipalAccountType::Enum::LocalUser);
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreLocation = wformatString(cert2.StoreLocation());
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreName = cert2.StoreName();
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509Thumbprint = t2;

    PrincipalsDescriptionUPtr principalsDesc;
    auto error = Utility::GetPrincipalsDescriptionFromConfigByThumbprint(principalsDesc);
    VERIFY_IS_TRUE(error.IsSuccess());

    ConfigureSecurityPrincipalRequest req1(nodeId, appId, AppPackageCounter, *principalsDesc, 1, true, false);
    auto appPrincipals = make_shared<ApplicationPrincipals>(move(req1));
    VERIFY_ARE_EQUAL(appPrincipals->ApplicationId, appId);

    // Verify states:
    // Inactive: 2
    // Opening: 4
    // Opened: 8
    // RetryScheduling: 16
    // RetryScheduled: 32
    // Updating: 64
    // UpdatePrincipal: 128
    // GetPrincipal: 256
    // Closing: 512
    // Closed: 1024
    // Failed: 2048

    Trace.WriteInfo(TraceType, "Create app principals, state {0}", appPrincipals->GetState());
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 2u /*Inactive*/);

    error = appPrincipals->Open();
    VERIFY_IS_TRUE(error.IsSuccess());
    // Principals are retrying, because the second user can't be created.
    // The state cycles through RetryScheduling, RetryScheduled and UpatePrincipal.
    VERIFY_ARE_NOT_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    wstring primaryUserName = Management::FileStoreService::Constants::PrimaryFileStoreServiceUser;
    wstring secondaryUserName = Management::FileStoreService::Constants::SecondaryFileStoreServiceUser;
    wstring groupName = *FileStoreService::Constants::FileStoreServiceUserGroup;

    vector<wstring> expectedUsersAndGroups;
    expectedUsersAndGroups.push_back(primaryUserName);
    expectedUsersAndGroups.push_back(groupName);

    vector<SecurityPrincipalInformationSPtr> principalInformation;
    // GetPrincipalInformation can be called in RetryScheduled state.
    error = appPrincipals->GetSecurityPrincipalInformation(principalInformation);
    VERIFY_IS_TRUE(error.IsSuccess());
    VerifyPrincipalInformation(principalInformation, expectedUsersAndGroups);
    VERIFY_ARE_NOT_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    // GetSecurityUser can be called in RetryScheduled state.
    SecurityUserSPtr secUser;
    error = appPrincipals->GetSecurityUser(primaryUserName, secUser);
    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_ARE_EQUAL(primaryUserName, secUser->Name);
    VERIFY_ARE_NOT_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    error = appPrincipals->GetSecurityUser(secondaryUserName, secUser);
    VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ApplicationPrincipalDoesNotExist));
    VERIFY_ARE_NOT_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    SecurityGroupSPtr secGroup;
    error = appPrincipals->GetSecurityGroup(groupName, secGroup);
    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_ARE_EQUAL(groupName, secGroup->Name);
    VERIFY_ARE_NOT_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    // Re-open fails
    error = appPrincipals->Open();
    VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidState));
    VERIFY_IS_FALSE(error.IsError(ErrorCodeValue::ApplicationPrincipalAbortableError));

    // Update principals fails because of invalid state
    ConfigureSecurityPrincipalRequest req2(nodeId, appId, AppPackageCounter, *principalsDesc, 1, true, true);
    error = appPrincipals->UpdateApplicationPrincipals(move(req2));
    VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidState));
    VERIFY_IS_FALSE(error.IsError(ErrorCodeValue::ApplicationPrincipalAbortableError));

    // sleep a while to make sure that the user creation has been retried for at least once
    // 112 = RetryScheduling | RetryScheduled | Updating
    Sleep(3000);
    VERIFY_ARE_NOT_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    // Re-install the certificate
    // On retry, create users will succeed, and there will be 2 users in principals
    error = cert2.Install();
    VERIFY_IS_TRUE(error.IsSuccess());

    int maxRetry = 30;
    SecurityUserSPtr secUser2;
    for (int i = 0; i < maxRetry; ++i)
    {
        error = appPrincipals->GetSecurityUser(secondaryUserName, secUser2);
        if (error.IsSuccess())
        {
            break;
        }

        Trace.WriteInfo(TraceType, "Error getting secondaryUserName {0}: {1}, retry {2}", secondaryUserName, error, i);
        Sleep(1000);
    }

    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_ARE_EQUAL(secondaryUserName, secUser2->Name);

    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    vector<wstring> actualGroupNames;
    actualGroupNames.push_back(secGroup->AccountName);
    vector<wstring> actualUserNames;
    actualUserNames.push_back(secUser->AccountName);
    actualUserNames.push_back(secUser2->AccountName);

    error = appPrincipals->Close();
    VERIFY_IS_TRUE(error.IsSuccess());

    CheckPrincipalsAreDeleted(actualGroupNames, actualUserNames);

    FileStoreServiceConfig::GetConfig().Test_Reset();
    HostingConfig::GetConfig().Test_Reset();
}

BOOST_AUTO_TEST_CASE(ApplicationPrincipalsOpenSuccessfulStateMachineTest)
{
    Trace.WriteInfo(TraceType, "Start ApplicationPrincipalsOpenSuccessfulStateMachineTest");

    wstring nodeId(L"ApplicationPrincipalsStateMachineTest-NodeId");
    wstring appId(L"ApplicationPrincipalsStateMachineTest-AppId");

    ApplicationPrincipals::CleanupEnvironment(nodeId, appId);

    wstring cn1 = wformatString("CN1-{0}", Guid::NewGuid());
    InstallTestCertInScope cert1(L"CN=" + cn1);
    wstring t1 = wformatString(cert1.Thumbprint());
    DateTime expirationTime;
    VERIFY_IS_TRUE(CryptoUtility::GetCertificateExpiration(cert1.GetCertContextUPtr(), expirationTime).IsSuccess());
    wstring userName = AccountHelper::GenerateUserAcountName(cert1.Thumbprint(), expirationTime);
    cert1.Uninstall(false);

    Trace.WriteInfo(TraceType, "Generate username {0} from thumbprint {1}", userName, t1);

    wstring groupName = L"MySpecialGroup";
    wstring groupName2 = L"MySpecialGroup2";
    wstring userName2 = L"MySpecialUser2";

    // Create security principals with 1 group and 2 users
    PrincipalsDescription principals;
    SecurityGroupDescription group1;
    group1.Name = groupName;
    group1.SystemGroupMembers.push_back(L"Users");
    principals.Groups.push_back(group1);

    SecurityUserDescription user1;
    user1.Name = userName;
    user1.ParentApplicationGroups.push_back(groupName);
    user1.ParentSystemGroups.push_back(L"Administrators");
    principals.Users.push_back(user1);

    SecurityUserDescription user2;
    user2.Name = DefaultRunAsUserName;
    user2.ParentApplicationGroups.push_back(groupName);
    principals.Users.push_back(user2);

    ConfigureSecurityPrincipalRequest req1(nodeId, appId, AppPackageCounter, principals, 0, true, false);
    auto appPrincipals = make_shared<ApplicationPrincipals>(move(req1));
    VERIFY_ARE_EQUAL(appPrincipals->ApplicationId, appId);

    Trace.WriteInfo(TraceType, "Create app principals, state {0}", appPrincipals->GetState());
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 2u /*Inactive*/);

    auto error = appPrincipals->Open();
    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_IS_FALSE(error.IsError(ErrorCodeValue::ApplicationPrincipalAbortableError));
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    vector<wstring> expectedUsersAndGroups;
    expectedUsersAndGroups.push_back(userName);
    expectedUsersAndGroups.push_back(DefaultRunAsUserName);
    expectedUsersAndGroups.push_back(groupName);

    vector<SecurityPrincipalInformationSPtr> principalInformation;
    // GetPrincipalInformation can be called in Opened state.
    error = appPrincipals->GetSecurityPrincipalInformation(principalInformation);
    VERIFY_IS_TRUE(error.IsSuccess());
    VerifyPrincipalInformation(principalInformation, expectedUsersAndGroups);
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    SecurityUserSPtr secUser;
    error = appPrincipals->GetSecurityUser(userName, secUser);
    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_ARE_EQUAL(userName, secUser->Name);
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    SecurityUserSPtr secUser2;
    error = appPrincipals->GetSecurityUser(DefaultRunAsUserName, secUser2);
    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_ARE_EQUAL(DefaultRunAsUserName, secUser2->Name);
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    error = appPrincipals->GetSecurityUser(userName2, secUser);
    VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ApplicationPrincipalDoesNotExist));
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    SecurityGroupSPtr secGroup;
    error = appPrincipals->GetSecurityGroup(groupName, secGroup);
    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_ARE_EQUAL(groupName, secGroup->Name);
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    error = appPrincipals->GetSecurityGroup(groupName2, secGroup);
    VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ApplicationPrincipalDoesNotExist));
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    // Re-open fails since it's already opened
    error = appPrincipals->Open();
    VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::Success));
    VERIFY_IS_FALSE(error.IsError(ErrorCodeValue::ApplicationPrincipalAbortableError));

    //
    // Update the principals with different group, fails
    //
    PrincipalsDescription principals2;
    SecurityGroupDescription group2;
    group2.Name = groupName2;
    principals2.Groups.push_back(group2);
    ConfigureSecurityPrincipalRequest req2(nodeId, appId, AppPackageCounter, principals2, 0, true, true);
    error = appPrincipals->UpdateApplicationPrincipals(move(req2));
    VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));
    VERIFY_IS_FALSE(error.IsError(ErrorCodeValue::ApplicationPrincipalAbortableError));
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    //
    // Update the principals with new users, append new users
    //
    PrincipalsDescription principals3;
    principals3.Groups.push_back(group1);
    SecurityUserDescription user3;
    user3.Name = userName2;
    user3.ParentApplicationGroups.push_back(groupName);
    user3.ParentSystemGroups.push_back(L"Administrators");
    principals3.Users.push_back(user3);

    ConfigureSecurityPrincipalRequest req3(nodeId, appId, AppPackageCounter, principals3, 0, true, true);
    error = appPrincipals->UpdateApplicationPrincipals(move(req3));
    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_IS_FALSE(error.IsError(ErrorCodeValue::ApplicationPrincipalAbortableError));
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    vector<wstring> expectedUsersAndGroups2;
    expectedUsersAndGroups2.push_back(userName);
    expectedUsersAndGroups2.push_back(DefaultRunAsUserName);
    expectedUsersAndGroups2.push_back(userName2);
    expectedUsersAndGroups2.push_back(groupName);

    principalInformation.clear();
    error = appPrincipals->GetSecurityPrincipalInformation(principalInformation);
    VERIFY_IS_TRUE(error.IsSuccess());
    VerifyPrincipalInformation(principalInformation, expectedUsersAndGroups2);
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    SecurityUserSPtr secUser3;
    error = appPrincipals->GetSecurityUser(userName2, secUser3);
    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_ARE_EQUAL(userName2, secUser3->Name);

    vector<wstring> actualGroupNames;
    actualGroupNames.push_back(secGroup->AccountName);
    vector<wstring> actualUserNames;
    actualUserNames.push_back(secUser->AccountName);
    actualUserNames.push_back(secUser2->AccountName);
    actualUserNames.push_back(secUser3->AccountName);

    //
    // Reject update principals because request has UpdateExisting false
    //
    ConfigureSecurityPrincipalRequest req4(nodeId, appId, AppPackageCounter, principals3, 0, true, false);
    error = appPrincipals->UpdateApplicationPrincipals(move(req4));
    VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidState));

    //
    // Update with the same users, request succeeds and there are no changes
    //
    ConfigureSecurityPrincipalRequest req5(nodeId, appId, AppPackageCounter, principals3, 0, true, true);
    error = appPrincipals->UpdateApplicationPrincipals(move(req5));
    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_IS_FALSE(error.IsError(ErrorCodeValue::ApplicationPrincipalAbortableError));
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    principalInformation.clear();
    error = appPrincipals->GetSecurityPrincipalInformation(principalInformation);
    VERIFY_IS_TRUE(error.IsSuccess());
    VerifyPrincipalInformation(principalInformation, expectedUsersAndGroups2);
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    //
    // Update with group with 0 users
    //
    PrincipalsDescription principals4;
    principals4.Groups.push_back(group1);
    ConfigureSecurityPrincipalRequest req6(nodeId, appId, AppPackageCounter, principals4, 0, true, true);
    error = appPrincipals->UpdateApplicationPrincipals(move(req6));
    VERIFY_IS_TRUE(error.IsSuccess());
    VERIFY_IS_FALSE(error.IsError(ErrorCodeValue::ApplicationPrincipalAbortableError));
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    principalInformation.clear();
    error = appPrincipals->GetSecurityPrincipalInformation(principalInformation);
    VERIFY_IS_TRUE(error.IsSuccess());
    VerifyPrincipalInformation(principalInformation, expectedUsersAndGroups2);
    VERIFY_ARE_EQUAL(appPrincipals->GetState(), 8u /*Opened*/);

    error = appPrincipals->Close();
    VERIFY_IS_TRUE(error.IsSuccess());

    CheckPrincipalsAreDeleted(actualGroupNames, actualUserNames);
}

BOOST_AUTO_TEST_CASE(SecurityUserTestNoLoadProfile)
{
    wstring nodeId(L"SecurityUserTestNoLoadProfile-NodeId");
    wstring appId(L"SecurityUserTestNoLoadProfile-AppId");

    SecurityUserDescription userDesc;
    userDesc.Name = L"User";


    // Delete any old existing accounts
    ApplicationPrincipals::CleanupEnvironment(nodeId, appId);

    wstring comment = *(ApplicationPrincipals::WinFabAplicationGroupCommentPrefix) + L" " + userDesc.Name + L" " + nodeId + L" " + appId;

    SecurityUserSPtr secuser = SecurityUser::CreateSecurityUser(appId, userDesc);
    auto user0 = shared_ptr<ISecurityPrincipal>(dynamic_pointer_cast<ISecurityPrincipal>(secuser));

    auto error = user0->CreateAccountwithRandomName(comment);
    Trace.WriteInfo(TraceType, "Create account {0}: error={1}", user0->AccountName, error);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Create UserAccount %s: %d", user0->AccountName.c_str(), error.ReadValue());

    Common::AccessTokenSPtr userToken;
    error = secuser->CreateLogonToken(userToken);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "CreateLogonToken for %s: %d", user0->AccountName.c_str(), error.ReadValue());
    //Verify profile dir does not exist
    wstring profileDir;
    error = GetUserProfile(user0->AccountName, userToken->TokenHandle, profileDir);
    VERIFY_IS_TRUE_FMT(!error.IsSuccess(), "GetUserProfileDirectory for %s: %d", user0->AccountName.c_str(), error.ReadValue());
    // Delete account should work fine, deleting the user account and not the profile
    error = user0->DeleteAccount();
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Delete UserAccount %s: %d", user0->AccountName.c_str(), error.ReadValue());
}

BOOST_AUTO_TEST_CASE(SecurityUserTestLoadProfile)
{
    wstring nodeId(L"SecurityUserTestLoadProfile-NodeId");
    wstring appId(L"SecurityUserTestLoadProfile-AppId");

    SecurityUserDescription userDesc;
    userDesc.Name = L"User";
    userDesc.LoadProfile = true;

    // Delete any old existing accounts
    ApplicationPrincipals::CleanupEnvironment(nodeId, appId);

    wstring commentUser1 = *(ApplicationPrincipals::WinFabAplicationGroupCommentPrefix) + L" " + userDesc.Name + L" " + nodeId + L" " + appId;

    // Create user, logon and load profile and delete
    SecurityUserSPtr secuser = SecurityUser::CreateSecurityUser(appId, userDesc);
    auto user1 = shared_ptr<ISecurityPrincipal>(dynamic_pointer_cast<ISecurityPrincipal>(secuser));
    auto error = user1->CreateAccountwithRandomName(commentUser1);
    Trace.WriteInfo(TraceType, "Create account {0}: error={1}", user1->AccountName, error);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Create UserAccount %s: %d", user1->AccountName.c_str(), error.ReadValue());

    Common::AccessTokenSPtr userToken;
    error = secuser->CreateLogonToken(userToken);

    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Logon UserAccount %s: %d", user1->AccountName.c_str(), error.ReadValue());

    // Verify that profile directory exists
    wstring profileDir;
    error = GetUserProfile(user1->AccountName, userToken->TokenHandle, profileDir);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "GetUserProfileDirectory for %s: %d", user1->AccountName.c_str(), error.ReadValue());
    if (!Directory::Exists(profileDir))
    {
        VERIFY_FAIL_FMT("Profile directory for %s: \"%s\" doesn't exist", user1->AccountName.c_str(), profileDir.c_str());
    }

    userToken.reset();

    error = user1->DeleteAccount();
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Delete UserAccount %s: %d", user1->AccountName.c_str(), error.ReadValue());
    // Verify that profile dir was deleted
    if (Directory::Exists(profileDir))
    {
        // There are cases where the profile files are still in the process of being deleted or in use,
        // even if the delete profile is successful.
        // Since the directory is left behind, try to manually delete it -
        // if this succeeds, that means all handles that the user created were correctly closed,
        // so consider success
        int retryCount = 5;
        for (int i = 0; i < retryCount; ++i)
        {
            error = Directory::Delete(profileDir, true);
            if (error.IsSuccess())
            {
                Trace.WriteInfo(TraceType, "Profile directory manually delete {0}: error={1}", user1->AccountName, error);
                return;
            }

            Trace.WriteInfo(TraceType, "Profile directory manually delete {0}: error={1}. Retry {2}", user1->AccountName, error, retryCount);
        }

        VERIFY_FAIL_FMT("Profile directory for %s: \"%s\" still exists", user1->AccountName.c_str(), profileDir.c_str());
    }
}

BOOST_AUTO_TEST_CASE(PrincipalsProviderTestWithSingleNode)
{
    wstring nodeId(L"PrincipalsProviderTestWithSingleNode-NodeId");
    wstring appId(L"PrincipalsProviderTestWithSingleNode-AppId");

    ApplicationPrincipals::CleanupEnvironment(nodeId, appId);

    wstring groupName = L"MySpecialGroup";
    wstring userName = L"MySpecialUser";

    PrincipalsDescription principals;
    SecurityGroupDescription group1;
    group1.Name = groupName;
    group1.SystemGroupMembers.push_back(L"Users");
    principals.Groups.push_back(group1);

    SecurityUserDescription user1;
    user1.Name = userName;
    user1.ParentApplicationGroups.push_back(groupName);
    user1.ParentSystemGroups.push_back(L"Administrators");
    principals.Users.push_back(user1);

    SecurityUserDescription user2;
    user2.Name = DefaultRunAsUserName;
    user2.ParentApplicationGroups.push_back(groupName);
    principals.Users.push_back(user2);

    // Create the principals provider
    DummyRoot root;
    SecurityPrincipalsProviderUPtr provider = make_unique<SecurityPrincipalsProvider>(root);
    auto error = provider->Open();

    VERIFY_IS_TRUE(error.IsSuccess(), L"Principal provider opened successfully");

    vector<SecurityPrincipalInformationSPtr> context;
    error = provider->Setup(ConfigureSecurityPrincipalRequest(nodeId, appId, AppPackageCounter, principals, 0, false, false), /*out*/context);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Principals Setup returned %d", error.ReadValue());

    vector<wstring> actualGroupNames1;
    vector<wstring> actualUserNames1;
    CheckPrincipals(appId, nodeId, provider, principals, actualGroupNames1, actualUserNames1);

    error = provider->Cleanup(nodeId, appId, false /*onNode*/);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Principals Cleanup returned %d", error.ReadValue());
    CheckPrincipalsAreDeleted(actualGroupNames1, actualUserNames1);

    // Create principals again
    vector<SecurityPrincipalInformationSPtr> context2;
    error = provider->Setup(ConfigureSecurityPrincipalRequest(nodeId, appId, AppPackageCounter, principals, 0, false, false), /*out*/context2);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Principals Setup returned %d", error.ReadValue());

    vector<wstring> actualGroupNames2;
    vector<wstring> actualUserNames2;
    CheckPrincipals(appId, nodeId, provider, principals, actualGroupNames2, actualUserNames2);

    // This time only call close rather than cleanup and ensure that principals are still deleted
    error = provider->Close();
    VERIFY_IS_TRUE(error.IsSuccess(), L"Principal provider closed successfully");
    CheckPrincipalsAreDeleted(actualGroupNames2, actualUserNames2);
    wstring appGroup = ApplicationPrincipals::GetApplicationLocalGroupName(nodeId, AppPackageCounter);
    SecurityPrincipalHelper::DeleteGroupAccount(appGroup).ReadValue();
}

BOOST_AUTO_TEST_CASE(PrincipalsProviderTestWithMultipleNodes)
{
    wstring nodeId1(L"PrincipalsProviderTestWithMultipleNodes-Node1");
    wstring nodeId2(L"PrincipalsProviderTestWithMultipleNodes-Node2");
    wstring nodeId3(L"PrincipalsProviderTestWithMultipleNodes-Node3");

    wstring appId(L"PrincipalsProviderTestWithMultipleNodes-AppId");

    ApplicationPrincipals::CleanupEnvironment(nodeId1, appId);
    ApplicationPrincipals::CleanupEnvironment(nodeId2, appId);
    ApplicationPrincipals::CleanupEnvironment(nodeId3, appId);

    // Opens 3 nodes and setup same application in all;
    // the shared apps setup fine by sharing the users and groups;
    // The last app that deletes the application should also delete the users and groups;
    // otherwise, only the ref counts on app groups are decremented.
    DummyRoot root;

    // Create an app with 2 groups, G1 and G2 and 1 user, U1
    wstring groupName1(L"G1");
    wstring groupName2(L"G2");
    wstring userName1(L"U1");
    PrincipalsDescription principals;
    SecurityGroupDescription group1;
    group1.Name = L"G1";
    group1.SystemGroupMembers.push_back(L"Users");
    principals.Groups.push_back(group1);

    SecurityGroupDescription group2;
    group2.Name = L"G2";
    principals.Groups.push_back(group2);

    SecurityUserDescription user1;
    user1.Name = userName1;
    user1.ParentApplicationGroups.push_back(groupName1);
    principals.Users.push_back(user1);

    // On top of the specified principals,
    // a new group belonging to the application and
    // a group belonging to each node should be created

    // Open Node 1
    SecurityPrincipalsProviderUPtr provider1 = make_unique<SecurityPrincipalsProvider>(root);
    // Open provider - nothing to delete for this node
    auto error = provider1->Open();
    VERIFY_IS_TRUE(error.IsSuccess(), L"Principal provider1 opened successfully");

    // Open Node 2
    SecurityPrincipalsProviderUPtr provider2 = make_unique<SecurityPrincipalsProvider>(root);
    // Open provider - nothing to delete for this node
    error = provider2->Open();
    VERIFY_IS_TRUE(error.IsSuccess(), L"Principal provider2 opened successfully");

    // Node 1: Setup the application
    vector<SecurityPrincipalInformationSPtr> context1;
    error = provider1->Setup(ConfigureSecurityPrincipalRequest(nodeId1, appId, AppPackageCounter, principals, 0, false, false), /*out*/context1);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Provider1 Setup returned %d", error.ReadValue());

    vector<wstring> actualGroupNames1;
    vector<wstring> actualUserNames1;
    CheckPrincipals(appId, nodeId1, provider1, principals, actualGroupNames1, actualUserNames1);


    // Node 2: Setup the application
    vector<SecurityPrincipalInformationSPtr> context2;
    error = provider2->Setup(ConfigureSecurityPrincipalRequest(nodeId2, appId, AppPackageCounter, principals, 0, false, false), /*out*/context2);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Provider2 Setup returned %d", error.ReadValue());

    vector<wstring> actualGroupNames2;
    vector<wstring> actualUserNames2;
    CheckPrincipals(appId, nodeId2, provider2, principals, actualGroupNames2, actualUserNames2);

    // Open Node 3
    SecurityPrincipalsProviderUPtr provider3 = make_unique<SecurityPrincipalsProvider>(root);
    // Open provider - nothing to delete for this node
    error = provider3->Open();
    VERIFY_IS_TRUE(error.IsSuccess(), L"Principal provider3 opened successfully");

    // Node 3: Setup the application
    vector<SecurityPrincipalInformationSPtr> context3;
    error = provider3->Setup(ConfigureSecurityPrincipalRequest(nodeId3, appId, AppPackageCounter, principals, 0, false, false), /*out*/context3);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Provider3 Setup returned %d", error.ReadValue());

    vector<wstring> actualGroupNames3;
    vector<wstring> actualUserNames3;
    CheckPrincipals(appId, nodeId3, provider3, principals, actualGroupNames3, actualUserNames3);

    // Node 1: cleanup the application
    error = provider1->Cleanup(nodeId1, appId, false);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Provider1 Cleanup returned %d", error.ReadValue());

    CheckPrincipalsAreDeleted(actualGroupNames1, actualUserNames1);

    // Node 2: Close without clean up
    error = provider2->Close();
    VERIFY_IS_TRUE(error.IsSuccess(), L"Principal provider2 closed successfully");

    CheckPrincipalsAreDeleted(actualGroupNames2, actualUserNames2);

    // Node 3: cleanup the application
    error = provider3->Cleanup(nodeId3, appId, false);
    VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Provider3 Cleanup returned %d", error.ReadValue());

    CheckPrincipalsAreDeleted(actualGroupNames3, actualUserNames3);

    // Node 1: Close
    error = provider1->Close();
    VERIFY_IS_TRUE(error.IsSuccess(), L"Principal provider1 closed successfully");


    // Node 3: Close
    error = provider3->Close();
    VERIFY_IS_TRUE(error.IsSuccess(), L"Principal provider3 closed successfully");
    wstring appGroup = ApplicationPrincipals::GetApplicationLocalGroupName(nodeId1, AppPackageCounter);
    SecurityPrincipalHelper::DeleteGroupAccount(appGroup).ReadValue();
    appGroup = ApplicationPrincipals::GetApplicationLocalGroupName(nodeId2, AppPackageCounter);
    SecurityPrincipalHelper::DeleteGroupAccount(appGroup).ReadValue();
    appGroup = ApplicationPrincipals::GetApplicationLocalGroupName(nodeId3, AppPackageCounter);
    SecurityPrincipalHelper::DeleteGroupAccount(appGroup).ReadValue();
}

BOOST_AUTO_TEST_CASE(DummyLogCollectionProviderTest)
{
    // Create fabric node
    shared_ptr<TestFabricNodeHost> fabricNodeHost = make_shared<TestFabricNodeHost>();
    VERIFY_IS_TRUE(fabricNodeHost->Open());

    // Get environment manager reference
    EnvironmentManagerUPtr const & environmentManager = fabricNodeHost->GetHosting().ApplicationManagerObj->EnvironmentManagerObj;

    // Dummy applicationId and applicationPackageDescription
    ApplicationIdentifier appId;
    ServiceModel::ApplicationPackageDescription appPackageDescription;

    // Add empty log policy to the applicationPackageDescription
    appPackageDescription.DigestedEnvironment.Policies.LogCollectionEntries.push_back(
        LogCollectionPolicyDescription());

    // Set-up application environment
    ApplicationEnvironmentContextSPtr context;
    AsyncOperationWaiterSPtr setupWaiter = make_shared<AsyncOperationWaiter>();
    environmentManager->BeginSetupApplicationEnvironment(
        appId,
        appPackageDescription,
        TimeSpan::MaxValue,
        [setupWaiter, &environmentManager, &context] (AsyncOperationSPtr const & operation)
    {
        setupWaiter->SetError(environmentManager->EndSetupApplicationEnvironment(operation, context));
        setupWaiter->Set();
    },
        fabricNodeHost->GetHosting().Root.CreateAsyncOperationRoot());
    setupWaiter->WaitOne();
    VERIFY_IS_TRUE(setupWaiter->GetError().IsSuccess());

    // Clean-up application environment
    AsyncOperationWaiterSPtr cleanupWaiter = make_shared<AsyncOperationWaiter>();
    environmentManager->BeginCleanupApplicationEnvironment(
        context,
        TimeSpan::MaxValue,
        [cleanupWaiter, &environmentManager] (AsyncOperationSPtr const & operation)
    {
        cleanupWaiter->SetError(environmentManager->EndCleanupApplicationEnvironment(operation));
        cleanupWaiter->Set();
    },
        fabricNodeHost->GetHosting().Root.CreateAsyncOperationRoot());
    cleanupWaiter->WaitOne();
    VERIFY_IS_TRUE(cleanupWaiter->GetError().IsSuccess());
    fabricNodeHost->Close();
}
#endif

BOOST_AUTO_TEST_CASE(DummyContainerCertificateSetupTest)
{
    VERIFY_IS_TRUE(this->fabricNodeHost_->Open(), L"DummyContainerCertificateSetupTest failed to open fabricNodeHost_");
    ApplicationEnvironmentContextSPtr appEnvironmentContext;
    ServicePackageInstanceIdentifier servicePackageInstanceId;
    int64 instanceId = 0;

    Directory::CreateDirectory(GetEnvironmentManager().Hosting.RunLayout.GetApplicationWorkFolder(servicePackageInstanceId.ApplicationId.ToString()));
    wstring dataPackagePath = Path::Combine(GetEnvironmentManager().Hosting.RunLayout.GetApplicationFolder(servicePackageInstanceId.ApplicationId.ToString()), L"ServicePackage.DataPackage.1.0");
    Directory::CreateDirectory(dataPackagePath);
    FileWriter file;
    auto error = file.TryOpen(Path::Combine(dataPackagePath, L"TestCert.pfx"));
    VERIFY_IS_TRUE(error.IsSuccess());
    file.Close();

    ServicePackageDescription servicePackageDescription;
    servicePackageDescription.ManifestName = L"ServicePackage";

    ContainerCertificateDescription certDescription;
    certDescription.Name = L"MyCert";
    certDescription.X509StoreName = L"My";
#if !defined(PLATFORM_UNIX)
    certDescription.X509FindValue = L"ad fc 91 97 13 16 8d 9f a8 ee 71 2b a2 f4 37 62 00 03 49 0d";
#else
    InstallTestCertInScope testCert(L"TestCert");
    certDescription.X509FindValue = testCert.Thumbprint()->PrimaryToString();
#endif
    vector<ContainerCertificateDescription> certDescriptions;
    certDescriptions.push_back(certDescription);

    ContainerCertificateDescription certDescription2;
    certDescription2.Name = L"MyCert2";
    certDescription2.DataPackageRef = L"DataPackage";
    certDescription2.DataPackageVersion = L"1.0";
    certDescription2.RelativePath = L"TestCert.pfx";
    certDescription2.Password = L"password";
    certDescriptions.push_back(certDescription2);

    ContainerPoliciesDescription policies;
    policies.CertificateRef = certDescriptions;
    DigestedCodePackageDescription digestedCodePackage;
    digestedCodePackage.ContainerPolicies = policies;
    servicePackageDescription.DigestedCodePackages.push_back(digestedCodePackage);

    // Set-up application environment
    ServicePackageInstanceEnvironmentContextSPtr packageEnvironmentContext;
    ManualResetEvent deployDone;
    wstring appName = L"friendlyAppName";
    
    GetEnvironmentManager().BeginSetupServicePackageInstanceEnvironment(
        appEnvironmentContext,
        appName,
        servicePackageInstanceId,
        instanceId,
        servicePackageDescription,
        TimeSpan::MaxValue,
        [this, &deployDone, &packageEnvironmentContext](AsyncOperationSPtr const & operation)
        {
            auto error = GetEnvironmentManager().EndSetupServicePackageInstanceEnvironment(operation, packageEnvironmentContext);
            VERIFY_IS_TRUE_FMT(error.IsSuccess(), "DummyContainerCertificateSetupTest returned %d", error.ReadValue());
            deployDone.Set();
        },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(deployDone.WaitOne(10000), L"DummyContainerCertificateSetupTest completed before timeout.");
    CheckPathsExist(packageEnvironmentContext->CertificatePaths, packageEnvironmentContext->CertificatePasswordPaths);

    // Clean-up environment
    ManualResetEvent cleanupDone;
    GetEnvironmentManager().BeginCleanupServicePackageInstanceEnvironment(
        packageEnvironmentContext,
        servicePackageDescription,
        instanceId,
        TimeSpan::MaxValue,
        [this, &cleanupDone](AsyncOperationSPtr const & operation)
        {
            auto error = GetEnvironmentManager().EndCleanupServicePackageInstanceEnvironment(operation);
            VERIFY_IS_TRUE_FMT(error.IsSuccess(), "DummyContainerCertificateSetupTest returned %d", error.ReadValue());
            cleanupDone.Set();
        },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(cleanupDone.WaitOne(10000), L"DummyContainerCertificateSetupTest completed before timeout.");

    this->fabricNodeHost_->Close();
}
BOOST_AUTO_TEST_SUITE_END()

EnvironmentManager & EnvironmentManagerTest::GetEnvironmentManager()
{
    return *this->fabricNodeHost_->GetHosting().ApplicationManagerObj->EnvironmentManagerObj;
}

void EnvironmentManagerTest::CheckPathsExist(
    std::map <std::wstring, std::wstring> pfxPaths,
    std::map <std::wstring, std::wstring> passwordPaths)
{
    for (auto it = pfxPaths.begin(); it != pfxPaths.end(); ++it)
    {
        VERIFY_IS_TRUE_FMT(File::Exists(it->second), "PFX path {0}", it->second);
    }

    for (auto it = passwordPaths.begin(); it != passwordPaths.end(); ++it)
    {
        VERIFY_IS_TRUE_FMT(File::Exists(it->second), "Password path {0}", it->second);
    }
}

#if !defined(PLATFORM_UNIX)
void EnvironmentManagerTest::CheckPrincipals(
    wstring const & applicationId,
    wstring const & nodeId,
    SecurityPrincipalsProviderUPtr const & provider,
    PrincipalsDescription const & principals,
    vector<wstring> & actualGroupNames,
    vector<wstring> & actualUserNames)
{
    UNREFERENCED_PARAMETER(nodeId);
    Trace.WriteInfo(TraceType, "---------- Check principals for app {0}", applicationId.c_str());

    // Check that all users and groups have been created
    Trace.WriteInfo(TraceType, "Check environment groups...");
    vector<SecurityGroupDescription> const & groups = principals.Groups;
    map<wstring, wstring> applicationGroupNameMap;
    map<wstring, vector<wstring>> applicationGroupNameMembershipMap;

    for(auto it = groups.begin(); it != groups.end(); ++it)
    {
        SecurityGroupSPtr group;
        auto error = provider->GetSecurityGroup(nodeId, applicationId, it->Name, group);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Group \"%s\" instantiated", (it->Name).c_str());
        actualGroupNames.push_back(group->AccountName);
        applicationGroupNameMap.insert(make_pair(group->Name, group->AccountName));
        applicationGroupNameMembershipMap.insert(make_pair(group->Name, vector<wstring>()));
    }

    Trace.WriteInfo(TraceType, "Check environment users...");
    vector<SecurityUserDescription> const & users = principals.Users;
    for(auto it = users.begin(); it != users.end(); ++it)
    {
        Trace.WriteInfo(TraceType, "Check user {0}...", it->Name);
        std::vector<wstring> actualParentApplicationGroupNames;
        SecurityUserSPtr user;
        auto error = provider->GetSecurityUser(nodeId, applicationId, it->Name, user);
        for (auto groupNameIter = it->ParentApplicationGroups.begin(); groupNameIter != it->ParentApplicationGroups.end(); ++groupNameIter)
        {
            auto mapIter = applicationGroupNameMap.find(*groupNameIter);
            VERIFY_IS_FALSE_FMT(mapIter == applicationGroupNameMap.end(), "{0} group nanme not found in applicationGroupNameMap", *groupNameIter);
            auto map2Iter = applicationGroupNameMembershipMap.find(*groupNameIter);
            VERIFY_IS_FALSE_FMT(map2Iter == applicationGroupNameMembershipMap.end(), "{0} group nanme not found in applicationGroupNameMembershipMap", *groupNameIter);
            actualParentApplicationGroupNames.push_back(mapIter->second);
            map2Iter->second.push_back(user->AccountName);
        }
        //also verify user is member of application group.
        wstring appGroupName = ApplicationPrincipals::GetApplicationLocalGroupName(nodeId, AppPackageCounter);
        actualParentApplicationGroupNames.push_back(appGroupName);

        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "User \"%s\" instantiated", (it->Name).c_str());

        // Replace userDescription.ParentApplicationGroup names with the acual names
        SecurityUserDescription userDescription = *it;
        userDescription.ParentApplicationGroups = actualParentApplicationGroupNames;
        CheckUserInfo(user, userDescription, true);
        actualUserNames.push_back(user->AccountName);
    }

    for(auto it = groups.begin(); it != groups.end(); ++it)
    {
        Trace.WriteInfo(TraceType, "Check group {0}...", it->Name);
        SecurityGroupSPtr group;
        auto error = provider->GetSecurityGroup(nodeId, applicationId, it->Name, group);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Group \"%s\" instantiated", (it->Name).c_str());
        auto map2Iter = applicationGroupNameMembershipMap.find(it->Name);
        VERIFY_IS_FALSE_FMT(map2Iter == applicationGroupNameMembershipMap.end(), "{0} group nanme not found in applicationGroupNameMembershipMap", it->Name);
        CheckGroupInfo(group, *it, map2Iter->second);
    }
}

void EnvironmentManagerTest::CheckPrincipalsAreDeleted(
    vector<wstring> const& actualGroupNames,
    vector<wstring> const& actualUserNames)
{
    for(auto it = actualGroupNames.begin(); it != actualGroupNames.end(); ++it)
    {
        wstring groupName = (*it);
        LPBYTE outBuf = NULL;
        NET_API_STATUS nStatus = ::NetLocalGroupGetInfo(
            NULL /*serverName*/,
            groupName.c_str(),
            1 /*LOCALGROUP_INFO level*/,
            &outBuf);

        VERIFY_IS_TRUE(nStatus == NERR_GroupNotFound, L"NetLocalGroupGetInfo returned success");
    }

    for(auto it = actualUserNames.begin(); it != actualUserNames.end(); ++it)
    {
        wstring userName = (*it);
        LPBYTE outBuf = NULL;
        NET_API_STATUS nStatus = ::NetUserGetInfo(
            NULL /*serverName*/,
            userName.c_str(),
            1 /*LOCALGROUP_INFO level*/,
            &outBuf);

        VERIFY_IS_TRUE(nStatus == NERR_UserNotFound , L"NetUserGetInfo returned success");
    }
}

void EnvironmentManagerTest::CheckGroupInfo(
    SecurityGroupSPtr const & group,
    SecurityGroupDescription const & groupDesc,
    vector<wstring> const& actualUserNames)
{
    LPBYTE outBuf = NULL;
    NET_API_STATUS nStatus = ::NetLocalGroupGetInfo(
        NULL /*serverName*/,
        group->AccountName.c_str(),
        1 /*LOCALGROUP_INFO level*/,
        &outBuf);

    if (nStatus == NERR_GroupNotFound)
    {
        Trace.WriteInfo(TraceType,"The group \"{0}\"wasn't successfully created!",
            group->AccountName);
    }

    VERIFY_IS_TRUE(nStatus == NERR_Success, L"NetLocalGroupGetInfo returned success");
    ASSERT_IF(outBuf == NULL, "outBuf should not be null");
    PLOCALGROUP_INFO_1 pGroupInfo = reinterpret_cast<PLOCALGROUP_INFO_1>(outBuf);
    Trace.WriteInfo(TraceType,"Group name = \"{0}\", comment = \"{1}\"",
        pGroupInfo->lgrpi1_name,
        pGroupInfo->lgrpi1_comment);

    nStatus = NetApiBufferFree(outBuf);
    if (nStatus != NERR_Success)
    {
        VERIFY_FAIL(L"Deallocating buffer returned error");
    }

    // Check group membership
    PLOCALGROUP_MEMBERS_INFO_2 pBuf = NULL;
    DWORD dwLevel = 2;
    DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    nStatus = NetLocalGroupGetMembers(
        NULL /*serverName*/,
        group->AccountName.c_str() /*accountName*/,
        dwLevel,
        (LPBYTE *) &pBuf,
        dwPrefMaxLen,
        &dwEntriesRead,
        &dwTotalEntries,
        NULL);

    VERIFY_IS_TRUE_FMT(nStatus == NERR_Success, "NetLocalGroupGetMembers returned %d", nStatus);

    vector<wstring> expectedMembership;
    for (auto itDomain = groupDesc.DomainGroupMembers.begin(); itDomain != groupDesc.DomainGroupMembers.end(); ++itDomain)
    {
        expectedMembership.push_back(*itDomain);
    }

    for (auto itUsers = groupDesc.DomainUserMembers.begin(); itUsers != groupDesc.DomainUserMembers.end(); ++itUsers)
    {
        expectedMembership.push_back(*itUsers);
    }

    for (auto itUsers = actualUserNames.begin(); itUsers != actualUserNames.end(); ++itUsers)
    {
        expectedMembership.push_back(*(LocalDomainName)+ L"\\" + *itUsers);
    }

    vector<wstring> actualMembership;
    PLOCALGROUP_MEMBERS_INFO_2 pTmpBuf;
    DWORD dwTotalCount = 0;
    if ((pTmpBuf = pBuf) != NULL)
    {
        Trace.WriteInfo(TraceType,"Group {0} has members:", group->AccountName);

        for (DWORD i = 0; i < dwEntriesRead; i++)
        {
            ASSERT_IF(pTmpBuf == NULL, "Buffer shouldn't be null");
            Trace.WriteInfo(TraceType,"\t{0}", pTmpBuf->lgrmi2_domainandname);
            actualMembership.push_back(wstring(pTmpBuf->lgrmi2_domainandname));
            pTmpBuf++;
            dwTotalCount++;
        }

        Trace.WriteInfo(TraceType,"{0} total entries, {1} entries read", dwTotalEntries, dwEntriesRead);
        DWORD expectedCount = static_cast<DWORD>(
            groupDesc.DomainGroupMembers.size() +
            groupDesc.DomainUserMembers.size());
        VERIFY_IS_TRUE(expectedCount <  dwTotalCount, L"Group has at least the desired number of members");
    }

    // Check that actual membership equals expected membership
    CheckStringCollection(expectedMembership, actualMembership);

    if (pBuf != NULL)
    {
        NetApiBufferFree(pBuf);
    }

    // Check sid
    CheckSid(group->AccountName, group->TryGetSid());
    wstring groupName = group->AccountName;
    StringUtility::ToLower(groupName);
    for (auto itSystem = groupDesc.SystemGroupMembers.begin(); itSystem != groupDesc.SystemGroupMembers.end(); ++itSystem)
    {
        PLOCALGROUP_MEMBERS_INFO_1 syspBuf = NULL;
        auto systemGrp = *itSystem;
        dwLevel = 1;
        nStatus = NetLocalGroupGetMembers(
            NULL /*serverName*/,
            systemGrp.c_str() /*accountName*/,
            dwLevel,
            (LPBYTE *) &syspBuf,
            dwPrefMaxLen,
            &dwEntriesRead,
            &dwTotalEntries,
            NULL);

        VERIFY_IS_TRUE_FMT(nStatus == NERR_Success, "NetLocalGroupGetMembers returned %d", nStatus);
        bool memberFound = false;
        PLOCALGROUP_MEMBERS_INFO_1 psysTmpBuf;
        if ((psysTmpBuf = syspBuf) != NULL)
        {

            for (DWORD i = 0; i < dwEntriesRead; i++)
            {
                ASSERT_IF(psysTmpBuf == NULL, "Buffer shouldn't be null");
                wstring member = wstring(psysTmpBuf->lgrmi1_name);
                StringUtility::ToLower(member);
                if(member == groupName)
                {
                    Trace.WriteInfo(TraceType,"System Group {0} has member: {1}", systemGrp.c_str(), member);
                    memberFound = true;
                    break;
                }
                psysTmpBuf++;
            }
            VERIFY_IS_TRUE_FMT(memberFound, "Local group %s not found in System group %s", groupName.c_str(), systemGrp.c_str());
        }
        if (syspBuf != NULL)
        {
            NetApiBufferFree(syspBuf);
        }
    }
}

void EnvironmentManagerTest::CheckStringCollection(
    __in vector<wstring> & expected,
    __in vector<wstring> & actual)
{
    if (actual.size() != expected.size())
    {
        VERIFY_FAIL_FMT("Expected string collection size %d != actual size %d",
            (int)expected.size(),
            (int)actual.size());
    }

    for (size_t i = 0; i < expected.size(); ++i)
    {
        StringUtility::ToLower(expected[i]);
        StringUtility::ToLower(actual[i]);
    }

    sort(actual.begin(), actual.end());
    sort(expected.begin(), expected.end());

    for (size_t i = 0; i < expected.size(); ++i)
    {
        if (expected[i] != actual[i])
        {
            VERIFY_FAIL_FMT("Expected: %s; actual %s",
                expected[i].c_str(),
                actual[i].c_str());
        }
    }
}

void EnvironmentManagerTest::CheckUserInfo(
    SecurityUserSPtr const & user,
    SecurityUserDescription const & userDesc,
    bool addWindowsFabricAllowedUsersGroupComment)
{
    // Check user membership
    LPLOCALGROUP_USERS_INFO_0 pBuf = NULL;
    DWORD dwLevel = 0;
    DWORD dwFlags = LG_INCLUDE_INDIRECT ;
    DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    NET_API_STATUS nStatus = NetUserGetLocalGroups(
        NULL /*serverName*/,
        user->AccountName.c_str() /*userName*/,
        dwLevel,
        dwFlags,
        (LPBYTE *) &pBuf,
        dwPrefMaxLen,
        &dwEntriesRead,
        &dwTotalEntries);

    VERIFY_IS_TRUE_FMT(nStatus == NERR_Success, "NetUserGetLocalGroups returned %d", nStatus);

    vector<wstring> expectedMembership;
    for (auto itSystem = userDesc.ParentSystemGroups.begin(); itSystem != userDesc.ParentSystemGroups.end(); ++itSystem)
    {
        expectedMembership.push_back(*itSystem);
    }

    for (auto itApp = userDesc.ParentApplicationGroups.begin(); itApp != userDesc.ParentApplicationGroups.end(); ++itApp)
    {
        expectedMembership.push_back(*itApp);
    }

    vector<wstring> actualMembership;
    LPLOCALGROUP_USERS_INFO_0 pTmpBuf;
    DWORD dwTotalCount = 0;
    if ((pTmpBuf = pBuf) != NULL)
    {
        Trace.WriteInfo(TraceType,"User {0} is member of:", user->AccountName);
        for (DWORD i = 0; i < dwEntriesRead; i++)
        {
            ASSERT_IF(pTmpBuf == NULL, "Buffer shouldn't be null");
            Trace.WriteInfo(TraceType,"\t{0}", pTmpBuf->lgrui0_name);
            actualMembership.push_back(wstring(pTmpBuf->lgrui0_name));
            pTmpBuf++;
            dwTotalCount++;
        }

        DWORD expectedGroupCount = static_cast<DWORD>(userDesc.ParentSystemGroups.size() + userDesc.ParentApplicationGroups.size());
        if(addWindowsFabricAllowedUsersGroupComment)
        {
            expectedGroupCount++;
        }

        VERIFY_ARE_EQUAL(expectedGroupCount, dwTotalCount, L"User belongs to the right number of groups");
    }

    if(addWindowsFabricAllowedUsersGroupComment)
    {
        expectedMembership.push_back(FabricConstants::WindowsFabricAllowedUsersGroupName);
    }

    // Check that actual membership equals expected membership
    CheckStringCollection(actualMembership, expectedMembership);

    Trace.WriteInfo(TraceType,"{0} total entries, {1} entries read", dwTotalEntries, dwEntriesRead);
    if (pBuf != NULL)
    {
        NetApiBufferFree(pBuf);
    }

    // Check user sid
    CheckSid(user->AccountName, user->SidObj->PSid);
}

ErrorCode EnvironmentManagerTest::GetUserProfile(
    std::wstring const& accountName,
    TokenHandleSPtr const& tokenHandle,
    __out wstring & profileDir)
{
    // Get the profile directory and make sure it exists
    DWORD profileDirSize;
    if (!::GetUserProfileDirectoryW(tokenHandle->Value, NULL, &profileDirSize))
    {
        DWORD dwLastError = GetLastError();
        auto error = ErrorCode::FromWin32Error(dwLastError);
        if (dwLastError != ERROR_INSUFFICIENT_BUFFER)
        {
            Trace.WriteWarning(TraceType, "GetUserProfileDirectory for {0}: {1}, error = {2}", accountName, dwLastError, error);
            return error;
        }
    }

    profileDir.resize(profileDirSize * sizeof(WCHAR));
    if (!::GetUserProfileDirectoryW(tokenHandle->Value, &profileDir[0], &profileDirSize))
    {
        DWORD dwLastError = GetLastError();
        auto error = ErrorCode::FromWin32Error(dwLastError);
        Trace.WriteWarning(TraceType, "GetUserProfileDirectory for {0}: {1}, error = {2}", accountName, dwLastError, error);
        return error;
    }

    Trace.WriteInfo(TraceType, "ProfileDirectory for {0}: \"{1}\"", accountName.c_str(), profileDir.c_str());
    return ErrorCode(ErrorCodeValue::Success);
}


void EnvironmentManagerTest::CheckSid(wstring const & accountName, __in PSID sid)
{
    VERIFY_IS_FALSE(sid == NULL, L"User SID should be set");

    DWORD dwNameLength = 0;
    DWORD dwDomainNameLength = 0;
    SID_NAME_USE use;
    if (!LookupAccountSid(
        NULL,
        sid,
        NULL,
        &dwNameLength,
        NULL,
        &dwDomainNameLength,
        &use))
    {
        const DWORD dwLastError = GetLastError();
        if (dwLastError != ERROR_INSUFFICIENT_BUFFER)
        {
            Trace.WriteWarning(TraceType, "Error looking up account SID: %d", dwLastError);
            VERIFY_FAIL_FMT("Error looking up account SID to get buffer length: error %d", dwLastError);
        }
    }

    vector<WCHAR> nameBuffer(dwNameLength);
    vector<WCHAR> domainNameBuffer(dwDomainNameLength);
    if (!LookupAccountSid(
        NULL,
        sid,
        nameBuffer.data(),
        &dwNameLength,
        domainNameBuffer.data(),
        &dwDomainNameLength,
        &use))
    {
        const DWORD dwLastError = GetLastError();
        if (dwLastError == ERROR_NONE_MAPPED)
        {
            Trace.WriteWarning(TraceType, "Error looking up account SID: couldn't map SID to account name");
        }
        else
        {
            Trace.WriteWarning(TraceType, "Error looking up account SID: %d", dwLastError);
        }
        VERIFY_FAIL(L"Error looking up account SID to get the strings");
    }

    wstring name = nameBuffer.data();
    wstring referenceDomain = domainNameBuffer.data();
    Trace.WriteInfo(TraceType, "Account for \"%s\": name=%s, domain name=%s", accountName, name, referenceDomain);
    VERIFY_ARE_EQUAL(accountName, name, L"Account information based on SID matches expected name");
}

void EnvironmentManagerTest::VerifyPrincipalInformation(
    vector<SecurityPrincipalInformationSPtr> const & principalInformation,
    vector<wstring> const & expectedUsersAndGroups)
{
    wstring principalInfoString;
    StringWriter writer(principalInfoString);
    for (auto const & entry : principalInformation)
    {
        writer.Write("{0}; ", *entry);
    }

    Trace.WriteInfo(TraceType, "GetSecurityPrincipalInformation returned {0}", principalInfoString);

    VERIFY_ARE_EQUAL(principalInformation.size(), expectedUsersAndGroups.size());
    for (size_t i = 0; i < principalInformation.size(); ++i)
    {
        bool found = false;
        for (auto const & expected : expectedUsersAndGroups)
        {
            if (principalInformation[i]->Name == expected)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            Trace.WriteWarning(
                TraceType,
                "VerifyPrincipalInformation failed: did not find {0} in expectedUsersAndGroups {1}",
                principalInformation[i]->Name,
                expectedUsersAndGroups);
        }

        VERIFY_IS_TRUE(found, L"Check expected users and groups failed.");
    }
}
#endif

bool EnvironmentManagerTest::Setup()
{
#if !defined(PLATFORM_UNIX)
    // Check whether the user is an admin
    bool isAdmin = false;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    BOOL result = AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdministratorsGroup);
    if (result == TRUE)
    {
        BOOL isMember;
        result = CheckTokenMembership(NULL, AdministratorsGroup, &isMember);
        if (result && isMember)
        {
            isAdmin = true;
        }
    }

    if (AdministratorsGroup != NULL)
    {
        FreeSid(AdministratorsGroup);
    }

    Trace.WriteInfo(TraceType, "Running as elevated admin: %d", isAdmin);
    return isAdmin;
#else
    return true;
#endif
}

bool EnvironmentManagerTest::Cleanup()
{
    return true;
}
