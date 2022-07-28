#include <bits/stdc++.h>
using namespace std;
int a[505][505];
int e[505];
int vis[505];
int con[505];

bool dfs(int u) {
    for(int i = 1; i <= e[u]; ++i) {
        int v = a[u][i];
        if(vis[v]) continue;
        vis[v] = 1;
        if(!con[v] || dfs(con[v])) {
            con[v] = u;
            return true;
        }
    }
    return false;
}
int main() {
    int n, m, ee;
    cin >> n >> m >> ee;
    for(int i = 1; i <= ee; ++i) {
        int u, v;
        cin >> u >> v;
        a[u][++e[u]] = v;
    }
    int ret = 0;
    for(int i = 1; i <= n; ++i) {
        memset(vis, 0, sizeof(vis));
        if(dfs(i)) ++ret;
    }
    cout << ret << endl;
}