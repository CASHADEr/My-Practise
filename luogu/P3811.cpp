#include <bits/stdc++.h>
#define rep(s, e, k) for (int i = s; i < e; i += k)
using namespace std;

// exgc
pair<int, int> exgc(int a, int b) {
  if (!b) return {1, 0};
  auto [x, y] = exgc(b, a % b);
  return {y, x - a / b * y};
}

// 费马小定理
int fastpower(long a, int p, int mod) {
  a %= mod;
  long ret = 1;
  for (; p; p >>= 1, a = a * a % mod)
    if (p & 1) (ret *= a) %= mod;
  return ret;
}

// linear inv
constexpr int sz = 1e7;
long inv[sz];
void linear(int n, int p) {
  inv[1] = 1;
  cout << 1 << endl;
  rep(2, n + 1, 1) { cout << (inv[i] = (p - p / i) * inv[p % i] % p) << endl; }
}

int main() {
  std::ios::sync_with_stdio(false);
  int n, p;
  cin >> n >> p;
  linear(n, p);
}
