// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class NodeLoadMetricInformation 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        NodeLoadMetricInformation();

        NodeLoadMetricInformation(NodeLoadMetricInformation && other);
        NodeLoadMetricInformation(NodeLoadMetricInformation const & other);

        NodeLoadMetricInformation const & operator = (NodeLoadMetricInformation const & other);
        NodeLoadMetricInformation const & operator = (NodeLoadMetricInformation && other);
        
        __declspec(property(get=get_Name, put=put_Name)) std::wstring const& Name;
        std::wstring const& get_Name() const { return name_; }
        void put_Name(std::wstring && name) { name_ = std::move(name); }
        
        __declspec(property(get=get_NodeCapacity, put=put_NodeCapacity)) int64 NodeCapacity;
        int64 get_NodeCapacity() const { return nodeCapacity_; }
        void put_NodeCapacity(int64 nodeCapacity) { nodeCapacity_ = nodeCapacity; }
        
        __declspec(property(get=get_NodeLoad, put=put_NodeLoad)) int64 NodeLoad;
        int64 get_NodeLoad() const { return nodeLoad_; }
        void put_NodeLoad(int64 nodeLoad) { nodeLoad_ = nodeLoad; }

        __declspec(property(get=get_NodeRemainingCapacity, put=put_NodeRemainingCapacity)) int64 NodeRemainingCapacity;
        int64 get_NodeRemainingCapacity() const { return nodeRemainingCapacity_; }
        void put_NodeRemainingCapacity(int64 nodeRemainingCapacity) { nodeRemainingCapacity_ = nodeRemainingCapacity; }

        __declspec(property(get=get_IsCapacityViolation, put=put_IsCapacityViolation)) bool IsCapacityViolation;
        bool get_IsCapacityViolation() const { return isCapacityViolation_; }
        void put_IsCapacityViolation(bool isCapacityViolation) { isCapacityViolation_ = isCapacityViolation; }

        __declspec(property(get=get_NodeBufferedCapacity, put=put_NodeBufferedCapacity)) int64 NodeBufferedCapacity;
        int64 get_NodeBufferedCapacity() const { return nodeBufferedCapacity_; }
        void put_NodeBufferedCapacity(int64 nodeBufferedCapacity) { nodeBufferedCapacity_ = nodeBufferedCapacity; }

        __declspec(property(get=get_NodeRemainingBufferedCapacity, put=put_NodeRemainingBufferedCapacity)) int64 NodeRemainingBufferedCapacity;
        int64 get_NodeRemainingBufferedCapacity() const { return nodeRemainingBufferedCapacity_; }
        void put_NodeRemainingBufferedCapacity(int64 nodeRemainingBufferedCapacity) { nodeRemainingBufferedCapacity_ = nodeRemainingBufferedCapacity; }

        __declspec(property(get = get_CurrentNodeLoad, put = put_CurrentNodeLoad)) double CurrentNodeLoad;
        double get_CurrentNodeLoad() const { return currentNodeLoad_; }
        void put_CurrentNodeLoad(double currentNodeLoad) { currentNodeLoad_ = currentNodeLoad; }

        __declspec(property(get = get_NodeCapacityRemaining, put = put_NodeCapacityRemaining)) double NodeCapacityRemaining;
        double get_NodeCapacityRemaining() const { return nodeCapacityRemaining_; }
        void put_NodeCapacityRemaining(double nodeCapacityRemaining) { nodeCapacityRemaining_ = nodeCapacityRemaining; }

        __declspec(property(get = get_BufferedNodeCapacityRemaining, put = put_BufferedNodeCapacityRemaining)) double BufferedNodeCapacityRemaining;
        double get_BufferedNodeCapacityRemaining() const { return bufferedNodeCapacityRemaining_; }
        void put_BufferedNodeCapacityRemaining(double bufferednodeCapacityRemaining) { bufferedNodeCapacityRemaining_ = bufferednodeCapacityRemaining; }

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_NODE_LOAD_METRIC_INFORMATION  &) const;
        Common::ErrorCode FromPublicApi(__in FABRIC_NODE_LOAD_METRIC_INFORMATION  const &);
        
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, name_)
            SERIALIZABLE_PROPERTY(Constants::NodeCapacity, nodeCapacity_)
            SERIALIZABLE_PROPERTY(Constants::NodeLoad, nodeLoad_)
            SERIALIZABLE_PROPERTY(Constants::NodeRemainingCapacity, nodeRemainingCapacity_)
            SERIALIZABLE_PROPERTY(Constants::IsCapacityViolation, isCapacityViolation_)
            SERIALIZABLE_PROPERTY(Constants::NodeBufferedCapacity, nodeBufferedCapacity_)
            SERIALIZABLE_PROPERTY(Constants::NodeRemainingBufferedCapacity, nodeRemainingBufferedCapacity_)
            SERIALIZABLE_PROPERTY(Constants::CurrentNodeLoad, currentNodeLoad_)
            SERIALIZABLE_PROPERTY(Constants::NodeCapacityRemaining, nodeCapacityRemaining_)
            SERIALIZABLE_PROPERTY(Constants::BufferedNodeCapacityRemaining, bufferedNodeCapacityRemaining_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_10(name_,
        nodeCapacity_,
        nodeLoad_,
        nodeRemainingCapacity_,
        isCapacityViolation_,
        nodeBufferedCapacity_,
        nodeRemainingBufferedCapacity_,
        currentNodeLoad_,
        nodeCapacityRemaining_,
        bufferedNodeCapacityRemaining_);

    private:
        std::wstring name_;
        int64 nodeCapacity_;
        int64 nodeLoad_;
        int64 nodeRemainingCapacity_;
        bool isCapacityViolation_;
        int64 nodeBufferedCapacity_;
        int64 nodeRemainingBufferedCapacity_;
        double currentNodeLoad_;
        double nodeCapacityRemaining_;
        double bufferedNodeCapacityRemaining_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::NodeLoadMetricInformation);

