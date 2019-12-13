#ifndef CON_HASH_
#define CON_HASH_
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <map>
#include <list>

//字符串哈希函数
inline uint32_t __FNVHash(const std::string &str) {
	const uint32_t fnv_prime = 0x811C9DC5;
	uint32_t hash = 0;
	for (std::size_t i = 0; i < str.length(); i++) {
		hash *= fnv_prime;
		hash ^= str[i];
	}

	return hash;
}

typedef uint32_t (*HashFunction)(const std::string &str);

struct ServerEntry {
	std::string pstrIP;
	int wdPort;
	std::string strIdentify;
	int dwInuse;
	ServerEntry(const std::string &identify, const std::string &IP, int Port,int Inuse = 0) :
			pstrIP(IP), wdPort(Port), strIdentify(identify),dwInuse(Inuse) {
	}
	std::string getAddrString() const {
		return strIdentify;//pstrIP + ":" + std::to_string(wdPort) + "#" + std::to_string(wdServerID);
	}
};

struct ServerInfo {
	std::string pstrIP;
	int wdPort;
	std::string strIdentify;
	ServerInfo(const std::string &identify, const std::string &IP, int Port) :
		pstrIP(IP), wdPort(Port),strIdentify(identify){
	}
	std::string getAddrString() const {//传输地址
		return pstrIP + ":" + std::to_string(wdPort);
	}
	std::string getIdentify() const {
		return strIdentify;
	}
};

struct ConNode {//实节点
	static const int kVirtualNodeDefault = 80;
	ConNode(const std::string &iden, uint32_t count, void *data) :
			node_iden(iden), virtual_node_count(count), nodedata(data) {
	}
	int getVNodeCount() const {
		return ConNode::kVirtualNodeDefault;
	}
	const std::string& getNodeIden() const {//传输地址
		return node_iden;
	}
	void* getData() {
		return nodedata;
	}
	std::string node_iden;
	uint32_t virtual_node_count;
	void *nodedata;
};
struct ConVirtualNode {//虚节点
	ConVirtualNode(const std::string &iden, uint32_t count, void *data) :
			node_iden(iden), virtual_node_count(count), nodedata(data) {
	}
	ConNode* getNode() const {
		return (ConNode*) nodedata;
	}
	uint32_t getHash() const {
		return __FNVHash(node_iden);
	}
	std::string node_iden;
	uint32_t virtual_node_count;
	void *nodedata;
};

class ConHash {
public:
	HashFunction hash_func_;
	typedef std::map<uint32_t, ConVirtualNode*> VNodeMap;
	typedef VNodeMap::const_iterator VNodeMap_CIT;
	typedef VNodeMap::iterator VNodeMap_IT;
	typedef VNodeMap::value_type VNodeMap_VT;

	typedef std::list<ConNode*> NodeList;
	typedef NodeList::const_iterator NodeList_CIT;
	typedef NodeList::iterator NodeList_IT;

	VNodeMap m_vnode_map;
	NodeList m_node_list;

	//一致性哈希列表
	ConHash() : hash_func_(__FNVHash) {
	}
	//删除服务器实节点和虚节点
	~ConHash() {
		VNodeMap_IT it = m_vnode_map.begin();
		for (; it != m_vnode_map.end(); ++it) {
			delete it->second;
		}
		m_vnode_map.clear();

		NodeList_IT nodeit = m_node_list.begin();
		for (; nodeit != m_node_list.end(); ++nodeit) {
			delete (*nodeit);
		}
		m_node_list.clear();
	}

	//设置哈希函数
	void setHashFunc(HashFunction func) {
		hash_func_ = func;
	}

	//添加节点（传输地址、虚节点数量、实节点）
	int addNode(const std::string &node_iden, uint32_t virtual_node_count,void *nodedata) {
		//    debug("%s(iden=%s, vcount=%u)", __FUNCTION__, node_iden.c_str(), virtual_node_count);
		if (node_iden.size() == 0 || virtual_node_count == 0) return -1;
		ConNode *pnode = new ConNode(node_iden, virtual_node_count, nodedata);
		if (pnode == NULL) return -1;
		m_node_list.push_back(pnode);
		for (uint32_t i = 0; i < pnode->getVNodeCount(); i++) {
			char vstr[16] = { 0 };
			snprintf(vstr, sizeof(vstr) - 1, "#%u", i);
			std::string vnodestr = pnode->getNodeIden() + vstr;
			uint32_t vhash = hash_func_(vnodestr); //哈希服务器虚节点
			ConVirtualNode *vnode = new ConVirtualNode(node_iden, vhash,pnode);
			if (vnode == NULL)return -1;
			m_vnode_map.insert(VNodeMap_VT(vhash, vnode));
		}
		return 0;
	}

