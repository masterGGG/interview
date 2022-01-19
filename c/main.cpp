#include <iostream>
#include <vector>
#include <stack>

 struct TreeNode {
     int val;
     TreeNode *left;
     TreeNode *right;
     TreeNode() : val(0), left(nullptr), right(nullptr) {}
     TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
     TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
 };

class TreeIterator {
public:
  std::vector<int> preorder(TreeNode* root) {
    std::vector<int> res;
    std::stack<TreeNode *> stk;
    if (root) {
      stk.push(root);
    }

    while (!stk.empty()) {
      root = stk.top();
      stk.pop();

      if (root->right) {
        stk.push(root->right);
      }

      if (root->left) {
        stk.push(root->left);
      }

      res.emplace_back(root->val);
    }

    return res;
  }

  std::vector<int> inorder(TreeNode* root) {
    std::vector<int> res;
    std::stack<TreeNode *> stk;

    while (root || !stk.empty()) {      
      while (root) {
        stk.push(root);
        root = root->left;
      }

      root = stk.top();
      stk.pop();
      res.emplace_back(root->val);
      root = root->right;
    }

    return res;    
  }

  std::vector<int> posorder(TreeNode* root) {
    std::vector<int> res;
    std::stack<TreeNode *> stk;
    std::stack<int> rstack;

    while (root || !stk.empty()) {
      //
    }
  }
};

