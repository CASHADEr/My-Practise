#include <iostream>
#include <algorithm>
using namespace std;

constexpr int sz = 5 * 1e5;
struct Edge{
    int u, v;
} e[sz];

int main() {
    int N;
    cin >> N;
    while(N--) {
        int n, m;
        cin >> n >> m;
        for(int i = 0; i < m; ++i) {
            cin >> e[i].u >> e[i].v;
        }
        sort(e, e + m, [](auto u, auto v) {
            if(u.u == v.u) return u.v < v.v;
            return u.u < v.u;
        });
        for(int i = 1, k = 0; i <= n; ++i) {
            while(k < m && e[k].u == i) cout << e[k++].v << ' ';
            cout << endl;
        }
    }
}