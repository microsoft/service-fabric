// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace AccessControl
{
    namespace ResourceType 
    {
        enum Enum
        {
            Invalid = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_INVALID,
            PathInImageStore = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_PATH_IN_FABRIC_IMAGESTORE,
            ApplicationType = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION_TYPE,
            Application = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION,
            Service = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_SERVICE,
            Name = FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_NAME,
        };

        bool IsValid(Enum value);
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(ResourceType);
    }
}
