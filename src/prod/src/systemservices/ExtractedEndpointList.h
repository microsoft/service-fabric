// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{   
    class ExtractedEndpointList : public Common::IFabricJsonSerializable
    {
    public:
        ExtractedEndpointList();       

        static bool FromString(std::wstring const& serializedString, __out ExtractedEndpointList & extractedEndpointList);              

        bool GetFirstEndpoint(__out std::wstring & firstEndpoint) const;
        
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"Endpoints", endPointList_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::map<std::wstring, std::wstring> endPointList_;
    };
}
