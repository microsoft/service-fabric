// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TestCommon.h"
#include <stdint.h>

namespace Transport
{
    using namespace Common;
    using namespace std;

    BOOST_AUTO_TEST_SUITE2(MessageTests)

    struct ExampleHeader : public MessageHeader<MessageHeaderId::Example>, public Serialization::FabricSerializable
    {
        ExampleHeader() {}
        ExampleHeader(std::wstring name, std::vector<LONG> samples)
            : name_(std::move(name)), samples_(std::move(samples)) {}
        ExampleHeader(std::wstring name, std::vector<LONG> samples, unsigned int seq)
            : name_(std::move(name)), samples_(std::move(samples)), seq_(seq) {}

        FABRIC_FIELDS_03(name_, samples_, seq_);

        std::wstring const & Name() const { return name_; }
        std::vector<LONG> const & Samples() const { return samples_; }
        unsigned int Seq() const { return seq_; }

    private:
        std::wstring name_;
        std::vector<LONG> samples_;
        uint seq_;
    };

    struct ExampleHeader2 : public MessageHeader<MessageHeaderId::Example2>, public Serialization::FabricSerializable
    {
        ExampleHeader2() {}
        ExampleHeader2(std::wstring key, std::wstring value)
            : key_(std::move(key)), value_(std::move(value)) {}
        ExampleHeader2(std::wstring key, std::wstring value, unsigned int seq)
            : key_(key), value_(value), seq_(seq) {}

        FABRIC_FIELDS_03(key_, value_, seq_);

        std::wstring const & Key() const { return key_; }
        std::wstring const & Value() const { return value_; }
        unsigned int Seq() const { return seq_; }

    private:
        std::wstring key_;
        std::wstring value_;
        uint seq_;
    };

    struct ExampleHeader3 : public MessageHeader<MessageHeaderId::Example3>, public Serialization::FabricSerializable
    {
        ExampleHeader3() { }
        ExampleHeader3(std::wstring key, std::wstring value)
            : key_(std::move(key)), value_(std::move(value)) {}
        ExampleHeader3(std::wstring key, std::wstring value, unsigned int seq)
            : key_(std::move(key)), value_(std::move(value)), seq_(seq) {}

        FABRIC_FIELDS_03(key_, value_, seq_);

        std::wstring const & Key() const { return key_; }
        std::wstring const & Value() const { return value_; }
        unsigned int Seq() const { return seq_; }

    private:
        std::wstring key_;
        std::wstring value_;
        uint seq_;
    };

    class MessageBodyExample : public Serialization::FabricSerializable
    {
    public:
        MessageBodyExample() {}
        MessageBodyExample(wstring&& key, wstring&& value) : key_(std::move(key)), value_(std::move(value)) {}
        std::wstring const & Key() const { return key_; }
        std::wstring const & Value() const { return value_; }
        FABRIC_FIELDS_02(key_, value_);

    private:
        wstring key_;
        wstring value_;
    };

    class MessagePropertyUserDefined
    {
    public:
        MessagePropertyUserDefined(const std::wstring& value) : value_(value) 
        {
            thisPtr_ = reinterpret_cast<std::uintptr_t>(this);
            tickCount_ = Stopwatch::Now();
            Trace.WriteNoise(TraceType, "MessagePropertyUserDefined contructed at {0}:{1}", thisPtr_, tickCount_);
        }

        MessagePropertyUserDefined(const MessagePropertyUserDefined& source) : value_(source.value_)
        {
            thisPtr_ = reinterpret_cast<std::uintptr_t>(this);
            tickCount_ = Stopwatch::Now();
            Trace.WriteNoise(TraceType, "MessagePropertyUserDefined copy-contructed at {0}:{1}", thisPtr_, tickCount_);
        }

        ~MessagePropertyUserDefined()
        {
            Trace.WriteNoise(TraceType, "~MessagePropertyUserDefined called on {0}:{1}", thisPtr_, tickCount_); 
        }

        const std::wstring& Value() const { return value_; } 

    private:
        std::wstring value_;

        // The following two fields are used to ID an object for tracing purpose
        std::uintptr_t thisPtr_;
        StopwatchTime tickCount_;
    };

    struct MessageHeadersDeserializationResult
    {
        size_t headerNumber = 0;
        size_t exampleHeaderNumber = 0;
        size_t example2HeaderNumber = 0;
        size_t example3HeaderNumber = 0;
        size_t bytesDeserialized = 0;
    };

    void AddMessageHeaders(MessageHeaders& messageHeaders, int number)
    {
        Trace.WriteInfo(TraceType, "Adding message headers ...");

        for (LONG i = 0; i < number; i ++)
        {
            switch (i % 3)
            {
            case 0:
                {
                    auto name = std::wstring(L"ExampleHeader");
                    auto samples = std::vector<LONG>();
                    for (LONG j = 0; j < i * 5; j++) { samples.push_back(i); }
                    ExampleHeader header(std::move(name), std::move(samples), i);
                    messageHeaders.Add(header);
                    Trace.WriteInfo(TraceType, "{0}: ExampleHeader added.", i);
                }
                break;

            case 1:
                {
                    auto key = std::wstring(L"ExampleHeader2 Key");
                    auto value = std::wstring(L"ExampleHeader2 Value");
                    ExampleHeader2 header(std::move(key), std::move(value), i);
                    messageHeaders.Add(header);
                    Trace.WriteInfo(TraceType, "{0}: ExampleHeader2 added.", i);
                }
                break;

            case 2:  
                {
                    auto key = std::wstring(L"ExampleHeader3 Key");
                    auto value = std::wstring(L"ExampleHeader3 Value");
                    ExampleHeader3 header(std::move(key), std::move(value), i);
                    messageHeaders.Add(header);
                    Trace.WriteInfo(TraceType, "{0}: ExampleHeader3 added.", i);
                }
                break;
            }
        }
    }

    template <class T> void AddHeader(BiqueWriteStream& biqueStream, T const & header)
    {
        biqueStream << header.Id;

        // Write a placeholder for header size and record its position
        MessageHeaderSize headerSize = 0;
        biqueStream << headerSize;
        ByteBiqueIterator headerSizeCursor = biqueStream.GetCursor() - sizeof(MessageHeaderSize);

        size_t beforeSize = biqueStream.Size();

        std::vector<byte> bytes;

        Serialization::IFabricSerializable * serializable = const_cast<T*>(&header);

        VERIFY_IS_TRUE(FabricSerializer::Serialize(serializable, bytes).IsSuccess());
        biqueStream.WriteBytes(bytes.data(), bytes.size());

        size_t afterSize = biqueStream.Size();

        CODING_ERROR_ASSERT((afterSize - beforeSize) <= std::numeric_limits<MessageHeaderSize>::max());
        headerSize = (MessageHeaderSize)(afterSize - beforeSize);

        // Write actual header size
        biqueStream.SeekTo(headerSizeCursor);
        biqueStream << headerSize; 
        biqueStream.SeekToEnd();
    }

    void AddMessageHeaders(BiqueWriteStream& biqueStream, int number)
    {
        Trace.WriteInfo(TraceType, "Adding message headers ...");

        for (LONG i = 0; i < number; i ++)
        {
            switch (i % 3)
            {
            case 0:
                {
                    auto name = std::wstring(L"ExampleHeader");
                    auto samples = std::vector<LONG>();
                    for (LONG j = 0; j < i * 5; j++) { samples.push_back(i); }
                    ExampleHeader header(std::move(name), std::move(samples), i);
                    AddHeader(biqueStream, header);
                    Trace.WriteInfo(TraceType, "{0}: ExampleHeader added.", i);
                }
                break;

            case 1:
                {
                    auto key = std::wstring(L"ExampleHeader2 Key");
                    auto value = std::wstring(L"ExampleHeader2 Value");
                    ExampleHeader2 header(std::move(key), std::move(value), i);
                    AddHeader(biqueStream, header);
                    Trace.WriteInfo(TraceType, "{0}: ExampleHeader2 added.", i);
                }
                break;

            case 2:  
                {
                    auto key = std::wstring(L"ExampleHeader3 Key");
                    auto value = std::wstring(L"ExampleHeader3 Value");
                    ExampleHeader3 header(std::move(key), std::move(value), i);
                    AddHeader(biqueStream, header);
                    Trace.WriteInfo(TraceType, "{0}: ExampleHeader3 added.", i);
                }
                break;
            }
        }
    }

    MessageHeadersDeserializationResult MessageHeadersIterateAllExplicitly(MessageHeaders& messageHeaders)
    {
        Trace.WriteInfo(TraceType, "Test iterator explicitly ...");
        MessageHeadersDeserializationResult result;
        for(auto header_iter = messageHeaders.Begin(); header_iter != messageHeaders.End(); ++header_iter)
        {
            Trace.WriteInfo(TraceType, "header[{0}] = {1}", result.headerNumber, header_iter->Id());
            ++ result.headerNumber;

            switch (header_iter->Id())
            {
            case MessageHeaderId::Example:
                {
                    ExampleHeader header = header_iter->Deserialize<ExampleHeader>();
                    ++ result.exampleHeaderNumber;
                    result.bytesDeserialized += header_iter->ByteTotalInStream();
                    Trace.WriteInfo(TraceType,
                        "{0}: ExampleHeader: name = {1}, sample size = {2}", 
                        header.Seq(), 
                        header.Name(),
                        header.Samples().size());
                }
                break;

            case MessageHeaderId::Example2:
                {
                    ExampleHeader2 header = header_iter->Deserialize<ExampleHeader2>();
                    ++ result.example2HeaderNumber;
                    result.bytesDeserialized += header_iter->ByteTotalInStream();
                    Trace.WriteInfo(TraceType,
                        "{0}: ExampleHeader2: key = {1}, value = {2}",
                        header.Seq(),
                        header.Key(),
                        header.Value());
                }
                break;

            case MessageHeaderId::Example3:
                {
                    ExampleHeader3 header = header_iter->Deserialize<ExampleHeader3>();
                    ++ result.example3HeaderNumber;
                    result.bytesDeserialized += header_iter->ByteTotalInStream();
                    Trace.WriteInfo(TraceType,
                        "{0}: ExampleHeader3: key = {1}, value = {2}",
                        header.Seq(),
                        header.Key(),
                        header.Value());
                }
                break;
            }
            Trace.WriteNoise(TraceType, "Bytes deserialized: {0}", result.bytesDeserialized);
        }

        return result;
    }

    MessageHeadersDeserializationResult MessageHeadersIterateAllByForEach(MessageHeaders& messageHeaders)
    {
        Trace.WriteInfo(TraceType, "Testing iterator with \"for each\" ...");
        MessageHeadersDeserializationResult result;
        for(MessageHeaders::HeaderReference & headerRef : messageHeaders)
        {
            ++ result.headerNumber;

            switch (headerRef.Id())
            {
            case MessageHeaderId::Example:
                {
                    ExampleHeader header = headerRef.Deserialize<ExampleHeader>();
                    ++ result.exampleHeaderNumber;
                    result.bytesDeserialized += headerRef.ByteTotalInStream();
                    Trace.WriteInfo(TraceType,
                        "{0}: ExampleHeader: name = {1}, sample size = {2}", 
                        header.Seq(), 
                        header.Name(),
                        header.Samples().size());
                }
                break;

            case MessageHeaderId::Example2:
                {
                    ExampleHeader2 header = headerRef.Deserialize<ExampleHeader2>();
                    ++ result.example2HeaderNumber;
                    result.bytesDeserialized += headerRef.ByteTotalInStream();
                    Trace.WriteInfo(TraceType,
                        "{0}: ExampleHeader2: key = {1}, value = {2}",
                        header.Seq(),
                        header.Key(),
                        header.Value());
                }
                break;

            case MessageHeaderId::Example3:
                {
                    ExampleHeader3 header = headerRef.Deserialize<ExampleHeader3>();
                    ++ result.example3HeaderNumber;
                    result.bytesDeserialized += headerRef.ByteTotalInStream();
                    Trace.WriteInfo(TraceType,
                        "{0}: ExampleHeader3: key = {1}, value = {2}",
                        header.Seq(),
                        header.Key(),
                        header.Value());
                }
                break;
            }
            Trace.WriteNoise(TraceType, "Bytes deserialized: {0}", result.bytesDeserialized);
        }

        return result;
    }

