// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "NodeIdGenerator.h"
#if !defined(PLATFORM_UNIX)
#include <wincrypt.h>
#endif

using namespace std;
using namespace Common;
using namespace Federation;

GlobalWString NodeIdGenerator::NodeIdNamePrefix = make_global<wstring>(L"nodeid:");
GlobalWString NodeIdGenerator::PrefixPearson = make_global<wstring>(L"UTzJ");
GlobalWString NodeIdGenerator::SuffixPearson = make_global<wstring>(L"X3if");

static const BYTE T[256] =
{
      1,  87,  49,  12, 176, 178, 102, 166, 121, 193,   6,  84, 249, 230,  44, 163,
     14, 197, 213, 181, 161,  85, 218,  80,  64, 239,  24, 226, 236, 142,  38, 200,
    110, 177, 104, 103, 141, 253, 255,  50,  77, 101,  81,  18,  45,  96,  31, 222,
     25, 107, 190,  70,  86, 237, 240,  34,  72, 242,  20, 214, 244, 227, 149, 235,
     97, 234,  57,  22,  60, 250,  82, 175, 208,   5, 127, 199, 111,  62, 135, 248,
    174, 169, 211,  58,  66, 154, 106, 195, 245, 171,  17, 187, 182, 179,   0, 243,
    132,  56, 148,  75, 128, 133, 158, 100, 130, 126,  91,  13, 153, 246, 216, 219,
    119,  68, 223,  78,  83,  88, 201,  99, 122,  11,  92,  32, 136, 114,  52,  10,
    138,  30,  48, 183, 156,  35,  61,  26, 143,  74, 251,  94, 129, 162,  63, 152,
    170,   7, 115, 167, 241, 206,   3, 150,  55,  59, 151, 220,  90,  53,  23, 131,
    125, 173,  15, 238,  79,  95,  89,  16, 105, 137, 225, 224, 217, 160,  37, 123,
    118,  73,   2, 157,  46, 116,   9, 145, 134, 228, 207, 212, 202, 215,  69, 229,
     27, 188,  67, 124, 168, 252,  42,   4,  29, 108,  21, 247,  19, 205,  39, 203,
    233,  40, 186, 147, 198, 192, 155,  33, 164, 191,  98, 204, 165, 180, 117,  76,
    140,  36, 210, 172,  41,  54, 159,   8, 185, 232, 113, 196, 231,  47, 146, 120,
     51,  65,  28, 144, 254, 221,  93, 189, 194, 139, 112,  43,  71, 109, 184, 209
};

wstring NodeIdGenerator::GenerateNodeName(NodeId nodeId)
{
    return *NodeIdNamePrefix + nodeId.ToString();
}

ErrorCode NodeIdGenerator::GenerateFromString(wstring const & input, NodeId & nodeId)
{
    wstring rolesForWhichToUseV1Generator = ServiceModel::ServiceModelConfig::GetConfig().NodeNamePrefixesForV1Generator;
    bool useV2NodeIdGenerator = ServiceModel::ServiceModelConfig::GetConfig().UseV2NodeIdGenerator;
    const wstring nodeIdGeneratorVersion = ServiceModel::ServiceModelConfig::GetConfig().NodeIdGeneratorVersion;
    return NodeIdGenerator::GenerateFromString(input, nodeId, rolesForWhichToUseV1Generator, useV2NodeIdGenerator, nodeIdGeneratorVersion);
}

