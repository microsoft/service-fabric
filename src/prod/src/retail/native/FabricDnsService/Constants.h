// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    static const WCHAR DnsServiceTypeName[] = L"DnsServiceType";
    static const WCHAR DnsServiceName[] = L"DnsService";
    static const WCHAR DnsPortName[] = L"DnsPort";
    static const WCHAR NumberOfConcurrentQueriesName[] = L"NumberOfConcurrentQueries";
    static const WCHAR MaxMessageSizeInKBName[] = L"MaxMessageSizeInKB";

    static const ULONG DEFAULT_MESSAGE_SIZE_KB = 8;
    static const ULONG DEFAULT_NUMBER_OF_CONCURRENT_QUERIES = 100;
    static const USHORT DEFAULT_PORT = 53;

    static const WCHAR FabricSchemeName[] = L"fabric:";
}
