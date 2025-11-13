#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <emscripten/bind.h>

using namespace emscripten;

struct Edge {
	int u;
	int v;
	double w;
};

struct Stats {
	int numNodes;
	int numEdges;
	int numMSTEdges;
	double mstCost;
	std::string algorithm;
};

static inline std::pair<int, int> keyPair(int a, int b) {
	if (a <= b) return {a, b};
	return {b, a};
}

struct PairHash {
	std::size_t operator()(const std::pair<int,int>& p) const noexcept {
		return (static_cast<std::size_t>(p.first) << 32) ^ static_cast<std::size_t>(p.second);
	}
};

class DisjointSet {
public:
	std::vector<int> parent;
	std::vector<int> rankv;

	DisjointSet(int n) { reset(n); }

	void reset(int n) {
		parent.resize(n);
		rankv.assign(n, 0);
		for (int i = 0; i < n; ++i) parent[i] = i;
	}

	int find(int x) {
		if (parent[x] == x) return x;
		parent[x] = find(parent[x]);
		return parent[x];
	}

	bool unite(int a, int b) {
		int ra = find(a);
		int rb = find(b);
		if (ra == rb) return false;
		if (rankv[ra] < rankv[rb]) std::swap(ra, rb);
		parent[rb] = ra;
		if (rankv[ra] == rankv[rb]) rankv[ra]++;
		return true;
	}
};

class MSTManager {
public:
	MSTManager(int n = 0) : numNodes(n), currentAlgorithm("Kruskal"), mstCost(0.0) {
		// nothing
	}

	int addNode() {
		// Nodes are 0..numNodes-1
		return numNodes++;
	}

	void deleteNode(int nodeId) {
		if (nodeId < 0 || nodeId >= numNodes) return;
		
		// Remove all edges connected to this node
		std::vector<std::pair<int, int>> toRemove;
		for (const auto &kv : allEdges) {
			int u = kv.first.first;
			int v = kv.first.second;
			if (u == nodeId || v == nodeId) {
				toRemove.push_back({u, v});
			}
		}
		
		for (const auto &p : toRemove) {
			deleteEdge(p.first, p.second);
		}
		
		// Decrement node count (keep node IDs 0..numNodes-1)
		// Shift nodes down
		numNodes--;
		if (numNodes <= 0) {
			numNodes = 0;
			allEdges.clear();
			inMST.clear();
			mstAdj.clear();
			mstCost = 0.0;
			return;
		}
		
		// Rebuild edge map with shifted node IDs
		std::unordered_map<std::pair<int,int>, double, PairHash> newAllEdges;
		std::unordered_map<std::pair<int,int>, double, PairHash> newInMST;
		std::unordered_map<int, std::vector<std::pair<int,double>>> newMstAdj;
		
		for (const auto &kv : allEdges) {
			int u = kv.first.first;
			int v = kv.first.second;
			if (u == nodeId || v == nodeId) continue; // Skip edges with deleted node
			
			int newU = u > nodeId ? u - 1 : u;
			int newV = v > nodeId ? v - 1 : v;
			newAllEdges[keyPair(newU, newV)] = kv.second;
		}
		
		for (const auto &kv : inMST) {
			int u = kv.first.first;
			int v = kv.first.second;
			if (u == nodeId || v == nodeId) continue;
			
			int newU = u > nodeId ? u - 1 : u;
			int newV = v > nodeId ? v - 1 : v;
			double w = kv.second;
			newInMST[keyPair(newU, newV)] = w;
			
			// Update adjacency
			int adjU = newU > nodeId ? newU : newU;
			int adjV = newV > nodeId ? newV : newV;
			newMstAdj[adjU].push_back({adjV, w});
			newMstAdj[adjV].push_back({adjU, w});
		}
		
		allEdges = newAllEdges;
		inMST = newInMST;
		mstAdj = newMstAdj;
		
		// Recalculate MST cost
		mstCost = 0.0;
		for (const auto &kv : inMST) {
			mstCost += kv.second;
		}
		
		// If MST is disconnected, rebuild it
		if (static_cast<int>(inMST.size()) < std::max(0, numNodes - 1) && numNodes > 0) {
			buildInitialMST(currentAlgorithm);
		}
	}

