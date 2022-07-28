// 二分图匹配
#include <bits/stdc++.h>
using namespace std;
int a[505][505];
int e[505];
int vis[505];
bool p[505];
int l, r, m;
bool dfs(int u) {
  for (int i = 1; i <= e[u]; ++i) {
    int v = a[u][i];
    if(p[v]) continue;
    p[v] = true;
    if (!vis[v] || dfs(vis[v])) {
      vis[v] = u;
      return true;
    }
  }
  return false;
}
int main() {
  cin >> l >> r >> m;
  for (int i = 1; i <= m; ++i) {
    int ll, rr;
    cin >> ll >> rr;
    a[ll][++e[ll]] = rr;
  }
  int ret = 0;
  for(int i = 1; i <= l; ++i) {
    memset(p, 0, sizeof(p));
    if(dfs(i)) ++ret;
  }
  std::cout << ret << endl;
}