    void RemoveHeadersAtOddIndex(MessageHeaders& messageHeaders)
    {
        Trace.WriteInfo(TraceType, "Remove message headers at odd index ...");
        int i = 0;
        for(auto header_iter = messageHeaders.Begin(); header_iter != messageHeaders.End(); )
        {
            if (i % 2 == 1)
            {
                messageHeaders.Remove(header_iter);
            }
            else
            {
                ++header_iter;
            }

            ++i;
        }
    }

    void RemoveHeadersAtEvenIndex(MessageHeaders& messageHeaders)
    {
        Trace.WriteInfo(TraceType, "Remove message headers at even index ...");
        int i = 0;
        for(auto header_iter = messageHeaders.Begin(); header_iter != messageHeaders.End(); )
        {
            if (i % 2 == 0)
            {
                messageHeaders.Remove(header_iter);
            }
            else
            {
                ++header_iter;
            }

            ++i;
        }
    }

    bool TryReadFirstExampleHeader(MessageHeaders& messageHeaders)
    {
        ExampleHeader headerRead;
        if (messageHeaders.TryReadFirst(headerRead))
        {
            Trace.WriteInfo(TraceType,
                "{0}: {1}: ExampleHeader: name = {2}, sample size = {3}", 
                __FUNCTION__,
                headerRead.Seq(), 
                headerRead.Name(),
                headerRead.Samples().size());
            return true;
        }
        else
        {
            Trace.WriteInfo(TraceType, "{0}: could not find ExampleHeader.", __FUNCTION__);
            return false;
        }
    }

    bool TryReadFirstExampleHeader2(MessageHeaders& messageHeaders)
    {
        ExampleHeader2 headerRead;
        if (messageHeaders.TryReadFirst(headerRead))
        {
            Trace.WriteInfo(TraceType,
                "{0}: {1}: header key = {1}, value = {2}", 
                __FUNCTION__,
                headerRead.Seq(),
                headerRead.Key(), 
                headerRead.Value());
            return true;
        }
        else
        {
            Trace.WriteInfo(TraceType, "{0}: could not find ExampleHeader2.", __FUNCTION__);
            return false;
        }
    }

    BOOST_AUTO_TEST_CASE(SerializeOnce_SingleThread) // Serialize headers once, copy to messages without buffer merging
    {
        // create a stream object to add all shared headers
        auto sharedStream = FabricSerializer::CreateSerializableStream();

        wstring testAction = L"SerializeOnce";
        auto status = MessageHeaders::Serialize(*sharedStream, ActionHeader(testAction));
        BOOST_REQUIRE(NT_SUCCESS(status));

        auto testActor = Actor::GenericTestActor;
        status = MessageHeaders::Serialize(*sharedStream, ActorHeader(testActor));
        BOOST_REQUIRE(NT_SUCCESS(status));

        ExampleHeader2 eh2(Guid::NewGuid().ToString(), Guid::NewGuid().ToString(), 101);
        status = MessageHeaders::Serialize(*sharedStream, eh2);
        BOOST_REQUIRE(NT_SUCCESS(status));

        ExampleHeader3 eh3(
            L"12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",
            L"asdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnm",
            202);
        status = MessageHeaders::Serialize(*sharedStream, eh3);
        BOOST_REQUIRE(NT_SUCCESS(status));

        // add shared headers to messages
        MessageId relatesTo1;
        auto msg1 = make_unique<Message>();
        msg1->Headers.Add(MessageIdHeader()); // non-shared
        msg1->Headers.Add(*sharedStream, true); // shared
        msg1->Headers.Add(RelatesToHeader(relatesTo1)); // no-shared

        VERIFY_ARE_EQUAL2(msg1->Actor, testActor);
        VERIFY_ARE_EQUAL2(msg1->Action, testAction);
        VERIFY_ARE_EQUAL2(msg1->RelatesTo, relatesTo1);

        auto msg2 = make_unique<Message>();
        msg2->Headers.Add(MessageIdHeader()); // non-shared
        msg2->Headers.Add(*sharedStream, true);
        MessageId relatesTo2;
        msg2->Headers.Add(RelatesToHeader(relatesTo2)); // no-shared

        VERIFY_ARE_EQUAL2(msg2->Actor, testActor);
        VERIFY_ARE_EQUAL2(msg2->Action, testAction);
        VERIFY_ARE_EQUAL2(msg2->RelatesTo, relatesTo2);

        ExampleHeader2 deh2;
        auto foundExampleHeader2 = msg1->Headers.TryReadFirst(deh2);
        VERIFY_IS_TRUE(foundExampleHeader2);
        VERIFY_ARE_EQUAL2(deh2.Key(), eh2.Key());
        VERIFY_ARE_EQUAL2(deh2.Value(), eh2.Value());
        VERIFY_ARE_EQUAL2(deh2.Seq(), eh2.Seq());

        deh2 = ExampleHeader2();
        foundExampleHeader2 = msg2->Headers.TryReadFirst(deh2);
        VERIFY_IS_TRUE(foundExampleHeader2);
        VERIFY_ARE_EQUAL2(deh2.Key(), eh2.Key());
        VERIFY_ARE_EQUAL2(deh2.Value(), eh2.Value());
        VERIFY_ARE_EQUAL2(deh2.Seq(), eh2.Seq());

        ExampleHeader3 deh3;
        auto foundExampleHeader3 = msg1->Headers.TryReadFirst(deh3);
        VERIFY_IS_TRUE(foundExampleHeader3);
        VERIFY_ARE_EQUAL2(deh3.Key(), eh3.Key());
        VERIFY_ARE_EQUAL2(deh3.Value(), eh3.Value());
        VERIFY_ARE_EQUAL2(deh3.Seq(), eh3.Seq());

        foundExampleHeader3 = msg2->Headers.TryReadFirst(deh3);
        VERIFY_IS_TRUE(foundExampleHeader3);
        VERIFY_ARE_EQUAL2(deh3.Key(), eh3.Key());
        VERIFY_ARE_EQUAL2(deh3.Value(), eh3.Value());
        VERIFY_ARE_EQUAL2(deh3.Seq(), eh3.Seq());

        VERIFY_ARE_NOT_EQUAL2(msg1->MessageId, msg2->MessageId);
    }

    BOOST_AUTO_TEST_CASE(SerializeOnce_MultiThread) // Serialize headers once, copy to messages without buffer merging
    {
        auto testAction = wstring(L"SerializeOnce");
        auto testActor = Actor::GenericTestActor;
        ExampleHeader2 eh2(Guid::NewGuid().ToString(), Guid::NewGuid().ToString(), 101);
        ExampleHeader3 eh3(
            L"12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",
            L"asdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnm",
            202);

        // create a stream object to add all shared headers
        auto sharedStream = FabricSerializer::CreateSerializableStream();
        {
            auto status = MessageHeaders::Serialize(*sharedStream, ActionHeader(testAction));
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = MessageHeaders::Serialize(*sharedStream, ActorHeader(testActor));
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = MessageHeaders::Serialize(*sharedStream, eh2);
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = MessageHeaders::Serialize(*sharedStream, eh3);
            BOOST_REQUIRE(NT_SUCCESS(status));
        }

        // add shared headers to many messages
        auto completeCount = make_shared<atomic_uint64>(0);
        auto allCompleted = make_shared<AutoResetEvent>(false);
        const size_t threadCount = 12;

        auto messageIdSet = make_shared<set<MessageId>>();
        auto lock = make_shared<RwLock>();

        for (int i = 0; i < threadCount; ++i)
        {
            Threadpool::Post([=, &sharedStream]
            {
                MessageId relatesTo1;
                auto msg1 = make_unique<Message>();
                msg1->Headers.Add(MessageIdHeader()); // non-shared
                msg1->Headers.Add(*sharedStream, true); // shared
                msg1->Headers.Add(RelatesToHeader(relatesTo1)); // no-shared

                VERIFY_ARE_EQUAL2(msg1->Actor, testActor);
                VERIFY_ARE_EQUAL2(msg1->Action, testAction);
                VERIFY_ARE_EQUAL2(msg1->RelatesTo, relatesTo1);

                auto msg2 = make_unique<Message>();
                msg2->Headers.Add(MessageIdHeader()); // non-shared
                msg2->Headers.Add(*sharedStream, true);
                MessageId relatesTo2;
                msg2->Headers.Add(RelatesToHeader(relatesTo2)); // no-shared

                VERIFY_ARE_EQUAL2(msg2->Actor, testActor);
                VERIFY_ARE_EQUAL2(msg2->Action, testAction);
                VERIFY_ARE_EQUAL2(msg2->RelatesTo, relatesTo2);

                ExampleHeader2 deh2;
                auto foundExampleHeader2 = msg1->Headers.TryReadFirst(deh2);
                VERIFY_IS_TRUE(foundExampleHeader2);
                VERIFY_ARE_EQUAL2(deh2.Key(), eh2.Key());
                VERIFY_ARE_EQUAL2(deh2.Value(), eh2.Value());
                VERIFY_ARE_EQUAL2(deh2.Seq(), eh2.Seq());

                deh2 = ExampleHeader2();
                foundExampleHeader2 = msg2->Headers.TryReadFirst(deh2);
                VERIFY_IS_TRUE(foundExampleHeader2);
                VERIFY_ARE_EQUAL2(deh2.Key(), eh2.Key());
                VERIFY_ARE_EQUAL2(deh2.Value(), eh2.Value());
                VERIFY_ARE_EQUAL2(deh2.Seq(), eh2.Seq());

                ExampleHeader3 deh3;
                auto foundExampleHeader3 = msg1->Headers.TryReadFirst(deh3);
                VERIFY_IS_TRUE(foundExampleHeader3);
                VERIFY_ARE_EQUAL2(deh3.Key(), eh3.Key());
                VERIFY_ARE_EQUAL2(deh3.Value(), eh3.Value());
                VERIFY_ARE_EQUAL2(deh3.Seq(), eh3.Seq());

                foundExampleHeader3 = msg2->Headers.TryReadFirst(deh3);
                VERIFY_IS_TRUE(foundExampleHeader3);
                VERIFY_ARE_EQUAL2(deh3.Key(), eh3.Key());
                VERIFY_ARE_EQUAL2(deh3.Value(), eh3.Value());
                VERIFY_ARE_EQUAL2(deh3.Seq(), eh3.Seq());

                // verify message IDs are all different
                {
                    AcquireWriteLock grab(*lock);

                    VERIFY_IS_TRUE(messageIdSet->insert(msg1->MessageId).second);
                    VERIFY_IS_TRUE(messageIdSet->insert(msg2->MessageId).second);
                }

                if (++(*completeCount) == threadCount)
                {
                    allCompleted->Set();
                }
            });
        }

        BOOST_REQUIRE(allCompleted->WaitOne(TimeSpan::FromSeconds(30)));
    }

