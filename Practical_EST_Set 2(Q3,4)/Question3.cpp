// Reoraganizing String
#include <bits/stdc++.h>
using namespace std;

string reorganizeString(string s) {
    vector<int> freq(26, 0);
    for(char c : s) freq[c - 'a']++;

    priority_queue<pair<int,char>> pq;
    for(int i = 0; i < 26; i++) {
        if(freq[i] > 0) pq.push({freq[i], char(i + 'a')});
    }

    string result = "";
    pair<int,char> prev = {0, '#'};
    
    while(!pq.empty()) {
        auto cur = pq.top();
        pq.pop();

        result += cur.second;
        cur.first--;

        if(prev.first > 0) pq.push(prev);
        prev = cur;
    }

    if(result.size() != s.size()) return "";
    return result;
}

int main() {
    string s;
    cin >> s;

    string ans = reorganizeString(s);
    cout << ans << endl;

    return 0;
}
