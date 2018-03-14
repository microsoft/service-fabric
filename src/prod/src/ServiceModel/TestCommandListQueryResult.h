// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class TestCommandListQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DENY_COPY(TestCommandListQueryResult)

    public:
    
        TestCommandListQueryResult();
        
        TestCommandListQueryResult(
            Common::Guid const & operationId,
            FABRIC_TEST_COMMAND_PROGRESS_STATE state,
            FABRIC_TEST_COMMAND_TYPE type);

        TestCommandListQueryResult(TestCommandListQueryResult && other);
        TestCommandListQueryResult & operator = (TestCommandListQueryResult && other);

        __declspec(property(get=get_OperationId)) Common::Guid const & OperationId;
        Common::Guid const & get_OperationId() const { return operationId_; }

        __declspec(property(get = get_TestCommandState)) FABRIC_TEST_COMMAND_PROGRESS_STATE TestCommandState;
        FABRIC_TEST_COMMAND_PROGRESS_STATE get_TestCommandState() const { return testCommandState_; }

        __declspec(property(get=get_TestCommandType)) FABRIC_TEST_COMMAND_TYPE TestCommandType;
        FABRIC_TEST_COMMAND_TYPE get_TestCommandType() const { return testCommandType_; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out TEST_COMMAND_QUERY_RESULT_ITEM & publicApplicationQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

      
        FABRIC_FIELDS_03(operationId_, testCommandState_, testCommandType_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::OperationId, operationId_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::State, testCommandState_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::Type, testCommandType_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        Common::Guid operationId_;

        FABRIC_TEST_COMMAND_PROGRESS_STATE testCommandState_;
        FABRIC_TEST_COMMAND_TYPE testCommandType_;
    };
}

