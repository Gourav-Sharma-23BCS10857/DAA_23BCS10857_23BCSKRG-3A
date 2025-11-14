// Sliding Window Maximum
#include <bits/stdc++.h>
using namespace std;

class Solution {
public:
    vector<int> maxSlidingWindow(vector<int>& nums, int k) {
        deque<int> d;
        vector<int> ans;
        for(int i = 0; i < nums.size(); i++) {
            if(!d.empty() && d.front() <= i - k) d.pop_front();
            while(!d.empty() && nums[d.back()] <= nums[i]) d.pop_back();
            d.push_back(i);
            if(i >= k - 1) ans.push_back(nums[d.front()]);
        }
        return ans;
    }
};

int main() {
    int n, k;
    cin >> n >> k;
    vector<int> nums(n);
    for(int i = 0; i < n; i++) cin >> nums[i];
    Solution obj;
    vector<int> res = obj.maxSlidingWindow(nums, k);
    for(int x : res) cout << x << " ";
}
