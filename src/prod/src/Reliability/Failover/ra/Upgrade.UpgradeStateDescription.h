// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            class UpgradeStateDescription
            {
                DENY_COPY(UpgradeStateDescription);

                enum StateType
                {
                    Normal,
                    NormalWithAsyncApi,
                    TimerWithConstantRetryInterval,
                    TimerWithDynamicRetryInterval
                };

            public:
                typedef std::function<Common::TimeSpan(Reliability::FailoverConfig const&)> RetryIntervalFuncPtr;

                UpgradeStateDescription(
                    UpgradeStateName::Enum name,
                    UpgradeStateCancelBehaviorType::Enum cancelBehaviorType,
                    bool isAsyncApiState = false)
                : name_(name),
                  retryInterval_(Common::TimeSpan::MaxValue),
                  targetState_(UpgradeStateName::Invalid),
                  cancelBehaviorType_(cancelBehaviorType),
                  type_(isAsyncApiState ? NormalWithAsyncApi : Normal)
                {
                }

                UpgradeStateDescription(
                    UpgradeStateName::Enum name, 
                    Common::TimeSpan retryInterval, 
                    UpgradeStateName::Enum targetStateOnTimer,
                    UpgradeStateCancelBehaviorType::Enum cancelBehaviorType)    
                : name_(name),
                  retryInterval_(retryInterval),
                  targetState_(targetStateOnTimer),
                  cancelBehaviorType_(cancelBehaviorType),
                  type_(TimerWithConstantRetryInterval)
                {
                    ASSERT_IF(retryInterval == Common::TimeSpan::MaxValue, "RetryInterval cannot be max value");
                }

                UpgradeStateDescription(
                    UpgradeStateName::Enum name,
                    RetryIntervalFuncPtr retryIntervalFuncPtr,
                    UpgradeStateName::Enum targetStateOnTimer,
                    UpgradeStateCancelBehaviorType::Enum cancelBehaviorType)
                : name_(name),
                  retryInterval_(Common::TimeSpan::MaxValue),
                  targetState_(targetStateOnTimer),
                  cancelBehaviorType_(cancelBehaviorType),
                  retryIntervalFuncPtr_(retryIntervalFuncPtr),
                  type_(TimerWithDynamicRetryInterval)
                {
                    ASSERT_IF(retryIntervalFuncPtr_ == nullptr, "Cannot be null");
                }

                __declspec(property(get=get_Name)) UpgradeStateName::Enum Name;
                UpgradeStateName::Enum get_Name() const { return name_; }

                __declspec(property(get=get_IsTimer)) bool IsTimer;
                bool get_IsTimer() const { return type_ == TimerWithConstantRetryInterval || type_ == TimerWithDynamicRetryInterval; }

                __declspec(property(get = get_IsAsyncApiState)) bool IsAsyncApiState;
                bool get_IsAsyncApiState() const { return type_ == NormalWithAsyncApi; }

                __declspec(property(get=get_TargetStateOnTimer)) UpgradeStateName::Enum TargetStateOnTimer;
                UpgradeStateName::Enum get_TargetStateOnTimer() const { return targetState_; }

                __declspec(property(get = get_TargetStateOnFailure)) UpgradeStateName::Enum TargetStateOnFailure;
                UpgradeStateName::Enum get_TargetStateOnFailure() const { return targetState_; }

                __declspec(property(get = get_CancelBehaviorType)) UpgradeStateCancelBehaviorType::Enum CancelBehaviorType;
                UpgradeStateCancelBehaviorType::Enum get_CancelBehaviorType() const { return cancelBehaviorType_; }

                Common::TimeSpan GetRetryInterval(Reliability::FailoverConfig const & cfg) const
                {
                    ASSERT_IF(!IsTimer, "Must be timer");
                    if (type_ == TimerWithConstantRetryInterval)
                    {
                        return retryInterval_;
                    }
                    else
                    {
                        return retryIntervalFuncPtr_(cfg);
                    }
                }

            private:
                UpgradeStateName::Enum const name_;
                
                UpgradeStateCancelBehaviorType::Enum const cancelBehaviorType_;
                
                StateType const type_;

                UpgradeStateName::Enum const targetState_;
                Common::TimeSpan const retryInterval_;
                mutable RetryIntervalFuncPtr retryIntervalFuncPtr_;
            };
        }           
    }
}