	void reset(int n) {
		numNodes = n;
		allEdges.clear();
		inMST.clear();
		mstAdj.clear();
		mstCost = 0.0;
	}

	void setAlgorithm(std::string algo) {
		if (algo == "Kruskal" || algo == "Prim") currentAlgorithm = algo; else currentAlgorithm = "Kruskal";
	}

	std::string getAlgorithm() const { return currentAlgorithm; }

	void addEdge(int u, int v, double w) {
		if (u == v) return;
		ensureNode(u);
		ensureNode(v);
		auto k = keyPair(u, v);
		// If edge exists, delete first
		if (allEdges.count(k)) {
			deleteEdge(u, v);
		}
		allEdges[k] = w;
		// Dynamic insert logic
		if (!hasFullMST()) {
			// Try to grow MST until it has up to numNodes-1 edges when possible
			buildInitialMST(currentAlgorithm);
			return;
		}

		// If u and v are disconnected in MST, simply add edge to MST
		if (!connectedInMST(u, v)) {
			addMSTEdge(u, v, w);
			return;
		}

		// Find maximum weight edge on path u-v in current MST
		int remU = -1, remV = -1; double maxW = -1.0;
		findMaxEdgeOnPath(u, v, remU, remV, maxW);
		if (maxW >= 0.0 && w < maxW) {
			// Replace
			removeMSTEdge(remU, remV);
			addMSTEdge(u, v, w);
		}
		// else: keep edge as non-tree
	}

	void deleteEdge(int u, int v) {
		auto k = keyPair(u, v);
		if (!allEdges.count(k)) return;
		double w = allEdges[k];
		allEdges.erase(k);
		if (edgeInMST(u, v)) {
			// Removing MST edge splits MST; try to reconnect
			removeMSTEdge(u, v);
			reconnectAfterCut();
		}
		// Non-MST edge: nothing else
	}

	void buildInitialMST(const std::string &algo) {
		currentAlgorithm = (algo == "Prim" ? "Prim" : "Kruskal");
		mstAdj.clear();
		inMST.clear();
		mstCost = 0.0;
		if (numNodes == 0) return;
		if (currentAlgorithm == "Kruskal") buildMSTKruskal(); else buildMSTPrim();
	}

	std::vector<Edge> getGraphEdges() const {
		std::vector<Edge> res;
		res.reserve(allEdges.size());
		for (const auto &kv : allEdges) {
			res.push_back(Edge{kv.first.first, kv.first.second, kv.second});
		}
		return res;
	}

	std::vector<Edge> getMSTEdges() const {
		std::vector<Edge> res;
		for (const auto &kv : inMST) {
			int u = kv.first.first;
			int v = kv.first.second;
			double w = kv.second;
			res.push_back(Edge{u, v, w});
		}
		return res;
	}

	Stats getStats() const {
		Stats s;
		s.numNodes = numNodes;
		s.numEdges = static_cast<int>(allEdges.size());
		s.numMSTEdges = static_cast<int>(inMST.size());
		s.mstCost = mstCost;
		s.algorithm = currentAlgorithm;
		return s;
	}

private:
	int numNodes;
	std::string currentAlgorithm;
	double mstCost;
	// All edges: key -> weight
	std::unordered_map<std::pair<int,int>, double, PairHash> allEdges;
	// MST edges
	std::unordered_map<std::pair<int,int>, double, PairHash> inMST;
	// MST adjacency list: node -> vector of (neighbor, weight)
	std::unordered_map<int, std::vector<std::pair<int,double>>> mstAdj;

	void ensureNode(int node) {
		if (node >= numNodes) numNodes = node + 1;
	}

