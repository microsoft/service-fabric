// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
	namespace HostIsolationMode
	{
		enum Enum
		{
			None = 0,
			Process = 1,
			HyperV = 2,
			FirstValidEnum = None,
			LastValidEnum = HyperV
		};

		void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

		::FABRIC_HOST_ISOLATION_MODE ToPublicApi(Enum const & val);
		Common::ErrorCode FromPublicApi(FABRIC_HOST_ISOLATION_MODE const & publicVal, __out Enum & val);

        HostIsolationMode::Enum FromContainerIsolationMode(ServiceModel::ContainerIsolationMode::Enum val);

		DECLARE_ENUM_STRUCTURED_TRACE(HostIsolationMode)

		BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
			ADD_ENUM_VALUE(None)
			ADD_ENUM_VALUE(Process)
			ADD_ENUM_VALUE(HyperV)
		END_DECLARE_ENUM_SERIALIZER()

	}
}
