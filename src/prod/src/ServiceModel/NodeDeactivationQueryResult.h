// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NodeDeactivationTaskId
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
    {
        DEFAULT_COPY_CONSTRUCTOR(NodeDeactivationTaskId)
        DEFAULT_COPY_ASSIGNMENT(NodeDeactivationTaskId)

    public:

        NodeDeactivationTaskId();

        NodeDeactivationTaskId(
            std::wstring const& id,
            FABRIC_NODE_DEACTIVATION_TASK_TYPE type);

        NodeDeactivationTaskId(std::wstring const& batchId);

        NodeDeactivationTaskId(NodeDeactivationTaskId && other);

        NodeDeactivationTaskId & operator=(NodeDeactivationTaskId && other);

        virtual ~NodeDeactivationTaskId();

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_NODE_DEACTIVATION_TASK_ID & publicNodeDeactivationTaskId) const;

        Common::ErrorCode FromPublicApi(__in FABRIC_NODE_DEACTIVATION_TASK_ID const& publicNodeDeactivationTaskId);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        FABRIC_FIELDS_02(
            id_,
            type_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Id, id_);
            SERIALIZABLE_PROPERTY_ENUM(Constants::NodeDeactivationTaskType, type_);
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(id_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(type_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:

        std::wstring id_;
        FABRIC_NODE_DEACTIVATION_TASK_TYPE type_;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class NodeDeactivationTask
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
    {
        DEFAULT_COPY_CONSTRUCTOR(NodeDeactivationTask)
        DEFAULT_COPY_ASSIGNMENT(NodeDeactivationTask)

    public:
        NodeDeactivationTask();

        NodeDeactivationTask(
            NodeDeactivationTaskId const& taskId,
            FABRIC_NODE_DEACTIVATION_INTENT intent);

        NodeDeactivationTask(NodeDeactivationTask && other);

        NodeDeactivationTask & operator=(NodeDeactivationTask && other);

        virtual ~NodeDeactivationTask();

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_NODE_DEACTIVATION_TASK & publicNodeDeactivationTask) const;

        Common::ErrorCode FromPublicApi(__in FABRIC_NODE_DEACTIVATION_TASK const& publicNodeDeactivationTask);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        FABRIC_FIELDS_02(
            taskId_,
            intent_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::NodeDeactivationTaskId, taskId_);
            SERIALIZABLE_PROPERTY_ENUM(Constants::NodeDeactivationIntent, intent_);
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(taskId_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(intent_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:

        NodeDeactivationTaskId taskId_;
        FABRIC_NODE_DEACTIVATION_INTENT intent_;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class NodeDeactivationQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
    {
        DEFAULT_COPY_ASSIGNMENT(NodeDeactivationQueryResult)

    public:
        NodeDeactivationQueryResult();

        NodeDeactivationQueryResult(
            FABRIC_NODE_DEACTIVATION_INTENT effectiveIntent,
            FABRIC_NODE_DEACTIVATION_STATUS status,
            std::vector<NodeDeactivationTask> && tasks,
            std::vector<Reliability::SafetyCheckWrapper> const& pendingSafetyChecks);

        NodeDeactivationQueryResult(NodeDeactivationQueryResult && other);

        NodeDeactivationQueryResult & operator = (NodeDeactivationQueryResult && other);

        virtual ~NodeDeactivationQueryResult();

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_NODE_DEACTIVATION_QUERY_RESULT_ITEM & publicNodeDeactivationQueryResult) const;

        Common::ErrorCode FromPublicApi(__in FABRIC_NODE_DEACTIVATION_QUERY_RESULT_ITEM const& publicNodeDeactivationQueryResult);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        FABRIC_FIELDS_04(
            effectiveIntent_,
            status_,
            tasks_,
            pendingSafetyChecks_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::NodeDeactivationIntent, effectiveIntent_);
            SERIALIZABLE_PROPERTY_ENUM(Constants::NodeDeactivationStatus, status_);
            SERIALIZABLE_PROPERTY(Constants::NodeDeactivationTask, tasks_);
            SERIALIZABLE_PROPERTY(Constants::PendingSafetyChecks, pendingSafetyChecks_);
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_ENUM_ESTIMATION_MEMBER(effectiveIntent_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(status_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(tasks_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(pendingSafetyChecks_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:

        FABRIC_NODE_DEACTIVATION_INTENT effectiveIntent_;
        FABRIC_NODE_DEACTIVATION_STATUS status_;
        std::vector<NodeDeactivationTask> tasks_;

        std::vector<Reliability::SafetyCheckWrapper> pendingSafetyChecks_;
    };

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_USER_ARRAY_UTILITY(ServiceModel::NodeDeactivationTask);
DEFINE_USER_ARRAY_UTILITY(Reliability::SafetyCheck);
