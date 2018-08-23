// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#ifdef PLATFORM_UNIX
#define MSG_PROPERTY_KEY(T) typeid(T).name()
#else
#define MSG_PROPERTY_KEY(T) typeid(T).raw_name()
#endif

namespace Transport
{
    typedef std::function<void(Common::ErrorCode const & error, MessageUPtr && message)> SendStatusCallback;
    typedef std::vector<Common::const_buffer>::iterator BufferIterator;

    class Message : public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>, public TransportObject
    {
        //DENY_COPY(Message);
        class MessageProperty;
        // The key type of message property table is "const char*", key value by default is typeid(property).raw_name().
        // By default, std::map compares "const char*" pointer value in key comparison, so we need our own key_compare.
        typedef std::map<const char*, std::shared_ptr<MessageProperty>, ConstCharPtrLess> PropertyMap;
        class BodyBuffers;

    public:

        // Constructing an outgoing message with no body
        Message();

        Message(Message const & other); // deep copy, should only be used for memory transport

        // Constructing an outgoing message with a body
        template <class T> explicit Message(T const & t, bool isBodyComplete = true);

        // Constructing an outgoing message with a list of buffers as message body
        typedef std::function<void (std::vector<Common::const_buffer> const &, void *)> DeleteCallback;

        // deleteCallback must be specified
        Message(std::vector<Common::const_buffer> const & buffers, DeleteCallback const & deleteCallback, void * state);

        // Constructing an incoming message from buffer, intended for framing layer
        Message(
            ByteBiqueRange && headersRange,
            ByteBiqueRange && bodyRange,
            Common::StopwatchTime recvTime,
            size_t reserveBodyBufferCount = 0);

        ~Message() override;

        NTSTATUS GetStatus() const override;

        bool HasReceiveTime() const { return headers_.HasReceiveTime(); }
        Common::StopwatchTime ReceiveTime() const { return headers_.ReceiveTime(); }
        static Common::StopwatchTime NullReceiveTime() { return MessageHeaders::NullReceiveTime(); }

        Transport::MessageId const & TraceId() const;
        void SetTraceId(MessageId const & value);

        std::wstring const & LocalTraceContext() const;
        void SetLocalTraceContext(std::wstring const & context);
        void SetLocalTraceContext(std::wstring && context);
        void MoveLocalTraceContextFrom(Message & rhs);

        void SetSendStatusCallback(SendStatusCallback const & sendStatusCallback);
        bool HasSendStatusCallback() const;
        void OnSendStatus(Common::ErrorCode const & error, MessageUPtr && message);

        // For tracing
        bool IsReply() const;

        __declspec(property(get=get_Headers)) MessageHeaders & Headers;
        __declspec(property(get=get_Action)) std::wstring const & Action;
        __declspec(property(get=get_Actor)) Actor::Enum Actor;
        __declspec(property(get=get_MessageId)) Transport::MessageId const & MessageId;
        __declspec(property(get=get_RelatesTo)) Transport::MessageId const & RelatesTo;
        __declspec(property(get=get_RetryCount)) LONG const & RetryCount;
        __declspec(property(get=get_ExpectsReply)) bool ExpectsReply;
        __declspec(property(get=get_Idempotent, put=set_Idempotent)) bool Idempotent;
        __declspec(property(get=get_HighPriority)) bool HighPriority;
        __declspec(property(get=get_FaultErrorCodeValue)) Common::ErrorCodeValue::Enum FaultErrorCodeValue;
        __declspec(property(get=get_HasFaultBody)) bool HasFaultBody;
        __declspec(property(get=get_IsUncorrelatedReply)) bool IsUncorrelatedReply;

        std::wstring const & get_Action() const;
        SendStatusCallback const & get_SendStatusCallback() const;
        Actor::Enum get_Actor() const;
        Transport::MessageId const & get_MessageId() const;
        Transport::MessageId const & get_RelatesTo() const;
        LONG const & get_RetryCount() const;
        bool get_ExpectsReply() const;
        bool get_Idempotent() const { return headers_.Idempotent; }
        void set_Idempotent(bool value) { headers_.Idempotent = value; }
        bool get_HighPriority() const;
        Common::ErrorCodeValue::Enum get_FaultErrorCodeValue() const;
        bool get_HasFaultBody() const;
        bool get_IsUncorrelatedReply() const;

        // Clone() returns a "copy" of a Message instance. The clone shares message body and "existing headers" (headers passed to Message
        // constructor) with the orginal. "New headers" (headers added after construction) are copied. Message properties are copied.
        // Note: removing a header from a clone will affect the original, if the header is in "sharedHeaders_" 
        MessageUPtr Clone();

        // This merges "existing headers" and new headers together into "existing headers", so that clone operation can be done more 
        // efficiently.
        void CheckPoint();

        // This behaves the same as parameter-less Clone(), other than adding extraHeaders as "new headers" to the clone.
        MessageUPtr Clone(MessageHeaders && extraHeaders);

        uint SerializedSize();

