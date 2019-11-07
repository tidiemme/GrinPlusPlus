#include <PMMR/TxHashSetManager.h>

#include "TxHashSetImpl.h"
#include "Zip/TxHashSetZip.h"
#include "Zip/Zipper.h"

#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

TxHashSetManager::TxHashSetManager(const Config& config)
	: m_config(config), m_pTxHashSet(nullptr)
{

}

std::shared_ptr<Locked<ITxHashSet>> TxHashSetManager::Open(const BlockHeader& confirmedTip)
{
	Close();

	std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(m_config.GetTxHashSetDirectory());
	std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(m_config.GetTxHashSetDirectory());
	std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(m_config.GetTxHashSetDirectory());

	auto pTxHashSet = std::shared_ptr<TxHashSet>(new TxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, confirmedTip));
	m_pTxHashSet = std::make_shared<Locked<ITxHashSet>>(Locked<ITxHashSet>(pTxHashSet));

	return m_pTxHashSet;
}

std::shared_ptr<ITxHashSet> TxHashSetManager::LoadFromZip(const Config& config, std::shared_ptr<Locked<IBlockDB>> pBlockDB, const std::string& zipFilePath, const BlockHeader& blockHeader)
{
	const TxHashSetZip zip(config);
	if (zip.Extract(zipFilePath, blockHeader))
	{
		LOG_INFO_F("%s extracted successfully", zipFilePath);
		FileUtil::RemoveFile(zipFilePath);

		std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(config.GetTxHashSetDirectory());
		pKernelMMR->Rewind(blockHeader.GetKernelMMRSize());
		pKernelMMR->Commit();

		std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(config.GetTxHashSetDirectory());
		pOutputPMMR->Rewind(blockHeader.GetOutputMMRSize(), Roaring());
		pOutputPMMR->Commit();

		std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(config.GetTxHashSetDirectory());
		pRangeProofPMMR->Rewind(blockHeader.GetOutputMMRSize(), Roaring());
		pRangeProofPMMR->Commit();

		return std::shared_ptr<TxHashSet>(new TxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, blockHeader));
	}

	return nullptr;
}

bool TxHashSetManager::SaveSnapshot(std::shared_ptr<const IBlockDB> pBlockDB, const BlockHeader& blockHeader, const std::string& zipFilePath)
{
	if (m_pTxHashSet == nullptr)
	{
		return false;
	}

	const std::string snapshotDir = StringUtil::Format("%sSnapshots/%s/", fs::temp_directory_path().string(), blockHeader.ShortHash());
	std::unique_ptr<BlockHeader> pFlushedHeader = nullptr;

	{
		// 1. Lock TxHashSet
		auto reader = m_pTxHashSet->Read();

		// 2. Copy to Snapshots/Hash // TODO: If already exists, just use that.
		const std::string txHashSetDirectory = m_config.GetTxHashSetDirectory();
		if (!FileUtil::CopyDirectory(txHashSetDirectory, snapshotDir))
		{
			return false;
		}

		pFlushedHeader = std::make_unique<BlockHeader>(BlockHeader(reader->GetFlushedBlockHeader()));
	}

	{
		// 4. Load Snapshot TxHashSet
		std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(snapshotDir);
		std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(snapshotDir);
		std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(snapshotDir);
		TxHashSet snapshotTxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, *pFlushedHeader);

		// 5. Rewind Snapshot TxHashSet
		if (!snapshotTxHashSet.Rewind(pBlockDB, blockHeader))
		{
			return false;
		}

		// 6. Flush Snapshot TxHashSet
		snapshotTxHashSet.Commit();

		// 7. Rename pmmr_leaf files
		FileUtil::RenameFile(snapshotDir + "output/pmmr_leaf.bin", snapshotDir + "output/pmmr_leaf.bin." + blockHeader.ShortHash());
		FileUtil::RenameFile(snapshotDir + "rangeproof/pmmr_leaf.bin", snapshotDir + "rangeproof/pmmr_leaf.bin." + blockHeader.ShortHash());

		// 8. Create Zip
		const std::vector<std::string> pathsToZip = { snapshotDir + "kernel", snapshotDir + "output", snapshotDir + "rangeproof" };
		if (!Zipper::CreateZipFile(zipFilePath, pathsToZip))
		{
			return false;
		}
	}

	// 9. Delete Snapshots/Hash folder
	FileUtil::RemoveFile(snapshotDir);

	return true;
}

std::shared_ptr<Locked<ITxHashSet>> TxHashSetManager::GetTxHashSet()
{
	return m_pTxHashSet;
}

std::shared_ptr<const Locked<ITxHashSet>> TxHashSetManager::GetTxHashSet() const
{
	return m_pTxHashSet;
}

void TxHashSetManager::SetTxHashSet(std::shared_ptr<ITxHashSet> pTxHashSet)
{
	Close();
	m_pTxHashSet = std::make_shared<Locked<ITxHashSet>>(Locked<ITxHashSet>(pTxHashSet));
}

void TxHashSetManager::Close()
{
	m_pTxHashSet.reset();
}