// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Hosting2/FabricNodeHost.Test.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;
using namespace Management::FileStoreService;

const StringLiteral TraceType("DownloadManagerTestFSSSetup");

class DownloadManagerTestFSSSetup
{
public:
    static void VerifyUser(
        SecurityUserDescription const & user,
        wstring const & name,
        wstring const & thumbprint);
    static void VerifyUserInPricipal(
        wstring const & expectedAccountName,
        wstring const & expectedThumbprint,
        PrincipalsDescriptionUPtr const & principals);
    static void TestFSSThumbprintPrincipals(
        __in InstallTestCertInScope const * primaryCert,
        __in InstallTestCertInScope const * secondaryCert);
    static void TestFSSCommonNamePrincipals(
        vector<wstring> const & previousThumbprints,
        vector<InstallTestCertInScope*> const & certs,
        __out vector<wstring> & newThumbprints);
    void CheckDownloadManagerNTLMUsers(
        __in DownloadManager & hosting,
        vector<wstring> const & expectedThumbprints,
        int const & expectedNewTokenCount,
        __inout AccessTokensCollection & tokenMap);
    void CheckDownloadManagerNTLMUsersWithRetry(
        __in DownloadManager & hosting,
        vector<wstring> const & expectedThumbprints,
        int const & expectedNewTokenCount,
        __inout AccessTokensCollection & tokenMap);

    void CleanUpFileStoreUsers();

protected:
    // Delay create fabric node host based on configuration
    DownloadManagerTestFSSSetup() : fabricNodeHost_()
    {
        BOOST_REQUIRE(Setup());
    }

    TEST_CLASS_SETUP(Setup);
    ~DownloadManagerTestFSSSetup() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);

    void CreateFabricNodeHost();

    DownloadManager & GetDownloadManager();

    vector<wstring> calculatorAppFiles;
    vector<wstring> calculatorPackageFiles;
    vector<wstring> sandboxedPQueueAppFiles;
    vector<wstring> sandboxedPQueuePackageFiles;

    shared_ptr<TestFabricNodeHost> fabricNodeHost_;
};

BOOST_FIXTURE_TEST_SUITE(DownloadManagerTestFSSSetupSuite, DownloadManagerTestFSSSetup)

