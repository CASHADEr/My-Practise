#include <bits/stdc++.h>
#define rep(s, e, k) for(int i = s; i < e; i += k)
using namespace std;

constexpr int sz = 1e6 + 7;
constexpr int p = 1145141;
long inv[p], num[sz];
int main() {
    int n, q;
    scanf("%d %d", &n, &q);
    for(int i = 1; i <= n; ++i) scanf("%ld", num + i);
    num[0] = inv[1] = 1;
    for(int i = 2; i < p; ++i) inv[i] = (p - p / i) * inv[p % i] % p;
    long ret = 0;
    for(int i = 1; i <= n; ++i) {
        num[i] = (num[i] * num[i - 1]) % p;
    }
    for(int i = 1; i <= q; ++i) {
        int l, r;
        scanf("%d %d", &l, &r);
        ret ^= (num[r] * inv[num[l - 1]]) % p;
    }
    printf("%ld\n", ret);
}