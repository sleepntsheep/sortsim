#include <bits/stdc++.h>
using namespace std;

int n, w;
vector<int> a;
int64_t r;
deque<int> q;
int64_t mn = 0, mw = 1e9;

int main() {
    cin.tie(nullptr);
    ios::sync_with_stdio(false);
    cin >> n >> w;
    a.resize(n + 5);
    fill(a.begin(), a.end(), 0);
    for (int i = 0; i < n; i++)
        cin >> a[i];

    for (int i = 0; i < n; i++) {
        q.push_back(i);
        r += a[i];
        if (r <= 0) {
            while (q.size())
                q.pop_front();
            r = 0;
        }

        while (q.size() && (q.front() <= i - w || a[q.front()] <= 0)) {
            r -= a[q.front()];
            q.pop_front();
        }
        if (r >= mn) {
            mn = r;
            if (q.size() < mw) {
                mw = q.size();
            }
        }
    }
    cout << mn << '\n' << mw;
}
