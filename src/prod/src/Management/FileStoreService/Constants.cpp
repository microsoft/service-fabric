// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace FileStoreService;

GlobalWString Constants::FileStoreServicePrimaryCountName = make_global<wstring>(L"__FileStoreServicePrimaryCount__");
GlobalWString Constants::FileStoreServiceReplicaCountName = make_global<wstring>(L"__FileStoreServiceReplicaCount__");

GlobalWString Constants::FileStoreServiceUserGroup = make_global<wstring>(L"FSSGroup");
GlobalWString Constants::PrimaryFileStoreServiceUser = make_global<wstring>(L"P_FSSUser");
GlobalWString Constants::SecondaryFileStoreServiceUser = make_global<wstring>(L"S_FSSUser");

GlobalWString Constants::StoreRootDirectoryName = make_global<wstring>(L"Store");
GlobalWString Constants::StagingRootDirectoryName = make_global<wstring>(L"Staging");
GlobalWString Constants::StoreShareName= make_global<wstring>(L"StoreShare");
GlobalWString Constants::StagingShareName = make_global<wstring>(L"StagingShare");

GlobalWString Constants::DatabaseDirectory = make_global<wstring>(L"FS");
GlobalWString Constants::DatabaseFilename = make_global<wstring>(L"FS.edb");
GlobalWString Constants::SharedLogFilename = make_global<wstring>(L"fsshared.log");
GlobalWString Constants::SMBShareRemark = make_global<wstring>(L"WindowsFabric share");

GlobalWString Constants::LanmanServerParametersRegistryPath = make_global<wstring>(L"System\\CurrentControlSet\\Services\\LanmanServer\\Parameters");
GlobalWString Constants::NullSessionSharesRegistryValue = make_global<wstring>(L"NullSessionShares");

GlobalWString Constants::LsaRegistryPath = make_global<wstring>(L"SYSTEM\\CurrentControlSet\\Control\\Lsa");
GlobalWString Constants::EveryoneIncludesAnonymousRegistryValue = make_global<wstring>(L"EveryoneIncludesAnonymous");

GlobalWString Constants::StoreType::FileMetadata = make_global<wstring>(L"FileMetadata");
GlobalWString Constants::StoreType::LastDeletedVersionNumber = make_global<wstring>(L"LastDeletedVersionNumber");

GlobalWString Constants::Actions::OperationSuccess = make_global<wstring>(L"OperationSuccessAction");

GlobalWString Constants::ErrorReason::MissingActivityHeader = make_global<wstring>(L"Missing Activity Header");
GlobalWString Constants::ErrorReason::MissingTimeoutHeader = make_global<wstring>(L"Missing Timeout Header");
GlobalWString Constants::ErrorReason::IncorrectActor = make_global<wstring>(L"Incorrect Actor");
GlobalWString Constants::ErrorReason::InvalidAction = make_global<wstring>(L"Invalid Action");

double const Constants::StagingFileLockAcquireTimeoutInSeconds = 5.0;



