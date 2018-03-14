// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceType_Serializer("Serializer");

ErrorCode Serializer::WriteServicePackage(
	wstring const & fileName,
	ServicePackageDescription & servicePackage)
{
	return WriteElement<ServicePackageDescription>(
		fileName,
		L"ServicePackage",
		servicePackage);
}

ErrorCode Serializer::WriteServiceManifest(
	wstring const & fileName,
	ServiceManifestDescription & serviceManifest)
{
	return WriteElement<ServiceManifestDescription>(
		fileName,
		L"ServiceManifest",
		serviceManifest);
}



ErrorCode Serializer::WriteApplicationManifest(
	std::wstring const & fileName,
	ApplicationManifestDescription & applicationManifest)
{
	return WriteElement<ApplicationManifestDescription>(
		fileName,
		L"ApplicationManifest",
		applicationManifest);
}

ErrorCode Serializer::WriteApplicationPackage(
	std::wstring const & fileName,
	ApplicationPackageDescription & applicationPackage)
{
	auto error = WriteElement<ApplicationPackageDescription>(
		fileName,
		L"ApplicationPackage",
		applicationPackage);


	return error;
}



ErrorCode Serializer::WriteApplicationInstance(
	std::wstring const & fileName,
	ApplicationInstanceDescription & applicationInstance)
{
	return WriteElement<ApplicationInstanceDescription>(
		fileName,
		L"ApplicationInstance",
		applicationInstance);
}




template <typename ElementType>
ErrorCode Serializer::WriteElement(
	wstring const & fileName,
	wstring const & elementTypeName,
	ElementType & element)
{
	auto error = ErrorCode(ErrorCodeValue::Success);

	XmlWriterUPtr xmlWriter;
	error = XmlWriter::Create(fileName, xmlWriter);
	if (error.IsSuccess())
	{
		 error = element.WriteToXml(xmlWriter);
	}
	if (!error.IsSuccess())
	{
		WriteWarning(
			TraceType_Serializer,
			"Write{0}: ErrorCode={1}, FileName={2}",
			elementTypeName,
			error,
			fileName);
	}
	return error;
}



