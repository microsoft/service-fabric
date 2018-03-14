// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class NodeIdGenerator
    {
    public:
        static Common::ErrorCode GenerateFromString(std::wstring const & string, NodeId & nodeId, std::wstring const & rolesForWhichToUseV1generator, bool useV2NodeIdGenerator, std::wstring const & nodeIdGeneratorVersion);
        static Common::ErrorCode GenerateFromString(std::wstring const & string, NodeId & nodeId);
        static std::wstring GenerateNodeName(NodeId nodeId);
        static Common::GlobalWString NodeIdNamePrefix;
        static Common::GlobalWString PrefixPearson;
        static Common::GlobalWString SuffixPearson;

    private:
        static Common::ErrorCode GenerateHashedNodeId(std::wstring const & string, NodeId & nodeId, std::wstring const & nodeIdGeneratorVersion);

        static Common::ErrorCode GenerateHash(std::wstring const & string, BYTE * hash, DWORD dWHashLen, std::wstring const & nodeIdGeneratorVersion);
        static Common::ErrorCode GenerateMD5Hash(std::wstring const & string, BYTE * hash, DWORD dWHashLen);
        static Common::ErrorCode GeneratePearsonHash(std::wstring const & string, BYTE * hash, DWORD dWHashLen);
        static bool UseV1NodeIdGenerator(std::wstring const & roleName, std::wstring const & rolesForWhichToUseV1generator);

        static Common::ErrorCode GenerateV4NodeId(std::wstring const & string, NodeId & nodeId, size_t index);
    };
}
