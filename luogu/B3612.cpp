// 前缀和

#include <iostream>
#include <string>
using namespace std;

constexpr int sz = 1e5 + 7;
long long pre[sz];

int main() {
    int n, m;
    cin >> n;
    for(int i = 1, cur = 0; i <= n; ++i) cin >> cur, pre[i] += pre[i - 1] + cur;
    cin >> m;
    for(int i = 0; i < m; ++i) {
        int l, r;
        cin >> l >> r;
        cout << pre[r] - pre[l - 1] << endl;
    }
}