BOOST_AUTO_TEST_CASE(FileStoreServiceConfigUsers)
{
#if defined(NEVER_BUILD)

    Trace.WriteInfo(TraceType, "FileStoreServiceConfigUsers Start");

    //
    // Tests loading correct principals through configuration,
    // by calling direct methods on Management::FileStoreService::Utility
    //
    FileStoreServiceConfig::GetConfig().Test_Reset();

    VERIFY_IS_FALSE(Utility::HasCommonNameAccountsConfigured());
    VERIFY_IS_FALSE(Utility::HasThumbprintAccountsConfigured());
    VERIFY_IS_FALSE(Utility::HasAccountsConfigured());

    // Install a certificate for primary account
    wstring primaryCommonName = wformatString("Primary{0}", Guid::NewGuid());
    InstallTestCertInScope primaryCert(L"CN=" + primaryCommonName);
    wstring primaryThumbprint = wformatString(primaryCert.Thumbprint());

    FileStoreServiceConfig::GetConfig().PrimaryAccountType = wformatString(SecurityPrincipalAccountType::Enum::LocalUser);
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreLocation = wformatString(primaryCert.StoreLocation());
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreName = primaryCert.StoreName();
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509Thumbprint = primaryThumbprint;   

    Trace.WriteInfo(
        TraceType,
        "Test GetPrincipalsDescriptionFromConfig with Principal account configured: common name {0}, thumbprint {1}",
        primaryCommonName,
        primaryThumbprint);
    TestFSSThumbprintPrincipals(&primaryCert, NULL);

    AccessTokensCollection tokenMap;
    AccessTokensList newTokens;
    auto error = Utility::GetAccessTokens(tokenMap, newTokens);
    // The AccessToken::GetUserTokenAndProfileHandler fails with "the user name and password is incorrect" because of the test certificate used.
    VERIFY_IS_FALSE(error.IsSuccess());

    VERIFY_IS_FALSE(Utility::HasCommonNameAccountsConfigured());

    // Install a certificate for secondary
    wstring secondaryCommonName = wformatString("Secondary{0}", Guid::NewGuid());
    InstallTestCertInScope secondaryCert(L"CN=" + secondaryCommonName);
    wstring secondaryThumbprint = wformatString(secondaryCert.Thumbprint());

    FileStoreServiceConfig::GetConfig().SecondaryAccountType = wformatString(SecurityPrincipalAccountType::Enum::LocalUser);
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreLocation = wformatString(secondaryCert.StoreLocation());
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreName = secondaryCert.StoreName();
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509Thumbprint = wformatString(secondaryCert.Thumbprint());

    Trace.WriteInfo(
        TraceType,
        "Test GetPrincipalsDescriptionFromConfig with Principal account configured: primary common name {0}, thumbprint {1}, secondary common name {2}, thumbprint {3}",
        primaryCommonName,
        primaryThumbprint,
        secondaryCommonName,
        secondaryThumbprint);
    TestFSSThumbprintPrincipals(&primaryCert, &secondaryCert);
    
    error = Utility::GetAccessTokens(tokenMap, newTokens);
    VERIFY_IS_FALSE(error.IsSuccess());

    VERIFY_IS_FALSE(Utility::HasCommonNameAccountsConfigured());

    Trace.WriteInfo(TraceType, "Install a certificate for X509 certificates specified by common names");
    wstring commonName = wformatString("CommonName{0}", Guid::NewGuid());
    InstallTestCertInScope cnCert1(L"CN=" + commonName);
    wstring cnThumbprint1 = wformatString(cnCert1.Thumbprint());

    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509StoreLocation = wformatString(cnCert1.StoreLocation());
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509StoreName = cnCert1.StoreName();
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509CommonName = commonName;

    // Generate both v1 and v2 accounts
    FileStoreServiceConfig::GetConfig().GenerateV1CommonNameAccount = true;
    vector<wstring> thumbprints;
    {
        vector<InstallTestCertInScope*> certs;
        certs.push_back(&cnCert1);
        vector<wstring> previousThumbprints;
        TestFSSCommonNamePrincipals(previousThumbprints, certs, thumbprints);
    }

    Trace.WriteInfo(TraceType, "Install another certificate with the same common name");
    InstallTestCertInScope cnCert2(L"CN=" + commonName);
    {
        vector<InstallTestCertInScope*> certs;
        certs.push_back(&cnCert2);
        vector<wstring> newThumbprints;
        TestFSSCommonNamePrincipals(thumbprints, certs, newThumbprints);
    }

    // Add another common name in config - Primary cert
    // 3 matching certs - one for primary and 2 for the previous common name
    Trace.WriteInfo(TraceType, "Add another common name in config - Primary cert");
    FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509StoreLocation = wformatString(primaryCert.StoreLocation());
    FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509StoreName = primaryCert.StoreName();
    FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509CommonName = primaryCommonName;
    thumbprints.clear();
    {
        vector<InstallTestCertInScope*> certs;
        certs.push_back(&cnCert1);
        certs.push_back(&cnCert2);
        certs.push_back(&primaryCert);
        vector<wstring> previousThumbprints;
        TestFSSCommonNamePrincipals(previousThumbprints, certs, thumbprints);
    }

    // Replace one of the common names with a common name without certificates
    Trace.WriteInfo(TraceType, "Replace one of the common names with a common name without certificates");
    FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509CommonName = L"InvalidCommonNamehiahia";
    {
        vector<InstallTestCertInScope*> certs;
        vector<wstring> previousThumbprints = move(thumbprints);
        TestFSSCommonNamePrincipals(previousThumbprints, certs, thumbprints);
    }

    // Do not generate v1 accounts. Find accounts for CommonName1.
    FileStoreServiceConfig::GetConfig().GenerateV1CommonNameAccount = false;
    {
        vector<InstallTestCertInScope*> certs;
        certs.push_back(&cnCert1);
        certs.push_back(&cnCert2);
        vector<wstring> previousThumbprints;
        TestFSSCommonNamePrincipals(previousThumbprints, certs, thumbprints);
    }

    // Cleanup for next tests
    FileStoreServiceConfig::GetConfig().Test_Reset();

    Trace.WriteInfo(TraceType, "FileStoreServiceConfigUsers Succeed");

#endif
}

