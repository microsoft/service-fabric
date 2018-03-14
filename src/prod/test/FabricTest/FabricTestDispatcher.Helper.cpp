// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace std;
using namespace Common;
using namespace TestCommon;

void FabricTestDispatcherHelper::AddRepairCommands(__in TestFabricClient & fabricClient, __inout std::map<std::wstring, CommandHandler> & commandHandlers)
{
	commandHandlers[FabricTestCommands::CreateRepairCommand] = [&](StringCollection const & paramCollection)
	{
		return fabricClient.CreateRepair(paramCollection);
	};
	commandHandlers[FabricTestCommands::CancelRepairCommand] = [&](StringCollection const & paramCollection)
	{
		return fabricClient.CancelRepair(paramCollection);
	};
	commandHandlers[FabricTestCommands::ForceApproveRepairCommand] = [&](StringCollection const & paramCollection)
	{
		return fabricClient.ForceApproveRepair(paramCollection);
	};
	commandHandlers[FabricTestCommands::DeleteRepairCommand] = [&](StringCollection const & paramCollection)
	{
		return fabricClient.DeleteRepair(paramCollection);
	};
	commandHandlers[FabricTestCommands::UpdateRepairCommand] = [&](StringCollection const & paramCollection)
	{
		return fabricClient.UpdateRepair(paramCollection);
	};
	commandHandlers[FabricTestCommands::ShowRepairsCommand] = [&](StringCollection const & paramCollection)
	{
		return fabricClient.ShowRepairs(paramCollection);
	};
	commandHandlers[FabricTestCommands::UpdateRepairHealthPolicyCommand] = [&](StringCollection const & paramCollection)
	{
		return fabricClient.UpdateRepairHealthPolicy(paramCollection);
	};
	commandHandlers[FabricTestCommands::GetRepairCommand] = [&](StringCollection const & paramCollection)
	{
		return fabricClient.GetRepair(paramCollection);
	};
}
