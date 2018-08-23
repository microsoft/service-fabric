// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// ************************************************************************
// This is a generated file.
// See src\prod\src\scripts\preprocess_config_macros.pl
//
// Do not modify it manually, but include it in your commit if it's updated
// by the build.
// ************************************************************************

INTERNAL_CONFIG_ENTRY(bool, L"RepairManager/SharedLog", EnableHashSharedLogIdFromPath, false, Common::ConfigEntryUpgradePolicy::Static)
INTERNAL_CONFIG_ENTRY(uint, L"RepairManager/SharedLog", CreateFlags, 0, Common::ConfigEntryUpgradePolicy::Static)
INTERNAL_CONFIG_ENTRY(int, L"RepairManager/SharedLog", MaximumRecordSize, 0, Common::ConfigEntryUpgradePolicy::Static)
INTERNAL_CONFIG_ENTRY(int, L"RepairManager/SharedLog", MaximumNumberStreams, 24 * 1024, Common::ConfigEntryUpgradePolicy::Static)
INTERNAL_CONFIG_ENTRY(int64, L"RepairManager/SharedLog", LogSize, 1 * 1024 * 1024 * 1024, Common::ConfigEntryUpgradePolicy::Static)
INTERNAL_CONFIG_ENTRY(std::wstring, L"RepairManager/SharedLog", ContainerId, L"", Common::ConfigEntryUpgradePolicy::Static)
INTERNAL_CONFIG_ENTRY(std::wstring, L"RepairManager/SharedLog", ContainerPath, L"", Common::ConfigEntryUpgradePolicy::Static)
