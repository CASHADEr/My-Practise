// dijkstra 堆优化

#include <iostream>
#include <string.h>
#include <queue>
using namespace std;

constexpr int N = 3 * 1e5 + 7;
long long dis[N], ret[N];

int main() {
    int n, m;
    cin >> n >> m;
    vector<vector<pair<int, int>>> graph(n + 1);
    for(int i = 0; i < m; ++i) {
        int u, v, w;
        cin >> u >> v >> w;
        graph[u].emplace_back(v, w);
    }
    memset(dis, 0x3f, sizeof(dis));
    fill(ret + 1, ret + n + 1, -1);
    dis[1] = 0;
    using pii = pair<long, int>;
    priority_queue<pii, vector<pii>, std::greater<pii>> pq;
    pq.emplace(0, 1);
    while(!pq.empty()) {
        auto [ndis, node] = pq.top();
        pq.pop();
        if(ret[node] != -1) continue;
        ret[node] = ndis;
        for(auto [child, cdis]: graph[node]) {
            if(ret[child] != -1) continue;
            long long newdis = ndis + cdis;
            if(newdis < dis[child]) {
                dis[child] = newdis;
                pq.emplace(newdis, child);
            }
        }
    }
    for(int i = 1; i <= n; ++i) cout << ret[i] << ' ';
    cout << endl;
}