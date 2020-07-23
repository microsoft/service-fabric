// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace NetworkType
    {
        enum Enum
        {
            Other = 0, 
            Open = 1,
            Isolated = 2
        };

        inline Enum operator|(Enum a, Enum b)
        {
            return static_cast<Enum>(static_cast<int>(a) | static_cast<int>(b));
        }

        inline Enum operator&(Enum a, Enum b)
        {
            return static_cast<Enum>(static_cast<int>(a) & static_cast<int>(b));
        }

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        std::wstring EnumToString(ServiceModel::NetworkType::Enum val);
        Common::ErrorCode FromString(std::wstring const & value, __out Enum & val);
        FABRIC_CONTAINER_NETWORK_TYPE ToPublicApi(Enum networkType);
        bool IsMultiNetwork(std::wstring const & networkingMode);

        static std::wstring const NetworkSeparator = L";";
        static std::wstring const OtherStr = L"Other";
        static std::wstring const OpenStr = L"Open";
        static std::wstring const IsolatedStr = L"Isolated";
    };
}