ErrorCode NodeIdGenerator::GenerateFromString(wstring const & input, NodeId & nodeId, wstring const & rolesForWhichToUseV1generator, bool useV2NodeIdGenerator, wstring const & nodeIdGeneratorVersion)
{
    if (StringUtility::StartsWith<wstring>(input, *NodeIdNamePrefix))
    {
        wstring nodeIdString = input.substr(NodeIdNamePrefix->size());
        bool parseResult = NodeId::TryParse(nodeIdString, nodeId);
        return (parseResult ? ErrorCode::Success() : ErrorCodeValue::InvalidArgument);
    }

    if (!nodeIdGeneratorVersion.empty()
        && find(ServiceModel::Constants::ValidNodeIdGeneratorVersionList->begin(), ServiceModel::Constants::ValidNodeIdGeneratorVersionList->end(), nodeIdGeneratorVersion) == ServiceModel::Constants::ValidNodeIdGeneratorVersionList->end())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    ErrorCode errorCode = NodeIdGenerator::GenerateHashedNodeId(input, nodeId, nodeIdGeneratorVersion);
    if (!errorCode.IsSuccess())
    {
        return errorCode;
    }

    bool useV4NodeIdGenerator = !nodeIdGeneratorVersion.empty() && StringUtility::AreEqualCaseInsensitive(nodeIdGeneratorVersion, L"V4");

    size_t index = string::npos;
    if (useV4NodeIdGenerator)
    {
        wchar_t delims[] = { L'.', L'_' };
        index = input.find_last_of(delims, input.length(), _countof(delims));
    }
    else
    {
        index = input.find_last_of(L".");
    }

    if (index != string::npos)
    {
        if (useV4NodeIdGenerator)
        {
            return GenerateV4NodeId(input, nodeId, index);
        }

        bool useV1NodeIdGenerator = UseV1NodeIdGenerator(input.substr(0, index), rolesForWhichToUseV1generator);

        if (!nodeIdGeneratorVersion.empty() && (useV1NodeIdGenerator || useV2NodeIdGenerator))
        {
            return ErrorCodeValue::InvalidConfiguration;
        }

        // Generating the 8 bit mask based on the index.
        wstring instance = input.substr(index + 1);
        unsigned char mask8Bit = static_cast<unsigned char>(_wtoi(instance.c_str()));

        // Avoid bit-reversal for V3
        if (!StringUtility::AreEqualCaseInsensitive(nodeIdGeneratorVersion, L"V3"))
        {
            mask8Bit = (mask8Bit & 0x0f) << 4 | (mask8Bit & 0xf0) >> 4;
            mask8Bit = (mask8Bit & 0x33) << 2 | (mask8Bit & 0xcc) >> 2;
            mask8Bit = (mask8Bit & 0x55) << 1 | (mask8Bit & 0xaa) >> 1;
        }

        uint64 high;
        if (StringUtility::AreEqualCaseInsensitive(nodeIdGeneratorVersion, L"V3") || (useV2NodeIdGenerator && !useV1NodeIdGenerator))
        {
            BYTE prefixHash[16];
            errorCode = GenerateHash(input.substr(0, index), prefixHash, 16 * sizeof(BYTE), nodeIdGeneratorVersion);
            if (!errorCode.IsSuccess())
            {
                return errorCode;
            }

            // Map mask to 64 bit.
            uint64 empty32bitMask = _UI64_MAX >> 16;
            uint64 mask = static_cast<uint64>(mask8Bit);
            mask = mask << 48;
            uint64 maskPrefix = static_cast<uint64>(prefixHash[0]);
            maskPrefix = maskPrefix << 56;

            // Regenerate high component.
            high = (nodeId.IdValue.High & empty32bitMask) | mask | maskPrefix;
        }
        else
        {
            // Map mask to 64 bit.
            uint64 empty32bitMask = _UI64_MAX >> 8;
            uint64 mask = static_cast<uint64>(mask8Bit);
            mask = mask << 56;

            // Regenerate high component.
            high = (nodeId.IdValue.High & empty32bitMask) | mask;
        }

        nodeId = NodeId(LargeInteger(high, nodeId.IdValue.Low));
    }

    return ErrorCodeValue::Success;
}

bool NodeIdGenerator::UseV1NodeIdGenerator(wstring const & roleName, wstring const & rolesForWhichToUseV1Generator)
{
    if (rolesForWhichToUseV1Generator.empty())
    {
        return false;
    }

    StringCollection NodeNamePrefixesForV1Generator;
    StringUtility::Split<wstring>(rolesForWhichToUseV1Generator, NodeNamePrefixesForV1Generator, L",");
    for (wstring role : NodeNamePrefixesForV1Generator)
    {
        if (role == roleName)
        {
            return true;
        }
    }
    return false;
}

ErrorCode NodeIdGenerator::GenerateHash(wstring const & input, BYTE * hash, DWORD dwHashLen, wstring const & nodeIdGeneratorVersion)
{
    if (nodeIdGeneratorVersion.empty())
    {
        return GenerateMD5Hash(input, hash, dwHashLen);
    }
    else if (find(ServiceModel::Constants::ValidNodeIdGeneratorVersionList->begin(), ServiceModel::Constants::ValidNodeIdGeneratorVersionList->end(), nodeIdGeneratorVersion) != ServiceModel::Constants::ValidNodeIdGeneratorVersionList->end())
    {
        return GeneratePearsonHash(input, hash, dwHashLen);
    }

    return ErrorCodeValue::InvalidArgument;
}