        //
        // Headers
        //
        MessageHeaders& get_Headers();
        // Iterate over the serialized chunks of the headers.
        BiqueChunkIterator BeginHeaderChunks();
        BiqueChunkIterator EndHeaderChunks();
        uint SerializedHeaderSize(); // The total size of serialized message headers, incoming plus locally added

        //
        // properties
        //
        // Property "key" parameter has a default value: typeid(T).raw_name()
        // ASSERT_IF(another property of the same key exists).
        template <class T> void AddProperty(T const & prop, const char* key = MSG_PROPERTY_KEY(T)); // copy-by-value

        // ASSERT_IF(another property of the same key exists).
        template <class T> void AddProperty(std::unique_ptr<T> && prop, const char* key = MSG_PROPERTY_KEY(T)); // move semantics
        // ASSERT_IF(another property of the same key exists).
        template <class T> void AddProperty(std::shared_ptr<T> const & prop, const char* key = MSG_PROPERTY_KEY(T)); // copy/addref semantics
        // Returns true if the property exists.
        template <class T> bool RemoveProperty(const char* key = MSG_PROPERTY_KEY(T));
        // Returns NULL if the property does not exist.
        template <class T> T* TryGetProperty(const char* key = MSG_PROPERTY_KEY(T)) const;
        // ASSERT_IF(property does not exist).
        template <class T> T& GetProperty(const char* key = MSG_PROPERTY_KEY(T)) const;

        //
        // Body
        //
        // Currently, only one body object per Messge is supported, and also it is assumed that 
        // receiver knows how to deserialize based one information in MessageHeaders
        template <class T> bool GetBody(T& t)
        {
            return GetBodyInternal(&t);
        }

        template <>
        inline bool GetBody<std::vector<Common::const_buffer>>(std::vector<Common::const_buffer> & t)
        {
            t = std::vector<Common::const_buffer>(this->bodyBuffers_->GetBuffers());
            return true;
        }

        Serialization::IFabricSerializableStreamUPtr GetBodyStream();

        // Iterate over the serialized chunks of the message body.
        BufferIterator BeginBodyChunks();
        BufferIterator EndBodyChunks();
        uint SerializedBodySize();

        template <class T> void AddBody(T const & t, bool isBodyComplete);

        void SetSecurityContext(SecurityContext* context);
        SecurityContext const* SecurityContextPtr() const { return securityContext_.get(); }
        bool IsInRole(RoleMask::Enum roleMask);
        bool AccessCheck(AccessControl::FabricAcl const & acl, DWORD desiredAccess) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;

    private:

        Message(
            ByteBiqueRange && headersRange,
            ByteBiqueRange && bodyRange,
            std::shared_ptr<BodyBuffers> const & buffers,
            Common::StopwatchTime recvTime);

        bool GetBodyInternal(Serialization::IFabricSerializable* body);

        MessageUPtr CloneSharedParts();

        // Usage scenario: after SSL decryption, this function allows skipping SSL header and trailer without copying decrypted data.
        // This operation does not affect lifetime management of the buffers, which is still managed by ByteBiqueRange Message::body_.
        void UpdateBodyBufferList(BodyBuffers && other);

        // todo: make this a smart pointer, so that we do not incur the cost of initial map memory allocation until properties are added.
        PropertyMap properties_;
        MessageHeaders headers_;

        // On the incoming side, body_ always contains serialized message body. On the outgoing side, when user provides serialized 
        // message body buffers to Message constructor, body_ doesn't contain actual message body.
        ByteBiqueRange body_;
        std::shared_ptr<BodyBuffers> bodyBuffers_; // This always contains the buffer segment list for message body

        SecurityContextSPtr securityContext_;

        std::wstring localTraceContext_;

        SendStatusCallback sendStatusCallback_;

        // This class is used to  wrap message property, so that all message properties of different type can be stored in a single container.
        class MessageProperty
        {
        public:
            virtual ~MessageProperty() {}
            virtual void* GetPointer() const = 0;
        };

        // This is for small size message property type
        template <class T>
        class MessagePropertyByValue : public MessageProperty
        {
        public:
            MessagePropertyByValue(const T& t) : t_(t) {}
            virtual void* GetPointer() const
            {
                return const_cast<T*>(&t_);
            }

        private:
            T t_;
        };

        template <class T>
        class MessagePropertyByUPtr : public MessageProperty
        {
        public:
            MessagePropertyByUPtr(std::unique_ptr<T>&& uptr) : uptr_(std::move(uptr)) {}
            virtual void* GetPointer() const
            {
                return static_cast<void*>(uptr_.get());
            }

        private:
            std::unique_ptr<T> uptr_;
        };

        template <class T>
        class MessagePropertyBySPtr : public MessageProperty
        {
        public:
            MessagePropertyBySPtr(const std::shared_ptr<T>& sptr) : sptr_(sptr) {}
            virtual void* GetPointer() const
            {
                return static_cast<void*>(sptr_.get());
            }

