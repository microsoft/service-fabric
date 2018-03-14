// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    template<typename TBody>
    struct BodyTypeTraits
    {
        static ReplicaDescription const * GetLocalReplicaDescription(TBody const & body)
        {
            static_assert(false, "Must use specialization");
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(TBody const & body)
        {
            static_assert(false, "Must use specialization");
        }
    };

    template<>
    struct BodyTypeTraits<ReplicaMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(ReplicaMessageBody const & body)
        {
            return &body.ReplicaDescription;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(ReplicaMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<DeleteReplicaMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(DeleteReplicaMessageBody const & body)
        {
            return &body.ReplicaDescription;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(DeleteReplicaMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<RAReplicaMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(RAReplicaMessageBody const & body)
        {
            return &body.ReplicaDescription;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(RAReplicaMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<GetLSNReplyMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(GetLSNReplyMessageBody const & body)
        {
            return &body.ReplicaDescription;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(GetLSNReplyMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<ReplicaReplyMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(ReplicaReplyMessageBody const & body)
        {
            return &body.ReplicaDescription;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(ReplicaReplyMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<ConfigurationMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(ConfigurationMessageBody const &)
        {
            return nullptr;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(ConfigurationMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<DoReconfigurationMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(DoReconfigurationMessageBody const &)
        {
            return nullptr;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(DoReconfigurationMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<DeactivateMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(DeactivateMessageBody const &)
        {
            return nullptr;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(DeactivateMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<ActivateMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(ActivateMessageBody const &)
        {
            return nullptr;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(ActivateMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<FailoverUnitReplyMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(FailoverUnitReplyMessageBody const &)
        {
            return nullptr;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(FailoverUnitReplyMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<ReconfigurationAgentComponent::ProxyRequestMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(ReconfigurationAgentComponent::ProxyRequestMessageBody const & body)
        {
            return &body.LocalReplicaDescription;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(ReconfigurationAgentComponent::ProxyRequestMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<ReconfigurationAgentComponent::ProxyReplyMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(ReconfigurationAgentComponent::ProxyReplyMessageBody const & body)
        {
            return &body.LocalReplicaDescription;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(ReconfigurationAgentComponent::ProxyReplyMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<ReconfigurationAgentComponent::ReportFaultMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(ReconfigurationAgentComponent::ReportFaultMessageBody const & body)
        {
            return &body.LocalReplicaDescription;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(ReconfigurationAgentComponent::ReportFaultMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };

    template<>
    struct BodyTypeTraits<ReconfigurationAgentComponent::ProxyUpdateServiceDescriptionReplyMessageBody>
    {
        static ReplicaDescription const * GetLocalReplicaDescription(ReconfigurationAgentComponent::ProxyUpdateServiceDescriptionReplyMessageBody const &)
        {
            return nullptr;
        }

        static FailoverUnitDescription const * GetFailoverUnitDescription(ReconfigurationAgentComponent::ProxyUpdateServiceDescriptionReplyMessageBody const & body)
        {
            return &body.FailoverUnitDescription;
        }
    };
}