BOOST_AUTO_TEST_CASE(SetupFileStoreServiceUsersForArmScenario)
{
#if defined(NEVER_BUILD)

    Trace.WriteInfo(TraceType, "SetupFileStoreServiceUsersForArmScenario Start");

    ImageStore::ImageStoreServiceConfig::GetConfig().Test_Reset();
    ImageStore::ImageStoreServiceConfig::GetConfig().Enabled = true;

    FileStoreServiceConfig::GetConfig().Test_Reset();

    HostingConfig::GetConfig().Test_Reset();
    HostingConfig::GetConfig().NTLMSecurityUsersByX509CommonNamesRefreshInterval = TimeSpan::FromSeconds(2);
    
    // Install first certificate
    wstring commonName = wformatString("CN1-{0}", Guid::NewGuid());
    InstallTestCertInScope cert1(L"CN=" + commonName);
    wstring t1 = wformatString(cert1.Thumbprint());

    // Change config to enable thumbprint accounts with cert1
    FileStoreServiceConfig::GetConfig().PrimaryAccountType = wformatString(SecurityPrincipalAccountType::Enum::LocalUser);
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreLocation = wformatString(cert1.StoreLocation());
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreName = cert1.StoreName();
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509Thumbprint = t1;
    FileStoreServiceConfig::GetConfig().SecondaryAccountType = wformatString(SecurityPrincipalAccountType::Enum::LocalUser);
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreLocation = wformatString(cert1.StoreLocation());
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreName = cert1.StoreName();
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509Thumbprint = t1;

    // Config but do not install another certificate with the same common name
    wstring commonName2 = wformatString("CN2-{0}", Guid::NewGuid());
    InstallTestCertInScope cert2(false, L"CN=" + commonName2);
    wstring t2 = wformatString(cert2.Thumbprint());

    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509StoreLocation = wformatString(cert2.StoreLocation());
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509StoreName = cert2.StoreName();
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509CommonName = commonName2;

    // Enable V1 account name generation, so for each common name certificate we create 2 users
    FileStoreServiceConfig::GetConfig().GenerateV1CommonNameAccount = true;

    CreateFabricNodeHost();
    bool success = fabricNodeHost_->Open();
    VERIFY_IS_TRUE(success);

    auto & hosting = GetDownloadManager();
    vector<wstring> expectedThumbprints;
    AccessTokensCollection tokenMap;
    AccessTokensList newTokens;
    auto error = Utility::GetAccessTokens(tokenMap, newTokens);
    // The AccessToken::GetUserTokenAndProfileHandler fails with "the user name and password is incorrect" because of the test certificate used.
    VERIFY_IS_FALSE(error.IsSuccess());

    Trace.WriteInfo(TraceType, "Checking if no user is picked up");
    CheckDownloadManagerNTLMUsers(hosting, expectedThumbprints, 0, tokenMap);

    Trace.WriteInfo(TraceType, "install cert2, and wait for the timer to pick it up");
    cert2.Install();
    
    expectedThumbprints.push_back(t2);
    // Expect one thumbprint and 2 new tokens, since the V1 account is enabled
    CheckDownloadManagerNTLMUsersWithRetry(hosting, expectedThumbprints, 2, tokenMap);
    
    // Cleanup for next tests
    FileStoreServiceConfig::GetConfig().Test_Reset();
    HostingConfig::GetConfig().Test_Reset();
    ImageStore::ImageStoreServiceConfig::GetConfig().Test_Reset();

    CleanUpFileStoreUsers();
    Cleanup();

    Trace.WriteInfo(TraceType, "SetupFileStoreServiceUsersForArmScenario Succeeded");

#endif
}

