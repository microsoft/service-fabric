// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
	struct Serializer:
		Common::TextTraceComponent<Common::TraceTaskCodes::ServiceModel>
	{
	public:


		static Common::ErrorCode WriteServicePackage(
			std::wstring const & fileName,
			ServicePackageDescription & servicePackage);

		static Common::ErrorCode WriteServiceManifest(
			std::wstring const & fileName,
			ServiceManifestDescription & serviceManifest);


		static Common::ErrorCode WriteApplicationManifest(
			std::wstring const & fileName,
			ApplicationManifestDescription & applicationManifest);

		static Common::ErrorCode WriteApplicationPackage(
			std::wstring const & fileName,
			ApplicationPackageDescription & applicationPackage);


		static Common::ErrorCode WriteApplicationInstance(
			std::wstring const & fileName,
			ApplicationInstanceDescription & applicationInstance);



	private:
		template <typename ElementType>
		static Common::ErrorCode WriteElement(
			std::wstring const & fileName,
			std::wstring const & elementTypeName,
			ElementType & element);
	};
}