	bool hasFullMST() const {
		// If there are k connected components c, MST edges are n-c; we approximate: if MST has n-1 edges, call it full.
		if (numNodes == 0) return true;
		return static_cast<int>(inMST.size()) == std::max(0, numNodes - 1);
	}

	bool edgeInMST(int u, int v) const {
		return inMST.count(keyPair(u, v)) > 0;
	}

	void addMSTEdge(int u, int v, double w) {
		inMST[keyPair(u, v)] = w;
		mstAdj[u].push_back({v, w});
		mstAdj[v].push_back({u, w});
		mstCost += w;
	}

	void removeMSTEdge(int u, int v) {
		auto kp = keyPair(u, v);
		auto it = inMST.find(kp);
		if (it == inMST.end()) return;
		double w = it->second;
		inMST.erase(it);
		// remove from adjacency lists
		auto &au = mstAdj[u];
		usRemove(au, v, w);
		auto &av = mstAdj[v];
		usRemove(av, u, w);
		mstCost -= w;
	}

	static void usRemove(std::vector<std::pair<int,double>>& vec, int nb, double w) {
		for (size_t i = 0; i < vec.size(); ++i) {
			if (vec[i].first == nb && std::fabs(vec[i].second - w) < 1e-9) {
				vec.erase(vec.begin() + static_cast<long>(i));
				break;
			}
		}
	}

	bool connectedInMST(int s, int t) const {
		if (s == t) return true;
		std::unordered_set<int> vis;
		std::queue<int> q;
		q.push(s); vis.insert(s);
		while (!q.empty()) {
			int u = q.front(); q.pop();
			auto it = mstAdj.find(u);
			if (it == mstAdj.end()) continue;
			for (const auto &p : it->second) {
				int v = p.first;
				if (!vis.count(v)) {
					if (v == t) return true;
					vis.insert(v);
					q.push(v);
				}
			}
		}
		return false;
	}

	void findMaxEdgeOnPath(int s, int t, int &outU, int &outV, double &outW) const {
		outU = -1; outV = -1; outW = -1.0;
		std::unordered_map<int, std::pair<int,double>> parent; // node -> (parent, weight)
		std::queue<int> q;
		std::unordered_set<int> vis;
		q.push(s); vis.insert(s);
		bool found = false;
		while (!q.empty() && !found) {
			int u = q.front(); q.pop();
			auto it = mstAdj.find(u);
			if (it == mstAdj.end()) continue;
			for (const auto &p : it->second) {
				int v = p.first; double w = p.second;
				if (vis.count(v)) continue;
				parent[v] = {u, w};
				if (v == t) { found = true; break; }
				vis.insert(v);
				q.push(v);
			}
		}
		if (!found) return;
		// Reconstruct path and find max
		double maxW = -1.0; int mu = -1, mv = -1;
		int cur = t;
		while (cur != s) {
			auto pr = parent[cur];
			int p = pr.first; double w = pr.second;
			if (w > maxW) { maxW = w; mu = cur; mv = p; }
			cur = p;
		}
		outU = mu; outV = mv; outW = maxW;
	}

	void reconnectAfterCut() {
		// Identify components in current MST using BFS from any node
		std::vector<int> comp(numNodes, -1);
		int cid = 0;
		for (int i = 0; i < numNodes; ++i) {
			if (comp[i] != -1) continue;
			// BFS
			std::queue<int> q; q.push(i); comp[i] = cid;
			while (!q.empty()) {
				int u = q.front(); q.pop();
				auto it = mstAdj.find(u);
				if (it == mstAdj.end()) continue;
				for (const auto &p : it->second) {
					int v = p.first;
					if (comp[v] == -1) { comp[v] = cid; q.push(v); }
				}
			}
			cid++;
		}
		if (cid <= 1) return; // Already connected or empty

		// Find min weight non-MST edge connecting different components
		double bestW = INFINITY; int bu=-1, bv=-1;
		for (const auto &kv : allEdges) {
			int u = kv.first.first, v = kv.first.second; double w = kv.second;
			if (edgeInMST(u, v)) continue;
			if (u < numNodes && v < numNodes && comp[u] != -1 && comp[v] != -1 && comp[u] != comp[v]) {
				if (w < bestW) { bestW = w; bu = u; bv = v; }
			}
		}
		if (bu != -1) {
			addMSTEdge(bu, bv, bestW);
			// Might still have more than 2 components if graph is sparse; try to connect iteratively
			reconnectAfterCut();
		}
	}

