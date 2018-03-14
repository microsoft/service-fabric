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
            // This class provides an identifier for an entity set
            // Entity sets can be identified by the name
            // In some cases, for the same name, the RA may need to maintain separate
            // entity sets and timers and so on
            // Example: Message retries that are pending to FM are different from those pending to the FM
            // The entity set id encapsulates all these differences, the entity creates the correct set identifier
            // in the set membership flag 
            // and then the sets are correctly registered with the RA 
            class EntitySetIdentifier
            {
            public:
                static const Common::Global<EntitySetIdentifier> ReconfigurationMessageRetry;
                static const Common::Global<EntitySetIdentifier> StateCleanup;
                static const Common::Global<EntitySetIdentifier> ReplicaCloseMessageRetry;
                static const Common::Global<EntitySetIdentifier> ReplicaOpenMessageRetry;
                static const Common::Global<EntitySetIdentifier> UpdateServiceDescriptionMessageRetry;
                static const Common::Global<EntitySetIdentifier> FmMessageRetry;
                static const Common::Global<EntitySetIdentifier> FmmMessageRetry;
                static const Common::Global<EntitySetIdentifier> FmLastReplicaUp;
                static const Common::Global<EntitySetIdentifier> FmmLastReplicaUp;

                EntitySetIdentifier() :
                name_(EntitySetName::Invalid),
                fmId_(*FailoverManagerId::Fmm)
                {
                }

                EntitySetIdentifier(EntitySetName::Enum name) :
                name_(name),
                fmId_(*FailoverManagerId::Fmm)
                {
                }

                EntitySetIdentifier(EntitySetName::Enum name, FailoverManagerId const & fmId) :
                name_(name),
                fmId_(fmId)
                {
                }

                __declspec(property(get = get_Name)) EntitySetName::Enum Name;
                EntitySetName::Enum get_Name() const { return name_; }

                __declspec(property(get = get_FailoverManager)) Reliability::FailoverManagerId const & FailoverManager;
                Reliability::FailoverManagerId const & get_FailoverManager() const { return fmId_; }

                bool operator==(EntitySetIdentifier const & rhs) const
                {
                    if (name_ != rhs.name_)
                    {
                        return false;
                    }
                    else if (!IsDifferentForEachFailoverManager())
                    {
                        return true;
                    }
                    else
                    {
                        return fmId_ == rhs.fmId_;
                    }
                }

                bool operator!=(EntitySetIdentifier const & rhs) const
                {
                    return !(*this == rhs);
                }

                void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
                {
                    if (IsDifferentForEachFailoverManager())
                    {
                        w << Common::wformatString("[{0}.{1}]", name_, fmId_);
                    }
                    else
                    {
                        w << name_;
                    }
                }

            private:
                bool IsDifferentForEachFailoverManager() const
                {
                    return name_ == EntitySetName::FailoverManagerMessageRetry || name_ == EntitySetName::ReplicaUploadPending;
                }

                EntitySetName::Enum name_;
                FailoverManagerId fmId_;
            };
        }
    }
}



