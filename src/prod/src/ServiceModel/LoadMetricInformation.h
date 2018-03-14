// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class LoadMetricInformation 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        LoadMetricInformation();

        LoadMetricInformation(LoadMetricInformation && other);
        LoadMetricInformation(LoadMetricInformation const & other);

        LoadMetricInformation const & operator = (LoadMetricInformation const & other);
        LoadMetricInformation const & operator = (LoadMetricInformation && other);
        
        __declspec(property(get=get_Name, put=put_Name)) std::wstring const& Name;
        std::wstring const& get_Name() const { return name_; }
        void put_Name(std::wstring const& name) { name_ = name; }

        __declspec(property(get=get_IsBalancedBefore, put=put_IsBalancedBefore)) bool IsBalancedBefore;
        bool get_IsBalancedBefore() const { return isBlanacedBefore_; }
        void put_IsBalancedBefore(bool isBlanacedBefore) { isBlanacedBefore_ = isBlanacedBefore; }

        __declspec(property(get=get_IsBalancedAfter, put=put_IsBalancedAfter)) bool IsBalancedAfter;
        bool get_IsBalancedAfter() const { return isBlanacedAfter_; }
        void put_IsBalancedAfter(bool isBlanacedAfter) { isBlanacedAfter_ = isBlanacedAfter; }

        __declspec(property(get=get_DeviationBefore, put=put_DeviationBefore)) double DeviationBefore;
        double get_DeviationBefore() const { return deviationBefore_; }
        void put_DeviationBefore(double deviationBefore) { deviationBefore_ = deviationBefore; }
               
        __declspec(property(get=get_DeviationAfter, put=put_DeviationAfter)) double DeviationAfter;
        double get_DeviationAfter() const { return deviationAfter_; }
        void put_DeviationAfter(double deviationAfter) { deviationAfter_ = deviationAfter; }
        
        __declspec(property(get=get_BalancingThreshold, put=put_BalancingThreshold)) double BalancingThreshold;
        double get_BalancingThreshold() const { return balancingThreshold_; }
        void put_BalancingThreshold(double balancingThreshold) { balancingThreshold_ = balancingThreshold; }

        __declspec(property(get=get_Action, put=put_Action)) std::wstring & Action;
        std::wstring const & get_Action() const { return action_; }
        void put_Action(std::wstring const& action) { action_ = action; }

        __declspec(property(get=get_ActivityThreshold, put=put_ActivityThreshold)) uint ActivityThreshold;
        uint get_ActivityThreshold() const { return activityThreshold_; }
        void put_ActivityThreshold(uint activityThreshold) { activityThreshold_ = activityThreshold; }

        __declspec(property(get=get_ClusterCapacity, put=put_ClusterCapacity)) int64 ClusterCapacity;
        int64 get_ClusterCapacity() const { return clusterCapacity_; }
        void put_ClusterCapacity(int64 clusterCapacity) { clusterCapacity_ = clusterCapacity; }

        __declspec(property(get=get_ClusterLoad, put=put_ClusterLoad)) int64 ClusterLoad;
        int64 get_ClusterLoad() const { return clusterLoad_; }
        void put_ClusterLoad(int64 clusterLoad) { clusterLoad_ = clusterLoad; }

        __declspec(property(get=get_RemainingUnbufferedCapacity, put=put_RemainingUnbufferedCapacity)) int64 RemainingUnbufferedCapacity;
        int64 get_RemainingUnbufferedCapacity() const { return remainingUnbufferedCapacity_; }
        void put_RemainingUnbufferedCapacity(int64 remainingUnbufferedCapacity) { remainingUnbufferedCapacity_ = remainingUnbufferedCapacity; }

        __declspec(property(get=get_NodeBufferPercentage, put=put_NodeBufferPercentage)) double NodeBufferPercentage;
        double get_NodeBufferPercentage() const { return nodeBufferPercentage_; }
        void put_NodeBufferPercentage(double nodeBufferPercentage) { nodeBufferPercentage_ = nodeBufferPercentage; }

        __declspec(property(get=get_BufferedCapacity, put=put_BufferedCapacity)) int64 BufferedCapacity;
        int64 get_BufferedCapacity() const { return bufferedCapacity_; }
        void put_BufferedCapacity(int64 bufferedCapacity) { bufferedCapacity_ = bufferedCapacity; }

        __declspec(property(get=get_RemainingBufferedCapacity, put=put_RemainingBufferedCapacity)) int64 RemainingBufferedCapacity;
        int64 get_RemainingBufferedCapacity() const { return remainingBufferedCapacity_; }
        void put_RemainingBufferedCapacity(int64 remainingBufferedCapacity) { remainingBufferedCapacity_ = remainingBufferedCapacity; }

        __declspec(property(get=get_IsClusterCapacityViolation, put=put_IsClusterCapacityViolation)) bool IsClusterCapacityViolation;
        bool get_IsClusterCapacityViolation() const { return isClusterCapacityViolation_; }
        void put_IsClusterCapacityViolation(bool isClusterCapacityViolation) { isClusterCapacityViolation_ = isClusterCapacityViolation; }

        __declspec(property(get = get_MinNodeLoadValue, put = put_MinNodeLoadValue)) int64 MinNodeLoadValue;
        int64 get_MinNodeLoadValue() const { return minNodeLoadValue_; }
        void put_MinNodeLoadValue(int64 minNodeLoadValue) { minNodeLoadValue_ = minNodeLoadValue; }

        __declspec(property(get = get_MinNodeLoadNodeId, put = put_MinNodeLoadNodeId)) Federation::NodeId const & MinNodeLoadNodeId;
        Federation::NodeId const & get_MinNodeLoadNodeId() const { return minNodeLoadNodeId_; }
        void put_MinNodeLoadNodeId(Federation::NodeId  minNodeLoadNodeId) { minNodeLoadNodeId_ = minNodeLoadNodeId; }

        __declspec(property(get = get_MaxNodeLoadValue, put = put_MaxNodeLoadValue)) int64 MaxNodeLoadValue;
        int64 get_MaxNodeLoadValue() const { return maxNodeLoadValue_; }
        void put_MaxNodeLoadValue(int64 maxNodeLoadValue) { maxNodeLoadValue_ = maxNodeLoadValue; }

        __declspec(property(get = get_MaxNodeLoadNodeId, put = put_MaxNodeLoadNodeId)) Federation::NodeId const & MaxNodeLoadNodeId;
        Federation::NodeId const & get_MaxNodeLoadNodeId() const { return maxNodeLoadNodeId_; }
        void put_MaxNodeLoadNodeId(Federation::NodeId  maxNodeLoadNodeId) { maxNodeLoadNodeId_ = maxNodeLoadNodeId; }

        __declspec(property(get = get_CurrentClusterLoad, put = put_currentClusterLoad)) double CurrentClusterLoad;
        double get_CurrentClusterLoad() const { return currentClusterLoad_; }
        void put_currentClusterLoad(double value) { currentClusterLoad_ = value; }

        __declspec(property(get = get_BufferedClusterCapacityRemaining, put = put_BufferedClusterCapacityRemaining)) double BufferedClusterCapacityRemaining;
        double get_BufferedClusterCapacityRemaining() const { return bufferedClusterCapacityRemaining_; }
        void put_BufferedClusterCapacityRemaining(double value) { bufferedClusterCapacityRemaining_ = value; }

        __declspec(property(get = get_ClusterCapacityRemaining, put = put_ClusterCapacityRemaining)) double ClusterCapacityRemaining;
        double get_ClusterCapacityRemaining() const { return clusterCapacityRemaining_; }
        void put_ClusterCapacityRemaining(double value) { clusterCapacityRemaining_ = value; }

        __declspec(property(get = get_MaximumNodeLoad, put = put_MaximumNodeLoad)) double MaximumNodeLoad;
        double get_MaximumNodeLoad() const { return maximumNodeLoad_; }
        void put_MaximumNodeLoad(double value) { maximumNodeLoad_ = value; }

        __declspec(property(get = get_MinimumNodeLoad, put = put_MinimumNodeLoad)) double MinimumNodeLoad;
        double get_MinimumNodeLoad() const { return minimumNodeLoad_; }
        void put_MinimumNodeLoad(double value) { minimumNodeLoad_ = value; }

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_LOAD_METRIC_INFORMATION  &) const;
        Common::ErrorCode FromPublicApi(__in FABRIC_LOAD_METRIC_INFORMATION  const &);
        
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, name_)
            SERIALIZABLE_PROPERTY(Constants::IsBalancedBefore, isBlanacedBefore_)
            SERIALIZABLE_PROPERTY(Constants::IsBalancedAfter, isBlanacedAfter_)
            SERIALIZABLE_PROPERTY(Constants::DeviationBefore, deviationBefore_)
            SERIALIZABLE_PROPERTY(Constants::DeviationAfter, deviationAfter_)
            SERIALIZABLE_PROPERTY(Constants::BalancingThreshold, balancingThreshold_)
            SERIALIZABLE_PROPERTY(Constants::Action, action_)
            SERIALIZABLE_PROPERTY(Constants::ActivityThreshold, activityThreshold_)
            SERIALIZABLE_PROPERTY(Constants::ClusterCapacity, clusterCapacity_)
            SERIALIZABLE_PROPERTY(Constants::ClusterLoad, clusterLoad_)
            SERIALIZABLE_PROPERTY(Constants::RemainingUnbufferedCapacity, remainingUnbufferedCapacity_)
            SERIALIZABLE_PROPERTY(Constants::NodeBufferPercentage, nodeBufferPercentage_)
            SERIALIZABLE_PROPERTY(Constants::BufferedCapacity, bufferedCapacity_)
            SERIALIZABLE_PROPERTY(Constants::RemainingBufferedCapacity, remainingBufferedCapacity_)
            SERIALIZABLE_PROPERTY(Constants::IsClusterCapacityViolation, isClusterCapacityViolation_)
            SERIALIZABLE_PROPERTY(Constants::MinNodeLoadValue, minNodeLoadValue_)
            SERIALIZABLE_PROPERTY(Constants::MinNodeLoadId, minNodeLoadNodeId_)
            SERIALIZABLE_PROPERTY(Constants::MaxNodeLoadValue, maxNodeLoadValue_)
            SERIALIZABLE_PROPERTY(Constants::MaxNodeLoadId, maxNodeLoadNodeId_)
            SERIALIZABLE_PROPERTY(Constants::CurrentClusterLoad, currentClusterLoad_)
            SERIALIZABLE_PROPERTY(Constants::BufferedClusterCapacityRemaining, bufferedClusterCapacityRemaining_)
            SERIALIZABLE_PROPERTY(Constants::ClusterCapacityRemaining, clusterCapacityRemaining_)
            SERIALIZABLE_PROPERTY(Constants::MaximumNodeLoad, maximumNodeLoad_)
            SERIALIZABLE_PROPERTY(Constants::MinimumNodeLoad, minimumNodeLoad_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_24(name_,
        isBlanacedBefore_,
        isBlanacedAfter_, 
        deviationBefore_, 
        deviationAfter_, 
        balancingThreshold_,
        action_,
        activityThreshold_,
        clusterCapacity_,
        clusterLoad_,
        remainingUnbufferedCapacity_,
        nodeBufferPercentage_,
        bufferedCapacity_,
        remainingBufferedCapacity_,
        isClusterCapacityViolation_,
        minNodeLoadValue_,
        minNodeLoadNodeId_,
        maxNodeLoadValue_,
        maxNodeLoadNodeId_,
        currentClusterLoad_,
        bufferedClusterCapacityRemaining_,
        clusterCapacityRemaining_,
        maximumNodeLoad_,
        minimumNodeLoad_);

    private:
        std::wstring name_;
        bool isBlanacedBefore_;
        bool isBlanacedAfter_;
        double deviationBefore_;
        double deviationAfter_;
        double balancingThreshold_;
        std::wstring action_;
        uint activityThreshold_;
        int64 clusterCapacity_;
        int64 clusterLoad_;
        bool isClusterCapacityViolation_;
        int64 remainingUnbufferedCapacity_;
        double nodeBufferPercentage_;
        int64 bufferedCapacity_;
        int64 remainingBufferedCapacity_;
        int64 minNodeLoadValue_;
        Federation::NodeId minNodeLoadNodeId_;
        int64 maxNodeLoadValue_;
        Federation::NodeId maxNodeLoadNodeId_;
        double currentClusterLoad_;
        double bufferedClusterCapacityRemaining_;
        double clusterCapacityRemaining_;
        double maximumNodeLoad_;
        double minimumNodeLoad_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::LoadMetricInformation);
