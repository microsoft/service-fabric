// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;

Common::StringLiteral const TraceType("CertificateManagerTest");

namespace Common
{
	class CertificateManagerTest
	{
	protected:
		static StringLiteral const TestSource;
		bool VerifyInStore(wstring subName, wstring store, wstring flag);
        bool VerifyFile(wstring path);
        wstring GetDirectory();
	};

	StringLiteral const CertificateManagerTest::TestSource("CertificateManagerTest");

	BOOST_FIXTURE_TEST_SUITE(CertificateManagerSuite, CertificateManagerTest);

	BOOST_AUTO_TEST_CASE(VerifyCertInStoreWithExpireDateNoDNS)
	{
		Trace.WriteInfo(CertificateManagerTest::TestSource, "*** Verify if Cert exists in Store");
		wstring subName = L"TestCert_1";
		wstring store = L"ROOT";
		wstring profile = L"M";
		SYSTEMTIME dt;
		GetSystemTime(&dt);
		dt.wDay += 1;
		FILETIME ft;
		SystemTimeToFileTime(&dt, &ft);
		DateTime dtExpire(ft);
		CertificateManager *cf = new CertificateManager(subName,dtExpire);
		auto error = cf->ImportToStore(store,profile);
		VERIFY_IS_TRUE(error.IsSuccess());
		VERIFY_IS_TRUE_FMT(VerifyInStore(subName,store,profile), "No certificate found with given subject name {0}",subName);
	}

	BOOST_AUTO_TEST_CASE(VerifyCertInStoreWithDNSNoExpireDate)
	{
		Trace.WriteInfo(CertificateManagerTest::TestSource, "*** Verify if Cert exists in Store");
		wstring subName = L"TestCert_2";
		wstring DNS = L"client.sf.lrc.com";
		wstring store = L"ROOT";
		wstring profile = L"M";
		CertificateManager *cf = new CertificateManager(subName, DNS);
		auto error = cf->ImportToStore(store, profile);
		VERIFY_IS_TRUE(error.IsSuccess());
		VERIFY_IS_TRUE_FMT(VerifyInStore(subName, store, profile), "No certificate found with given subject name {0}", subName);
	}

	BOOST_AUTO_TEST_CASE(VerifyCertPFXNoDNSNoDate)
	{
		Trace.WriteInfo(CertificateManagerTest::TestSource, "*** Verify if Cert generated as PFX");
		wstring subName = L"TestCert_3";
		wstring path = L"TestCert_3.pfx";
		CertificateManager *cf = new CertificateManager(subName);	
		SecureString passWord(L"");
		auto error = cf->SaveAsPFX(path, passWord);	
		VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE_FMT(VerifyFile(path), "Certificate with given subject name {0} could not be opened", subName);
	}
	

	BOOST_AUTO_TEST_CASE(VerifyCertDeletion)
	{
		Trace.WriteInfo(CertificateManagerTest::TestSource, "*** Verify if Cert deletes from store");
		wstring subName = L"TestCert_4";
		wstring store = L"ROOT";
		wstring profile = L"M";
		CertificateManager *cf = new CertificateManager(subName);
		auto error = cf->ImportToStore(store, profile);
		VERIFY_IS_TRUE(error.IsSuccess());
		error = CertificateManager::DeleteCertFromStore(L"TestCert", store, profile, false);
		VERIFY_IS_TRUE(error.IsSuccess());
	}

    BOOST_AUTO_TEST_CASE(VerifyPFXGeneration)
    {
        Trace.WriteInfo(CertificateManagerTest::TestSource, "Verify if PFX is generated for Alice cert");

        wstring path = GetDirectory();
        Trace.WriteInfo(CertificateManagerTest::TestSource, "Current Directory Path {0} ", path);
        wstring PFXfileName = path + L"\\certificate.pfx";
        SecureString passWord(L"");
        std::vector<wstring> accountNamesForACL = { L"SYSTEM", L"Administrators"} ;

        auto error = CertificateManager::GenerateAndACLPFXFromCer(
            X509StoreLocation::LocalMachine,
            L"My",
            L"ad fc 91 97 13 16 8d 9f a8 ee 71 2b a2 f4 37 62 00 03 49 0d",
            PFXfileName,
            passWord,
            accountNamesForACL);

        VERIFY_IS_TRUE(error.IsSuccess());
    }

	BOOST_AUTO_TEST_SUITE_END();

	bool CertificateManagerTest::VerifyInStore(wstring subName, wstring store, wstring flag)
	{
		HCERTSTORE hStore = NULL;
		PCCERT_CONTEXT pTargetCert = NULL;
		LPCTSTR store_w = store.c_str();
		
		if (flag == L"U")
			hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER, store_w);
		else if (flag == L"M")
			hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_LOCAL_MACHINE, store_w);
		
		//Enumerate over all certs and retreive those which match subName
		pTargetCert = CertEnumCertificatesInStore(hStore, pTargetCert);
		while (pTargetCert)
		{
			DWORD cbSize;
			//Gets size in cbSize
			cbSize = CertGetNameString(pTargetCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
			if (!cbSize)
			{
				return false;
			}

			//Provide cbSize to get cert name in pszName
			vector<wchar_t> buffer(cbSize, 0);
			wchar_t *pszName = &buffer[0];
			if (!CertGetNameString(pTargetCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, pszName, cbSize))
			{
				return false;
			}

			if (subName.compare(pszName) == 0)
			{
				CertDuplicateCertificateContext(pTargetCert);
				if (!CertDeleteCertificateFromStore(pTargetCert))
				{
					return false;
				}
			}

			pTargetCert = CertEnumCertificatesInStore(hStore, pTargetCert);
		}

		CertFreeCertificateContext(pTargetCert);
		return true;
	}

    bool CertificateManagerTest::VerifyFile(wstring path)
	{
		File f;
		if (f.TryOpen(path).IsSuccess())
		{
			f.Close();
			File::Delete(path);
			return true;
		}
		else
		{
			return false;
		}
	}

    wstring CertificateManagerTest::GetDirectory()
    {
        wstring path;
        auto winSize = GetCurrentDirectory(0, NULL);
        auto stlSize = static_cast<size_t>(winSize);
        path.resize(stlSize);
        GetCurrentDirectory(winSize, const_cast<wchar_t *>(path.data()));
        path.resize(winSize - 1);
        return path;
    }
}
   
