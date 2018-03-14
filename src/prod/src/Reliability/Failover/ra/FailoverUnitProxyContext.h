// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Common message processing context for FailoverUnit
        template <class T>
        class FailoverUnitProxyContext
        {
            DENY_COPY(FailoverUnitProxyContext);
        public:

            FailoverUnitProxyContext(FailoverUnitProxyContext && other)
                     : action_(std::move(other.action_)),
                       body_(std::move(other.body_)),
                       failoverUnitProxy_(std::move(other.failoverUnitProxy_)),
                       reconfigurationAgentProxy_(other.reconfigurationAgentProxy_),
                       wasFailoverUnitProxyCreated_(other.wasFailoverUnitProxyCreated_),
                       receiverContext_(std::move(other.receiverContext_))
            {}

            ~FailoverUnitProxyContext()
            {
                if (!IsInvalid)
                {
                    TraceState();
                }
            }
            
            __declspec(property(get=get_IsInvalid)) bool IsInvalid;
            bool get_IsInvalid() const { return failoverUnitProxy_ == nullptr; }

            __declspec(property(get=get_FailoverUnitProxy)) FailoverUnitProxySPtr & FailoverUnitProxy;
            FailoverUnitProxySPtr & get_FailoverUnitProxy() { return failoverUnitProxy_; }

            __declspec(property(get=get_Action)) std::wstring const & Action;
            std::wstring const & get_Action() { return action_; }

            __declspec(property(get = get_WasFailoverUnitProxyCreated)) bool WasFailoverUnitProxyCreated;
            bool get_WasFailoverUnitProxyCreated() { return wasFailoverUnitProxyCreated_; }

            __declspec(property(get=get_Body)) T const & Body;
            T const & get_Body() { return body_; }

            __declspec(property(get = get_ReceiverContext)) Transport::IpcReceiverContext * ReceiverContext;
            Transport::IpcReceiverContext * get_ReceiverContext() const
            {
                return receiverContext_.get();
            }

            // Perform common actions for all messages on arrival
            static FailoverUnitProxyContext CreateContext(
                    Transport::MessageUPtr && proxyMessage, 
                    Transport::IpcReceiverContextUPtr && receiverContext,
                    ReconfigurationAgentProxy & reconfigurationAgentProxy)
            {
                T body;
                if (!proxyMessage->GetBody<T>(body))
                {
                    return FailoverUnitProxyContext(reconfigurationAgentProxy);
                }

                bool createFlag = false;
                
                std::wstring const & action = proxyMessage->Action;

                if (action == RAMessage::GetReplicaOpen().Action ||
                    action == RAMessage::GetReplicaClose().Action ||
                    action == RAMessage::GetStatefulServiceReopen().Action
                    )
                {
                    // Create a new FUP only for these types of messages
                    createFlag = true;
                }

                LocalFailoverUnitProxyMap & lfupm = reconfigurationAgentProxy.LocalFailoverUnitProxyMapObj;

                // Get or create the FUP, it will be locked if found or created
                bool wasFailoverUnitProxyCreated = false;
                FailoverUnitProxySPtr fup = 
                    lfupm.GetOrCreateFailoverUnitProxy(
                        reconfigurationAgentProxy,
                        body.FailoverUnitDescription,
                        body.RuntimeId,
                        createFlag,
                        wasFailoverUnitProxyCreated);

                if (!fup)
                {
                    // Message replies contain the error code, if the message is not valid
                    // then read the error code value since the message will not be
                    // processed
                    __if_exists(T::ErrorCode)
                    {
                        body.ErrorCode.ReadValue();
                    }

                    return FailoverUnitProxyContext(reconfigurationAgentProxy);
                }

                {
                    LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(fup);

                    if (lockedFailoverUnitProxyPtr->IsDeleted)
                    {
                        __if_exists(T::ErrorCode)
                        {
                            body.ErrorCode.ReadValue();
                        }

                        return FailoverUnitProxyContext(reconfigurationAgentProxy);
                    }

                    // Check for stale message
                    if (lockedFailoverUnitProxyPtr->IsValid && IsStale(action, body, fup))
                    {
                        // Message replies contain the error code, if the message is not valid
                        // then read the error code value since the message will not be
                        // processed
                        __if_exists(T::ErrorCode)
                        {
                            body.ErrorCode.ReadValue();
                        }

                        return FailoverUnitProxyContext(reconfigurationAgentProxy);
                    }
                }

                return FailoverUnitProxyContext(
                    std::wstring(action), 
                    std::move(body), 
                    std::move(fup), 
                    wasFailoverUnitProxyCreated, 
                    std::move(receiverContext), 
                    reconfigurationAgentProxy);
            }

            bool Validate()
            {
                if (!IsInvalid)
                {
                    return !IsStale(action_, body_, failoverUnitProxy_);
                }

                return false;
            }

            void TraceState()
            {
                LockedFailoverUnitProxyPtr lockedFailoverUnitProxyPtr(failoverUnitProxy_);

                RAPEventSource::Events->RAPFailoverUnitContext(
                    lockedFailoverUnitProxyPtr->FailoverUnitId.Guid,
                    lockedFailoverUnitProxyPtr->RAPId,
                    action_, 
                    Common::TraceCorrelatedEvent<T>(body_), 
                    *failoverUnitProxy_);
            }

        private:

            explicit FailoverUnitProxyContext(
                        ReconfigurationAgentProxy & reconfigurationAgentProxy)
                        : action_(),
                          failoverUnitProxy_(nullptr),
                          reconfigurationAgentProxy_(reconfigurationAgentProxy),
                          wasFailoverUnitProxyCreated_(false)
            {}

            FailoverUnitProxyContext(
                std::wstring && action,
                T && body,
                FailoverUnitProxySPtr && failoverUnitProxy, 
                bool wasFailoverUnitProxyCreated,
                Transport::IpcReceiverContextUPtr && receiverContext,
                ReconfigurationAgentProxy & reconfigurationAgentProxy) : 
                action_(std::move(action)),
                body_(std::move(body)),
                failoverUnitProxy_(std::move(failoverUnitProxy)),
                reconfigurationAgentProxy_(reconfigurationAgentProxy),
                wasFailoverUnitProxyCreated_(wasFailoverUnitProxyCreated),
                receiverContext_(std::move(receiverContext))
            {   
            }

            static bool IsStale(
                std::wstring const & action,
                T const& body,
                FailoverUnitProxySPtr const & fup)
            {
                Epoch incomingVersion = body.FailoverUnitDescription.CurrentConfigurationEpoch;
                Epoch currentVersion = fup->CurrentConfigurationEpoch;

                if (currentVersion > incomingVersion)
                {
                    // Current version is greater, stale message
                    RAPEventSource::Events->MessageDropStale(
                        fup->RAPId,
                        action,
                        Common::TraceCorrelatedEvent<T>(body),
                        *fup);

                    return true;
                }

                __if_exists(T::LocalReplicaDescription)
                {
                    if (!fup->IsClosed)
                    {
                        // Check staleness based on replica instance
                        ReplicaDescription const & incomingReplica = body.LocalReplicaDescription;

                        if (fup->ReplicaDescription.InstanceId > incomingReplica.InstanceId)
                        {
                            // Current version is greater, stale message
                            RAPEventSource::Events->MessageDropStale(
                                fup->RAPId,
                                action,
                                Common::TraceCorrelatedEvent<T>(body),
                                *fup);

                            return true;
                        }

                        // Reopen is allowed with higher intance id
                        // Close is allowed with higher instance id if the reopen with the higher instance was dropped
                        
                        // Query is allowed with higher instance id :- if the FUP is closed for business due to a dropped reopen while some other
                        // action was running, a new query could be issued before the retry of reopen happens. This query will have higher instance id
                        bool allowedActionWithHigherInstanceId = action == RAMessage::GetStatefulServiceReopen().Action ||
                                                                 action == RAMessage::GetReplicaClose().Action ||
                                                                 action == RAMessage::GetRAPQuery().Action;

                        ASSERT_IF(
                            fup->ReplicaDescription.InstanceId < incomingReplica.InstanceId && !allowedActionWithHigherInstanceId,
                            "RAP received message:{0}\n{1}with incoming replica instance newer than LFUPM:\n{2}",
                            action, body, *fup);
                    }
                }

                return false;
            }

            std::wstring action_;
            T body_;
            FailoverUnitProxySPtr failoverUnitProxy_;
            ReconfigurationAgentProxy & reconfigurationAgentProxy_;
            bool wasFailoverUnitProxyCreated_;
            Transport::IpcReceiverContextUPtr receiverContext_;
        };
    }
}