BOOST_AUTO_TEST_CASE(SetupFileStoreServiceUsersByCommonNameOnly)
{
#if defined(NEVER_BUILD)

    Trace.WriteInfo(TraceType, "SetupFileStoreServiceUsersByCommonNameOnly Start");

    ImageStore::ImageStoreServiceConfig::GetConfig().Test_Reset();
    ImageStore::ImageStoreServiceConfig::GetConfig().Enabled = true;

    FileStoreServiceConfig::GetConfig().Test_Reset();

    HostingConfig::GetConfig().Test_Reset();
    HostingConfig::GetConfig().NTLMSecurityUsersByX509CommonNamesRefreshInterval = TimeSpan::FromSeconds(2);
    DWORD hostingRefreshInterval = static_cast<DWORD>(HostingConfig::GetConfig().NTLMSecurityUsersByX509CommonNamesRefreshInterval.TotalMilliseconds());

    //
    // Set CommonName1 and create certificates.
    //

    Trace.WriteInfo(TraceType, "Install first certificate");
    wstring cn1 = wformatString("CN1-{0}", Guid::NewGuid());
    InstallTestCertInScope cert1(L"CN=" + cn1);
    wstring t1 = wformatString(cert1.Thumbprint());

    // Change config to enable common names account
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509StoreLocation = wformatString(cert1.StoreLocation());
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509StoreName = cert1.StoreName();
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509CommonName = cn1;

    // Enable V1 account name generation, so for each common name certificate we create 2 users
    FileStoreServiceConfig::GetConfig().GenerateV1CommonNameAccount = true;

    CreateFabricNodeHost();
    bool success = fabricNodeHost_->Open();
    VERIFY_IS_TRUE(success);

    auto & hosting = GetDownloadManager();

    vector<wstring> expectedThumbprints;
    expectedThumbprints.push_back(t1);
    int expectedNewTokenCount = 0;
    AccessTokensCollection tokenMap;
    AccessTokensList newTokens;
    auto error = Utility::GetAccessTokens(tokenMap, newTokens);
    VERIFY_IS_TRUE(error.IsSuccess());
    CheckDownloadManagerNTLMUsers(hosting, expectedThumbprints, expectedNewTokenCount, tokenMap);

    Trace.WriteInfo(TraceType, "Install another certificate with the same common name");
    InstallTestCertInScope cert2(L"CN=" + cn1);
    wstring t2 = wformatString(cert2.Thumbprint());

    Trace.WriteInfo(TraceType, "Installed cert2, wait for DownloadManager to find it");

    expectedThumbprints.push_back(t2);
    // V1 generation is enabled, expect 2 new users
    expectedNewTokenCount = 2;
    CheckDownloadManagerNTLMUsersWithRetry(hosting, expectedThumbprints, expectedNewTokenCount, tokenMap);
    
    // Disable V1 account name generation, so for each common name certificate we create only v2 user
    FileStoreServiceConfig::GetConfig().GenerateV1CommonNameAccount = false;

    Trace.WriteInfo(TraceType, "Set CommonName2 but do not install the certificate. No new users should be created.");
    wstring cn2 = wformatString("CN2-{0}", Guid::NewGuid());
    InstallTestCertInScope cert3(false, L"CN=" + cn2);
    wstring t3 = wformatString(cert3.Thumbprint());

    FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509StoreLocation = wformatString(cert3.StoreLocation());
    FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509StoreName = cert3.StoreName();
    FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509CommonName = cn2;
    Trace.WriteInfo(TraceType, "start sleeping: {0}", hostingRefreshInterval);
    Sleep(hostingRefreshInterval * 5);
    Trace.WriteInfo(TraceType, "end sleeping");

    CheckDownloadManagerNTLMUsers(hosting, expectedThumbprints, 0, tokenMap);

    Trace.WriteInfo(TraceType, "Reinstall certificate, check that Hosting is picking it up.");
    error = cert3.Install();
    VERIFY_IS_TRUE(error.IsSuccess());

    expectedThumbprints.push_back(t3);
    // v1 disabled, only one access token expected, for the V2 account
    expectedNewTokenCount = 1;
    CheckDownloadManagerNTLMUsersWithRetry(hosting, expectedThumbprints, expectedNewTokenCount, tokenMap);

    Trace.WriteInfo(TraceType, "Change CommonName1 to another value, check that Hosting finds the new certificates.");
    wstring cn3 = wformatString("CN3-{0}", Guid::NewGuid());
    InstallTestCertInScope cert4(L"CN=" + cn3);
    wstring t4 = wformatString(cert4.Thumbprint());
    InstallTestCertInScope cert5(L"CN=" + cn3);
    wstring t5 = wformatString(cert5.Thumbprint());
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509CommonName = cn3;

    expectedThumbprints.push_back(t4);
    expectedThumbprints.push_back(t5);
    expectedNewTokenCount = 2;
    CheckDownloadManagerNTLMUsersWithRetry(hosting, expectedThumbprints, expectedNewTokenCount, tokenMap);

    Trace.WriteInfo(TraceType, "Reset CommonName2, no certificates are deleted");
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509CommonName = L"";
    Trace.WriteInfo(TraceType, "start sleeping: {0}", hostingRefreshInterval);
    Sleep(hostingRefreshInterval * 5);
    Trace.WriteInfo(TraceType, "end sleeping");

    expectedNewTokenCount = 0;
    CheckDownloadManagerNTLMUsers(hosting, expectedThumbprints, expectedNewTokenCount, tokenMap);

    // Cleanup for next tests
    FileStoreServiceConfig::GetConfig().Test_Reset();
    HostingConfig::GetConfig().Test_Reset();
    ImageStore::ImageStoreServiceConfig::GetConfig().Test_Reset();

    Cleanup();

    Trace.WriteInfo(TraceType, "SetupFileStoreServiceUsersByCommonNameOnly Succeed");

#endif
}

