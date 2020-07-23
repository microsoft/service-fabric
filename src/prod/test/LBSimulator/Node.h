// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LBSimulator
{
    class Node
    {
        DENY_COPY(Node);
    public:
        Node(int index, std::map<std::wstring, uint> && capacities, Common::Uri && faultDomainId,
            std::wstring && upgradeDomainId, std::map<std::wstring, std::wstring> && nodeProperties, bool isUp = true);

        Node(Node && other);

        Node & operator = (Node && other);

        static Federation::NodeId Node::CreateNodeId(int id);

        static Federation::NodeInstance Node::CreateNodeInstance(int id, int instance);

        __declspec (property(get=get_Index)) int Index;
        int get_Index() const {return index_;}

        __declspec (property(get = get_NodeName)) std::wstring NodeName;
        std::wstring get_NodeName() const {
            return nodeProperties_.find(
                Common::StringUtility::ToWString(Reliability::LoadBalancingComponent::Constants::ImplicitNodeName))->second;
        }

        __declspec (property(get=get_Instance, put=set_Instance)) int Instance;
        int get_Instance() const {return instance_;}
        void set_Instance(int value) { instance_ = value; }

        __declspec (property(get=get_FederationNodeInstance)) Federation::NodeInstance FederationNodeInstance;
        Federation::NodeInstance get_FederationNodeInstance() const {return CreateNodeInstance(static_cast<int>(index_), instance_);}

        __declspec (property(get=get_IsUp, put=set_IsUp)) bool IsUp;
        bool get_IsUp() const {return isUp_;}
        void set_IsUp(bool value) { isUp_ = value; }

        __declspec (property(get = get_Properties)) std::map<std::wstring, std::wstring> const& NodeProperties;
        std::map<std::wstring, std::wstring> const& get_Properties() const { return nodeProperties_; }

        __declspec (property(get=get_CapacityRatios)) std::map<std::wstring, uint> const& CapacityRatios;
        std::map<std::wstring, uint> const& get_CapacityRatios() const {return capacityRatios_;}

        __declspec (property(get=get_Capacities)) std::map<std::wstring, uint> const& Capacities;
        std::map<std::wstring, uint> const& get_Capacities() const {return capacities_;}

        __declspec (property(get=get_UpgradeDomainId)) std::wstring const& UpgradeDomainId;
        std::wstring const& get_UpgradeDomainId() const {return upgradeDomainId_;}

        Reliability::LoadBalancingComponent::NodeDescription GetPLBNodeDescription() const;

        Common::Uri GetDomainId(std::wstring const& scheme) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    private:
        std::map<std::wstring, std::wstring> nodeProperties_;
        int index_;
        int instance_;
        bool isUp_;
        std::map<std::wstring, uint> capacityRatios_;
        std::map<std::wstring, uint> capacities_;
        Common::Uri faultDomainId_;
        std::wstring upgradeDomainId_;
    };
}
