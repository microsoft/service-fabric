// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace UpgradeType
    {
        enum Enum
        {
            Invalid = 0,
            Rolling = 1,
            Rolling_ForceRestart = 2,
            Rolling_NotificationOnly = 3,

            LastValidEnum = Rolling_NotificationOnly
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        DECLARE_ENUM_STRUCTURED_TRACE( UpgradeType )

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Rolling)
            ADD_ENUM_VALUE(Rolling_ForceRestart)
            ADD_ENUM_VALUE(Rolling_NotificationOnly)
            ADD_ENUM_VALUE(LastValidEnum)
        END_DECLARE_ENUM_SERIALIZER()
    };
}