    BOOST_AUTO_TEST_CASE(SerializeOnceWithBufferMerging_SingleThread) // Serialize headers once, merge into a single buffer before copying to many messages
    {
        auto testAction = wstring(L"SerializeOnce");
        auto testActor = Actor::GenericTestActor;
        ExampleHeader2 eh2(Guid::NewGuid().ToString(), Guid::NewGuid().ToString(), 101);
        ExampleHeader3 eh3(
            L"12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",
            L"asdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnm",
            202);

        // Since shared headers will be added to many messages, so it is worthwhile to merge buffer list in stream 
        // to a single buffer, instead of going through the buffer list everytime when adding them to messages.
        KBuffer::SPtr sharedHeaders;
        {
            // create a stream object to add all shared headers
            auto sharedStream = FabricSerializer::CreateSerializableStream();

            auto status = MessageHeaders::Serialize(*sharedStream, ActionHeader(testAction));
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = MessageHeaders::Serialize(*sharedStream, ActorHeader(testActor));
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = MessageHeaders::Serialize(*sharedStream, eh2);
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = MessageHeaders::Serialize(*sharedStream, eh3);
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = FabricSerializer::CreateKBufferFromStream(*sharedStream, sharedHeaders);
            BOOST_REQUIRE(NT_SUCCESS(status));
        }

        // add shared headers to many messages
        set<MessageId> messageIdSet;
        for (int i = 0; i < 64; ++i)
        {
            MessageId relatesTo;
            auto msg = make_unique<Message>();
            msg->Headers.Add(MessageIdHeader()); // non-shared
            msg->Headers.Add(*sharedHeaders, true); // shared
            msg->Headers.Add(RelatesToHeader(relatesTo)); // no-shared

            VERIFY_ARE_EQUAL2(msg->Actor, testActor);
            VERIFY_ARE_EQUAL2(msg->Action, testAction);
            VERIFY_ARE_EQUAL2(msg->RelatesTo, relatesTo);

            ExampleHeader2 deh2;
            auto foundExampleHeader2 = msg->Headers.TryReadFirst(deh2);
            VERIFY_IS_TRUE(foundExampleHeader2);
            VERIFY_ARE_EQUAL2(deh2.Key(), eh2.Key());
            VERIFY_ARE_EQUAL2(deh2.Value(), eh2.Value());
            VERIFY_ARE_EQUAL2(deh2.Seq(), eh2.Seq());

            ExampleHeader3 deh3;
            auto foundExampleHeader3 = msg->Headers.TryReadFirst(deh3);
            VERIFY_IS_TRUE(foundExampleHeader3);
            VERIFY_ARE_EQUAL2(deh3.Key(), eh3.Key());
            VERIFY_ARE_EQUAL2(deh3.Value(), eh3.Value());
            VERIFY_ARE_EQUAL2(deh3.Seq(), eh3.Seq());

            // verify message IDs are all different
            VERIFY_IS_TRUE(messageIdSet.insert(msg->MessageId).second);
        }
    }

    BOOST_AUTO_TEST_CASE(SerializeOnceWithBufferMerging_MultiThread) // Serialize headers once, merge into a single buffer before copying to many messages
    {
        auto testAction = wstring(L"SerializeOnce");
        auto testActor = Actor::GenericTestActor;
        ExampleHeader2 eh2(Guid::NewGuid().ToString(), Guid::NewGuid().ToString(), 101);
        ExampleHeader3 eh3(
            L"12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",
            L"asdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnmasdfghjklqwertyuiopzxcvbnm",
            202);

        // Since shared headers will be added to many messages, so it is worthwhile to merge buffer list in stream 
        // to a single buffer, instead of going through the buffer list everytime when adding them to messages.
        KBuffer::SPtr sharedHeaders;
        {
            // create a stream object to add all shared headers
            auto sharedStream = FabricSerializer::CreateSerializableStream();

            auto status = MessageHeaders::Serialize(*sharedStream, ActionHeader(testAction));
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = MessageHeaders::Serialize(*sharedStream, ActorHeader(testActor));
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = MessageHeaders::Serialize(*sharedStream, eh2);
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = MessageHeaders::Serialize(*sharedStream, eh3);
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = FabricSerializer::CreateKBufferFromStream(*sharedStream, sharedHeaders);
            BOOST_REQUIRE(NT_SUCCESS(status));
        }

        // add shared headers to many messages
        auto completeCount = make_shared<atomic_uint64>(0);
        auto allCompleted = make_shared<AutoResetEvent>(false);
        const size_t threadCount = 64;

        auto messageIdSet = make_shared<set<MessageId>>();
        auto lock = make_shared<RwLock>();

        for (int i = 0; i < threadCount; ++i)
        {
            Threadpool::Post([=]
            {
                MessageId relatesTo1;
                auto msg1 = make_unique<Message>();
                msg1->Headers.Add(MessageIdHeader()); // non-shared
                msg1->Headers.Add(*sharedHeaders, true); // shared
                msg1->Headers.Add(RelatesToHeader(relatesTo1)); // no-shared

                VERIFY_ARE_EQUAL2(msg1->Actor, testActor);
                VERIFY_ARE_EQUAL2(msg1->Action, testAction);
                VERIFY_ARE_EQUAL2(msg1->RelatesTo, relatesTo1);

                auto msg2 = make_unique<Message>();
                msg2->Headers.Add(MessageIdHeader()); // non-shared
                msg2->Headers.Add(*sharedHeaders, true);
                MessageId relatesTo2;
                msg2->Headers.Add(RelatesToHeader(relatesTo2)); // no-shared

                VERIFY_ARE_EQUAL2(msg2->Actor, testActor);
                VERIFY_ARE_EQUAL2(msg2->Action, testAction);
                VERIFY_ARE_EQUAL2(msg2->RelatesTo, relatesTo2);

                ExampleHeader2 deh2;
                auto foundExampleHeader2 = msg1->Headers.TryReadFirst(deh2);
                VERIFY_IS_TRUE(foundExampleHeader2);
                VERIFY_ARE_EQUAL2(deh2.Key(), eh2.Key());
                VERIFY_ARE_EQUAL2(deh2.Value(), eh2.Value());
                VERIFY_ARE_EQUAL2(deh2.Seq(), eh2.Seq());

                deh2 = ExampleHeader2();
                foundExampleHeader2 = msg2->Headers.TryReadFirst(deh2);
                VERIFY_IS_TRUE(foundExampleHeader2);
                VERIFY_ARE_EQUAL2(deh2.Key(), eh2.Key());
                VERIFY_ARE_EQUAL2(deh2.Value(), eh2.Value());
                VERIFY_ARE_EQUAL2(deh2.Seq(), eh2.Seq());

                ExampleHeader3 deh3;
                auto foundExampleHeader3 = msg1->Headers.TryReadFirst(deh3);
                VERIFY_IS_TRUE(foundExampleHeader3);
                VERIFY_ARE_EQUAL2(deh3.Key(), eh3.Key());
                VERIFY_ARE_EQUAL2(deh3.Value(), eh3.Value());
                VERIFY_ARE_EQUAL2(deh3.Seq(), eh3.Seq());

                foundExampleHeader3 = msg2->Headers.TryReadFirst(deh3);
                VERIFY_IS_TRUE(foundExampleHeader3);
                VERIFY_ARE_EQUAL2(deh3.Key(), eh3.Key());
                VERIFY_ARE_EQUAL2(deh3.Value(), eh3.Value());
                VERIFY_ARE_EQUAL2(deh3.Seq(), eh3.Seq());

                // verify message IDs are all different
                {
                    AcquireWriteLock grab(*lock);

                    VERIFY_IS_TRUE(messageIdSet->insert(msg1->MessageId).second);
                    VERIFY_IS_TRUE(messageIdSet->insert(msg2->MessageId).second);
                }

                if (++(*completeCount) == threadCount)
                {
                    allCompleted->Set();
                }
            });
        }

        BOOST_REQUIRE(allCompleted->WaitOne(TimeSpan::FromSeconds(30)));
    }

    BOOST_AUTO_TEST_CASE(SerializeOnce_NoCopy_SingleThread) // Serialize headers once, no memory copy for sharing headers
    {
        MessageBodyExample messageBody(wstring(L"example message body key"), wstring(L"example message body value"));
        auto msg1 = make_unique<Message>(messageBody);
        auto msg2 = make_unique<Message>(messageBody);

        auto testAction = wstring(L"SerializeOnce_NoCopy");
        auto testActor = Actor::GenericTestActor;
        MessageId relatesTo1;
        MessageId relatesTo2;

        {
            // scope the following to show that the lifetime of "sharedBuffers"
            // below does not have to be the same as the messages that use it.

            // create a stream object to add all shared headers
            auto sharedStream = FabricSerializer::CreateSerializableStream();

            auto status = MessageHeaders::Serialize(*sharedStream, ActionHeader(testAction));
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = MessageHeaders::Serialize(*sharedStream, ActorHeader(testActor));
            BOOST_REQUIRE(NT_SUCCESS(status));

            ByteBique sharedBuffers;
            MessageHeaders::MakeSharedBuffers(*sharedStream, sharedBuffers);

            msg1->Headers.Add(MessageIdHeader()); // non-shared
            msg1->Headers.SetShared(sharedBuffers, true); //shared
            msg1->Headers.Add(RelatesToHeader(relatesTo1)); // no-shared

            msg2->Headers.Add(MessageIdHeader()); // non-shared
            msg2->Headers.SetShared(sharedBuffers, true); // shared
            msg2->Headers.Add(RelatesToHeader(relatesTo2)); // no-shared
        }

        VERIFY_ARE_EQUAL2(msg1->Actor, testActor);
        VERIFY_ARE_EQUAL2(msg1->Action, testAction);
        VERIFY_ARE_EQUAL2(msg1->RelatesTo, relatesTo1);

        VERIFY_ARE_EQUAL2(msg2->Actor, testActor);
        VERIFY_ARE_EQUAL2(msg2->Action, testAction);
        VERIFY_ARE_EQUAL2(msg2->RelatesTo, relatesTo2);

        VERIFY_ARE_NOT_EQUAL2(msg1->MessageId, msg2->MessageId);

        MessageBodyExample messageDeserialized1;
        VERIFY_IS_TRUE(msg1->GetBody(messageDeserialized1));
        VERIFY_ARE_EQUAL2(messageDeserialized1.Key(), messageBody.Key());
        VERIFY_ARE_EQUAL2(messageDeserialized1.Value(), messageBody.Value());

        MessageBodyExample messageDeserialized2;
        VERIFY_IS_TRUE(msg1->GetBody(messageDeserialized2));
        VERIFY_ARE_EQUAL2(messageDeserialized2.Key(), messageBody.Key());
        VERIFY_ARE_EQUAL2(messageDeserialized2.Value(), messageBody.Value());
    }

    BOOST_AUTO_TEST_CASE(SerializeOnce_NoCopy_MultipleThread) // Serialize headers once, no memory copy for sharing headers
    {
        auto testAction = wstring(L"SerializeOnce_NoCopy");
        auto testActor = Actor::GenericTestActor;

        auto completeCount = make_shared<atomic_uint64>(0);
        auto allCompleted = make_shared<AutoResetEvent>(false);
        const size_t threadCount = 64;

        auto messageIdSet = make_shared<set<MessageId>>();
        auto lock = make_shared<RwLock>();

        auto msg = make_unique<Message>();

        {
            // scope the following to show that the lifetime of "sharedBuffers"
            // below does not have to be the same as the messages that use it.

            // create a stream object to add all shared headers
            auto sharedStream = FabricSerializer::CreateSerializableStream();

            auto status = MessageHeaders::Serialize(*sharedStream, ActionHeader(testAction));
            BOOST_REQUIRE(NT_SUCCESS(status));

            status = MessageHeaders::Serialize(*sharedStream, ActorHeader(testActor));
            BOOST_REQUIRE(NT_SUCCESS(status));

            ByteBique sharedBuffers;
            MessageHeaders::MakeSharedBuffers(*sharedStream, sharedBuffers);

            // add sharedBuffers to keep it alive when leaving this scope
            msg->Headers.Add(MessageIdHeader()); // non-shared
            msg->Headers.SetShared(sharedBuffers, true); //shared

            for (uint i = 0; i < threadCount; ++i)
            {
                Threadpool::Post([=, &sharedBuffers]
                {
                    MessageBodyExample messageBody(wstring(L"example message body key"), wstring(L"example message body value"));

                    auto msg1 = make_unique<Message>(messageBody);
                    msg1->Headers.Add(MessageIdHeader()); // non-shared
                    msg1->Headers.SetShared(sharedBuffers, true); //shared
                    MessageId relatesTo1;
                    msg1->Headers.Add(RelatesToHeader(relatesTo1)); // no-shared

                    auto msg2 = make_unique<Message>(messageBody);
                    msg2->Headers.Add(MessageIdHeader()); // non-shared
                    msg2->Headers.SetShared(sharedBuffers, true); // shared
                    MessageId relatesTo2;
                    msg2->Headers.Add(RelatesToHeader(relatesTo2)); // no-shared

                    VERIFY_ARE_EQUAL2(msg1->Actor, testActor);
                    VERIFY_ARE_EQUAL2(msg1->Action, testAction);
                    VERIFY_ARE_EQUAL2(msg1->RelatesTo, relatesTo1);

                    VERIFY_ARE_EQUAL2(msg2->Actor, testActor);
                    VERIFY_ARE_EQUAL2(msg2->Action, testAction);
                    VERIFY_ARE_EQUAL2(msg2->RelatesTo, relatesTo2);

                    MessageBodyExample messageDeserialized1;
                    VERIFY_IS_TRUE(msg1->GetBody(messageDeserialized1));
                    VERIFY_ARE_EQUAL2(messageDeserialized1.Key(), messageBody.Key());
                    VERIFY_ARE_EQUAL2(messageDeserialized1.Value(), messageBody.Value());

                    MessageBodyExample messageDeserialized2;
                    VERIFY_IS_TRUE(msg1->GetBody(messageDeserialized2));
                    VERIFY_ARE_EQUAL2(messageDeserialized2.Key(), messageBody.Key());
                    VERIFY_ARE_EQUAL2(messageDeserialized2.Value(), messageBody.Value());

                    // verify message IDs are all different
                    {
                        AcquireWriteLock grab(*lock);

                        VERIFY_IS_TRUE(messageIdSet->insert(msg1->MessageId).second);
                        VERIFY_IS_TRUE(messageIdSet->insert(msg2->MessageId).second);
                    }

                    if (++(*completeCount) == threadCount)
                    {
                        allCompleted->Set();
                    }
                });
            }
        }

        BOOST_REQUIRE(allCompleted->WaitOne(TimeSpan::FromSeconds(30)));
    }