BOOST_AUTO_TEST_CASE(SetupFileStoreServiceUsersByCommonNameAndThumbprint)
{
#if defined(NEVER_BUILD)

    Trace.WriteInfo(TraceType, "SetupFileStoreServiceUsersByCommonNameAndThumbprint Start");

    ImageStore::ImageStoreServiceConfig::GetConfig().Test_Reset();
    ImageStore::ImageStoreServiceConfig::GetConfig().Enabled = true;

    FileStoreServiceConfig::GetConfig().Test_Reset();

    HostingConfig::GetConfig().Test_Reset();
    HostingConfig::GetConfig().NTLMSecurityUsersByX509CommonNamesRefreshInterval = TimeSpan::FromSeconds(2);
    DWORD hostingRefreshInterval = static_cast<DWORD>(HostingConfig::GetConfig().NTLMSecurityUsersByX509CommonNamesRefreshInterval.TotalMilliseconds());

    // Install first certificate
    wstring commonName = wformatString("CN1-{0}", Guid::NewGuid());
    InstallTestCertInScope cert1(L"CN=" + commonName);
    wstring t1 = wformatString(cert1.Thumbprint());

    // Change config to enable primary account
    FileStoreServiceConfig::GetConfig().PrimaryAccountType = wformatString(SecurityPrincipalAccountType::Enum::LocalUser);
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreLocation = wformatString(cert1.StoreLocation());
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509StoreName = cert1.StoreName();
    FileStoreServiceConfig::GetConfig().PrimaryAccountNTLMX509Thumbprint = t1;

    // Change config to enable secondary account
    FileStoreServiceConfig::GetConfig().SecondaryAccountType = wformatString(SecurityPrincipalAccountType::Enum::LocalUser);
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreLocation = wformatString(cert1.StoreLocation());
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509StoreName = cert1.StoreName();
    FileStoreServiceConfig::GetConfig().SecondaryAccountNTLMX509Thumbprint = t1;

    // Change config to enable common names account
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509StoreLocation = wformatString(cert1.StoreLocation());
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509StoreName = cert1.StoreName();
    FileStoreServiceConfig::GetConfig().CommonName1Ntlmx509CommonName = commonName;
    
    // Enable V1 account name generation, so for each common name certificate we create 2 users
    FileStoreServiceConfig::GetConfig().GenerateV1CommonNameAccount = true;

    CreateFabricNodeHost();
    bool success = fabricNodeHost_->Open();
    VERIFY_IS_TRUE(success);

    auto & hosting = GetDownloadManager();
    vector<wstring> expectedThumbprints;
    AccessTokensCollection tokenMap;

    expectedThumbprints.push_back(t1);
    AccessTokensList newTokens;
    auto error = Utility::GetAccessTokens(tokenMap, newTokens);
    VERIFY_IS_TRUE(error.IsSuccess());

    CheckDownloadManagerNTLMUsers(hosting, expectedThumbprints, 0, tokenMap);

    Trace.WriteInfo(TraceType, "Install another certificate with the same common name");
    InstallTestCertInScope cert2(L"CN=" + commonName);
    wstring t2 = wformatString(cert2.Thumbprint());

    expectedThumbprints.push_back(t2);
    CheckDownloadManagerNTLMUsersWithRetry(hosting, expectedThumbprints, 2, tokenMap);
 
    Trace.WriteInfo(TraceType, "test with invalid common name, no cert change");
    FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509StoreLocation = wformatString(cert1.StoreLocation());
    FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509StoreName = cert1.StoreName();
    FileStoreServiceConfig::GetConfig().CommonName2Ntlmx509CommonName = L"invalidCommonName";

    Trace.WriteInfo(TraceType, "start sleeping: {0}", hostingRefreshInterval);
    Sleep(hostingRefreshInterval * 5);
    Trace.WriteInfo(TraceType, "end sleeping");

    CheckDownloadManagerNTLMUsersWithRetry(hosting, expectedThumbprints, 0, tokenMap);

    // Cleanup for next tests
    FileStoreServiceConfig::GetConfig().Test_Reset();
    HostingConfig::GetConfig().Test_Reset();
    ImageStore::ImageStoreServiceConfig::GetConfig().Test_Reset();

    CleanUpFileStoreUsers();
    Cleanup();

    Trace.WriteInfo(TraceType, "SetupFileStoreServiceUsersByCommonNameAndThumbprint Succeed");

#endif
}

BOOST_AUTO_TEST_SUITE_END()

DownloadManager & DownloadManagerTestFSSSetup::GetDownloadManager()
{
    return *this->fabricNodeHost_->GetHosting().DownloadManagerObj;
}

