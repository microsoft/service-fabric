// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace HostType
    {
        enum Enum
        {
            Invalid = 0,
            ExeHost = 1,
            ContainerHost = 2,
			FirstValidEnum = Invalid,
			LastValidEnum = ContainerHost
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
		
		::FABRIC_HOST_TYPE ToPublicApi(Enum const & val);
        Common::ErrorCode FromPublicApi(FABRIC_HOST_TYPE const & publicVal, __out Enum & val);

        Enum FromEntryPointType(EntryPointType::Enum const & entryPointType);

		DECLARE_ENUM_STRUCTURED_TRACE(HostType)

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(ExeHost)
            ADD_ENUM_VALUE(ContainerHost)
        END_DECLARE_ENUM_SERIALIZER()

    }
}
