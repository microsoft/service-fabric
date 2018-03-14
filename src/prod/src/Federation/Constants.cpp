// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;

GlobalWString Constants::SeedNodeVoteType = make_global<wstring>(L"SeedNode");
GlobalWString Constants::SqlServerVoteType = make_global<wstring>(L"SqlServer");
GlobalWString Constants::WindowsAzureVoteType = make_global<wstring>(L"WindowsAzure");

GlobalWString Constants::GlobalTimestampEpochName = make_global<wstring>(L"GlobalTimestampEpoch");

char const* Constants::RejectedTokenKey = "RejectedTokens";
char const* Constants::NeighborHeadersIgnoredKey = "NeighborHeadersIgnored";

StringLiteral Constants::VersionTraceType("Version");

