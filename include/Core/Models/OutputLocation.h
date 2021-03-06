#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <Core/Util/JsonUtil.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Traits/Serializable.h>

class OutputLocation : public Traits::ISerializable
{
public:
	//
	// Constructor
	//
	OutputLocation(const uint64_t mmrIndex, const uint64_t blockHeight)
		: m_mmrIndex(mmrIndex), m_blockHeight(blockHeight) { }
	~OutputLocation() = default;

	//
	// Getters
	//
	uint64_t GetMMRIndex() const { return m_mmrIndex; }
	uint64_t GetBlockHeight() const { return m_blockHeight; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const noexcept final
	{
		serializer.Append(m_mmrIndex);
		serializer.Append(m_blockHeight);
	}

	static OutputLocation Deserialize(ByteBuffer& byteBuffer)
	{
		const uint64_t mmrIndex = byteBuffer.ReadU64();
		const uint64_t blockHeight = byteBuffer.ReadU64();
		return OutputLocation(mmrIndex, blockHeight);
	}

	static OutputLocation FromJSON(const Json::Value& json)
	{
		uint64_t mmrIndex = (std::max)((uint64_t)1, JsonUtil::GetRequiredUInt64(json, "mmr_index")) - 1;
		uint64_t height = JsonUtil::GetRequiredUInt64(json, "block_height");

		return OutputLocation(mmrIndex, height);
	}

private:
	uint64_t m_mmrIndex;
	uint64_t m_blockHeight;
};