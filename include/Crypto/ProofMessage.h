#pragma once

#include <Crypto/BigInteger.h>
#include <stdint.h>

class ProofMessage
{
public:
	ProofMessage(CBigInteger<16>&& proofMessageBytes)
		: m_proofMessageBytes(proofMessageBytes)
	{

	}

	static ProofMessage FromKeyIndices(const std::vector<uint32_t>& keyIndices)
	{
		Serializer serializer;
		for (const uint32_t keyIndex : keyIndices)
		{
			serializer.Append(keyIndex);
		}

		std::vector<unsigned char> paddedPath(16, 0);
		for (size_t i = 0; i < serializer.GetBytes().size(); i++)
		{
			paddedPath[i] = serializer.GetBytes()[i];
		}

		return ProofMessage(CBigInteger<16>(paddedPath));
	}

	inline const CBigInteger<16>& GetBytes() const { return m_proofMessageBytes; }

private:
	CBigInteger<16> m_proofMessageBytes;
};