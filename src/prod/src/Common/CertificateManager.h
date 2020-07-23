// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace std;

namespace Common
{
	class CertificateManager
	{
		
		public:
			CertificateManager(wstring subName);
			CertificateManager(wstring subName, wstring DNS);
			CertificateManager(wstring subName, DateTime expireDate);
			CertificateManager(wstring subName, wstring DNS, DateTime expireDate);
			
			ErrorCode ImportToStore(wstring storeName, wstring profile);
			ErrorCode SaveAsPFX(wstring fileName, SecureString pwd);
			static ErrorCode DeleteCertFromStore(wstring certName, wstring storeName, wstring profile, bool exactMatch);
            static ErrorCode GenerateAndACLPFXFromCer(X509StoreLocation::Enum cerStoreLocation, wstring x509StoreName, wstring x509Thumbprint, wstring pfxFilePath, SecureString password, vector<wstring> machineAccountNamesForACL);
			~CertificateManager();

		private:
			DENY_COPY(CertificateManager);

			ErrorCode CheckContainerAndGenerateKey();
			ErrorCode GetSidOfCurrentUser(wstring &sid);
			ErrorCode GenerateSelfSignedCertificate();

            static ErrorCode SavePFXBlobInFile(CRYPT_DATA_BLOB pfxBlob, wstring PFXfileName);
            static ErrorCode ACLPFXFile(vector<wstring> machineAccountNamesForACL, wstring PFXfileName);

			wstring subjectName_;
			wstring DNS_;
			SYSTEMTIME expirationDate_;
			PCCERT_CONTEXT certContext_ ;					
	};
}