    BOOST_AUTO_TEST_CASE(TestSimpleExampleHeader)
    {
        auto name = std::wstring(L"SamplesName");
        auto samples = std::vector<LONG>();
        for (LONG i=0; i<10; i++) { samples.push_back(i); }

        ExampleHeader header(std::move(name), std::move(samples));
        std::vector<byte> buffer;

        VERIFY_IS_TRUE(FabricSerializer::Serialize(&header, buffer).IsSuccess());

        ExampleHeader header2;

        VERIFY_IS_TRUE(FabricSerializer::Deserialize(header2, buffer).IsSuccess());

        VERIFY_IS_TRUE(header.Name() == header2.Name());
        VERIFY_IS_TRUE(header.Samples() == header2.Samples());
    }

    void TestRemoveAll(MessageHeaders & messageHeaders)
    {
        messageHeaders.RemoveAll();

        int headerCount = 0;
        for(auto iter = messageHeaders.Begin(); iter != messageHeaders.End(); ++iter)
        {
            ++headerCount;
        }

        VERIFY_ARE_EQUAL2(headerCount, 0);

        VERIFY_ARE_EQUAL2(messageHeaders.Action, wstring());
        VERIFY_ARE_EQUAL2(messageHeaders.Actor, Actor::Enum::Empty); 
        VERIFY_ARE_EQUAL2(messageHeaders.MessageId, MessageId(Common::Guid::Empty(), 0));
        VERIFY_ARE_EQUAL2(messageHeaders.RelatesTo, MessageId(Common::Guid::Empty(), 0));
        VERIFY_ARE_EQUAL2(messageHeaders.RetryCount, 0); 
        VERIFY_ARE_EQUAL2(messageHeaders.FaultErrorCodeValue, Common::ErrorCodeValue::Success); 
        VERIFY_IS_FALSE(messageHeaders.ExpectsReply);
        VERIFY_IS_FALSE(messageHeaders.Idempotent);
        VERIFY_IS_FALSE(messageHeaders.HighPriority);
        VERIFY_IS_FALSE(messageHeaders.HasFaultBody);
        VERIFY_IS_FALSE(messageHeaders.IsUncorrelatedReply);
    }