	void buildMSTKruskal() {
		std::vector<std::tuple<double,int,int>> edges;
		edges.reserve(allEdges.size());
		for (const auto &kv : allEdges) {
			edges.emplace_back(kv.second, kv.first.first, kv.first.second);
		}
		std::sort(edges.begin(), edges.end());
		DisjointSet ds(std::max(1, numNodes));
		mstCost = 0.0;
		for (const auto &e : edges) {
			double w; int u, v; std::tie(w, u, v) = e;
			if (ds.unite(u, v)) {
				addMSTEdge(u, v, w);
				if (static_cast<int>(inMST.size()) == std::max(0, numNodes - 1)) break;
			}
		}
	}

	void buildMSTPrim() {
		// Build adjacency for all edges for Prim
		std::vector<std::vector<std::pair<int,double>>> adj(numNodes);
		for (const auto &kv : allEdges) {
			int u = kv.first.first, v = kv.first.second; double w = kv.second;
			if (u >= numNodes || v >= numNodes) continue;
			adj[u].push_back({v, w});
			adj[v].push_back({u, w});
		}
		std::vector<char> used(numNodes, 0);
		struct Item { double w; int u; int p; };
		auto cmp = [](const Item &a, const Item &b){ return a.w > b.w; };
		std::priority_queue<Item, std::vector<Item>, decltype(cmp)> pq(cmp);
		// Start from node 0 if exists
		if (numNodes == 0) return;
		used[0] = 1;
		for (auto &p : adj[0]) pq.push({p.second, p.first, 0});
		mstCost = 0.0;
		while (!pq.empty() && static_cast<int>(inMST.size()) < std::max(0, numNodes - 1)) {
			Item it = pq.top(); pq.pop();
			if (used[it.u]) continue;
			used[it.u] = 1;
			addMSTEdge(it.u, it.p, it.w);
			for (auto &p : adj[it.u]) if (!used[p.first]) pq.push({p.second, p.first, it.u});
		}
	}
};

EMSCRIPTEN_BINDINGS(GraphMSTModule) {
	value_object<Edge>("Edge")
		.field("u", &Edge::u)
		.field("v", &Edge::v)
		.field("w", &Edge::w);

	value_object<Stats>("Stats")
		.field("numNodes", &Stats::numNodes)
		.field("numEdges", &Stats::numEdges)
		.field("numMSTEdges", &Stats::numMSTEdges)
		.field("mstCost", &Stats::mstCost)
		.field("algorithm", &Stats::algorithm);

	register_vector<Edge>("VectorEdge");

	class_<MSTManager>("MSTManager")
		.constructor<int>()
		.function("addNode", &MSTManager::addNode)
		.function("deleteNode", &MSTManager::deleteNode)
		.function("reset", &MSTManager::reset)
		.function("setAlgorithm", &MSTManager::setAlgorithm)
		.function("getAlgorithm", &MSTManager::getAlgorithm)
		.function("addEdge", &MSTManager::addEdge)
		.function("deleteEdge", &MSTManager::deleteEdge)
		.function("buildInitialMST", &MSTManager::buildInitialMST)
		.function("getGraphEdges", &MSTManager::getGraphEdges)
		.function("getMSTEdges", &MSTManager::getMSTEdges)
		.function("getStats", &MSTManager::getStats);
}


