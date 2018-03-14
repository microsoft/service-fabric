// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{ 
    class TestFabricClient;
    
    struct TestNamingEntry
    {
    public:
        TestNamingEntry(Common::NamingUri name, ULONG propertyCount);

        __declspec (property(get=get_Name)) Common::NamingUri const& Name;
        Common::NamingUri const& get_Name() const {return name_;}

        __declspec (property(get=get_IsCreated,put=put_IsCreated)) bool IsCreated;
        bool get_IsCreated() const {return isCreated_;}
        void put_IsCreated(bool value);         

        wstring GetRandomPropertyName(Random & random)
        {
            int index = random.Next(0, static_cast<int>(properties_.size()));
            return name_.Name + StringUtility::ToWString(index);
        }

        wstring GetValue(wstring const& propertyName)
        {
            return properties_[propertyName];
        }

        void SetProperty(wstring const& propertyName, wstring const& propertyValue)
        {
            properties_[propertyName] = propertyValue;
        }

    private:
        Common::NamingUri name_;
        bool isCreated_;
        map<wstring, wstring> properties_;
    };

    class ComCallbackWaiter : 
        public IFabricAsyncOperationCallback,
        public ComUnknownBase
    {
        COM_INTERFACE_LIST1(
            ComCallbackWaiter,
            IID_IFabricAsyncOperationCallback,
            IFabricAsyncOperationCallback);

    public:
        ComCallbackWaiter() : 
            event_(false)
        {
        }
        
        void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext *) 
        {
            event_.Set();
        }

        bool WaitOne(TimeSpan timeout)
        { 
            return event_.WaitOne(timeout); 
        }

    private:
        AutoResetEvent event_;
    };

    class ComCallbackWrapper : 
        public IFabricAsyncOperationCallback,
        public ComUnknownBase
    {
        COM_INTERFACE_LIST1(
            ComCallbackWrapper,
            IID_IFabricAsyncOperationCallback,
            IFabricAsyncOperationCallback);

    public:
        typedef function<void(__in IFabricAsyncOperationContext *)> InnerCallback;

        ComCallbackWrapper(InnerCallback const & callback) : callback_(callback) { }

        void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext * context) 
        { 
            this->callback_(context);
        }

    private:
        InnerCallback callback_;
    };

}