    BOOST_AUTO_TEST_CASE(EnumerateAllHeaderOfSimpleType)
    {
        ENTER;

        MessageHeaders messageHeaders;
        AddMessageHeaders(messageHeaders, 10);

        auto result = MessageHeadersIterateAllExplicitly(messageHeaders);
        BOOST_REQUIRE_EQUAL(result.headerNumber, 10);
        BOOST_REQUIRE_EQUAL(result.exampleHeaderNumber, 4);
        BOOST_REQUIRE_EQUAL(result.example2HeaderNumber, 3);
        BOOST_REQUIRE_EQUAL(result.example3HeaderNumber, 3);

        result = MessageHeadersIterateAllByForEach(messageHeaders);
        VERIFY_IS_TRUE(result.headerNumber == 10);
        VERIFY_IS_TRUE(result.exampleHeaderNumber == 4);
        VERIFY_IS_TRUE(result.example2HeaderNumber == 3);
        VERIFY_IS_TRUE(result.example3HeaderNumber == 3);

        RemoveHeadersAtOddIndex(messageHeaders);

        result = MessageHeadersIterateAllExplicitly(messageHeaders);
        VERIFY_IS_TRUE(result.headerNumber == 5);
        VERIFY_IS_TRUE(result.exampleHeaderNumber == 2);
        VERIFY_IS_TRUE(result.example2HeaderNumber == 1);
        VERIFY_IS_TRUE(result.example3HeaderNumber == 2);

        result = MessageHeadersIterateAllByForEach(messageHeaders);
        VERIFY_IS_TRUE(result.headerNumber == 5);
        VERIFY_IS_TRUE(result.exampleHeaderNumber == 2);
        VERIFY_IS_TRUE(result.example2HeaderNumber == 1);
        VERIFY_IS_TRUE(result.example3HeaderNumber == 2);

        MessageHeaders anotherMessageHeaders;
        AddMessageHeaders(anotherMessageHeaders, 3);
        messageHeaders.AppendFrom(anotherMessageHeaders);

        result = MessageHeadersIterateAllExplicitly(messageHeaders);
        VERIFY_IS_TRUE(result.headerNumber == 8);
        VERIFY_IS_TRUE(result.exampleHeaderNumber == 3);
        VERIFY_IS_TRUE(result.example2HeaderNumber == 2);
        VERIFY_IS_TRUE(result.example3HeaderNumber == 3);
        Trace.WriteInfo(TraceType, "{0}", result.bytesDeserialized);

        result = MessageHeadersIterateAllByForEach(messageHeaders);
        VERIFY_IS_TRUE(result.headerNumber == 8);
        VERIFY_IS_TRUE(result.exampleHeaderNumber == 3);
        VERIFY_IS_TRUE(result.example2HeaderNumber == 2);
        VERIFY_IS_TRUE(result.example3HeaderNumber == 3);

        RemoveHeadersAtEvenIndex(messageHeaders);
        result = MessageHeadersIterateAllExplicitly(messageHeaders);
        VERIFY_IS_TRUE(result.headerNumber == 4);
        VERIFY_IS_TRUE(result.exampleHeaderNumber == 2);
        VERIFY_IS_TRUE(result.example2HeaderNumber == 0);
        VERIFY_IS_TRUE(result.example3HeaderNumber == 2);
        Trace.WriteInfo(TraceType, "{0}", result.bytesDeserialized);

        result = MessageHeadersIterateAllByForEach(messageHeaders);
        VERIFY_IS_TRUE(result.headerNumber == 4);
        VERIFY_IS_TRUE(result.exampleHeaderNumber == 2);
        VERIFY_IS_TRUE(result.example2HeaderNumber == 0);
        VERIFY_IS_TRUE(result.example3HeaderNumber == 2);

        TestRemoveAll(messageHeaders);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(CommonHeaders)
    {
        ENTER;

        wstring action(L"MyAction");
        Actor::Enum actor = Actor::Enum::GenericTestActor;
        MessageId messageId(Common::Guid::NewGuid(), 5);
        Message message;
        MessageHeaders& headers = message.Headers;
        bool expectsReply = true;

        headers.Add(ActionHeader(action));
        headers.Add(ActorHeader(actor));
        headers.Add(MessageIdHeader(messageId));
        headers.Add(RelatesToHeader(messageId));
        headers.Add(ExpectsReplyHeader(expectsReply));
        Trace.WriteInfo(TraceType, "{0}", headers);
        Trace.WriteInfo(TraceType, "{0}", message);

        VERIFY_IS_TRUE(headers.Action == action);
        VERIFY_IS_TRUE(headers.Actor == actor);
        VERIFY_IS_TRUE(headers.MessageId.Guid.Equals(messageId.Guid));
        VERIFY_IS_TRUE(headers.MessageId.Index == messageId.Index);
        VERIFY_IS_TRUE(headers.ExpectsReply == expectsReply);

        bool gotAction = false;
        bool gotMessageId = false;
        bool gotRelatesTo = false;
        bool gotExpectsReply = false;

        for(auto headerRef : headers)
        {
            switch (headerRef.Id())
            {
            case MessageHeaderId::Action:
                {
                    ActionHeader header = headerRef.Deserialize<ActionHeader>();
                    VERIFY_IS_TRUE(header.Action == action);
                    gotAction = true;
                    break;
                }

            case MessageHeaderId::MessageId:
                {
                    MessageIdHeader header = headerRef.Deserialize<MessageIdHeader>();
                    VERIFY_IS_TRUE(header.MessageId.Guid.Equals(messageId.Guid));
                    VERIFY_IS_TRUE(header.MessageId.Index == messageId.Index);
                    gotMessageId = true;
                    break;
                }

            case MessageHeaderId::RelatesTo:
                {
                    RelatesToHeader header = headerRef.Deserialize<RelatesToHeader>();
                    VERIFY_IS_TRUE(header.MessageId.Guid.Equals(messageId.Guid));
                    VERIFY_IS_TRUE(header.MessageId.Index == messageId.Index);
                    gotRelatesTo = true;
                    break;
                }

            case MessageHeaderId::ExpectsReply:
                {
                    ExpectsReplyHeader header = headerRef.Deserialize<ExpectsReplyHeader>();
                    VERIFY_IS_TRUE(header.ExpectsReply);
                    gotExpectsReply = true;
                    break;
                }
            }
        }

        VERIFY_IS_TRUE(gotAction);
        VERIFY_IS_TRUE(gotMessageId);
        VERIFY_IS_TRUE(gotRelatesTo);
        VERIFY_IS_TRUE(gotExpectsReply);

        // Simulate incoming message headers
        ByteBique byteBique;
        BiqueWriteStream biqueStream(byteBique);
        AddHeader(biqueStream, ActionHeader(action));
        AddHeader(biqueStream, ActorHeader(actor));
        AddHeader(biqueStream, MessageIdHeader(messageId));
        AddHeader(biqueStream, RelatesToHeader(messageId));
        AddHeader(biqueStream, ExpectsReplyHeader(expectsReply));

        MessageHeaders incomingHeaders(std::move(ByteBiqueRange(std::move(byteBique))), MessageHeaders::NullReceiveTime());
        VERIFY_IS_TRUE(incomingHeaders.Action == action);
        VERIFY_IS_TRUE(incomingHeaders.Actor == actor);
        VERIFY_IS_TRUE(incomingHeaders.MessageId.Guid.Equals(messageId.Guid));
        VERIFY_IS_TRUE(incomingHeaders.MessageId.Index == messageId.Index);
        VERIFY_IS_TRUE(incomingHeaders.ExpectsReply == expectsReply);

        TestRemoveAll(headers);
        TestRemoveAll(incomingHeaders);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(MessageProperty)
    {
        Message message;

        // Add an interger
        int testInteger = 12345;
        message.AddProperty(testInteger);

        // Add a wstring
        std::wstring testString = L"test string";
        message.AddProperty(testString);

        // Add a user defined type
        MessagePropertyUserDefined testClass(L"User defined message property");
        message.AddProperty(testClass);

        // Add a user defined type with a user specified name
        const char* propertyName = "User specified name for user defined message property";
        MessagePropertyUserDefined testClass2(L"User defined message property with name");
        message.AddProperty(testClass2, propertyName);

        // Retrieving properties
        int integerRetrieved = message.GetProperty<int>();
        VERIFY_IS_TRUE(integerRetrieved == testInteger);
        std::wstring stringRetrived = message.GetProperty<std::wstring>();
        VERIFY_IS_TRUE(stringRetrived == testString);
        MessagePropertyUserDefined userDefinedPropertyRetrieved = message.GetProperty<MessagePropertyUserDefined>();
        VERIFY_IS_TRUE(userDefinedPropertyRetrieved.Value() == testClass.Value());
        userDefinedPropertyRetrieved = message.GetProperty<MessagePropertyUserDefined>(propertyName);
        VERIFY_IS_TRUE(userDefinedPropertyRetrieved.Value() == testClass2.Value());

        // Remove the unamed user defined type
        bool removed = message.RemoveProperty<MessagePropertyUserDefined>();
        VERIFY_IS_TRUE(removed);
        removed = message.RemoveProperty<MessagePropertyUserDefined>();
        VERIFY_IS_FALSE(removed);

        // Add the user defined type with unqiue_ptr
        std::wstring uniquePtrpropertyValue = L"User defined message property with unique pointer";
        std::unique_ptr<MessagePropertyUserDefined> uniquePtr(new MessagePropertyUserDefined(uniquePtrpropertyValue));
        message.AddProperty(std::move(uniquePtr));
        VERIFY_IS_FALSE(uniquePtr);

        // Add the user defined type with shared_ptr
        const char* propertyKey = "Property name for user defined message property with shared pointer";
        std::wstring sharedPtrPropertyValue = L"User defined message property with shared pointer";
        std::shared_ptr<MessagePropertyUserDefined> sharedPtr(new MessagePropertyUserDefined(sharedPtrPropertyValue));
        message.AddProperty(sharedPtr, propertyKey);
        VERIFY_IS_TRUE(sharedPtr);
        sharedPtr.reset();

        // Retrieving properties
        MessagePropertyUserDefined* retrievedPointer = message.TryGetProperty<MessagePropertyUserDefined>();
        VERIFY_IS_TRUE(retrievedPointer != nullptr);
        #pragma warning(suppress : 6011) // there is assert above
        VERIFY_IS_TRUE(retrievedPointer->Value() == uniquePtrpropertyValue);
        userDefinedPropertyRetrieved = message.GetProperty<MessagePropertyUserDefined>();
        VERIFY_IS_TRUE(userDefinedPropertyRetrieved.Value() == uniquePtrpropertyValue);
        retrievedPointer = message.TryGetProperty<MessagePropertyUserDefined>(propertyKey);
        VERIFY_IS_TRUE(retrievedPointer != nullptr);
        VERIFY_IS_TRUE(retrievedPointer->Value() == sharedPtrPropertyValue);
        userDefinedPropertyRetrieved = message.GetProperty<MessagePropertyUserDefined>(propertyKey);
        VERIFY_IS_TRUE(userDefinedPropertyRetrieved.Value() == sharedPtrPropertyValue);
    }

    BOOST_AUTO_TEST_CASE(SimpleMessage)
    {
        //
        // Test message headers has both incoming message headers and locally added headers
        //
        {
            ByteBique incomingHeaderBytes;
            BiqueWriteStream biqueStream(incomingHeaderBytes);
            AddMessageHeaders(biqueStream, 2);
            ByteBique emptyBody;

            Message message(
                std::move(ByteBiqueRange(std::move(incomingHeaderBytes))), 
                std::move(ByteBiqueRange(std::move(emptyBody))),
                Message::NullReceiveTime());

            MessageHeaders& headers = message.Headers;
            AddMessageHeaders(headers, 3);

            auto result = MessageHeadersIterateAllExplicitly(headers);
            VERIFY_IS_TRUE(result.headerNumber == 5);
            VERIFY_IS_TRUE(result.exampleHeaderNumber == 2);
            VERIFY_IS_TRUE(result.example2HeaderNumber == 2);
            VERIFY_IS_TRUE(result.example3HeaderNumber == 1);

            RemoveHeadersAtOddIndex(headers);
            result = MessageHeadersIterateAllExplicitly(headers);
            VERIFY_IS_TRUE(result.headerNumber == 3);
            VERIFY_IS_TRUE(result.exampleHeaderNumber == 2);
            VERIFY_IS_TRUE(result.example2HeaderNumber == 0);
            VERIFY_IS_TRUE(result.example3HeaderNumber == 1);

            RemoveHeadersAtEvenIndex(headers);
            result = MessageHeadersIterateAllExplicitly(headers);
            VERIFY_IS_TRUE(result.headerNumber == 1);
            VERIFY_IS_TRUE(result.exampleHeaderNumber == 1);
            VERIFY_IS_TRUE(result.example2HeaderNumber == 0);
            VERIFY_IS_TRUE(result.example3HeaderNumber == 0);

            RemoveHeadersAtEvenIndex(headers);
            result = MessageHeadersIterateAllExplicitly(headers);
            VERIFY_IS_TRUE(result.headerNumber == 0);
            VERIFY_IS_TRUE(result.exampleHeaderNumber == 0);
            VERIFY_IS_TRUE(result.example2HeaderNumber == 0);
            VERIFY_IS_TRUE(result.example3HeaderNumber == 0);
            VERIFY_IS_TRUE(result.bytesDeserialized == 0);
        } 

        //
        // Test message header and body
        //
        {
            // test outgoing message
            MessageBodyExample messageBodyExample(wstring(L"example message body key"), wstring(L"example message body value"));
            Message message(messageBodyExample);
            VERIFY_IS_TRUE(message.SerializedHeaderSize() == 0);
            VERIFY_IS_TRUE(message.SerializedBodySize() == 116);

            MessageBodyExample messageDeserialized;
            VERIFY_IS_TRUE(message.GetBody(messageDeserialized));
            VERIFY_IS_TRUE(messageDeserialized.Key() == messageBodyExample.Key());
            VERIFY_IS_TRUE(messageDeserialized.Value() == messageBodyExample.Value());
            VERIFY_IS_TRUE(message.SerializedHeaderSize() == 0);
            VERIFY_IS_TRUE(message.SerializedBodySize() == 116); 

            // test incoming message
            ByteBique headerBytes;
            BiqueWriteStream headerStream(headerBytes);
            AddMessageHeaders(headerStream, 2);

            ByteBique bodyBytes;
            BiqueWriteStream bodyStream(bodyBytes);

            std::vector<byte> messageBytes;
            VERIFY_IS_TRUE(FabricSerializer::Serialize(&messageBodyExample, messageBytes).IsSuccess());
            bodyStream.WriteBytes(messageBytes.data(), messageBytes.size());

            Message incomingMessage(
                std::move(ByteBiqueRange(std::move(headerBytes))),
                std::move(ByteBiqueRange(std::move(bodyBytes))),
                Message::NullReceiveTime());

            MessageBodyExample deserializedFromChunks;
            VERIFY_IS_TRUE(incomingMessage.GetBody(deserializedFromChunks));
            VERIFY_IS_TRUE(deserializedFromChunks.Key() == messageBodyExample.Key());
            VERIFY_IS_TRUE(deserializedFromChunks.Value() == messageBodyExample.Value());
        }
    }

    BOOST_AUTO_TEST_CASE(DeepCopyIncomingMessage)
    {
        ByteBique headerBytes;
        BiqueWriteStream headerStream(headerBytes);
        AddMessageHeaders(headerStream, 2);

        MessageBodyExample msgBody(wstring(L"example message body key"), wstring(L"example message body value"));
        ByteBique bodyBytes;
        BiqueWriteStream bodyStream(bodyBytes);

        vector<byte> messageBytes;
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&msgBody, messageBytes).IsSuccess());
        bodyStream.WriteBytes(messageBytes.data(), messageBytes.size());

        auto msg = make_unique<Message>(msgBody);

        MessageId msgId;
        msg->Headers.Add(MessageIdHeader(msgId));

        wstring action = Guid::NewGuid().ToString();
        msg->Headers.Add(ActionHeader(action));

        Message deepCopy = *msg;

        // destroy the original message
        msg.reset();

        VERIFY_ARE_EQUAL2(deepCopy.MessageId, msgId);
        VERIFY_ARE_EQUAL2(deepCopy.Action, action);

        MessageBodyExample bodyInDeepCopy;
        VERIFY_IS_TRUE(deepCopy.GetBody(bodyInDeepCopy));
        VERIFY_ARE_EQUAL2(bodyInDeepCopy.Key(), msgBody.Key());
        VERIFY_ARE_EQUAL2(bodyInDeepCopy.Value(), msgBody.Value());
    }

    BOOST_AUTO_TEST_CASE(DeepCopyOutgoingMessage)
    {
        MessageBodyExample msgBody(wstring(L"example message body key"), wstring(L"example message body value"));
        auto msg = make_unique<Message>(msgBody);

        MessageId msgId;
        msg->Headers.Add(MessageIdHeader(msgId));

        wstring action = Guid::NewGuid().ToString();
        msg->Headers.Add(ActionHeader(action));

        Message deepCopy(*msg);

        // destroy the original message
        msg.reset();

        VERIFY_ARE_EQUAL2(deepCopy.MessageId, msgId);
        VERIFY_ARE_EQUAL2(deepCopy.Action, action);

        MessageBodyExample bodyInDeepCopy;
        VERIFY_IS_TRUE(deepCopy.GetBody(bodyInDeepCopy));
        VERIFY_ARE_EQUAL2(bodyInDeepCopy.Key(), msgBody.Key());
        VERIFY_ARE_EQUAL2(bodyInDeepCopy.Value(), msgBody.Value());
    }

    BOOST_AUTO_TEST_CASE(MessageHeadersTryReadFirstTest)
    {
        // simulate incoming message headers
        ByteBique byteBique;
        BiqueWriteStream biqueStream(byteBique);
        AddMessageHeaders(biqueStream, 2);
        MessageHeaders incomingHeaders(std::move(ByteBiqueRange(std::move(byteBique))), MessageHeaders::NullReceiveTime());
        AddMessageHeaders(incomingHeaders, 1);

        bool found = TryReadFirstExampleHeader(incomingHeaders);
        VERIFY_IS_TRUE(found);
        found = TryReadFirstExampleHeader2(incomingHeaders);
        VERIFY_IS_TRUE(found);
        found = TryReadFirstExampleHeader(incomingHeaders);
        VERIFY_IS_TRUE(found);
    }

    void TestBiqueChunIterator(size_t size1, size_t size2)
    {
        ByteBique b1;
        for (size_t i = 0; i < size1; ++ i)
        {
            b1.push_back('0');
        }

        ByteBique b2;
        for (size_t i = 0; i < size2; ++ i)
        {
            b2.push_back('0');
        }

        ByteBiqueRange br1(std::move(b1));
        ByteBiqueRange br2(std::move(b2));

        auto end = BiqueChunkIterator::End(br1, br2);
        size_t total = 0;
        for(auto iter = BiqueChunkIterator(br1, br2); iter != end; ++ iter)
        {
            total += iter->size();
        }
        
        VERIFY_IS_TRUE(total == (size1 + size2));
    }

    BOOST_AUTO_TEST_CASE(BiqueChunkIteratorTest)
    {
        TestBiqueChunIterator(36, 36); // 0 == 36 % 3
        TestBiqueChunIterator(37, 37); // 1 == 37 % 3
        TestBiqueChunIterator(38, 38); // 2 == 38 % 3
        TestBiqueChunIterator(36, 38);
        TestBiqueChunIterator(38, 36);
        TestBiqueChunIterator(37, 38);
        TestBiqueChunIterator(38, 37);
        TestBiqueChunIterator(37, 36);
        TestBiqueChunIterator(36, 37);
    }


