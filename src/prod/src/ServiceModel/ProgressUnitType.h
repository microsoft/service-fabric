// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ProgressUnitType
    {
        enum Enum
        {
            Invalid = 0,
            Bytes = 1,
            ServiceSubPackages = 2,
            Files = 3,

            LastValidEnum = Files
        };

        FABRIC_PROGRESS_UNIT_TYPE ToPublicApi(Enum val);
        Enum FromPublicApi(FABRIC_PROGRESS_UNIT_TYPE val);

        void WriteToTextWriter(__in Common::TextWriter & w, Enum val);

        DECLARE_ENUM_STRUCTURED_TRACE( ProgressUnitType )
    };
}