// TODO: Another hash generator has been added based on Pearson algorithm; this one, at some point,
// needs to be deprecated; because, it uses MD5
ErrorCode NodeIdGenerator::GenerateMD5Hash(wstring const & input, BYTE * hash, DWORD dwHashLen)
{
#if !defined(PLATFORM_UNIX)
    HCRYPTPROV hCryptProv = NULL;
    HCRYPTHASH hHash = NULL;

    ErrorCode errorCode = ErrorCodeValue::Success;
    BOOL result = CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (result)
    {
        result = CryptCreateHash(hCryptProv, CALG_MD5, 0, 0, &hHash);
    }

    if (result)
    {
        result = CryptHashData(hHash, (BYTE *)input.c_str(), (DWORD)input.size() * sizeof(WCHAR), 0);
    }

    if (result)
    {
        result = CryptGetHashParam(hHash, HP_HASHVAL, hash, &dwHashLen, 0);
    }

    ErrorCode error = result ? ErrorCodeValue::Success : ErrorCode::FromWin32Error(::GetLastError());

    if (hHash)
    {
        CryptDestroyHash(hHash);
    }

    if (hCryptProv)
    {
        CryptReleaseContext(hCryptProv, 0);
    }

    return error;
#else
    UNREFERENCED_PARAMETER(input);
    UNREFERENCED_PARAMETER(hash);
    UNREFERENCED_PARAMETER(dwHashLen);

    Assert::CodingError("V1/V2 node id generation not supported on Linux");
#endif 
}

ErrorCode NodeIdGenerator::GeneratePearsonHash(wstring const & input, BYTE * hash, DWORD dWHashLen)
{
    // Currently only 128-bit hash is supported
    if (dWHashLen != 16)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    const wstring paddedInput = *PrefixPearson + input + *SuffixPearson;
    const size_t nBytes = paddedInput.size() * sizeof(wchar_t);
    const BYTE * const HEAD = (BYTE *)paddedInput.c_str();

    int i, j;
    BYTE hashedBytes[16];

    for (j = 0; j < 16; ++j)
    {
        const BYTE * b = HEAD;
        BYTE h = T[(*b + j) & 0xFF];
        for (i = 1; i < nBytes; ++i)
        {
            h = T[h ^ *++b];
        }

        hashedBytes[j] = h;
    }

    memcpy(hash, hashedBytes, 16);

    return ErrorCodeValue::Success;
}

// P = 73, M = 2^24, P_inv mod M = 14938617
// nodeId already contains hash of the entire node name
ErrorCode NodeIdGenerator::GenerateV4NodeId(wstring const & input, NodeId & nodeId, size_t index)
{
    ErrorCode errorCode = ErrorCodeValue::Success;

    wstring roleName = input.substr(0, index);
    wstring instance_s = input.substr(index + 1);

    int64 instance_i;
    if (!TryParseInt64(instance_s, instance_i) || instance_i < 0)
    {
        // ASSUMPTION: instance_s is either empty or a valid representation of an integer
        return ErrorCodeValue::Success;
    }

    NodeId roleNameHash;
    errorCode = GeneratePearsonHash(roleName, (BYTE *)&roleNameHash, sizeof(NodeId));
    uint64 lo = roleNameHash.IdValue.Low;
    uint64 mask = ~(~static_cast<uint64>(0) << 24);
    uint64 offset = lo & mask;
    uint64 instance_x = (offset + instance_i) & mask;
    uint64 instance_y = (instance_x * 14938617) & mask;
    uint64 hi = (nodeId.IdValue.High & 0x000000FFFFFFFFFF) | (instance_y << 40);
    nodeId = NodeId(LargeInteger(hi, nodeId.IdValue.Low));

    return errorCode;
}

ErrorCode NodeIdGenerator::GenerateHashedNodeId(wstring const & input, NodeId & nodeId, wstring const & nodeIdGeneratorVersion)
{
    return GenerateHash(input, (BYTE *)&nodeId, sizeof(NodeId), nodeIdGeneratorVersion);
}
