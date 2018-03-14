// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <typename T>
    class ConfigEntry : public ConfigEntryBase
    {
        DENY_COPY(ConfigEntry);

    public:
        ConfigEntry()
            : isEncrypted_(false),
            encryptedValue_(L"")
        {
        }

        T GetValue() const
        { 
            if(isEncrypted_)
            {
                return DecryptValue(section_, key_, encryptedValue_);
            }
            else
            {
                return value_;
            }
        }

        bool IsEncrypted() { return isEncrypted_; }

        std::wstring GetEncryptedValue() { return encryptedValue_; }

        __declspec(property(get=get_DefaultValue)) T const & DefaultValue;
        T const & get_DefaultValue() const { return defaultValue_; }

        bool LoadValue(std::wstring & stringValue, bool isEncrypted, bool isCheckOnly)
        {
            bool isValid = true;         
            T newValue;

            if(stringValue.empty())
            {
                ASSERT_IF(
                    isEncrypted_ , 
                    "Encrypted property cannot be empty. Property {0}.{1} is encrypted but has no value", section_, key_);

                newValue = defaultValue_;
                stringValue = wformatString(defaultValue_);
            }            
            else
            {
                if(isEncrypted)
                {                    
                    newValue = DecryptValue(section_, key_, stringValue);
                }
                else
                {
                    isValid = Config::TryParse(newValue, stringValue);
                }
            }
            
            if (isValid && validator_)
            {
                isValid = validator_->IsValid(newValue);
            }

            if (!isValid)
            {                
                Assert::CodingError(
                    "Property {0}.{1} configured with invalid value {2}.  IsPropertyValueEncrypted = {3}.",
                    section_, key_, stringValue, isEncrypted);
            }   

            bool updated = (!loaded_ || (isEncrypted_ != isEncrypted) || (isEncrypted ? (newValue != DecryptValue(section_, key_, encryptedValue_)) : (value_ != newValue)));
            if (updated)
            {
                ConfigEventSource::Events.ValueUpdated(componentConfig_->Name, key_, stringValue, isEncrypted);
            }

            if (!isCheckOnly)
            {
                if(!isEncrypted)
                {
                    encryptedValue_ = L"";
                    value_ = newValue;
                }
                else
                {
                    encryptedValue_ = stringValue;
                }

                isEncrypted_ = isEncrypted;
                loaded_ = hasValue_ = true;
            }

            return updated;
        }

        bool LoadValue()
        {
            bool isEncrypted;
            std::wstring stringValue = componentConfig_->FabricGetConfigStore().ReadString(section_, key_, isEncrypted);

            return LoadValue(stringValue, isEncrypted, false);
        }

        static T DecryptValue(std::wstring const & section, std::wstring const & key, std::wstring const & encryptedValue)
        {
#if !defined(PLATFORM_UNIX)
            SecureString decryptedValue;
            ErrorCode error = CryptoUtility::DecryptText(encryptedValue, decryptedValue);
            if(error.IsSuccess())
            {
                T result;                    
                if(Config::TryParse<T>(result, decryptedValue.GetPlaintext()))
                {
                    return result;
                }
                else
                {
                    Assert::CodingError(
                        "Property {0}.{1} configured with invalid value.",
                        section, key);
                }
            }
            else
            {
                Assert::CodingError(
                    "Property {0}.{1} could not be decrypted. Property Value: {2}, Error: {3}",
                    section,
                    key,
                    encryptedValue,
                    error);                    
            }
#else
            //LINUXTODO
            UNREFERENCED_PARAMETER(section);
            UNREFERENCED_PARAMETER(key);
            UNREFERENCED_PARAMETER(encryptedValue);
            
            Assert::CodingError("Encrypted config entry not yet supported on Linux");
#endif
        }

        void Load(ComponentConfig const * componentConfig, std::wstring const & section, std::wstring const & key, T const & defaultValue, ConfigEntryUpgradePolicy::Enum upgradePolicy)
        {
            defaultValue_ = defaultValue;
            Initialize(componentConfig, section, key, upgradePolicy);

            LoadValue();
        }

        template <class Pred>
        void Load(ComponentConfig const * componentConfig, std::wstring const & section, std::wstring const & key, T const & defaultValue,  ConfigEntryUpgradePolicy::Enum upgradePolicy, Pred pred)
        {
    		validator_ = std::unique_ptr<Validator<T>>(static_cast<Validator<T>*>(new Pred(pred)));

            Load(componentConfig, section, key, defaultValue, upgradePolicy);
        }

		void SetValue(ComponentConfig const * componentConfig, std::wstring const & section, std::wstring const & key, T const & value, T const & defaultValue, ConfigEntryUpgradePolicy::Enum upgradePolicy)
		{
			defaultValue_ = defaultValue;
			Initialize(componentConfig, section, key, upgradePolicy);

			SetValueInternalAndTrace(value);
		}

		void Test_SetValue(T const & value)
		{
            if (value != value_)
            {
                SetValueInternal(value);
                event_.Fire(EventArgs(), true);
            }
		}

        template <class Pred>
        void SetValue(ComponentConfig const * componentConfig, std::wstring const & section, std::wstring const & key, T const & value, T const & defaultValue, ConfigEntryUpgradePolicy::Enum upgradePolicy, Pred pred)
        {
    		validator_ = std::unique_ptr<Validator<T>>(static_cast<Validator<T>*>(new Pred(pred)));

            SetValue(componentConfig, section, key, value, defaultValue, upgradePolicy);
        }

    private:
        T value_;
        T defaultValue_;
        std::wstring encryptedValue_;
        bool isEncrypted_;        
        std::unique_ptr<Validator<T>> validator_;

		void SetValueInternal(T const & value)
		{
			value_ = value;
			hasValue_ = true;
		}

        void SetValueInternalAndTrace(T const & value)
        {
            SetValueInternal(value);

            ComponentConfig::WriteInfo(componentConfig_->Name,
                "Set property {0}.{1} with value {2}",
                section_, key_, value);
        }
    };
}