    BOOST_AUTO_TEST_CASE(MessageHeadersAppendFromTest)
    {
        ByteBique byteBique;
        BiqueWriteStream biqueStream(byteBique);
        AddMessageHeaders(biqueStream, 4);
        MessageHeaders headers(std::move(ByteBiqueRange(std::move(byteBique))), MessageHeaders::NullReceiveTime());
        AddMessageHeaders(headers, 2);

        MessageHeaders from;
        AddMessageHeaders(from, 5);

        headers.AppendFrom(from);
        auto result = MessageHeadersIterateAllExplicitly(headers);

        VERIFY_IS_TRUE(result.headerNumber == 11);
        VERIFY_IS_TRUE(result.exampleHeaderNumber == 5);
        VERIFY_IS_TRUE(result.example2HeaderNumber == 4);
        VERIFY_IS_TRUE(result.example3HeaderNumber == 2);

        result = MessageHeadersIterateAllExplicitly(from);
        VERIFY_IS_TRUE(from.SerializedSize() == 0);
        VERIFY_IS_TRUE(result.bytesDeserialized == 0);
        VERIFY_IS_TRUE(result.headerNumber == 0);
    }

    BOOST_AUTO_TEST_CASE(CloneTest_SharedHeaderNoCopy)
    {
        auto msg = make_unique<Message>();
        msg->Headers.Add(ActionHeader(L"TestAction"));

        // Enables sharing of ActionHeader added above, without CheckPoint(), copy will happen
        msg->CheckPoint();

        // Add separate MessageIdHeader to each clone
        auto clone1 = msg->Clone();
        clone1->Headers.Add(MessageIdHeader());

        auto clone2 = msg->Clone();
        clone2->Headers.Add(MessageIdHeader());

        VERIFY_ARE_EQUAL2(msg->Action, clone1->Action);
        VERIFY_ARE_EQUAL2(msg->Action, clone2->Action);
        BOOST_REQUIRE(msg->MessageId != clone1->MessageId);
        BOOST_REQUIRE(msg->MessageId != clone2->MessageId);
        BOOST_REQUIRE(clone1->MessageId != clone2->MessageId);
    }

