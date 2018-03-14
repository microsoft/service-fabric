// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ServiceTypeStatusEventArgs
    {
    public:
        //TODO: Remove default value to force all references to explicitly pass isolation context.
        // The fault value specified here is temporary to get the code to compile.
        ServiceTypeStatusEventArgs(
            uint64 const sequenceNumber, 
            ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext activationContext,
            std::wstring const & servicePackagePublicActivationId);
        
        ServiceTypeStatusEventArgs(ServiceTypeStatusEventArgs const & other);
        ServiceTypeStatusEventArgs(ServiceTypeStatusEventArgs && other);

        __declspec(property(get=get_SequenceNumber)) uint64 const SequenceNumber;
        inline uint64 const get_SequenceNumber() const { return sequenceNumber_; };

         __declspec(property(get=get_ServiceTypeIdentifier)) ServiceModel::ServiceTypeIdentifier const & ServiceTypeId;
        inline ServiceModel::ServiceTypeIdentifier const & get_ServiceTypeIdentifier() const { return serviceTypeId_; };

        __declspec(property(get = get_ActivationContext)) ServiceModel::ServicePackageActivationContext const & ActivationContext;
        ServiceModel::ServicePackageActivationContext const & get_ActivationContext() const { return activationContext_; }
        
        __declspec(property(get = get_ServicePackagePublicActivationId)) std::wstring const & ServicePackagePublicActivationId;
        std::wstring const & get_ServicePackagePublicActivationId() const;

        ServiceTypeStatusEventArgs const & operator = (ServiceTypeStatusEventArgs const & other);
        ServiceTypeStatusEventArgs const & operator = (ServiceTypeStatusEventArgs && other);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    private:
         ServiceModel::ServiceTypeIdentifier serviceTypeId_;
         uint64 sequenceNumber_;
         ServiceModel::ServicePackageActivationContext activationContext_;
         std::wstring servicePackagePublicActivationId_;
    };
}
