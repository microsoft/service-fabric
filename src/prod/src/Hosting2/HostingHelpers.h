//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class EndpointsHelper : Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
    public:

        static Common::ErrorCode ConvertToJsonString(
            std::wstring const & serviceInfo,
            std::vector<ServiceModel::EndpointDescription> const & endpointDescriptions, 
            __out std::wstring & endpointsJsonString);
    
    private:
        class EndpointsJsonWrapper;
    };

    class ComCompletedOperationHelper
    {
    public:
        static HRESULT BeginCompletedComOperation(
            IFabricAsyncOperationCallback *callback,
            IFabricAsyncOperationContext **context);

        static HRESULT EndCompletedComOperation(
            IFabricAsyncOperationContext *context);
    };

    class GuestServiceTypeInfo
    {
    public:
        GuestServiceTypeInfo()
            : serviceTypeName_()
            , isStateful_(false)
        {
        }

        GuestServiceTypeInfo(
            std::wstring serviceTypeName,
            bool isStateful)
            : serviceTypeName_(serviceTypeName)
            , isStateful_(isStateful)
        {
        }

        __declspec(property(get = get_ServiceTypeName)) std::wstring const & ServiceTypeName;
        std::wstring const & get_ServiceTypeName() const { return this->serviceTypeName_; }

        __declspec(property(get = get_IsStatefulGuestServiceType)) bool IsStateful;
        bool get_IsStatefulGuestServiceType() const { return this->isStateful_; }

    private:
        wstring serviceTypeName_;
        bool isStateful_;
    };

    class HelperUtilities
    {
    public:
        static std::wstring GetSecondaryCodePackageEventName(ServicePackageInstanceIdentifier const & servicePackageId, bool forPrimary, bool eventCompleted);
    };
}