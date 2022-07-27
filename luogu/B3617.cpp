// 16进制与8进制 3:4为一组，每组占用12bit用一个整数临时存储

#include <iostream>
#include <string>
using namespace std;

char map[16]{'0', '1', '2', '3', '4', '5', '6', '7',
             '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
int main() {
  string s, ret;
  cin >> s;
  int n = s.length();
  for (int i = n % 4, v = 0; i <= n; i += 4) {
    v = 0;
    for (int k = max(0, i - 4); k < i; ++k) {
      v <<= 3;
      v |= (s[k] - '0');
    }
    for (int k = 2; k >= 0; --k) {
      ret.push_back(map[(v >> (4 * k)) & 15]);
    }
  }
  int i = 0, len = ret.length();
  while(i < len && ret[i] == '0') ++i;
  while(i < len) cout << ret[i++];
  cout << endl;
}