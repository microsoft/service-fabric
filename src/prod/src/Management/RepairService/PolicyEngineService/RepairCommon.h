// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    Common::StringLiteral const ComponentName("RepairPolicyEngine");

    std::wstring const RepairTypeNone(L"None");

    std::wstring const RepairTypeDefault(L"Default");

    enum NodePolicyState
    {
        AnyHealthState = 0, // equivalent to null/not specified - until nullable concept is used
        UnknownHealthState = 1, 
        Healthy = 2,
        Failing = 3, 
        Probation = 4
    };
    
    enum RepairStateTransition
    {
        NotStartedToCreatingRepair,
        CreatingRepairToCreatedRepair,
        RepairCompleted,
        RepairCancelled,
    };

    enum RepairProgressState
    {
        NotStarted = 0,
        CreatingRepair = 1, 
        CreatedRepair = 2,
    };


    class EnumToString
    {
    public:
        static std::wstring NodePolicyStateToString(NodePolicyState nodePolicyState)
        {
            switch (nodePolicyState)
            {
            case NodePolicyState::AnyHealthState:
                return L"AnyHealthState";
            case NodePolicyState::UnknownHealthState:
                return L"UnknownHealthState";
            case NodePolicyState::Healthy:
                return L"Healthy";
            case NodePolicyState::Failing:
                return L"Failing";
            case NodePolicyState::Probation:
                return L"Probation";
            default:
                Common::Assert::CodingError("Translation of NodePolicyState=={0} not implemented in NodePolicyStateToString(NodePolicyState nodePolicyState)", (LONG)nodePolicyState);
            }
        };

        static std::wstring RepairProgressStateToString(RepairProgressState repairProgressState)
        {
            switch (repairProgressState)
            {
            case RepairProgressState::NotStarted:
                return L"RepairProgressState::NotStarted";
            case RepairProgressState::CreatingRepair:
                return L"RepairProgressState::CreatingRepair";
            case RepairProgressState::CreatedRepair:
                return L"RepairProgressState::CreatedRepair";
            default:
                Common::Assert::CodingError("Translation of RepairProgressState=={0} not implemented in RepairProgressStateToString(RepairProgressState repairProgressState)", (LONG)repairProgressState);
            }
        };

        static HRESULT StringToNodePolicyState(std::wstring nodeRepairStateW, __out NodePolicyState& nodePolicyState)
        {
            if (Common::StringUtility::AreEqualCaseInsensitive(nodeRepairStateW, L"AnyHealthState"))
            {
                nodePolicyState = NodePolicyState::AnyHealthState;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(nodeRepairStateW, L"UnknownHealthState"))
            {
                nodePolicyState = NodePolicyState::UnknownHealthState;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(nodeRepairStateW, L"Healthy"))
            {
                nodePolicyState = NodePolicyState::Healthy;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(nodeRepairStateW, L"Failing"))
            {
                nodePolicyState = NodePolicyState::Failing;
            }
            else if (Common::StringUtility::AreEqualCaseInsensitive(nodeRepairStateW, L"Probation"))
            {
                nodePolicyState = NodePolicyState::Probation;
            }
            else
            {
                // Don't fail here as we need to support future upgrades of the service that might add new values
                nodePolicyState = NodePolicyState::Healthy;
                return E_INVALIDARG;
            }

            return S_OK;
        };

        static std::wstring RepairStateTransitionToString(RepairStateTransition repairStateTransition)
        {
            switch (repairStateTransition)
            {
            case RepairStateTransition::NotStartedToCreatingRepair:
                return L"RepairStateTransition::NotStartedToCreatingRepair";
            case RepairStateTransition::CreatingRepairToCreatedRepair:
                return L"RepairStateTransition::CreatingRepairToCreatedRepair";
            case RepairStateTransition::RepairCompleted:
                return L"RepairStateTransition::RepairCompleted";
            case RepairStateTransition::RepairCancelled:
                return L"RepairStateTransition::RepairCancelled";
            default:
                Common::Assert::CodingError("Translation of RepairStateTransition=={0} not implemented in RepairStateTransitionToString(RepairStateTransition repairStateTransition)", (LONG)repairStateTransition);
            }
        };
    };
}
