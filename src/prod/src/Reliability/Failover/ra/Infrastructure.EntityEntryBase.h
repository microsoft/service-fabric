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
            /*
                The base class for all entity entries

                An entity entry is the value stored in an entity map

                It is always alloc'd on the heap and stored as an SPtr
            */
            class EntityEntryBase 
            {
                DENY_COPY(EntityEntryBase);

            public:
                EntityEntryBase(std::string && traceId) :
                    traceId_(traceId)
                {
                }

                virtual ~EntityEntryBase()
                {
                }

                virtual void AddEntityIdToMessage(Transport::Message & message) const = 0;

                virtual Common::AsyncOperationSPtr BeginExecuteJobItem(
                    EntityEntryBaseSPtr const & thisSPtr,
                    EntityJobItemBaseSPtr const & jobItem,
                    ReconfigurationAgent & ra,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) = 0;

                virtual Common::ErrorCode EndExecuteJobItem(
                    Common::AsyncOperationSPtr const & op,
                    __out std::vector<MultipleEntityWorkSPtr> & completedWork) = 0;

                virtual Common::AsyncOperationSPtr BeginCommit(
                    Storage::Api::OperationType::Enum operationType,
                    std::vector<byte> && bytes,
                    Common::TimeSpan const timeout,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) = 0;

                virtual Common::ErrorCode EndCommit(Common::AsyncOperationSPtr const & operation) = 0;

                template < typename T >
                T & As()
                {
                    return dynamic_cast<T&>(*this);
                }

                template < typename T >
                T const & As() const
                {
                    return dynamic_cast<T const &>(*this);
                }

                std::string const & GetEntityIdForTrace() const
                {
                    return traceId_;
                }

                void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
                {
                    w << GetEntityIdForTrace();
                }

                static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
                {
                    std::string format = "{0}";
                    size_t index = 0;

                    traceEvent.AddEventField<std::string>(format, name + ".entityId", index);

                    return format;
                }

                void FillEventData(Common::TraceEventContext & context) const
                {
                    context.Write(GetEntityIdForTrace());
                }

                void WriteToEtw(uint16 contextSequenceId) const
                {
                    Common::CommonEventSource::Events->TraceDynamicString(contextSequenceId, traceId_);
                }

            private:
                std::string traceId_;
            };
        }
    }
}

