#pragma once

#include <Config/Config.h>
#include <Common/Secure.h>
#include <Common/Util/StringUtil.h>
#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/RestoreWalletCriteria.h>
#include <API/Wallet/Owner/Models/LoginResponse.h>
#include <optional>

class RestoreWalletHandler : RPCMethod
{
public:
	RestoreWalletHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~RestoreWalletHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		RestoreWalletCriteria criteria = RestoreWalletCriteria::FromJSON(request.GetParams().value());
		ValidateInput(criteria);

		auto response = m_pWalletManager->RestoreFromSeed(criteria);

		return request.BuildResult(response.ToJSON());
	}

private:
	void ValidateInput(const RestoreWalletCriteria& criteria) const
	{
		// TODO: Should we allow usernames to contain spaces or special characters?

		std::vector<std::string> accounts = m_pWalletManager->GetAllAccounts();
		for (const std::string& account : accounts)
		{
			if (StringUtil::ToLower(account) == criteria.GetUsername())
			{
				throw API_EXCEPTION_F(
					RPC::ErrorCode::INVALID_PARAMS,
					"Username {} already exists",
					criteria.GetUsername()
				);
			}
		}

		if (criteria.GetPassword().empty())
		{
			throw API_EXCEPTION(
				RPC::ErrorCode::INVALID_PARAMS,
				"Password cannot be empty"
			);
		}

		const size_t numWords = StringUtil::Split((const std::string&)criteria.GetSeedWords(), " ").size();
		if (numWords < 12 || numWords > 24 || numWords % 3 != 0)
		{
			throw API_EXCEPTION_F(
				RPC::ErrorCode::INVALID_PARAMS,
				"Invalid num_seed_words ({}). Only 12, 15, 18, 21, and 24 are supported.",
				numWords
			);
		}
	}

	IWalletManagerPtr m_pWalletManager;
};