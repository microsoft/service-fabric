// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "TraceWrapper.Linux.h"
#include "TraceTemplates.Linux.h"

void TraceWrapperBinaryStructured(
    unsigned char *ProviderIdArg,
    unsigned short IdArg,
    unsigned char  VersionArg,
    unsigned char  ChannelArg,
    unsigned char  LevelArg,
    unsigned char  OpcodeArg,
    unsigned short TaskArg,
    unsigned long long KeywordArg,
    unsigned char *dataArg,
    unsigned long  dataNumBytesArg)
{
    switch (LevelArg)
    {
        case 0:
            tracepoint(service_fabric,
                    tracepoint_structured_silent,
                    ProviderIdArg,
                    IdArg,
                    VersionArg,
                    ChannelArg,
                    OpcodeArg,
                    TaskArg,
                    KeywordArg,
                    dataArg,
                    dataNumBytesArg);
            break;
        case 1:
            tracepoint(service_fabric,
                    tracepoint_structured_critical,
                    ProviderIdArg,
                    IdArg,
                    VersionArg,
                    ChannelArg,
                    OpcodeArg,
                    TaskArg,
                    KeywordArg,
                    dataArg,
                    dataNumBytesArg);
        case 2:
            tracepoint(service_fabric,
                    tracepoint_structured_error,
                    ProviderIdArg,
                    IdArg,
                    VersionArg,
                    ChannelArg,
                    OpcodeArg,
                    TaskArg,
                    KeywordArg,
                    dataArg,
                    dataNumBytesArg);
            break;
        case 3:
            tracepoint(service_fabric,
                    tracepoint_structured_warning,
                    ProviderIdArg,
                    IdArg,
                    VersionArg,
                    ChannelArg,
                    OpcodeArg,
                    TaskArg,
                    KeywordArg,
                    dataArg,
                    dataNumBytesArg);
            break;
        case 4:
            tracepoint(service_fabric,
                    tracepoint_structured_info,
                    ProviderIdArg,
                    IdArg,
                    VersionArg,
                    ChannelArg,
                    OpcodeArg,
                    TaskArg,
                    KeywordArg,
                    dataArg,
                    dataNumBytesArg);
            break;
        case 5:
            tracepoint(service_fabric,
                    tracepoint_structured_noise,
                    ProviderIdArg,
                    IdArg,
                    VersionArg,
                    ChannelArg,
                    OpcodeArg,
                    TaskArg,
                    KeywordArg,
                    dataArg,
                    dataNumBytesArg);
            break;
        case 99:
            tracepoint(service_fabric,
                    tracepoint_structured_all,
                    ProviderIdArg,
                    IdArg,
                    VersionArg,
                    ChannelArg,
                    OpcodeArg,
                    TaskArg,
                    KeywordArg,
                    dataArg,
                    dataNumBytesArg);
            break;
        default:
            printf("Unknown trace loglevel %d\n", LevelArg);
    }
}
