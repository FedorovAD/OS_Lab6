#include <iostream>
#include <vector>
#include "tree.h"
using namespace std;

tree_el::tree_el(int new_val) : value(new_val), left(nullptr), right(nullptr){}

int& tree_el::get_value(){
	return value;
}

tree_el*& tree_el::get_left(){
	return left;
}

tree_el*& tree_el::get_right(){
	return right;
}

void tree::delete_tree(tree_el*& cur){
	if(!cur) return;
	delete_tree(cur->get_left());
	delete_tree(cur->get_right());
	delete cur;
	cur = nullptr;
}

void tree::print_tree(tree_el*& cur, int h){
	if(!cur) return;
	for(int i = 0; i < h; i++){
		cout << " ";
	}
	cout << cur->get_value() << endl;
	print_tree(cur->get_left(), h + 1);
	print_tree(cur->get_right(), h);
}

bool tree::find_el(tree_el* cur, int val, tree_el*& found, string cur_string, string& etalon){
	if(cur == nullptr){
		return false;
	}
	if(cur->get_value() == val){
		found = cur;
		etalon = cur_string;
		return true;
	}
	return find_el(cur->get_left(), val, found, cur_string + 'c', etalon) || find_el(cur->get_right(), val, found, cur_string + 'b', etalon);
}

find_res tree::find(int val){
	tree_el* tmp;
	string res;
	return {find_el(root, val, tmp, "", res), res};
}

bool tree::empty(){
	return root == nullptr;
}

int tree::insert_el(tree_el*& cur, int new_val, int parent_val){
	if(empty()){
		root = new tree_el(new_val);
		return -1;
	}
	tree_el* tmp;
	string res;
	if(!find_el(root, parent_val, tmp, string(), res)){
		return -1;
	}
	tree_el** child = &tmp->get_left();
	int real_parent = tmp->get_value();
	while(*child != nullptr){
		real_parent = (*child)->get_value();
		cout << '!' << (*child)->get_value() << '\n';
		child = &(*child)->get_right();
	}
	*child = new tree_el(new_val);
	return real_parent;
}

tree::tree():root(nullptr){}

tree::tree(int new_val){
	root = new tree_el(new_val);
}

int tree::insert(int new_val, int parent_val){
	return insert_el(root, new_val, parent_val);
}

void tree::print(){
	print_tree(root);
}

void tree::delete_el_tree(tree_el*& cur, int val, int parent_val, int& parent_del){
	if(!cur) return;
	if(cur->get_value() == val){
		parent_del = parent_val;
		delete_tree(cur);
	} else{
		delete_el_tree(cur->get_left(), val, -(cur->get_value()), parent_del);
		delete_el_tree(cur->get_right(), val, cur->get_value(), parent_del);
	}
}

int tree::delete_el(int val){
	tree_el* tmp;
	string res;
	if(!find_el(root, val, tmp, string(), res)){
		return -255;
	}
	int parent_del;
	delete_el_tree(root, val, -1, parent_del);
	return parent_del;
}

tree::~tree() {
	delete_tree(root);
}