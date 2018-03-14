// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ChangeNotificationChallenge : public Serialization::FabricSerializable
    {
    public:
            
        ChangeNotificationChallenge()
        {
        }

        ChangeNotificationChallenge(std::vector<Federation::NodeInstance> && instances)
            : instances_(std::move(instances))
        {
        }

        __declspec (property(get=get_Instances)) std::vector<Federation::NodeInstance> const& Instances;
        std::vector<Federation::NodeInstance> const& get_Instances() const { return instances_; }

        FABRIC_FIELDS_01(instances_);

    private:

        std::vector<Federation::NodeInstance> instances_;
    };
}
