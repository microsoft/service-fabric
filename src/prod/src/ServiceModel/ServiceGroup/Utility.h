// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceGroup
{
    class Utility
    {
    public:
        static bool ExtractMemberServiceLocation(
            Naming::ResolvedServicePartitionSPtr const & serviceGroupLocation,
            std::wstring const & serviceName,
            __out Naming::ResolvedServicePartitionSPtr & serviceMemberLocation);

        static bool ParseServiceGroupAddress(
            std::wstring const & serviceGroupAddress,
            std::wstring const & pattern,
            __out std::wstring & memberServiceAddress); 

        static bool TryConvertToServiceGroupMembers(
            Reliability::ServiceTableEntry const & ste,
            __out std::vector<Reliability::ServiceTableEntrySPtr> & results);

        static bool TryExtractServiceGroupMemberAddresses(
            std::wstring const & serviceGroupAddress,
            __out std::map<std::wstring, std::wstring> & memberServiceAddresses);
    };
}
