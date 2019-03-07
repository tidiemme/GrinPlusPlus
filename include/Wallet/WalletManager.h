#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <Common/ImportExport.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/SessionToken.h>
#include <Wallet/SelectionStrategy.h>
#include <Wallet/Slate.h>
#include <Wallet/WalletSummary.h>
#include <Common/Secure.h>

#ifdef MW_WALLET
#define WALLET_API EXPORT
#else
#define WALLET_API IMPORT
#endif

class IWalletManager
{
public:
	//
	// Creates a new wallet with the username and password given, and returns the space-delimited wallet words (BIP39 mnemonics).
	// If a wallet for the user already exists, an empty string will be returned.
	//
	virtual std::optional<std::pair<SecureString, SessionToken>> InitializeNewWallet(const std::string& username, const SecureString& password) = 0;

	//
	// Authenticates the user, and if successful, returns a session token that can be used in lieu of credentials for future calls.
	//
	virtual std::unique_ptr<SessionToken> Login(const std::string& username, const SecureString& password) = 0;

	//
	// Deletes the session information.
	//
	virtual void Logout(const SessionToken& token) = 0;

	virtual WalletSummary GetWalletSummary(const SessionToken& token, const uint64_t minimumConfirmations) const = 0;

	//
	// Sends coins to the given destination using the specified method. Returns a valid slate if successful.
	// Exceptions thrown:
	// * SessionTokenException - If no matching session found, or if the token is invalid.
	// * InsufficientFundsException - If there are not enough funds ready to spend after calculating and including the fee.
	//
	virtual std::unique_ptr<Slate> Send(const SessionToken& token, const uint64_t amount, const uint64_t feeBase, const std::optional<std::string>& messageOpt, const ESelectionStrategy& strategy) = 0;

	virtual bool Receive(const SessionToken& token, Slate& slate, const std::optional<std::string>& messageOpt) = 0;

	virtual std::unique_ptr<Transaction> Finalize(const SessionToken& token, const Slate& slate) = 0;
};

namespace WalletAPI
{
	//
	// Creates a new instance of the Wallet server.
	//
	WALLET_API IWalletManager* StartWalletManager(const Config& config, const INodeClient& nodeClient);

	//
	// Stops the Wallet server and clears up its memory usage.
	//
	WALLET_API void ShutdownWalletManager(IWalletManager* pWalletManager);
}