bool DownloadManagerTestFSSSetup::Setup()
{
    calculatorAppFiles.clear();
    calculatorPackageFiles.clear();
    sandboxedPQueueAppFiles.clear();
    sandboxedPQueuePackageFiles.clear();

    calculatorAppFiles.push_back(L"CalculatorApp_App0\\App.1.0.xml");

    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.Manifest.1.0.xml");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.Package.1.0.xml");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Code.1.0\\Common.dll.txt");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Code.1.0\\Service.exe.txt");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Code.1.0\\Service.pdb.txt");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Code.1.0\\Binaries\\Shared.DllTxt");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Config.1.0\\Settings.xml");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorServicePackage.CalculatorService.Data.1.0\\Service.pdb.txt");

    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorGatewayPackage.Package.1.0.xml");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorGatewayPackage.Manifest.1.0.xml");
    calculatorPackageFiles.push_back(L"CalculatorApp_App0\\CalculatorGatewayPackage.CalculatorGateway.Code.1.0\\Gateway.exe.txt");

    sandboxedPQueueAppFiles.push_back(L"SandboxPersistentQueueApp_App1\\App.1.0.xml");

    sandboxedPQueuePackageFiles.push_back(L"SandboxPersistentQueueApp_App1\\PersistentQueueServicePackage.Package.1.0.xml");
    sandboxedPQueuePackageFiles.push_back(L"SandboxPersistentQueueApp_App1\\PersistentQueueServicePackage.Manifest.1.0.xml");
    sandboxedPQueuePackageFiles.push_back(L"SandboxPersistentQueueApp_App1\\PersistentQueueServicePackage.PersistentQueueService.Code.1.0\\Common.dll.txt");
    sandboxedPQueuePackageFiles.push_back(L"SandboxPersistentQueueApp_App1\\PersistentQueueServicePackage.PersistentQueueService.Code.1.0\\SandboxPersistentQueueService.exe.txt");

    return true;
}

void DownloadManagerTestFSSSetup::CreateFabricNodeHost()
{
    fabricNodeHost_ = make_shared<TestFabricNodeHost>();
}

bool DownloadManagerTestFSSSetup::Cleanup()
{
    if (fabricNodeHost_)
    {
        fabricNodeHost_->Close();
        fabricNodeHost_ = NULL;
    }

    return true;
}

void DownloadManagerTestFSSSetup::VerifyUser(
    SecurityUserDescription const & user,
    wstring const & expectedAccountName,
    wstring const & expectedThumbprint)
{
    Trace.WriteInfo(TraceType, "Verify user {0}, thumbprint {1}", expectedAccountName, expectedThumbprint);

    VERIFY_IS_TRUE(user.NTLMAuthenticationPolicy.IsEnabled);
    VERIFY_ARE_EQUAL(user.Name, expectedAccountName);
    VERIFY_ARE_EQUAL(user.AccountType, SecurityPrincipalAccountType::LocalUser);
    VERIFY_ARE_EQUAL(user.NTLMAuthenticationPolicy.X509Thumbprint, expectedThumbprint);

    auto it = find(user.ParentSystemGroups.begin(), user.ParentSystemGroups.end(), FabricConstants::WindowsFabricAdministratorsGroupName);
    VERIFY_IS_FALSE(it == user.ParentSystemGroups.end());

    auto it2 = find(user.ParentApplicationGroups.begin(), user.ParentApplicationGroups.end(), FileStoreService::Constants::FileStoreServiceUserGroup);
    VERIFY_IS_FALSE(it2 == user.ParentApplicationGroups.end());
}