    BOOST_AUTO_TEST_CASE(TestCloneWithoutExtraHeaders)
    {
        ByteBique headerBytes;
        BiqueWriteStream headerStream(headerBytes);
        AddMessageHeaders(headerStream, 5);

        MessageBodyExample messageBodyExample(wstring(L"example message body key"), wstring(L"example message body value"));
        ByteBique bodyBytes;
        BiqueWriteStream bodyStream(bodyBytes);

        std::vector<byte> messageBytes;
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&messageBodyExample, messageBytes).IsSuccess());
        bodyStream.WriteBytes(messageBytes.data(), messageBytes.size());

        Message message(
            std::move(ByteBiqueRange(std::move(headerBytes))),
            std::move(ByteBiqueRange(std::move(bodyBytes))),
            Message::NullReceiveTime());

        AddMessageHeaders(message.Headers, 4);
        int testInteger = 12345;
        message.AddProperty(testInteger);

        MessageUPtr clone = message.Clone();
        clone->Headers.Add(ExampleHeader2());

        MessageBodyExample deserializedFromChunks;
        VERIFY_IS_TRUE(clone->GetBody(deserializedFromChunks));
        VERIFY_IS_TRUE(deserializedFromChunks.Key() == messageBodyExample.Key());
        VERIFY_IS_TRUE(deserializedFromChunks.Value() == messageBodyExample.Value());

        auto result = MessageHeadersIterateAllExplicitly(clone->Headers);

        VERIFY_IS_TRUE(result.headerNumber == 10);
        VERIFY_IS_TRUE(result.exampleHeaderNumber == 4);
        VERIFY_IS_TRUE(result.example2HeaderNumber == 4);
        VERIFY_IS_TRUE(result.example3HeaderNumber == 2);

        int integerRetrieved = clone->GetProperty<int>();
        VERIFY_IS_TRUE(integerRetrieved == integerRetrieved);
    }

    BOOST_AUTO_TEST_CASE(TestCloneExtraHeaders)
    {
        ByteBique headerBytes;
        BiqueWriteStream headerStream(headerBytes);
        AddMessageHeaders(headerStream, 5);

        MessageBodyExample messageBodyExample(wstring(L"example message body key"), wstring(L"example message body value"));
        ByteBique bodyBytes;
        BiqueWriteStream bodyStream(bodyBytes);

        std::vector<byte> messageBytes;
        VERIFY_IS_TRUE(FabricSerializer::Serialize(&messageBodyExample, messageBytes).IsSuccess());
        bodyStream.WriteBytes(messageBytes.data(), messageBytes.size());

        Message message(
            std::move(ByteBiqueRange(std::move(headerBytes))),
            std::move(ByteBiqueRange(std::move(bodyBytes))),
            Message::NullReceiveTime());

        AddMessageHeaders(message.Headers, 4);
        int testInteger = 12345;
        message.AddProperty(testInteger);

        MessageHeaders extraHeaders;
        AddMessageHeaders(extraHeaders, 7);

        MessageUPtr clone = message.Clone(std::move(extraHeaders));

        MessageBodyExample deserializedFromChunks;
        VERIFY_IS_TRUE(clone->GetBody(deserializedFromChunks));
        VERIFY_IS_TRUE(deserializedFromChunks.Key() == messageBodyExample.Key());
        VERIFY_IS_TRUE(deserializedFromChunks.Value() == messageBodyExample.Value());

        auto result = MessageHeadersIterateAllExplicitly(clone->Headers);

        VERIFY_IS_TRUE(result.headerNumber == 16);
        VERIFY_IS_TRUE(result.exampleHeaderNumber == 7);
        VERIFY_IS_TRUE(result.example2HeaderNumber == 5);
        VERIFY_IS_TRUE(result.example3HeaderNumber == 4);

        int integerRetrieved = clone->GetProperty<int>();
        VERIFY_IS_TRUE(integerRetrieved == integerRetrieved);
    }

    // This sequence of clones would cause an AV after the original was destructed.  The fix
    // was in bique.h
    BOOST_AUTO_TEST_CASE(MultipleCloneTest)
    {
        auto m0 = make_unique<Message>();

        m0->Headers.Add(MessageIdHeader());

        auto m2 = m0->Clone();

        m2->CheckPoint();

        auto m3 = m2->Clone();
        auto m4 = m2->Clone();

        auto m5 = m3->Clone();
        auto m6 = m2->Clone();

        m3 = nullptr;
        m5 = nullptr;

        auto m7 = m6->Clone();
    }

    BOOST_AUTO_TEST_CASE(UserBufferTest)
    {
        wstring state(L"STATE");
        wstring body(L"body");

        MessageUPtr clone;
        Common::atomic_long count(0);

        vector<const_buffer> buffers;
        const_buffer buffer(body.c_str(), body.size() * 2 + 2);
        buffers.push_back(buffer);

        {
            auto message = make_unique<Message>(buffers,
                [&count](vector<const_buffer> const & buffers, void * state)
                {
                    auto * myBody = reinterpret_cast<wchar_t*>(buffers[0].buf);
                    auto * stringState = static_cast<wstring*>(state);
                    VERIFY_IS_TRUE(*stringState == L"STATE");

                    VERIFY_IS_TRUE(wstring(myBody) == L"body");
                    ++count;
                },
                &state);

            // Verify the buffer is being shared
            clone = message->Clone();
        }

        // Clone out of scope, verify callback not invoked yet
        VERIFY_IS_TRUE(count.load() == 0);

        // Try and get the body
        vector<const_buffer> newBuffers;
        clone->GetBody(newBuffers);

        VERIFY_IS_TRUE(newBuffers.size() == 1);
        auto newBuffer = newBuffers[0];
        VERIFY_IS_TRUE(buffer.buf == newBuffer.buf);
        VERIFY_IS_TRUE(buffer.len == newBuffer.len);
        VERIFY_IS_FALSE(&buffer == &newBuffer);

        clone = nullptr;

        // Callback now in
        VERIFY_IS_TRUE(count.load() == 1);
    }

    BOOST_AUTO_TEST_CASE(AppendFromTest)
    {
        auto message1 = make_unique<Message>();
        auto message2 = make_unique<Message>();

        wstring action = L"It's action time!";
        Actor::Enum actor = Actor::GenericTestActor;

        message1->Headers.Add(ActionHeader(action));
        message2->Headers.AppendFrom(message1->Headers);

        VERIFY_IS_TRUE(message1->Action.empty());
        VERIFY_IS_TRUE(message2->Action == action);

        message2->Headers.Add(ActorHeader(actor));
        message1->Headers.AppendFrom(message2->Headers);

        VERIFY_IS_TRUE(message2->Action.empty());
        VERIFY_IS_TRUE(message2->Actor == Actor::Empty);
        VERIFY_IS_TRUE(message1->Action == action);
        VERIFY_IS_TRUE(message1->Actor == actor);
    }

    BOOST_AUTO_TEST_CASE(DeletedHeaderByteCount)
    {
        ENTER;

        auto message1 = make_unique<Message>();

        wstring action = L"It's action time!";
        Actor::Enum actor = Actor::GenericTestActor;

        message1->Headers.Add(ActionHeader(action));
        message1->Headers.Add(ActorHeader(actor));
        message1->Headers.Add(MessageIdHeader());
        message1->Idempotent = true;
        message1->Headers.Add(RetryHeader(2));
        message1->Headers.Add(ExpectsReplyHeader(true));

        for (auto header_iter = message1->Headers.Begin(); header_iter != message1->Headers.End(); ++header_iter)
        {
            switch (header_iter->Id())
            {
            case MessageHeaderId::Actor:
            case MessageHeaderId::Idempotent: // removal of IdempotentHeader is no-op, as it is only added before sending
                message1->Headers.Remove(header_iter);
            }
        }

        const size_t ExpectedDeletedByteCount1 = 20;
        const size_t ExpectedDeletedByteCount2 = 38;

        cout << "Deleted bytes = " << message1->Headers.get_DeletedHeaderByteCount() << endl;
        VERIFY_IS_TRUE(message1->Headers.get_DeletedHeaderByteCount() == ExpectedDeletedByteCount1);

        auto message2 = make_unique<Message>();

        message2->Headers.AppendFrom(message1->Headers);

        VERIFY_IS_TRUE(message1->Headers.get_DeletedHeaderByteCount() == 0);
        VERIFY_IS_TRUE(message2->Headers.get_DeletedHeaderByteCount() == ExpectedDeletedByteCount1);

        for (auto header_iter = message2->Headers.Begin(); header_iter != message2->Headers.End(); ++header_iter)
        {
            switch (header_iter->Id())
            {
            case MessageHeaderId::Retry:
                message2->Headers.Remove(header_iter);
            }
        }

        cout << "Deleted bytes = " << message2->Headers.get_DeletedHeaderByteCount() << endl;
        VERIFY_IS_TRUE(message2->Headers.get_DeletedHeaderByteCount() == ExpectedDeletedByteCount2);
        cout << "Deleted bytes = " << message2->Headers.get_DeletedHeaderByteCount() << endl;

        message2->CheckPoint();
        cout << "Deleted bytes = " << message2->Headers.get_DeletedHeaderByteCount() << endl;
        VERIFY_IS_TRUE(message2->Headers.get_DeletedHeaderByteCount() == ExpectedDeletedByteCount2);

        message2->Headers.Compact();
        cout << "Deleted bytes = " << message2->Headers.get_DeletedHeaderByteCount() << endl;
        VERIFY_IS_TRUE(message2->Headers.get_DeletedHeaderByteCount() == 0);

        LEAVE;
    }

    void CompactTest(bool shouldClone, bool keepClone)
    {
        Trace.WriteInfo(TraceType, "CompactTest: shouldClone = {0}, keepClone = {1}", shouldClone, keepClone);

        wstring action(L"MyAction");
        Actor::Enum actor = Actor::Enum::GenericTestActor;
        MessageId messageId(Common::Guid::NewGuid(), 5);
        Message message;
        bool expectsReply = true;
        int retryCount = 2;

        // Simulate incoming message headers
        ByteBique byteBique;
        BiqueWriteStream biqueStream(byteBique);
        AddHeader(biqueStream, ActionHeader(action));
        AddHeader(biqueStream, ActorHeader(actor));
        AddHeader(biqueStream, RetryHeader(0));
        AddHeader(biqueStream, MessageIdHeader(messageId));
        AddHeader(biqueStream, RelatesToHeader(messageId));
        AddHeader(biqueStream, ExpectsReplyHeader(expectsReply));

        ByteBiqueRange headerRange(move(byteBique));
        ByteBique bodyBique;
        ByteBiqueRange bodyRange(move(bodyBique));
        auto msg = make_unique<Message>(move(headerRange), move(bodyRange), Message::NullReceiveTime());

        AddMessageHeaders(msg->Headers, 10); 
        Trace.WriteInfo(TraceType, "after AddMessageHeaders(10), msg->Headers: {0}", msg->Headers);

        VERIFY_IS_TRUE(msg->Headers.DeletedHeaderByteCount == 0);

        auto removed = msg->Headers.TryRemoveHeader<RetryHeader>();
        VERIFY_IS_TRUE(removed);

        auto removedSharedBytes = msg->Headers.DeletedHeaderByteCount;
        Trace.WriteInfo(TraceType, "removed shared bytes: {0}", removedSharedBytes); 
        VERIFY_IS_TRUE(removedSharedBytes > 0);

        int toRemove = 3;
        for (auto iter = msg->Headers.Begin(); iter != msg->Headers.End();)
        {
            switch(iter->Id())
            {
                case MessageHeaderId::Example:
                case MessageHeaderId::Example2:
                case MessageHeaderId::Example3:
                    msg->Headers.Remove(iter);
                    --toRemove;
                    break;

                default:
                    ++iter;
            }

            if (toRemove == 0) break;
        }

        msg->Headers.Add(RetryHeader(retryCount));

        Trace.WriteInfo(TraceType, "after adding RetryHeader again, msg->Headers: {0}", msg->Headers);

        auto removedBeforeCompact = msg->Headers.DeletedHeaderByteCount;
        Trace.WriteInfo(TraceType, "removed byte count before compact: {0}", removedBeforeCompact); 
        VERIFY_IS_TRUE(removedBeforeCompact > removedSharedBytes);

        auto iterateResult = MessageHeadersIterateAllExplicitly(msg->Headers);
        BOOST_REQUIRE_EQUAL(iterateResult.headerNumber, 13);
        BOOST_REQUIRE_EQUAL(iterateResult.exampleHeaderNumber, 3);
        BOOST_REQUIRE_EQUAL(iterateResult.example2HeaderNumber, 2);
        BOOST_REQUIRE_EQUAL(iterateResult.example3HeaderNumber, 2);

        Trace.WriteInfo(TraceType, "msg->Headers: {0}", msg->Headers);
        VERIFY_IS_TRUE(msg->Headers.Action == action);
        VERIFY_IS_TRUE(msg->Headers.Actor == actor);
        VERIFY_IS_TRUE(msg->Headers.MessageId.Guid.Equals(messageId.Guid));
        VERIFY_IS_TRUE(msg->Headers.MessageId.Index == messageId.Index);
        VERIFY_IS_TRUE(msg->Headers.ExpectsReply == expectsReply);
        VERIFY_IS_TRUE(msg->Headers.RetryCount == retryCount);

        MessageUPtr clone;
        if (shouldClone)
        {
            clone = msg->Clone();
            Trace.WriteInfo(TraceType, "clone->Header.DeletedHeaderByteCount: {0}", clone->Headers.DeletedHeaderByteCount); 
            VERIFY_ARE_EQUAL2(removedBeforeCompact, clone->Headers.DeletedHeaderByteCount);

            if (!keepClone)
            {
                clone.reset();
            }
        }

        auto serializedSizeBeforeCompact = msg->Headers.SerializedSize();
        msg->Headers.Compact();
        auto serializedSizeAfterCompact = msg->Headers.SerializedSize();
        Trace.WriteInfo(TraceType, "first explicit compact: beforeSize = {0}, afterSize = {1}", serializedSizeBeforeCompact, serializedSizeAfterCompact);
        VERIFY_IS_TRUE(serializedSizeBeforeCompact > serializedSizeAfterCompact);

        Trace.WriteInfo(TraceType, "msg->Headers: {0}", msg->Headers);
        VERIFY_IS_TRUE(msg->Headers.Action == action);
        VERIFY_IS_TRUE(msg->Headers.Actor == actor);
        VERIFY_IS_TRUE(msg->Headers.MessageId.Guid.Equals(messageId.Guid));
        VERIFY_IS_TRUE(msg->Headers.MessageId.Index == messageId.Index);
        VERIFY_IS_TRUE(msg->Headers.ExpectsReply == expectsReply);
        VERIFY_IS_TRUE(msg->Headers.RetryCount == retryCount);

        auto removedAfterCompact = msg->Headers.DeletedHeaderByteCount;
        Trace.WriteInfo(TraceType, "removed byte count after compact: {0}", removedAfterCompact); 
        VERIFY_IS_TRUE(removedAfterCompact < removedBeforeCompact);
        VERIFY_IS_TRUE(removedAfterCompact == 0);

        if (clone)
        {
            Trace.WriteInfo(TraceType, "clone should not be affected by compact on the original, clone->Header.DeletedHeaderByteCount: {0}", clone->Headers.DeletedHeaderByteCount); 
            VERIFY_ARE_EQUAL2(removedBeforeCompact, clone->Headers.DeletedHeaderByteCount);
            VERIFY_IS_TRUE(removedAfterCompact < clone->Headers.DeletedHeaderByteCount);
        }

        msg->Headers.TryRemoveHeader<ExampleHeader2>();

        auto removed5 = msg->Headers.DeletedHeaderByteCount;
        Trace.WriteInfo(TraceType, "removed byte count 5: {0}", removed5);

        Trace.WriteInfo(TraceType, "msg->Headers: {0}", msg->Headers);
        VERIFY_IS_TRUE(msg->Headers.Action == action);
        VERIFY_IS_TRUE(msg->Headers.Actor == actor);
        VERIFY_IS_TRUE(msg->Headers.MessageId.Guid.Equals(messageId.Guid));
        VERIFY_IS_TRUE(msg->Headers.MessageId.Index == messageId.Index);
        VERIFY_IS_TRUE(msg->Headers.ExpectsReply == expectsReply);
        VERIFY_IS_TRUE(msg->Headers.RetryCount == retryCount);

        iterateResult = MessageHeadersIterateAllExplicitly(msg->Headers);
        BOOST_REQUIRE_EQUAL(iterateResult.headerNumber, 11);
        BOOST_REQUIRE_EQUAL(iterateResult.exampleHeaderNumber, 3);
        BOOST_REQUIRE_EQUAL(iterateResult.example2HeaderNumber, 0);
        BOOST_REQUIRE_EQUAL(iterateResult.example3HeaderNumber, 2);

        // trigger auto compact by removing and adding repeatedly
        for(int i = 0; ; ++i)
        {
            msg->Headers.TryRemoveHeader<ExampleHeader2>();

            auto sizeBeforeAdd = msg->Headers.SerializedSize();
            msg->Headers.Add(ExampleHeader2(
                L"keyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy",
                L"valueeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"));

            auto sizeAfterAdd = msg->Headers.SerializedSize();
            Trace.WriteInfo(TraceType, "new serialized size = {0}", sizeAfterAdd); 

            if (sizeBeforeAdd >= sizeAfterAdd) break; // auto compact happened
        }

        Trace.WriteInfo(
            TraceType,
            "after repeated removing and adding, serialized size = {0}, msg->Headers: {1}",
            msg->Headers.SerializedSize(), msg->Headers);

        VERIFY_IS_TRUE(msg->Headers.Action == action);
        VERIFY_IS_TRUE(msg->Headers.Actor == actor);
        VERIFY_IS_TRUE(msg->Headers.MessageId.Guid.Equals(messageId.Guid));
        VERIFY_IS_TRUE(msg->Headers.MessageId.Index == messageId.Index);
        VERIFY_IS_TRUE(msg->Headers.ExpectsReply == expectsReply);
        VERIFY_IS_TRUE(msg->Headers.RetryCount == retryCount);

        iterateResult = MessageHeadersIterateAllExplicitly(msg->Headers);
        BOOST_REQUIRE_EQUAL(iterateResult.headerNumber, 12);
        BOOST_REQUIRE_EQUAL(iterateResult.exampleHeaderNumber, 3);
        BOOST_REQUIRE_EQUAL(iterateResult.example2HeaderNumber, 1);
        BOOST_REQUIRE_EQUAL(iterateResult.example3HeaderNumber, 2);

        removed = msg->Headers.TryRemoveHeader<ExampleHeader>();
        VERIFY_IS_TRUE(removed);

        auto removed6 = msg->Headers.DeletedHeaderByteCount; 
        Trace.WriteInfo(TraceType, "removed byte count 6: {0}", removed6); 

        serializedSizeBeforeCompact = msg->Headers.SerializedSize();
        msg->Headers.Compact();
        serializedSizeAfterCompact = msg->Headers.SerializedSize();
        Trace.WriteInfo(TraceType, "second explicit compact: beforeSize = {0}, afterSize = {1}", serializedSizeBeforeCompact, serializedSizeAfterCompact);
        VERIFY_IS_TRUE(serializedSizeBeforeCompact > serializedSizeAfterCompact);
        VERIFY_ARE_EQUAL2(msg->Headers.DeletedHeaderByteCount, 0);

        if (clone)
        {
            VERIFY_IS_TRUE(msg->Headers.DeletedHeaderByteCount < clone->Headers.DeletedHeaderByteCount);

            Trace.WriteInfo(
                TraceType,
                "clone should not be affected by compact on the original: clone->Headers.DeletedHeaderByteCount = {0}",
                clone->Headers.DeletedHeaderByteCount);

            VERIFY_ARE_EQUAL2(clone->Headers.DeletedHeaderByteCount, removedBeforeCompact);
        }

        Trace.WriteInfo(TraceType, "after second explicit compact, msg->Headers: {0}", msg->Headers);
        VERIFY_IS_TRUE(msg->Headers.Action == action);
        VERIFY_IS_TRUE(msg->Headers.Actor == actor);
        VERIFY_IS_TRUE(msg->Headers.MessageId.Guid.Equals(messageId.Guid));
        VERIFY_IS_TRUE(msg->Headers.MessageId.Index == messageId.Index);
        VERIFY_IS_TRUE(msg->Headers.ExpectsReply == expectsReply);
        VERIFY_IS_TRUE(msg->Headers.RetryCount == retryCount);

        iterateResult = MessageHeadersIterateAllExplicitly(msg->Headers);
        BOOST_REQUIRE_EQUAL(iterateResult.headerNumber, 9);
        BOOST_REQUIRE_EQUAL(iterateResult.exampleHeaderNumber, 0);
        BOOST_REQUIRE_EQUAL(iterateResult.example2HeaderNumber, 1);
        BOOST_REQUIRE_EQUAL(iterateResult.example3HeaderNumber, 2);

        auto removed7 = msg->Headers.DeletedHeaderByteCount; 
        Trace.WriteInfo(TraceType, "removed byte count 7: {0}", removed7); 
        VERIFY_IS_TRUE(removed7 < removed6); 
        VERIFY_ARE_EQUAL2(removed7, 0);

        if (clone)
        {
            Trace.WriteInfo(
                TraceType,
                "clone should not be affected by compact on the original: clone->Headers.DeletedHeaderByteCount = {0}",
                clone->Headers.DeletedHeaderByteCount);

            VERIFY_ARE_EQUAL2(clone->Headers.DeletedHeaderByteCount, removedBeforeCompact);

            clone->Headers.Compact();
            Trace.WriteInfo(
                TraceType,
                "afer clone is compacted: clone->Headers.DeletedHeaderByteCount = {0}", 
                clone->Headers.DeletedHeaderByteCount);

            VERIFY_ARE_EQUAL2(clone->Headers.DeletedHeaderByteCount, 0);

            TestRemoveAll(clone->Headers);
        }

        TestRemoveAll(msg->Headers);
    }

    BOOST_AUTO_TEST_CASE(Compact)
    {
        ENTER;
        CompactTest(false, false);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Compact_Cloned)
    {
        ENTER;
        CompactTest(true, false);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(Compact_Cloned2) // keep clone alive
    {
        ENTER;
        CompactTest(true, true);
        LEAVE;
    }

    struct  VersionableMessageSample : public Serialization::FabricSerializable
    {
        ULONG id_;
        bool isAwesome_;

        VersionableMessageSample()
            : id_(10)
            , isAwesome_(true)
        {
        }

        FABRIC_FIELDS_02(id_, isAwesome_);
    };

    BOOST_AUTO_TEST_CASE(VersionableMessage)
    {
        VersionableMessageSample sampleObject;

        sampleObject.id_ = 0x20;

        VERIFY_IS_TRUE(sampleObject.isAwesome_);

        //IKSerializable * objectPointer = &sampleObject;

        //Message message(*objectPointer);
        Message message(sampleObject);

        VERIFY_IS_TRUE(message.Actor == Actor::Empty);

        Actor::Enum actor = Actor::GenericTestActor;
        message.Headers.Add(ActorHeader(actor));

        VERIFY_IS_TRUE(message.Actor == actor);

        // The below code is simply to copy the bytes to a new buffer and construct a message from them
        bique<byte> headersBique(1024);
        BiqueWriteStream headersWriteStream(headersBique);

        for (BiqueChunkIterator chunk = message.BeginHeaderChunks(); !(chunk == message.EndHeaderChunks()); ++chunk)
        {
            Common::const_buffer buffer = *chunk;

            headersWriteStream.WriteBytes(buffer.buf, buffer.len);
        }

        bique<byte> bodyBique(1024);
        BiqueWriteStream bodyWriteStream(bodyBique);

        for (BufferIterator chunk = message.BeginBodyChunks(); !(chunk == message.EndBodyChunks()); ++chunk)
        {
            Common::const_buffer buffer = *chunk;

            bodyWriteStream.WriteBytes(buffer.buf, buffer.len);
        }

        ByteBiqueRange headersRange(headersBique.begin(), headersBique.end(), false);
        ByteBiqueRange bodyRange(bodyBique.begin(), bodyBique.end(),false);

        Message receivedMessage(std::move(headersRange), std::move(bodyRange), Message::NullReceiveTime());

        VersionableMessageSample deserializedSampleObject;
        receivedMessage.GetBody(deserializedSampleObject);

        VERIFY_IS_TRUE(deserializedSampleObject.id_ == sampleObject.id_);
        VERIFY_IS_TRUE(deserializedSampleObject.isAwesome_ == sampleObject.isAwesome_);
        VERIFY_IS_TRUE(receivedMessage.Actor == actor);
    }

    struct LargeHeader : public MessageHeader<MessageHeaderId::Example>, public Serialization::FabricSerializable
    {
        LargeHeader() {}
        LargeHeader(std::vector<byte> && samples) : samples_(std::move(samples)) {}

        FABRIC_FIELDS_01(samples_);

    private:
        std::vector<byte> samples_;
    };

    BOOST_AUTO_TEST_CASE(HeaderAddFailure)
    {
        ENTER;

        Assert::DisableTestAssertInThisScope disableTestAssert;

        vector<byte> v(48*1024); // two such headers will exceed 64KB header limit
        for(auto & e : v)
        {
            e = 0xff; // choose a value that avoids integer compression in serialization 
        }

        LargeHeader lh(move(v));

        MessageHeaders mhs;
        auto status =  mhs.Add(MessageIdHeader());
        VERIFY_ARE_EQUAL2(status, STATUS_SUCCESS);

        status = mhs.Add(lh);
        Trace.WriteInfo(TraceType, "add first LargeHeader: {0:x}", (uint)status);
        VERIFY_ARE_EQUAL2(status, STATUS_SUCCESS);

        bool removed = mhs.TryRemoveHeader<MessageIdHeader>(); //remove MessageIdHeader to test auto-compact in Add
        VERIFY_IS_TRUE(removed);

        status = mhs.Add(lh);
        Trace.WriteInfo(TraceType, "add second LargeHeader: {0:x}", (uint)status);
        VERIFY_ARE_EQUAL2(status, STATUS_BUFFER_OVERFLOW);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SharedHeaderRemoveSaftyTest)
    {
        //Need to disable message CRC for this tset, as shared message headers are removed while sending
        auto saved = TransportConfig::GetConfig().MessageErrorCheckingEnabled;
        TransportConfig::GetConfig().MessageErrorCheckingEnabled = false;
        KFinally([=] { TransportConfig::GetConfig().MessageErrorCheckingEnabled = saved; }); 

        auto sender = TcpDatagramTransport::CreateClient();
        auto receiver = TcpDatagramTransport::Create(L"127.0.0.1:0");

        static int replyToSend = 20;
        static int idHeaderPerMessage = 1600;
        static int testMsgBodySize = 32;

        wstring testAction = TTestUtil::GetGuidAction();

        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [receiver, testAction](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
            {
                for (int i = 0; i < replyToSend; ++i)
                {
                    auto clone = msg->Clone();
                    clone->Headers.Add(ActionHeader(testAction));
                    MoveUPtr<Message>  msgMover(move(clone));

                    Threadpool::Post([receiver, st, msgMover] () mutable {
                        receiver->SendOneWay(st, msgMover.TakeUPtr());
                    });

                    // remove header while reply is being sent to test safety
                    for (auto iter = msg->Headers.Begin(); ; )
                    {
                        iter.Remove(); //Remove move iter by one position
                        if (iter == msg->Headers.End()) break;

                        ++iter; // move iter again to skip a header
                        if (iter == msg->Headers.End()) break;

                        ++iter; // move iter again to skip a header
                        if (iter == msg->Headers.End()) break;
                    }
                }
            });

        Common::atomic_long toReceive(replyToSend);
        AutoResetEvent allReplyReceived;
        TTestUtil::SetMessageHandler(
            sender,
            testAction,
            [&allReplyReceived, &toReceive](MessageUPtr & msg, ISendTarget::SPtr const &) -> void
            {
                int msgIdHeaderCount = 0;
                for (auto iter = msg->Headers.Begin(); iter != msg->Headers.End(); ++iter)
                {
                    if (iter->Id() == MessageHeaderId::MessageId)
                    {
                        ++msgIdHeaderCount;
                    }
                }

                Trace.WriteInfo(TraceType, "sender: there are {0} MessageIdHeader on reply {1}", msgIdHeaderCount, msg->TraceId());

                VERIFY_IS_TRUE(msgIdHeaderCount <= idHeaderPerMessage);

                TestMessageBody msgBody;
                VERIFY_IS_TRUE(msg->GetBody<TestMessageBody>(msgBody));
                VERIFY_IS_TRUE(msgBody.Verify());
                VERIFY_ARE_EQUAL2(msgBody.size(), testMsgBodySize);

                if (--toReceive == 0)
                {
                    allReplyReceived.Set();
                }
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto request(make_unique<Message>(TestMessageBody(testMsgBodySize)));
        request->Headers.Add(ActionHeader(testAction));
        for (int i = 0; i < idHeaderPerMessage; ++i)
        {
            VERIFY_ARE_EQUAL2(request->Headers.Add(MessageIdHeader()), STATUS_SUCCESS);
        }

        Trace.WriteInfo(TraceType, "sender sends {0}", request->TraceId());
        sender->SendOneWay(target, std::move(request));

        bool receivedMessageOne = allReplyReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedMessageOne);

        sender->Stop(TimeSpan::FromSeconds(10));
        receiver->Stop(TimeSpan::FromSeconds(10));
    }

    BOOST_AUTO_TEST_CASE(SharedHeaderConcurrentRemove)
    {
        //Need to disable message CRC for this tset, as shared message headers are removed while sending
        auto saved = TransportConfig::GetConfig().MessageErrorCheckingEnabled;
        TransportConfig::GetConfig().MessageErrorCheckingEnabled = false;
        KFinally([=] { TransportConfig::GetConfig().MessageErrorCheckingEnabled = saved; }); 

        auto sender = TcpDatagramTransport::CreateClient();
        auto receiver = TcpDatagramTransport::Create(L"127.0.0.1:0");

        static int replyToSend = 20;
        static int idHeaderPerMessage = 1600;
        static int testMsgBodySize = 32;

        wstring testAction = TTestUtil::GetGuidAction();

        TTestUtil::SetMessageHandler(
            receiver,
            testAction,
            [receiver, testAction](MessageUPtr & msg, ISendTarget::SPtr const & st) -> void
            {
                for (int s = 0; s < replyToSend; ++s)
                {
                    auto cloneToSend = msg->Clone();

                    vector<MoveUPtr<Message>> clonesForHeaderRemoval;
                    for (int thr = 0; thr < 3; ++thr)
                    {
                        auto clone = msg->Clone();
                        MoveUPtr<Message>  msgMover(move(clone));
                        clonesForHeaderRemoval.emplace_back(msgMover);
                    }

                    for(auto & msgMover : clonesForHeaderRemoval)
                    {
                        Threadpool::Post([msgMover]() mutable
                        {
                            auto c = msgMover.TakeUPtr();
                            // remove header while reply is being sent to test safety
                            for (auto iter = c->Headers.Begin(); ; )
                            {
                                iter.Remove(); //Remove move iter by one position
                                if (iter == c->Headers.End()) break;

                                for (int skip = 0; skip < 9; ++skip)
                                {
                                    ++iter; // move iter again to skip a header
                                    if (iter == c->Headers.End()) break;
                                }
                            }
                        });
                    }

                    cloneToSend->Headers.Add(ActionHeader(testAction));
                    receiver->SendOneWay(st, std::move(cloneToSend));
                }
            });

        Common::atomic_long toReceive(replyToSend);
        AutoResetEvent allReplyReceived;
        TTestUtil::SetMessageHandler(
            sender,
            testAction,
            [&allReplyReceived, &toReceive](MessageUPtr & msg, ISendTarget::SPtr const &) -> void
            {
                int msgIdHeaderCount = 0;
                for (auto iter = msg->Headers.Begin(); iter != msg->Headers.End(); ++iter)
                {
                    if (iter->Id() == MessageHeaderId::MessageId)
                    {
                        ++msgIdHeaderCount;
                    }
                }

                Trace.WriteInfo(TraceType, "sender: there are {0} MessageIdHeader on reply {1}", msgIdHeaderCount, msg->TraceId());

                VERIFY_IS_TRUE(msgIdHeaderCount <= idHeaderPerMessage);

                TestMessageBody msgBody;
                VERIFY_IS_TRUE(msg->GetBody<TestMessageBody>(msgBody));
                VERIFY_IS_TRUE(msgBody.Verify());
                VERIFY_ARE_EQUAL2(msgBody.size(), testMsgBodySize);

                if (--toReceive == 0)
                {
                    allReplyReceived.Set();
                }
            });

        VERIFY_IS_TRUE(receiver->Start().IsSuccess());
        VERIFY_IS_TRUE(sender->Start().IsSuccess());

        ISendTarget::SPtr target = sender->ResolveTarget(receiver->ListenAddress());
        VERIFY_IS_TRUE(target);

        auto request(make_unique<Message>(TestMessageBody(testMsgBodySize)));
        request->Headers.Add(ActionHeader(testAction));
        for (int i = 0; i < idHeaderPerMessage; ++i)
        {
            VERIFY_ARE_EQUAL2(request->Headers.Add(MessageIdHeader()), STATUS_SUCCESS);
        }

        Trace.WriteInfo(TraceType, "sender sends {0}", request->TraceId());
        sender->SendOneWay(target, std::move(request));

        bool receivedMessageOne = allReplyReceived.WaitOne(TimeSpan::FromSeconds(30));
        VERIFY_IS_TRUE(receivedMessageOne);

        sender->Stop(TimeSpan::FromSeconds(10));
        receiver->Stop(TimeSpan::FromSeconds(10));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
