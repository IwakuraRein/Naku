#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include "naku.hpp"
#include "utils/device.hpp"

namespace naku {

class Resource {
public:
	Resource(Device& device, const std::string& name)
		:_device{ device }, _name{ name } {}
	~Resource() {};
	Resource(const Resource&&) = delete;
	Resource() = delete;
	Resource& operator=(const Resource&&) = delete;

	std::string name() const { return _name; }
	ResId id() const { return _id; }
	virtual void setId(ResId id) { _id = id; }
	VkDevice device() const { return _device.device(); }

protected:
	Device& _device;
	std::string _name;
	ResId _id;
};

class ResourceCollectionBase {
public:
	ResourceCollectionBase(const std::string& typeName) : _typeName{ typeName } {}
	ResourceCollectionBase(const ResourceCollectionBase&&) = delete;
	ResourceCollectionBase& operator=(const ResourceCollectionBase&&) = delete;

	size_t size() const { return _baseMap.size(); }

	ResId push(const std::string& name, std::shared_ptr<void> pRes) {
		if (MapHas(_name2id, name)) {
			std::cerr << "Error: " << _typeName << ": " << name << " already exists." << std::endl;
			return ERROR_RES_ID;
		}
		_name2id.emplace(name, _nextId);
		_id2name.emplace(_nextId, name);
		_baseMap.emplace(_nextId, pRes);
		return _nextId++;
	}
	bool exist(const ResId& id) const { if (id >= _nextId || MapHas(_deleted, id)) return false; return true; }
	bool exist(const std::string& name) const { return MapHas(_name2id, name); }
	std::string name(const ResId& id) const { return _id2name.at(id); }
	void remove(const ResId& id) {
		if (exist(id)) {
			_baseMap[id].reset();
			_deleted.emplace(id, id);
			_name2id.erase(_id2name[id]);
			_id2name.erase(id);
		}
		else std::cerr << "Warning: " << _typeName << ": No. " << id << " isn't in storage." << std::endl;
	}
	void remove(const std::string& name) {
		if (MapHas(_name2id, name)) {
			remove(_name2id[name]);
		}
		else std::cerr << "Warning: " << _typeName << ": " << name << " isn't in storage." << std::endl;
	}
	void reName(const ResId& id, const std::string& name) {
		if (exist(id)) {
			if (MapHas(_name2id, name)) {
				std::cerr << "Error: " << _typeName << ": New name " << name << " already exists." << std::endl;
				return;
			}
			_name2id.erase(_id2name[id]);
			_name2id[name] = id;
			_id2name[id] = name;
		}
		else std::cerr << "Warning: " << _typeName << ": No. " << id << " isn't in storage." << std::endl;
	}
	void reName(const std::string& oldName, const std::string& newName) {
		if (MapHas(_name2id, oldName)) {
			if (!MapHas(_name2id, newName))
				reName(_name2id[oldName], newName);
			else std::cerr << "Warning: " << newName << " already exists." << std::endl;
		}
		else std::cerr << "Warning: " << oldName << " isn't in storage." << std::endl;
	}
	void createCollect(size_t type) {
		_collections.emplace(type, std::move(std::unordered_map<ResId, std::list<ResId>>{}));
	}
	std::list<ResId>& getCollect(size_t type, ResId id) {
		return _collections[type][id];
	}

	template<typename T>
	friend class ResourceCollection;
	friend class ResourceManager;

private:
	std::string _typeName;
	ResId _nextId{ 0 };
	std::unordered_map<ResId, std::shared_ptr<void>> _baseMap;
	std::unordered_map<std::string, ResId> _name2id;
	std::unordered_map<ResId, std::string> _id2name;
	std::unordered_map<ResId, ResId> _deleted;

	std::unordered_map<size_t, std::unordered_map<ResId, std::list<ResId>>> _collections;
	std::unordered_map<ResId, std::unordered_map<size_t, std::list<ResId>>> _collected;
};

template<typename T>
class ResourceCollection : public ResourceCollectionBase {
public:
	ResourceCollection() : ResourceCollectionBase(typeid(T).name()) {};
	ResourceCollection(ResourceCollectionBase&&) = delete;
	ResourceCollection(ResourceCollection&&) = delete;
	ResourceCollection& operator=(const ResourceCollection&&) = delete;
	ResourceCollection& operator=(const ResourceCollectionBase&&) = delete;
	std::shared_ptr<T> operator[](const ResId& id) {
		auto it = _baseMap.find(id);
		if (it == _baseMap.end()) return nullptr;
		return std::static_pointer_cast<T>(it->second);
	}
	std::shared_ptr<T> operator[](const std::string& name) {
		auto it = _name2id.find(name);
		if (it == _name2id.end()) return nullptr;
		auto it2 = _baseMap.find(it->second);
		return std::static_pointer_cast<T>(it2->second);
	}
	ResId push(const std::string& name, std::shared_ptr<T> pRes) {
		return ResourceCollectionBase::push(name, std::static_pointer_cast<void>(std::move(pRes)));
	}
	template<typename TT>
	void createCollect() {
		if (MapHas(_collections, typeid(TT).hash_code())) {
			std::cerr << "Warning: " << _typeName << ": Connection to resource " << typeid(TT).name() << "already created" << std::endl;
			return;
		}
		ResourceCollectionBase::createCollect(typeid(TT).hash_code());
	}
	template<typename TT>
	std::list<ResId>& getCollect(ResId id) {
		if (!MapHas(_collections, typeid(TT).hash_code())) {
			std::cerr << "Error: " << _typeName << ": The connection to " << typeid(TT).name() << " isn't created yet." << std::endl;
		}
		return ResourceCollectionBase::getCollect(typeid(TT).hash_code(), id);
	}