void DownloadManagerTestFSSSetup::TestFSSThumbprintPrincipals(
    __in InstallTestCertInScope const * expectedPrimaryCert,
    __in InstallTestCertInScope const * expectedSecondaryCert)
{
    VERIFY_IS_TRUE(Utility::HasThumbprintAccountsConfigured());
    VERIFY_IS_TRUE(Utility::HasAccountsConfigured());

    PrincipalsDescriptionUPtr principals;
    auto error = Management::FileStoreService::Utility::GetPrincipalsDescriptionFromConfigByThumbprint(principals);
    VERIFY_IS_TRUE(error.IsSuccess(), L"GetPrincipalsDescriptionFromConfig with primary/secondary accounts failed");

    VERIFY_IS_NOT_NULL(principals);

    wstring expectedPrimaryAccountName = FileStoreService::Constants::Constants::PrimaryFileStoreServiceUser;
    auto itPrimary = find_if(principals->Users.begin(), principals->Users.end(), [&expectedPrimaryAccountName](SecurityUserDescription const & user) { return user.Name == expectedPrimaryAccountName; });

    wstring expectedSecondaryAccountName = FileStoreService::Constants::Constants::SecondaryFileStoreServiceUser;
    auto itSecondary = find_if(principals->Users.begin(), principals->Users.end(), [&expectedSecondaryAccountName](SecurityUserDescription const & user) { return user.Name == expectedSecondaryAccountName; });

    size_t expectedCount = 0;
    if (expectedPrimaryCert != NULL)
    {
        ++expectedCount;
        wstring expectedPrimaryThumbprint = wformatString(expectedPrimaryCert->Thumbprint());
        Trace.WriteInfo(TraceType, "Verify GetPrincipalsDescriptionFromConfig primary cert: thumbprint {0}", expectedPrimaryThumbprint);

        VERIFY_IS_TRUE(itPrimary != principals->Users.end());
        VerifyUser(*itPrimary, expectedPrimaryAccountName, expectedPrimaryThumbprint);
    }
    else
    {
        VERIFY_IS_TRUE(itPrimary == principals->Users.end());
    }

    if (expectedSecondaryCert != NULL)
    {
        ++expectedCount;
        wstring expectedSecondaryThumbprint = wformatString(expectedSecondaryCert->Thumbprint());
        Trace.WriteInfo(TraceType, "Verify GetPrincipalsDescriptionFromConfig secondary cert: thumbprint {0}", expectedSecondaryThumbprint);

        VERIFY_IS_TRUE(itSecondary != principals->Users.end());
        VerifyUser(*itSecondary, expectedSecondaryAccountName, expectedSecondaryThumbprint);
    }
    else
    {
        VERIFY_IS_TRUE(itSecondary == principals->Users.end());
    }

    VERIFY_ARE_EQUAL(principals->Users.size(), expectedCount);
}

void DownloadManagerTestFSSSetup::TestFSSCommonNamePrincipals(
    vector<wstring> const & previousThumbprints,
    vector<InstallTestCertInScope *> const & certs,
    __out vector<wstring> & newThumbprints)
{
    VERIFY_IS_TRUE(Utility::HasAccountsConfigured());
    VERIFY_IS_TRUE(Utility::HasCommonNameAccountsConfigured());

    PrincipalsDescriptionUPtr principals;
    auto error = Management::FileStoreService::Utility::GetPrincipalsDescriptionFromConfigByCommonName(
        previousThumbprints,
        principals,
        newThumbprints);
    VERIFY_IS_TRUE(error.IsSuccess(), L"GetNTLMSecurityUsersByX509CommonNamesFromConfig with common name accounts failed");

    size_t expectedUserCount = certs.size();
    if (FileStoreServiceConfig::GetConfig().GenerateV1CommonNameAccount)
    {
        // For each certificate, generate 2 users, one with V1 and one with V2
        expectedUserCount *= 2;
    }
    
    Trace.WriteInfo(TraceType, "GetNTLMSecurityUsersByX509CommonNamesFromConfig returned {0} users, expected {1}", principals->Users.size(), expectedUserCount);
    VERIFY_ARE_EQUAL(principals->Users.size(), expectedUserCount);

    VERIFY_ARE_EQUAL(newThumbprints.size(), certs.size());

    for (auto it = certs.begin(); it != certs.end(); ++it)
    {
        // Found the user with the specified thumbprint
        wstring expectedThumbprint = wformatString((*it)->Thumbprint());
        Trace.WriteInfo(TraceType, "Verify user(s) with thumbprint {0}", expectedThumbprint);
        
        // Find v2 account
        DateTime expirationTime;
        VERIFY_IS_TRUE(CryptoUtility::GetCertificateExpiration((*it)->GetCertContextUPtr(), expirationTime).IsSuccess());
        wstring expectedAccountName = Common::AccountHelper::GenerateUserAcountName((*it)->Thumbprint(), expirationTime);
        VerifyUserInPricipal(expectedAccountName, expectedThumbprint, principals);

        if (FileStoreServiceConfig::GetConfig().GenerateV1CommonNameAccount)
        {
            wstring expectedAccountNameV1 = Common::AccountHelper::GenerateUserAcountNameV1((*it)->Thumbprint());
            VerifyUserInPricipal(expectedAccountNameV1, expectedThumbprint, principals);
        }

        auto itThumbprint = find_if(newThumbprints.begin(), newThumbprints.end(), [&expectedThumbprint](wstring const & t) {return t == expectedThumbprint; });
        VERIFY_IS_TRUE(itThumbprint != newThumbprints.end());
    }
}

