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
                Represents a piece of an entity that supports a particular form of retry
                Provides support for:
                - A minimum interval between retry of items
                - Handling updated items and retrying immediately
                - Supporting the retry happening after some time

                Example: A replica goes down and retry is needed
            */
            template <typename T>
            class EntityRetryComponent
            {
            public:
                // Inner class used to generate the retry data as well as to mark that retry has happend
                // The caller is responsible for ensuring that the retry component has been locked appropriately
                class RetryContext
                {
                    static const int64 InvalidSequenceNumber = -1;
                public:
                    RetryContext(int64 sequenceNumber, T && data) :
                        isGenerated_(false),
                        sequenceNumber_(sequenceNumber),
                        data_(std::move(data))
                    {
                    }

                    RetryContext() :
                        isGenerated_(false),
                        sequenceNumber_(InvalidSequenceNumber)
                    {
                    }

                    RetryContext(RetryContext && other) :
                        isGenerated_(std::move(other.isGenerated_)),
                        sequenceNumber_(std::move(other.sequenceNumber_)),
                        data_(std::move(other.data_))
                    {
                    }

                    __declspec(property(get = get_HasData)) bool HasData;
                    bool get_HasData() const
                    {
                        return sequenceNumber_ != InvalidSequenceNumber;
                    }

                    __declspec(property(get = get_SequenceNumber)) int64 SequenceNumber;
                    int64 get_SequenceNumber() const
                    {
                        return sequenceNumber_;
                    }


                    void Generate(std::vector<T> & items)
                    {
                        ASSERT_IF(!HasData, "Coding error if no data");
                        ASSERT_IF(isGenerated_, "Cannot generate twice");
                        items.push_back(std::move(data_));
                        isGenerated_ = true;
                    }

                private:
                    bool isGenerated_;
                    int64 sequenceNumber_;
                    T data_;
                };

                EntityRetryComponent(Infrastructure::IRetryPolicySPtr const & retryPolicy) : 
                retryState_(retryPolicy)
                {
                }

                virtual ~EntityRetryComponent()
                {
                }

                __declspec(property(get = get_IsRetryPending)) bool IsRetryPending;
                bool get_IsRetryPending() const
                {
                    return retryState_.IsPending;
                }

                // Creates a retry context
                // The caller is responsible for taking the data out of the retry context
                RetryContext CreateRetryContext(Common::StopwatchTime now) const
                {
                    int64 sequenceNumber = 0;

                    auto shouldRetry = retryState_.ShouldRetry(now, sequenceNumber);

                    if (!shouldRetry)
                    {
                        return RetryContext();
                    }

                    return RetryContext(sequenceNumber, GenerateInternal());
                }

                void UpdateFlagIfRetryPending(__out bool & isRetryPending) const
                {
                    isRetryPending |= IsRetryPending;
                }

                void OnRetry(
                    RetryContext const & context, 
                    Common::StopwatchTime now) 
                {
                    retryState_.OnRetry(context.SequenceNumber, now);
                }

            protected:
                // Tells the component that the entity was updated and an immediate retry is required
                void MarkRetryRequired()
                {
                    retryState_.Start();
                }

                // Tells the component that the entity has been acknowledged and a retry is not required
                // Sequencing of this is handled by the owner
                void MarkRetryNotRequired()
                {
                    retryState_.Finish();
                }

            private:
                virtual T GenerateInternal() const = 0;

                RetryState retryState_;
            };
        }
    }
}


