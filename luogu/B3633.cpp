#include <bits/stdc++.h>
using namespace std;
ostream &coutEle(long long value) {
    for(int i = 0; i < 64; ++i) {
        if((1LL << i) & value) cout << i << ' ';
    }
    return cout;
}
int main() {
    int cntA, cntB;
    long long a = 0, b = 0;
    cin >> cntA;
    int v;
    for(int i = 0; i < cntA; ++i) {
        cin >> v;
        a |= 1LL << v;
    }
    cin >> cntB;
    for(int i = 0; i < cntB; ++i) {
        cin >> v;
        b |= 1LL << v;
    }
    // coutEle(a) << endl;
    // coutEle(b) << endl;
    cout << cntA << endl;
    coutEle(a & b) << endl;
    coutEle(a | b) << endl;
    coutEle(~a) << endl;
    cout << (a == b) << endl << ((a & b) == a) << endl << (a & 1) << endl;
}