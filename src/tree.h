#ifndef _TREE_H
#define _TREE_H

#include <vector>
#include <string>
using namespace std;

struct find_res{
	bool found;
	string path;
};

class tree_el{
	tree_el* left; //eldest son
	tree_el* right; //younger brother
	int value;
public:
	tree_el(int new_val);
	int& get_value();
	tree_el*& get_left();
	tree_el*& get_right();
};

class tree {
private:
	tree_el* root;
	void delete_tree(tree_el*& cur);
	static void print_tree(tree_el*& cur, int h = 0);
	bool find_el(tree_el* cur, int val, tree_el*& found, string cur_string, string& etalon);
	int insert_el(tree_el*& cur, int new_val, int parent_val);
	void delete_el_tree(tree_el*& cur, int val, int parent_val, int& parent_del);
	bool empty();
public:
	tree();
	tree(int new_val);
	int insert(int new_val, int parent_val = -1);
	find_res find(int val);
	void print();
	int delete_el(int val);
	~tree();
};

#endif