	//移除节点
	int removeNode(const std::string &strIpPort) {
		NodeList_IT nodeit = m_node_list.begin();
		for (; nodeit != m_node_list.end(); ++nodeit) {
			if ((*nodeit)->getNodeIden() == strIpPort) //移除实节点
				break;
		}
		if (nodeit == m_node_list.end()) return 0;
		ConNode *pnode = *nodeit;
		VNodeMap_IT it = m_vnode_map.begin();
		while (it != m_vnode_map.end()) {//移除虚节点
			if (it->second->getNode() == pnode){
				delete (it->second);
				m_vnode_map.erase(it++);
			} else {
				++it;
			}
		}
		delete pnode;
		m_node_list.erase(nodeit);
		return 1;
	}

	//移除所有实节点和虚节点
	void clear() {
		VNodeMap_IT it = m_vnode_map.begin();
		for (; it != m_vnode_map.end(); ++it) {
			delete it->second;
		}
		m_vnode_map.clear();

		NodeList_IT nodeit = m_node_list.begin();
		for (; nodeit != m_node_list.end(); ++nodeit) {
			delete (*nodeit);
		}
		m_node_list.clear();
	}

//根据缓存内容（频道ID）获取最相近的服务器哈希值
	void* lookupNode(const std::string &strModFactor) const {
		if (m_vnode_map.empty()) return NULL;
		uint32_t uiModFactorhash = hash_func_(strModFactor);
		VNodeMap_CIT cit = m_vnode_map.lower_bound(uiModFactorhash);
		if (cit == m_vnode_map.end()) cit = m_vnode_map.begin();
		//    debug("%s(%s)-->hash=%u->%s", __FUNCTION__,object.c_str(), objecthash, cit->second->getNode()->getNodeIden().c_str());
		return cit->second->getNode()->getData();
	}

	//输出所有实节点和虚节点的列表
	void dump() const {
		NodeList_CIT nodecit = m_node_list.begin();
		for (; nodecit != m_node_list.end(); ++nodecit) //遍历实节点列表
				{
			ConNode *pnode = *nodecit;
			std::ostringstream vnodess;

			VNodeMap_CIT cit = m_vnode_map.begin();//虚节点列表
			for (; cit != m_vnode_map.end(); ++cit) {
				if (cit->second->getNode() == pnode) {
					vnodess << cit->second->getHash() << " ";
				}
			}
			//        debug("[node]%s--[vnodecount=%u]:%s", pnode->getNodeIden().c_str(), pnode->getVNodeCount(),vnodess.str().c_str());
		}
	}
};
class ChannelConHash {
public:
	typedef std::unordered_map<std::string, ServerEntry> ServerInfoMap;
	typedef ServerInfoMap::const_iterator ServerInfoMap_CIT;
	typedef ServerInfoMap::iterator ServerInfoMap_IT;
	typedef ServerInfoMap::value_type ServerInfoMap_VT;
	ServerInfoMap m_server_info_map;
	ConHash m_ConHashProxy;
	HashFunction hash_func_;
	ChannelConHash() :
			hash_func_(__FNVHash) {
	}
	~ChannelConHash() {
		m_server_info_map.clear();
	}
	//频道哈希列表
	void setHashFunc(HashFunction func) {
		hash_func_ = func;
	}
	//加入新服务器节点到哈希列表
	int addNode(const ServerEntry &entry) {
		if (entry.dwInuse == 1) return -1;
		// 服务器信息
		ServerInfo *pnewinfo = new ServerInfo(entry.strIdentify, entry.pstrIP,entry.wdPort);
		if (pnewinfo == NULL) return -1;
		//加入服务器节点到列表 pnewinfo
		m_server_info_map.insert(ServerInfoMap_VT(entry.strIdentify, entry));
		return m_ConHashProxy.addNode(pnewinfo->getAddrString(),ConNode::kVirtualNodeDefault, pnewinfo);
	}

	//移除服务器对应的节点
	int removeNode(const std::string &strIdentify) {
		ServerInfoMap_IT it = m_server_info_map.find(strIdentify);
		if (it == m_server_info_map.end()) return 0;
		std::string strIpPort = it->second.getAddrString();
		int status = m_ConHashProxy.removeNode(strIpPort);
		if (status == 0) return status;
		m_server_info_map.erase(it);
		return 0;
	}

	//删除服务器列表信息
	void clear() {
		m_ConHashProxy.clear();
		m_server_info_map.clear();
	}
	//获取strIdentify
	std::string lookupNodeIdentify(uint32_t uiModFactor) const {
		if (uiModFactor == 0) return "";
		void *pnodedata = m_ConHashProxy.lookupNode(std::to_string(uiModFactor));
		if (pnodedata == NULL) return "";
		ServerInfo *pinfo = static_cast<ServerInfo*>(pnodedata);
		return pinfo->getIdentify();
	}
	//获取strIdentify列表
	uint32_t getIdentifyList(std::list<std::string> &idlist) const {
		ServerInfoMap_CIT cit = m_server_info_map.begin();
		for (; cit != m_server_info_map.end(); ++cit) {
			idlist.push_back(cit->first);
		}
		return idlist.size();
	}
};

#endif//ifndef
