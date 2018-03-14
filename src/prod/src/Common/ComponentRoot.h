// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ComponentRoot;
    typedef std::shared_ptr<ComponentRoot const> ComponentRootSPtr;
    typedef std::weak_ptr<ComponentRoot const> ComponentRootWPtr;

    class ComponentRoot : public std::enable_shared_from_this<ComponentRoot>
    {
        DENY_COPY_ASSIGNMENT(ComponentRoot)

    public:
        ComponentRoot(bool enableReferenceTracking = false);
        ComponentRoot(ComponentRoot const & other);
        ComponentRoot(ComponentRoot && other);
        virtual ~ComponentRoot();

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

        __declspec(property(get=get_IsReferenceTrackingEnabled)) bool IsReferenceTrackingEnabled;
        bool get_IsReferenceTrackingEnabled() const { return (referenceTracker_.get() != nullptr); }

        // Call this method when you need to capture and addref ComponentRootSPtr in lambda expressions, such as in 
        // event handler callbacks. For a given "async" operation (AsyncOperation, Timer, event handler), you only need to do addref once 
        // on the same ComponentRootSPtr by calling either CreateAsyncOperationRoot() or CreateComponentRoot() not both. For 
        // starting AsyncOperation, CreateAsyncOperationRoot() is the prefered way to do addref on ComponentRootSPtr
        //
        ComponentRootSPtr CreateComponentRoot() const;

        // Call this method to get the parent AsyncOperationSPtr when creating top level AsyncOperation, this will do an addref on 
        // ComponentRootSPtr
        AsyncOperationSPtr CreateAsyncOperationRoot() const;

        static AsyncOperationSPtr CreateAsyncOperationMultiRoot(std::vector<ComponentRootSPtr> && roots);

        virtual void TraceTrackedReferences() const;
        void TryStartLeakDetectionTimer();
        void TryStartLeakDetectionTimer(TimeSpan const);

        virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    protected:
        void SetTraceId(std::wstring const & traceId, size_t capacity = 0);

        // Allow derived classes to update the trace Id without memory allocation 
        // (i.e. avoid pointer/reference/iterator invalidation).
        // 
        bool TryPutReservedTraceId(std::wstring const & traceId);

    private:
        class ReferenceTracker;

        static void LeakDetectionTimerCallback(ComponentRootWPtr const &);

        std::wstring traceId_;
        std::unique_ptr<ReferenceTracker> referenceTracker_;
        TimerSPtr leakDetectionTimer_;
    };
}
