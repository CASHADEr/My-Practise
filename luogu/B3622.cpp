// dfs全排列

#include <iostream>
using namespace std;

char a[13];
int n;
void dfs(int i) {
    if(i == n) {
        cout << a << endl;
        return;
    }
    a[i] = 'N';
    dfs(i + 1);
    a[i] = 'Y';
    dfs(i + 1);
}
int main() {
    cin >> n;
    a[n] = 0;
    dfs(0);
}