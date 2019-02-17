#pragma once

#include <Wallet/PrivateExtKey.h>
#include <Wallet/KeyChainPath.h>
#include <Wallet/OutputStatus.h>
#include <Core/Models/TransactionOutput.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class OutputData
{
public:
	OutputData(KeyChainPath&& keyChainPath, TransactionOutput&& output, const uint64_t amount, const EOutputStatus status)
		: m_keyChainPath(std::move(keyChainPath)), m_output(std::move(output)), m_amount(amount), m_status(status)
	{

	}

	//
	// Getters
	//
	inline const KeyChainPath& GetKeyChainPath() const { return m_keyChainPath; }
	inline const TransactionOutput& GetOutput() const { return m_output; }
	inline const uint64_t GetAmount() const { return m_amount; }
	inline const EOutputStatus GetStatus() const { return m_status; }

	//
	// Setters
	//
	inline void SetStatus(const EOutputStatus status) { m_status = status; }

	//
	// Serialization
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.AppendVarStr(m_keyChainPath.ToString());
		m_output.Serialize(serializer);
		serializer.Append(m_amount);
		serializer.Append((uint8_t)m_status);
	}

	//
	// Deserialization
	//
	static OutputData Deserialize(ByteBuffer& byteBuffer)
	{
		KeyChainPath keyChainPath = KeyChainPath::FromString(byteBuffer.ReadVarStr());
		TransactionOutput output = TransactionOutput::Deserialize(byteBuffer);
		uint64_t amount = byteBuffer.ReadU64();
		EOutputStatus status = (EOutputStatus)byteBuffer.ReadU8();

		return OutputData(std::move(keyChainPath), std::move(output), amount, status);
	}

private:
	KeyChainPath m_keyChainPath;
	TransactionOutput m_output;
	uint64_t m_amount;
	EOutputStatus m_status;
};