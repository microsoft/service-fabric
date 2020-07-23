// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#ifdef __cplusplus
extern "C" {
#endif
void TraceWrapper(
	const char * taskName,
	const char * eventName,
	int level,
	const char * id,
	const char * data);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
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
    unsigned long  dataNumBytesArg);

#ifdef __cplusplus
}
#endif
