// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class RuntimeClosedEventArgs
    {
    public:

        RuntimeClosedEventArgs(
            uint64 const sequenceNumber, 
            std::wstring const & hostId, 
            std::wstring const & runtimeId, 
            std::vector<ServiceModel::ServiceTypeIdentifier> const & serviceTypes,
            ServiceModel::ServicePackageActivationContext isolationContext,
            std::wstring const & servicePackagePublicActivationId);
        
        
        RuntimeClosedEventArgs(RuntimeClosedEventArgs const & other);
        RuntimeClosedEventArgs(RuntimeClosedEventArgs && other);

        __declspec(property(get=get_SequenceNumber)) uint64 const SequenceNumber;
        inline uint64 const get_SequenceNumber() const { return sequenceNumber_; };

        __declspec(property(get=get_HostId)) std::wstring const & HostId;
        std::wstring const & get_HostId() const { return hostId_; }

        __declspec(property(get=get_RuntimeId)) std::wstring const & RuntimeId;
        std::wstring const & get_RuntimeId() const { return runtimeId_; }

        __declspec(property(get = get_ServiceTypes)) std::vector<ServiceModel::ServiceTypeIdentifier> const & ServiceTypes;
        std::vector<ServiceModel::ServiceTypeIdentifier> const & get_ServiceTypes() const { return serviceTypes_; }

        __declspec(property(get = get_ActivationContext)) ServiceModel::ServicePackageActivationContext const & ActivationContext;
        ServiceModel::ServicePackageActivationContext const & get_ActivationContext() const { return activationContext_; }

        __declspec(property(get = get_ServicePackagePublicActivationId)) std::wstring const & ServicePackagePublicActivationId;
        std::wstring const & get_ServicePackagePublicActivationId() const;

        RuntimeClosedEventArgs const & operator = (RuntimeClosedEventArgs const & other);
        RuntimeClosedEventArgs const & operator = (RuntimeClosedEventArgs && other);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    private:
        std::wstring hostId_;
        std::wstring runtimeId_;
        std::vector<ServiceModel::ServiceTypeIdentifier> serviceTypes_;
        uint64 sequenceNumber_;
        ServiceModel::ServicePackageActivationContext activationContext_;
        std::wstring servicePackagePublicActivationId_;
    };
}
