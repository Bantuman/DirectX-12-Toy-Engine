#pragma once

#include <unordered_map>

template<typename T>
class AssetHandler
{
public:
	virtual RefPtr<T> TryGet(u64 hash)
	{
		std::lock_guard<std::mutex> lock(m_RegistryMutex);
		if (auto registryIterator = m_Registry.find(hash); registryIterator != m_Registry.end())
		{
			return registryIterator->second;
		}
		return nullptr;
	}

protected:
	virtual RefPtr<T> InternalGetOrLoad(u64 hash, std::function<RefPtr<T>()>&& loadFunctor)
	{
		std::lock_guard<std::mutex> lock(m_RegistryMutex);
		if (auto registryIterator = m_Registry.find(hash); registryIterator != m_Registry.end())
		{
			return registryIterator->second;
		}
		auto const asset = loadFunctor();
		m_Registry.emplace(std::make_pair(hash, asset));
		return asset;
	}
	std::unordered_map<u64, RefPtr<T>> m_Registry;
private:
	std::mutex                              m_RegistryMutex;
};