// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            // This class is used in RAFailoverUnit to identify the membership in a FT Set
            // It allows adding and removing from a set in a undo-able manner by
            // adding Actions to the StateMachineActionQueue
            //
            // The usage is that when you set a flag 
            // (say the flag that indicates LocalReplicaUpMessageReplyPending)
            // you actually set the local bool and generate an action that adds to the FTSet object
            // NOTE: The lifetime of the FTSet itself is not the same as the lifetime of the FT
            // This also enables checking set membership in O(1) if you have a LockedFT
            class SetMembershipFlag
            {
            public:
                SetMembershipFlag(EntitySetIdentifier const & id);

                void SetValue(bool value, StateMachineActionQueue & queue);

                void Test_SetValue(bool value)
                {
                    isSet_ = value;
                }

                __declspec(property(get = get_IsSet)) bool IsSet;
                bool get_IsSet() const { return isSet_; }

                __declspec(property(get = get_Id)) EntitySetIdentifier const & Id;
                EntitySetIdentifier const & get_Id() const { return id_; }

            private:
                EntitySetIdentifier id_;
                bool isSet_;
            };
        }
    }
}

