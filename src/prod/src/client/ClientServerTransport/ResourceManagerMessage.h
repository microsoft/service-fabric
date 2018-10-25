// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
	typedef std::function<Client::ClientServerRequestMessageUPtr(
		std::wstring const & action,
		Transport::Actor::Enum const & actor,
		Management::ResourceManager::ResourceIdentifier const & resourceId,
		std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
		Common::ActivityId const & activityId)> ResourceManagerRequestMessageConstructor;

	class ResourceManagerMessage : public Client::ClientServerRequestMessage
	{
	public:
		static Common::GlobalWString ClaimResourceAction;
		static Common::GlobalWString ReleaseResourceAction;

		static Client::ClientServerRequestMessageUPtr CreateRequestMessage(
			std::wstring const & action,
			Management::ResourceManager::ResourceIdentifier const & resourceId,
			std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
			Common::ActivityId const & activityId);

		ResourceManagerMessage(
			std::wstring const & action,
			Transport::Actor::Enum const & actor,
			std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
			Common::ActivityId const & activityId = Common::ActivityId());

		ResourceManagerMessage(
			std::wstring const & action,
			Transport::Actor::Enum const & actor,
			Common::ActivityId const & activityId = Common::ActivityId());
	private:
		struct ResourceRegistration
		{
			ResourceRegistration(
				Transport::Actor::Enum const & actor,
				ResourceManagerRequestMessageConstructor const & messageConstructor);

			Transport::Actor::Enum const Actor;
			ResourceManagerRequestMessageConstructor const MessageConstructor;
		};

		static Client::ClientServerRequestMessageUPtr CreateDefaultRequestMessage(
			std::wstring const & action,
			Transport::Actor::Enum const & actor,
			Management::ResourceManager::ResourceIdentifier const & resourceId,
			std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
			Common::ActivityId const & activityId);

		static Client::ClientServerRequestMessageUPtr CreateSecretRequestMessage(
			std::wstring const & action,
			Transport::Actor::Enum const & actor,
			Management::ResourceManager::ResourceIdentifier const & resourceId,
			std::unique_ptr<ServiceModel::ClientServerMessageBody> && body,
			Common::ActivityId const & activityId);

		static std::unordered_map<std::wstring, ResourceRegistration const> const resourceRegistrationMap_;		
	};
}