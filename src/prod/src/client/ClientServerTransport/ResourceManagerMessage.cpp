// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Client;
using namespace ClientServerTransport;
using namespace ServiceModel;
using namespace SystemServices;

using namespace Management::ResourceManager;

/* Resource Registration */

unordered_map<wstring, ResourceManagerMessage::ResourceRegistration const> const ResourceManagerMessage::resourceRegistrationMap_{
    // Secret from Central Secret Store
    { *ResourceTypes::Secret, ResourceRegistration(Actor::CSS, CreateSecretRequestMessage) }
};

GlobalWString ResourceManagerMessage::ClaimResourceAction = make_global<wstring>(L"ClaimResourceAction");
GlobalWString ResourceManagerMessage::ReleaseResourceAction = make_global<wstring>(L"ReleaseResourceAction");

ResourceManagerMessage::ResourceManagerMessage(
    wstring const & action,
    Actor::Enum const & actor,
    unique_ptr<ClientServerMessageBody> && body,
    Common::ActivityId const & activityId)
    : ClientServerRequestMessage(action, actor, std::move(body), activityId)
{
}

ResourceManagerMessage::ResourceManagerMessage(
    wstring const & action,
    Actor::Enum const & actor,
    Common::ActivityId const & activityId)
    : ClientServerRequestMessage(action, actor, activityId)
{
}

ClientServerRequestMessageUPtr ResourceManagerMessage::CreateRequestMessage(
    wstring const & action,
    ResourceIdentifier const & resourceId,
    unique_ptr<ClientServerMessageBody> && body,
    Common::ActivityId const & activityId)
{
    auto element = resourceRegistrationMap_.find(resourceId.ResourceType);

    if (element == resourceRegistrationMap_.end())
    {
        Assert::CodingError("Unknown resource type: {0}, ResourceIdentifier = {1}.", resourceId.ResourceType, resourceId);
    }

    return element->second.MessageConstructor(action, element->second.Actor, resourceId, move(body), activityId);
}

ResourceManagerMessage::ResourceRegistration::ResourceRegistration(
    Actor::Enum const & actor,
    ResourceManagerRequestMessageConstructor const & messageConstructor)
    : Actor(actor)
    , MessageConstructor(messageConstructor)
{
}

/**
 * Resource Message Constructors
 */

 /* Default Resource Manager Request Message Constructor */
ClientServerRequestMessageUPtr ResourceManagerMessage::CreateDefaultRequestMessage(
    wstring const & action,
    Actor::Enum const & actor,
    ResourceIdentifier const & resourceId,
    unique_ptr<ClientServerMessageBody> && body,
    Common::ActivityId const & activityId)
{
    UNREFERENCED_PARAMETER(resourceId);

    return make_unique<ResourceManagerMessage>(action, actor, move(body), activityId);
}

/* Secret Store Service Request Message Constructor */
ClientServerRequestMessageUPtr ResourceManagerMessage::CreateSecretRequestMessage(
    wstring const & action,
    Actor::Enum const & actor,
    ResourceIdentifier const & resourceId,
    unique_ptr<ClientServerMessageBody> && body,
    Common::ActivityId const & activityId)
{
    UNREFERENCED_PARAMETER(actor);
    UNREFERENCED_PARAMETER(resourceId);

    return CentralSecretServiceMessage::CreateRequestMessage(action, move(body), activityId);
}