	friend class ResourceManager;
};
class ResourceManager {
public:
	ResourceManager() = default;
	ResourceManager(ResourceManager&&) = delete;
	ResourceManager& operator=(const ResourceManager&&) = delete;
	template<typename T>
	void addResource() {
		std::unique_ptr<ResourceCollectionBase> pRes(new ResourceCollection<T>);
		_resources.emplace(typeid(T).hash_code(), std::move(pRes));
	}
	template<typename T>
	ResourceCollection<T>& T() const {
		ResourceCollectionBase& res = *_resources.at(typeid(T).hash_code());
		return static_cast<ResourceCollection<T>&>(res);
	}
	template<typename T>
	ResourceCollection<T>& getResource() const {
		ResourceCollectionBase& res = *_resources.at(typeid(T).hash_code());
		return static_cast<ResourceCollection<T>&>(res);
	}
	template<typename T>
	std::shared_ptr<T> get(ResId id) const {
		ResourceCollection<T>& res = static_cast<ResourceCollection<T>&>(*_resources.at(typeid(T).hash_code()));
		return res[id];
	}
	template<typename T>
	std::shared_ptr<T> get(const std::string& name) const {
		ResourceCollection<T>& res = static_cast<ResourceCollection<T>&>(*_resources.at(typeid(T).hash_code()));
		return res[name];
	}
	template<typename T, typename TT>
	std::list<ResId>& getCollect(ResId id) const {
		ResourceCollection<T>& res = static_cast<ResourceCollection<T>&>(*_resources.at(typeid(T).hash_code()));
		return res.getCollect<TT>(id);
	}
	template<typename T>
	ResId push(const std::string& name, std::shared_ptr<T> pRes) const {
		ResourceCollection<T>& res = static_cast<ResourceCollection<T>&>(*_resources.at(typeid(T).hash_code()));
		return res.push(name, pRes);
	}
	template<typename T>
	size_t size() const {
		ResourceCollection<T>& res = static_cast<ResourceCollection<T>&>(*_resources.at(typeid(T).hash_code()));
		return res.size();
	}
	template<typename T>
	bool exist(ResId id) const {
		ResourceCollection<T>& res = static_cast<ResourceCollection<T>&>(*_resources.at(typeid(T).hash_code()));
		return res.exist(id);
	}
	template<typename T>
	bool exist(const std::string& name) const {
		ResourceCollection<T>& res = static_cast<ResourceCollection<T>&>(*_resources.at(typeid(T).hash_code()));
		return res.exist(name);
	}
	template<typename T, typename TT>
	void createCollect() {
		_collections.emplace(typeid(T).hash_code(), typeid(TT).hash_code());
		ResourceCollection<T>& res = static_cast<ResourceCollection<T>&>(*_resources.at(typeid(T).hash_code()));
		res.createCollect<TT>();
	}
	template<typename T, typename TT>
	void addCollect(ResId id1, ResId id2) const {
		ResourceCollectionBase& res1 = *_resources.at(typeid(T).hash_code());
		ResourceCollectionBase& res2 = *_resources.at(typeid(TT).hash_code());
		res1._collections[typeid(TT).hash_code()][id1].push_back(id2);
		res2._collected[id2][typeid(T).hash_code()].push_back(id1);
	}
	template<typename T>
	void removeItem(ResId id) const {
		ResourceCollectionBase& res = *_resources.at(typeid(T).hash_code());
		res.remove(id);
		size_t type = typeid(T).hash_code();
		for (auto& pair : res.collected) {
			if (pair.first == id) {
				for (auto& pair2 : pair.second) {
					ResourceCollectionBase& res = *_resources.at(pair2.first);
					for (ResId id2 : pair2.second)
						res._collections[type][id2].remove(id);
				}
			}
		}
	}
	template<typename T, typename TT>
	void removeFromCollect(ResId id1, ResId id2) const {
		ResourceCollectionBase& res1 = *_resources.at(typeid(T).hash_code());
		ResourceCollectionBase& res2 = *_resources.at(typeid(TT).hash_code());
		res1._collections[typeid(TT).hash_code()][id1].remove(id2);
		res2._collected[id2][typeid(T).hash_code()].remove(id1);
	}
private:
	std::unordered_map<size_t, std::unique_ptr<ResourceCollectionBase>> _resources;
	std::unordered_map<size_t, size_t> _collections;
};

}

#endif