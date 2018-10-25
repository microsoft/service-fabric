// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Federation;
    using namespace Reliability;
    using namespace Store;
    using namespace ServiceModel;

    StringLiteral const TraceComponent("ProcessEnumerateProperties");

    StoreService::ProcessEnumeratePropertiesRequestAsyncOperation::ProcessEnumeratePropertiesRequestAsyncOperation(
        MessageUPtr && request,
        __in NamingStore & namingStore,
        __in StoreServiceProperties & properties,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessRequestAsyncOperation(
            std::move(request),
            namingStore,
            properties,
            timeout,
            callback,
            root)
        , storeKeyPrefix_()
        , propertyResults_()
        , lastEnumeratedPropertyName_()
        , propertiesVersion_()
        , enumerationStatus_(FABRIC_ENUMERATION_CONSISTENT_MASK)
        , replySizeThreshold_(0)
        , replyItemsSizeEstimate_(0)
        , replyTotalSizeEstimate_(0)
    {
    }

    ErrorCode StoreService::ProcessEnumeratePropertiesRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        if (request->GetBody(body_))
        {
            this->SetName(body_.NamingUri);

            replySizeThreshold_ = NamingUtility::GetMessageContentThreshold();

            size_t maxReplySizeEstimate = EnumeratePropertiesResult::EstimateSerializedSize(
                this->NameString.size(),
                ServiceModelConfig::GetConfig().MaxPropertyNameLength,
                (body_.IncludeValues ? ServiceModel::Constants::NamedPropertyMaxValueSize : 0));

            if (!NamingUtility::CheckEstimatedSize(maxReplySizeEstimate, replySizeThreshold_).IsSuccess())
            {
                TRACE_WARNING_AND_TESTASSERT(
                    TraceComponent,
                    "{0}: (MaxMessageSize * MessageContentBufferRatio) is too small: allowed = {1} max reply estimate = {2}",
                    this->TraceId,
                    replySizeThreshold_,
                    maxReplySizeEstimate);
            }

            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    bool StoreService::ProcessEnumeratePropertiesRequestAsyncOperation::AllowNameFragment()
    {
        // Allow property operations on service group members
        return false;
    }

    void StoreService::ProcessEnumeratePropertiesRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "{0}: Enumerating properties at Name {1}: values = {2} continuation token = {3} reply size threshold = {4}",
            this->TraceId,
            this->NameString,
            body_.IncludeValues,
            body_.ContinuationToken,
            replySizeThreshold_);

        TransactionSPtr txSPtr;
        EnumerationSPtr enumSPtr;

        ErrorCode error = Store.CreateTransaction(this->ActivityHeader, txSPtr);

        if (error.IsSuccess())
        {
            error = InitializeEnumeration(txSPtr, enumSPtr);
        }

        if (error.IsSuccess())
        {
            error = EnumerateProperties(enumSPtr);
        }

        enumSPtr.reset();
        NamingStore::CommitReadOnly(move(txSPtr));

        if (error.IsSuccess())
        {
            auto replyBody = EnumeratePropertiesResult(
                enumerationStatus_,
                move(propertyResults_),
                EnumeratePropertiesToken(
                    lastEnumeratedPropertyName_,
                    propertiesVersion_));

            Reply = NamingMessage::GetPeerEnumeratePropertiesReply(replyBody);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: property enumeration failed with {1}",
                this->TraceId,
                error);
        }
        
        this->TryComplete(thisSPtr, error);
    }

    ErrorCode StoreService::ProcessEnumeratePropertiesRequestAsyncOperation::InitializeEnumeration(
        TransactionSPtr const & txSPtr,
        __out EnumerationSPtr & enumSPtr)
    {
        __int64 nameSequenceNumber = -1;
        EnumerationSPtr nameEnumSPtr;

        ErrorCode error = Store.CreateEnumeration(txSPtr, Constants::NonHierarchyNameEntryType, this->Name.Name, nameEnumSPtr);

        wstring nameOwnerName;

        if (error.IsSuccess())
        {
            nameOwnerName = this->Name.GetTrimQueryAndFragment().ToString();

            error = Store.TryGetCurrentSequenceNumber(
                txSPtr, 
                Constants::NonHierarchyNameEntryType, 
                nameOwnerName,
                nameSequenceNumber, 
                nameEnumSPtr);
        }

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: could not read Name {1}: error = {2}",
                this->TraceId,
                nameOwnerName,
                error);

            return (error.IsError(ErrorCodeValue::NotFound) ? ErrorCodeValue::NameNotFound : error);
        }

        NameData nameOwnerEntry;
        error = Store.ReadCurrentData<NameData>(nameEnumSPtr, nameOwnerEntry);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: could not deserialize Name {1}: error = {2}",
                this->TraceId,
                nameOwnerName,
                error);

            return error;
        }

        WriteNoise(
            TraceComponent,
            "{0}: properties version ({1})",
            this->TraceId,
            nameOwnerEntry.PropertiesVersion);

        storeKeyPrefix_ = NamePropertyKey::CreateKey(this->Name, L"");

        if (body_.ContinuationToken.IsValid)
        {
            lastEnumeratedPropertyName_ = body_.ContinuationToken.LastEnumeratedPropertyName;
            propertiesVersion_ = body_.ContinuationToken.PropertiesVersion;
            
            // The enumeration consistency status is only affected by property writes at this name.
            // Changes to subnames or services do not affect the enumeration status.
            //
            if (body_.ContinuationToken.PropertiesVersion != nameOwnerEntry.PropertiesVersion)
            {
                enumerationStatus_ = FABRIC_ENUMERATION_BEST_EFFORT_MASK;
            }
            
            wstring previousKey = NamePropertyKey::CreateKey(this->Name, body_.ContinuationToken.LastEnumeratedPropertyName);

            // Start from the last property we returned in previous enumeration request
            //
            error = Store.CreateEnumeration(
                txSPtr,
                Constants::NamedPropertyType,
                previousKey,
                enumSPtr);
        }
        else
        {
            propertiesVersion_ = nameOwnerEntry.PropertiesVersion;

            error = Store.CreateEnumeration(
                txSPtr,
                Constants::NamedPropertyType,
                storeKeyPrefix_,
                enumSPtr);
        }

        return error;
    }

    ErrorCode StoreService::ProcessEnumeratePropertiesRequestAsyncOperation::EnumerateProperties(
        EnumerationSPtr const & enumSPtr)
    {
        ErrorCode error;

        // Properties are stored in the database using the key: <Naming Uri><Delimiter><Property Name> 
        // For example: fabric:/a/b/c+MyProperty
        //
        // Since the database entries are sorted lexicographically by key, all properties created at
        // the same name will appear as contiguous entries.
        //
        while ((error = enumSPtr->MoveNext()).IsSuccess())
        {
            wstring storeKey;
            error = enumSPtr->CurrentKey(storeKey);

            // Check for end of contiguous property entries
            //
            if (error.IsSuccess() && !StringUtility::StartsWith(storeKey, storeKeyPrefix_))
            {
                error = ErrorCodeValue::EnumerationCompleted;
            }

            NamePropertyResult propertyResult;
            if (error.IsSuccess())
            {
                error = Store.ReadProperty(this->Name, enumSPtr, NamePropertyOperationType::EnumerateProperties, 0, propertyResult);
            }

            // If the property we enumerated last using this token is still there, skip it
            //
            if (error.IsSuccess() &&
                body_.ContinuationToken.IsValid && 
                propertyResult.Metadata.PropertyName ==  body_.ContinuationToken.LastEnumeratedPropertyName)
            {
                continue;
            }

            // Ensure that adding this result will not exceed NamingConfig::GetConfig().MaxMessageSize
            //
            bool fitsInReply = false;
            if (error.IsSuccess())
            {
                if (!body_.IncludeValues)
                {
                    propertyResult.ClearBytes();
                }

                fitsInReply = FitsInReplyMessage(propertyResult);
            }

            if (error.IsSuccess() && fitsInReply)
            {
                propertyResults_.push_back(propertyResult);
                lastEnumeratedPropertyName_ = propertyResult.Metadata.PropertyName;
            }
            else if (!error.IsSuccess())
            {
                WriteNoise(
                    TraceComponent,
                    "{0}: property enumeration stopping at '{1}': error = {2}",
                    this->TraceId,
                    storeKey,
                    error);
                break;
            }
            else
            {
                // next property does not fit in reply
                break;
            }
        } // while MoveNext()

        if (error.IsSuccess())
        {
            enumerationStatus_ = static_cast<FABRIC_ENUMERATION_STATUS>(enumerationStatus_ & FABRIC_ENUMERATION_MORE_DATA_MASK);
        }
        else if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            enumerationStatus_ = static_cast<FABRIC_ENUMERATION_STATUS>(enumerationStatus_ & FABRIC_ENUMERATION_FINISHED_MASK);
            error = ErrorCodeValue::Success;
        }

        WriteNoise(
            TraceComponent,
            "{0}: enumeration complete: status = {1} estimated size = {2}",
            this->TraceId,
            static_cast<int>(enumerationStatus_),
            replyTotalSizeEstimate_);

        return error;
    }

    bool StoreService::ProcessEnumeratePropertiesRequestAsyncOperation::FitsInReplyMessage(NamePropertyResult const & property)
    {
        // Estimate size with continuation token
        size_t estimate = EnumeratePropertiesResult::EstimateSerializedSize(property);

        // Account for EnumeratePropertiesResult overhead in final estimate
        //
        size_t finalEstimate = replyItemsSizeEstimate_ + estimate;

        if (finalEstimate <= replySizeThreshold_)
        {
            WriteNoise(
                TraceComponent,
                "{0}: fitting '{1}': size = {2} bytes",
                this->TraceId,
                property.Metadata.PropertyName,
                finalEstimate);

            // Only need to keep a running count of each NamePropertyResult
            //
            replyItemsSizeEstimate_ += property.EstimateSize();
            replyTotalSizeEstimate_ = finalEstimate;

            return true;
        }
        else
        {
            WriteNoise(
                TraceComponent,
                "{0}: stopping at '{1}': size = {2} limit = {3} (bytes)",
                this->TraceId,
                property.Metadata.PropertyName,
                finalEstimate,
                replySizeThreshold_);

            return false;
        }
    }
}
