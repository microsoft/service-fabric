// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Communication
        {
            class MessageMetadata : public IMessageMetadata
            {
            public:
                MessageMetadata(
                    MessageTarget::Enum target, 
                    bool processDuringNodeClose, 
                    bool createsEntity,
                    StalenessCheckType::Enum stalenessCheckType) :
                    isDeprecated_(false),
                    processDuringNodeClose_(processDuringNodeClose),
                    target_(target),
                    createsEntity_(createsEntity),
                    stalenessCheckType_(stalenessCheckType)
                {
                }

                MessageMetadata() :
                isDeprecated_(true),
                processDuringNodeClose_(false),
                target_(MessageTarget::RA),
                createsEntity_(false),
                stalenessCheckType_(StalenessCheckType::None)
                {
                }

                // Specifies whether this message can be processed while the node is closing
                __declspec(property(get = get_ProcessDuringNodeClose)) bool ProcessDuringNodeClose;
                bool get_ProcessDuringNodeClose() const { return processDuringNodeClose_; }

                // Specifies who this message is intended for
                __declspec(property(get = get_Target)) MessageTarget::Enum Target;
                MessageTarget::Enum get_Target() const { return target_; }

                // Specifies a deprecated message
                __declspec(property(get = get_IsDeprecated)) bool IsDeprecated;
                bool get_IsDeprecated() const { return isDeprecated_; }

                // Specifies whether processing of this message can create an entity
                // If this is false, then the message will be dropped by the infrastructure
                // if the target entity is null
                __declspec(property(get = get_CreatesEntity)) bool CreatesEntity;
                bool get_CreatesEntity() const { return createsEntity_; }

                __declspec(property(get = get_StalenessCheckType)) StalenessCheckType::Enum StalenessCheckType;
                StalenessCheckType::Enum get_StalenessCheckType() const { return stalenessCheckType_; }

            private:
                bool isDeprecated_;
                MessageTarget::Enum target_;
                bool processDuringNodeClose_;
                bool createsEntity_;
                Communication::StalenessCheckType::Enum stalenessCheckType_;
            };
        }
    }
}



