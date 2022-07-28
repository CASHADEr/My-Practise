#include <bits/stdc++.h>
using namespace std;

// floyd
// int a[110][110];
// int n;
// int main() {
//     cin >> n;
//     for(int i = 0; i < n; ++i) {
//         for(int j = 0; j < n; ++j) {
//             cin >> a[i][j];
//         }
//     }
//     for(int k = 0; k < n; ++k) {
//         for(int i = 0; i < n; ++i) {
//             for(int j = 0; j < n; ++j) {
//                 a[i][j] |= a[i][k] & a[k][j];
//             }
//         }
//     }
//     for(int i = 0; i < n; ++i) {
//         for(int j = 0; j < n; ++j) {
//             cout << a[i][j] << ' ';
//         }
//         cout << endl;
//     }
// }

// bit优化
bitset<110> a[110];
int n;
int main() {
    cin >> n;
    for(int i = 0; i < n; ++i) {
        for(int j = 0, v; j < n; ++j) {
            cin >> v;
            a[i][j] = v;
        }
    }
    for(int k = 0; k < n; ++k) {
        for(int i = 0; i < n; ++i) {
            if(a[i][k]) a[i] |= a[k];
        }
    }
    for(int i = 0; i < n; ++i) {
        for(int j = 0; j < n; ++j) {
            cout << a[i][j] << ' ';
        }
        cout << endl;
    }
}