        private:
            std::shared_ptr<T> sptr_;
        };

        class BodyBuffers
        {
        public:

            explicit BodyBuffers(Serialization::IFabricSerializableStreamUPtr && streamUPtr);
            explicit BodyBuffers(size_t reserveCount);
            BodyBuffers(ByteBiqueRange & body, size_t reserveCount = 0);
            BodyBuffers(std::vector<Common::const_buffer> const & buffers, DeleteCallback const & callback = nullptr, void * state = nullptr);
            ~BodyBuffers();

            BufferIterator begin();
            BufferIterator end();
            size_t size();
            void AddBuffer(void const *buffer, ULONG size); 
            void UpdateBufferList(BodyBuffers && other);
            void PrepareForSend();
            std::vector<Common::const_buffer> const & GetBuffers() const;
            Serialization::IFabricSerializableStreamUPtr const & GetStream() const { return streamUPtr_; }

        private:
            std::vector<Common::const_buffer> buffers_;
            DeleteCallback callback_;
            void * state_;
            size_t totalSize_;

            Serialization::IFabricSerializableStreamUPtr streamUPtr_;
        };

        friend class SecurityContextSsl;
    };

    // todo: find out if we can get rid of "body_(EmptyByteBique.begin(), EmptyByteBique.end(), false)" in the initialization list,
    template <class T> Message::Message(T const & t, bool isBodyComplete) 
        : body_(EmptyByteBique.begin(), EmptyByteBique.end(), false)
    {
        Serialization::IFabricSerializableStream * stream = nullptr;

        NTSTATUS status = Serialization::FabricSerializableStream::Create(&stream);

        Serialization::IFabricSerializableStreamUPtr streamUPtr(stream);

        // todo, consider setting failure status on message instead of assert
        if (NT_ERROR(status) || stream == nullptr)
        {
            Common::Assert::CodingError("Insufficient resources constructing a stream");
        }

        this->bodyBuffers_ = std::make_shared<BodyBuffers>(Ktl::Move(streamUPtr));

        AddBody(t, isBodyComplete);
    }

    template <class T>
    void Message::AddBody(T const & t, bool isBodyComplete)
    {
        Serialization::IFabricSerializableStreamUPtr const & streamUPtr = this->bodyBuffers_->GetStream();
        ASSERT_IF(this->bodyBuffers_->size() != 0, "Body already complete")

        Serialization::IFabricSerializable * body = const_cast<T*>(&t);
        NTSTATUS status = streamUPtr->WriteSerializable(body);
        if (NT_ERROR(status))
        {
            Common::Assert::CodingError("Writing object to stream failed: {0}", status);
        }

        if (isBodyComplete)
        {
            this->bodyBuffers_->PrepareForSend();
        }
    }

    template <class T>
    void Message::AddProperty(T const & prop, const char* key) // copy-by-value
    {        
        std::shared_ptr<MessageProperty> value = std::make_shared<MessagePropertyByValue<T>>(prop);
        bool inserted = properties_.emplace(std::move(std::pair<const char*, std::shared_ptr<MessageProperty>>(key, std::move(value)))).second;
        ASSERT_IF(!inserted, "Message property key already existed.");
    }

    template <class T> 
    void Message::AddProperty(std::unique_ptr<T> && prop, const char* key) // move semantics
    {
        std::shared_ptr<MessageProperty> value = std::make_shared<MessagePropertyByUPtr<T>>(std::move(prop));
        bool inserted = properties_.emplace(std::move(std::pair<const char*, std::shared_ptr<MessageProperty>>(key, std::move(value)))).second;
        ASSERT_IF(!inserted, "Message property key already existed.");
    }

    template <class T> 
    void Message::AddProperty(std::shared_ptr<T> const & prop, const char* key) // copy/addref semantics
    {
        std::shared_ptr<MessageProperty> value = std::make_shared<MessagePropertyBySPtr<T>>(prop);
        bool inserted = properties_.emplace(std::move(std::pair<const char*, std::shared_ptr<MessageProperty>>(key, std::move(value)))).second;
        ASSERT_IF(!inserted, "Message property key already existed.");
    }

    // Returns true if the property exists.
    template <class T> 
    bool Message::RemoveProperty(const char* key)
    {
        auto iter = properties_.find(key);
        if (iter != properties_.end())
        {
            properties_.erase(iter);
            return true;
        }
        else
        {
            return false;
        }
    } 

    // Returns NULL if the property does not exist.
    template <class T> 
    T* Message::TryGetProperty(const char* key) const
    {
        PropertyMap::const_iterator iter = properties_.find(key);
        if (iter != properties_.end())
        {
            return static_cast<T*>(iter->second->GetPointer());
        }
        else
        {
            return nullptr;
        }
    }

    template <class T>
    T& Message::GetProperty(const char* key) const
    {
        T* result = TryGetProperty<T>(key);
        ASSERT_IF(result == nullptr, "Message property key does not exist.");
        #pragma warning(suppress : 6011) // there is assert above
        return *result;
    } 
}
