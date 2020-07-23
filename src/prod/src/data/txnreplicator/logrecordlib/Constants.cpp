// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;

ULONG const Constants::InvalidRecordLength = MAXULONG;
ULONG64 const Constants::InvalidRecordPosition = MAXULONG64;
ULONG64 const Constants::InvalidPhysicalRecordOffset = MAXULONG64;
ULONG64 const Constants::InvalidLogicalRecordOffset = MAXULONG64;
ULONG64 const Constants::NumberLogRecordTypes = 14;
LONG64 const Constants::InvalidLsn = -1;
LONG64 const Constants::MaxLsn = MAXLONG64;
LONG64 const Constants::InvalidPsn = -1;
LONG64 const Constants::OneLsn = 1;
LONG64 const Constants::ZeroLsn = 0;
LONG64 const Constants::InvalidReplicaId = -1;
LONG64 const Constants::UniversalReplicaId = 0;
LONG64 const Constants::NullOperationDataCode = -1;
LONG64 const Constants::PhysicalLogWriterSlowFlushDurationInMs = 3000;
LONG64 const Constants::ProgressVectorMaxStringSizeInKb = 60;

wstring const Constants::StartingJSON = L"{\n";
wstring const Constants::Quote = L"\"";
wstring const Constants::DivisionJSON = L"\" : \"";
wstring const Constants::DivisionBoolJSON = L"\" : ";
wstring const Constants::EndlJSON = L"\",\n\"";
wstring const Constants::CompEndlJSON = L",\n\"";
wstring const Constants::CloseJSON = L"\n}";

ULONG const Constants::KGuid_KString_Length = 40;

Common::StringLiteral const Constants::SFLogSuffix(".SFLog");
Common::StringLiteral const Constants::Test_EmptyString("");

#if !defined(PLATFORM_UNIX) 
LPCWSTR const Constants::LogPathPrefix = L"\\??\\";
#else
LPCWSTR const Constants::LogPathPrefix = L"";
#endif

std::wstring const Constants::Test_Ktl_LoggingEngine(L"ktl");
std::wstring const Constants::Test_File_LoggingEngine(L"file");
std::wstring const Constants::SerialDispatchingMode(L"serial");

ULONG const Constants::PhysicalLogWriterMovingAverageHistory = 10;

std::wstring const Constants::SlowPhysicalLogWriteOperationName = L"Log Write I/O";
std::wstring const Constants::SlowPhysicalLogReadOperationName = L"Log Read I/O";
LONG64 const Constants::BytesInKBytes = 1024;

std::wstring const Constants::FabricHostApplicationDirectory = L"Fabric_Folder_Application_OnHost";
