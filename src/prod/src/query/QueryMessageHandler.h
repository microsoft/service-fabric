// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Query
{
    //QueryMessageHandler typedef
    class QueryMessageHandler ;
    typedef std::shared_ptr<QueryMessageHandler> QueryMessageHandlerSPtr;
    typedef std::unique_ptr<QueryMessageHandler> QueryMessageHandlerUPtr;
    
    // ProcessQuery function typedefs
    typedef std::function<Common::AsyncOperationSPtr(
        Query::QueryNames::Enum, 
        ServiceModel::QueryArgumentMap const &, 
        Common::ActivityId const &,
        Common::TimeSpan,
        Common::AsyncCallback const &, 
        Common::AsyncOperationSPtr const & )> BeginProcessQueryFunction;
    typedef std::function<Common::ErrorCode(
        Common::AsyncOperationSPtr const &, 
        _Out_ Transport::MessageUPtr & )> EndProcessQueryFunction;
    typedef std::function<Common::ErrorCode (
        Query::QueryNames::Enum,
        ServiceModel::QueryArgumentMap const &, 
        Common::ActivityId const &,
        _Out_ Transport::MessageUPtr & )> ProcessQueryFunction;

    // ForwardQuery function typedefs
    typedef std::function<Common::AsyncOperationSPtr(
        std::wstring const & , 
        std::wstring const & , 
        Transport::MessageUPtr & ,
        Common::TimeSpan ,
        Common::AsyncCallback const &,
        Common::AsyncOperationSPtr const &)> BeginForwardQueryMessageFunction;
    typedef std::function<Common::ErrorCode(
        Common::AsyncOperationSPtr const & , 
        _Out_ Transport::MessageUPtr &)> EndForwardQueryMessageFunction;
    typedef std::function<Common::ErrorCode(
        std::wstring const & , 
        std::wstring const & , 
        Transport::MessageUPtr & ,
        _Out_ Transport::MessageUPtr &)> ForwardQueryFunction;

    // QueryMessageHandler class
    class QueryMessageHandler        
        : public Common::FabricComponent
        , public Common::RootedObject
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
    public: 
        QueryMessageHandler(
            Common::ComponentRoot const & root, 
            std::wstring const & addressSegment);

        // Query handler registration
        Common::ErrorCode RegisterQueryHandler(ProcessQueryFunction const & processQuery);
        Common::ErrorCode RegisterQueryHandler(BeginProcessQueryFunction const & beginProcessQuery, EndProcessQueryFunction const & endProcessQuery);        

        // Forwarding registration
        Common::ErrorCode RegisterQueryForwardHandler(ForwardQueryFunction const & forwardQuery);
        Common::ErrorCode RegisterQueryForwardHandler(BeginForwardQueryMessageFunction const & beginForwardQuery, EndForwardQueryMessageFunction const & endForwardQuery);
        
        // Query processing methods (Async and Sync)
        Common::AsyncOperationSPtr BeginProcessQueryMessage(
            Transport::Message & message, 
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::AsyncOperationSPtr BeginProcessQueryMessage(
            Transport::Message & message, 
            QuerySpecificationSPtr const & querySpecSPtr,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndProcessQueryMessage(
            Common::AsyncOperationSPtr const & asyncOperation, 
            _Out_ Transport::MessageUPtr & reply);
        Transport::MessageUPtr ProcessQueryMessage(Transport::Message & requestMessage);

        // FabricComponent: State management   
        // Once the queryMessageHandler is aborted or closed, no more requests will be processed.
        // The query handler and forwarder function pointers will be release at the end of last 
        // operation that was started when the QueryMessageHandler was still open
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        class QueryMessageHandlerAsyncOperation;
        class ProcessMessageAsyncOperation;
        class ProcessParallelQueryAsyncOperation;
        class ProcessSequentialQueryAsyncOperation;
        friend class ProcessMessageAsyncOperation;
        class ProcessQuerySynchronousCallerAsyncOperation;
        class ForwardQuerySynchronousCallerAsyncOperation;

        // Parallel query processing 
        Common::AsyncOperationSPtr BeginProcessParallelQuery(
            Query::QueryNames::Enum queryName, 
            ServiceModel::QueryArgumentMap const & queryArgs, 
            QuerySpecificationSPtr const & querySpecSPtr,
            Transport::MessageHeaders & headers,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndProcessParallelQuery(
            Common::AsyncOperationSPtr const & asyncOperation, 
            _Out_ Transport::MessageUPtr & reply);

        // Sequential query processing 
        Common::AsyncOperationSPtr BeginProcessSequentialQuery(
            Query::QueryNames::Enum queryName, 
            ServiceModel::QueryArgumentMap const & queryArgs, 
            Transport::MessageHeaders & headers,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndProcessSequentialQuery(
            Common::AsyncOperationSPtr const & asyncOperation, 
            _Out_ Transport::MessageUPtr & reply);

        // Adapter functions to allow sync query handler
        Common::AsyncOperationSPtr BeginProcessQuerySynchronousCaller(
            ProcessQueryFunction const & processQueryFunction,
            Query::QueryNames::Enum queryName, 
            ServiceModel::QueryArgumentMap const & queryArgs, 
            Common::ActivityId const & activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndProcessQuerySynchronousCaller(
            Common::AsyncOperationSPtr const & asyncOperation, 
            _Out_ Transport::MessageUPtr & reply);

        // Adapter functions to allow sync query forwarder
        Common::AsyncOperationSPtr BeginForwardMessageSynchronousCaller(
            ForwardQueryFunction const & forwardQueryFunction,
            std::wstring const & childAddressSegment, 
            std::wstring const & childAddressSegmentMetadata, 
            Transport::MessageUPtr & message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndForwardMessageSynchronousCaller(
            Common::AsyncOperationSPtr const & asyncOperation, 
            _Out_ Transport::MessageUPtr & reply);

        // Remove the registrations
        void UnRegisterAll();

        // Address segment for this query message handler
        std::wstring addressSegment_;        

        // Processing/Forwarding function members
        BeginProcessQueryFunction beginProcessQueryFunction_;
        EndProcessQueryFunction endProcessQueryFunction_;

        BeginForwardQueryMessageFunction beginForwardQueryMessageFunction_;
        EndForwardQueryMessageFunction endForwardQueryMessageFunction_;

        // Query message process request counting
        Common::atomic_long requestCount_;

        // Closed flag
        Common::atomic_bool closedOrAborted_;
    };
}
