#include "WalletRefresher.h"
#include "WalletUtil.h"

#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>

WalletRefresher::WalletRefresher(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB)
	: m_config(config), m_nodeClient(nodeClient), m_walletDB(walletDB)
{

}

std::vector<OutputData> WalletRefresher::RefreshOutputs(const std::string& username, const CBigInteger<32>& masterSeed)
{
	std::vector<Commitment> commitments;

	std::vector<OutputData> outputs = m_walletDB.GetOutputs(username, masterSeed);
	for (const OutputData& outputData : outputs)
	{
		if (outputData.GetStatus() != EOutputStatus::SPENT)
		{
			// TODO: What if commitment has mmr_index?
			commitments.push_back(outputData.GetOutput().GetCommitment());
		}
	}

	const uint64_t lastConfirmedHeight = m_nodeClient.GetChainHeight();
	m_walletDB.UpdateRefreshBlockHeight(username, lastConfirmedHeight);

	std::vector<OutputData> outputsToUpdate;
	const std::map<Commitment, OutputLocation> outputLocations = m_nodeClient.GetOutputsByCommitment(commitments);
	for (OutputData& outputData : outputs)
	{
		// TODO: Figure out how to tell the difference between spent and awaitingConfirmation
		auto iter = outputLocations.find(outputData.GetOutput().GetCommitment());
		if (iter != outputLocations.cend())
		{
			if (outputData.GetStatus() != EOutputStatus::LOCKED)
			{
				const EOutputFeatures features = outputData.GetOutput().GetFeatures();
				const uint64_t outputBlockHeight = iter->second.GetBlockHeight();
				const uint32_t minimumConfirmations = m_config.GetWalletConfig().GetMinimumConfirmations();

				if (WalletUtil::IsOutputImmature(features, outputBlockHeight, lastConfirmedHeight, minimumConfirmations))
				{
					if (outputData.GetStatus() != EOutputStatus::IMMATURE)
					{
						outputData.SetStatus(EOutputStatus::IMMATURE);
						outputsToUpdate.push_back(outputData);
					}
				}
				else
				{
					if (outputData.GetStatus() != EOutputStatus::SPENDABLE)
					{
						outputData.SetStatus(EOutputStatus::SPENDABLE);
						outputsToUpdate.push_back(outputData);
					}
				}
			}
		}
		else if (outputData.GetStatus() != EOutputStatus::NO_CONFIRMATIONS)
		{
			outputData.SetStatus(EOutputStatus::SPENT);
			outputsToUpdate.push_back(outputData);
		}
	}

	m_walletDB.AddOutputs(username, masterSeed, outputsToUpdate);

	return outputs;
}