void DownloadManagerTestFSSSetup::VerifyUserInPricipal(
    wstring const & expectedAccountName,
    wstring const & expectedThumbprint,
    PrincipalsDescriptionUPtr const & principals)
{
    auto itUser = find_if(principals->Users.begin(), principals->Users.end(), [&expectedAccountName](SecurityUserDescription const & user) {return user.Name == expectedAccountName; });
    bool principalFound = itUser != principals->Users.end();
    if (!principalFound)
    {
        Trace.WriteInfo(TraceType, "principal is not found for account {0}", expectedAccountName);
        for (auto principal = principals->Users.begin(); principal != principals->Users.end(); ++principal)
        {
            Trace.WriteInfo(TraceType, "Existing principals: {0}", principal->Name);
        }
    }

    VERIFY_IS_TRUE(principalFound);

    VerifyUser(*itUser, expectedAccountName, expectedThumbprint);
}

void DownloadManagerTestFSSSetup::CheckDownloadManagerNTLMUsers(
    __in DownloadManager & hosting,
    vector<wstring> const & expectedThumbprints,
    int const & expectedNewTokenCount,
    __inout AccessTokensCollection & tokenMap)
{
    auto ntlmThumbprints = hosting.Test_GetNtlmUserThumbprints();
    Trace.WriteInfo(
        TraceType,
        "Hosting returned {0} configured thumbprints",
        ntlmThumbprints.size());
  
    VERIFY_ARE_EQUAL(ntlmThumbprints.size(), expectedThumbprints.size());
    for (size_t i = 0; i < expectedThumbprints.size(); ++i)
    {
        wstring expectedThumbprint = expectedThumbprints[i];
        auto it = find_if(ntlmThumbprints.begin(), ntlmThumbprints.end(), [&expectedThumbprint](wstring const & user) { return user == expectedThumbprint; });
        VERIFY_IS_TRUE(it != ntlmThumbprints.end());
    }

    AccessTokensList newTokens;
    size_t originalTokenMapSize = tokenMap.size();
    auto error = Utility::GetAccessTokens(tokenMap, newTokens);
    Trace.WriteInfo(
        TraceType,
        "GetAccessTokens is {0}. Returned {1} tokens. expectedNewTokenCount is {2}. originalTokenSize: {3}.",
        error.IsSuccess(),
        newTokens.size(),
        expectedNewTokenCount,
        originalTokenMapSize);

    VERIFY_IS_TRUE(error.IsSuccess() || expectedNewTokenCount == 0);
    VERIFY_ARE_EQUAL(tokenMap.size(), originalTokenMapSize + expectedNewTokenCount);
    VERIFY_ARE_EQUAL(newTokens.size(), expectedNewTokenCount);
}

void DownloadManagerTestFSSSetup::CheckDownloadManagerNTLMUsersWithRetry(
    __in DownloadManager & hosting,
    vector<wstring> const & expectedThumbprints,
    int const & expectedNewTokenCount,
    __inout AccessTokensCollection & tokenMap)
{
    // Retry for max 2 min
    const int maxRetryCount = 120;
    for (int i = 0; i <= maxRetryCount; ++i)
    {
        auto ntlmThumbprints = hosting.Test_GetNtlmUserThumbprints();
        Trace.WriteInfo(
            TraceType,
            "Hosting returned {0} configured thumbprints, retryCount={1}",
            ntlmThumbprints.size(),
            i);

        VERIFY_IS_TRUE(ntlmThumbprints.size() <= expectedThumbprints.size());
        if (ntlmThumbprints.size() == expectedThumbprints.size())
        {
            // Reached target, verify
            CheckDownloadManagerNTLMUsers(hosting, expectedThumbprints, expectedNewTokenCount, tokenMap);
            return;
        }
        else if (i == maxRetryCount)
        {
            Trace.WriteWarning(
                TraceType,
                "CheckDownloadManagerNTLMUsersWithRetry: Hosting did not return the expected thumbprints {0}, maxRetryCount={1}, actual users {2}",
                expectedThumbprints,
                i,
                ntlmThumbprints);
            VERIFY_FAIL(L"CheckDownloadManagerNTLMUsersWithRetry failed validation");
        }

        Sleep(1000);
    }
}

void DownloadManagerTestFSSSetup::CleanUpFileStoreUsers()
{
    SecurityPrincipalHelper::DeleteUserAccount(L"P_FSSUserffffffff");
    SecurityPrincipalHelper::DeleteUserAccount(L"S_FSSUserffffffff");
}
