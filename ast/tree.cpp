#include "tree.hpp"
#include "ast.hpp"

#include <iostream>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <type_traits>
#include <string>
#include <map>
#include <tuple>
#include <vector>
#include <utility>
#include <fstream>

const int VAR_STEP = 4;
int var_counter = 4;

Result* traverse_tree(ASTNode* ptr, std::map<std::string, int>& mp, std::map<std::string, std::pair<int, ArrayType>>& arrs, 
    int* loop_counter, int* if_counter, int* cond_counter, int* main_counter, int* arrayDecl_loop) {
    auto* num_node = dynamic_cast<NumNode*>(ptr);
    auto* var_node = dynamic_cast<VarNode*>(ptr);
    auto* arrElem_node = dynamic_cast<ArrayElemNode*>(ptr);
    auto* bin_op_node = dynamic_cast<BinaryNode*>(ptr);
    auto* main_node = dynamic_cast<MainNode*>(ptr);
    auto* assign_node = dynamic_cast<AssignNode*>(ptr);
    auto* print_node = dynamic_cast<PrintNode*>(ptr);
    auto* scan_node = dynamic_cast<ScanNode*>(ptr);
    auto* statArrDecl_node = dynamic_cast<StatArrayDeclNode*>(ptr);
    auto* dynArrDecl_node = dynamic_cast<DynArrayDeclNode*>(ptr);
    auto* arrElemAssign_node = dynamic_cast<ArrayElemAssignNode*>(ptr);
    auto* if_else_node = dynamic_cast<IfElseNode*>(ptr);
    auto* while_node = dynamic_cast<WhileNode*>(ptr);
    
    if (num_node) { return new Result(ErrType::_OK_, -1, "") ; }
    else if (var_node) { 
        if (mp.find(var_node->var_name) == mp.end()) {
            return new Result(ErrType::_ERR_VAR_, var_node->line_index, "Variable '" + var_node->var_name + "' not defined!");
        }
    }
    else if (arrElem_node) {
        if (arrs.find(arrElem_node->arr_name) == arrs.end()) {
            std::string arr = arrElem_node->arr_name.substr(1, arrElem_node->arr_name.length()-2);
            return new Result(ErrType::_ERR_ARR_, arrElem_node->line_index, "Unknown array '" + arr + "'!");
        }
        else {
            Result* index = traverse_tree(arrElem_node->elem_index, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        
            if (index->err == ErrType::_ERR_VAR_) return index;
        }
    }
    else if (bin_op_node) {
        Result* lhs = traverse_tree(bin_op_node->left, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        Result* rhs = traverse_tree(bin_op_node->right, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        
        if (lhs->err == ErrType::_ERR_VAR_ || lhs->err == ErrType::_ERR_ARR_) return lhs;
        else if (rhs->err == ErrType::_ERR_VAR_ || rhs->err == ErrType::_ERR_ARR_) return rhs;
    }
    else if (main_node) {
        Result* res = traverse_tree(main_node->next, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        
        if (res->err == ErrType::_ERR_VAR_ || res->err == ErrType::_ERR_ARR_) return res;
    }
    else if (assign_node) {
        Result* val = traverse_tree(assign_node->assign_val, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        
        auto it = mp.find(assign_node->var_name);
        if (it == mp.end()) {
            mp[assign_node->var_name] = var_counter;
            var_counter += VAR_STEP;
        }
        
        Result* res = traverse_tree(assign_node->next, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        
        if (val->err == ErrType::_ERR_VAR_ || val->err == ErrType::_ERR_ARR_) return val;
        else if (res->err == ErrType::_ERR_VAR_ || res->err == ErrType::_ERR_ARR_) return res;
    }
    else if (arrElemAssign_node) {
        if (arrs.find(arrElemAssign_node->arr_name) == arrs.end()) {
            std::string arr = arrElemAssign_node->arr_name.substr(1, arrElemAssign_node->arr_name.length()-2);
            return new Result(ErrType::_ERR_ARR_, arrElemAssign_node->line_index, "Unknown array '" + arr + "'!");
        }
        else {
            Result* index = traverse_tree(arrElemAssign_node->elem_index, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
            Result* val = traverse_tree(arrElemAssign_node->assign_val, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
            Result* res = traverse_tree(arrElemAssign_node->next, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
            
            if (index->err == ErrType::_ERR_VAR_) return index;
            else if (val->err == ErrType::_ERR_VAR_ || val->err == ErrType::_ERR_ARR_) return val;
            else if (res->err == ErrType::_ERR_VAR_) return res;
        }
    }
    else if (print_node) {
        Result* val = traverse_tree(print_node->print_val, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        Result* res = traverse_tree(print_node->next, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        
        if (val->err == ErrType::_ERR_VAR_ || val->err == ErrType::_ERR_ARR_) return val;
        else if (res->err == ErrType::_ERR_VAR_ || res->err == ErrType::_ERR_ARR_) return res;
    }
    else if (scan_node) {
        if (mp.find(scan_node->var_name) == mp.end())
            return new Result(ErrType::_ERR_VAR_, scan_node->line_index, "Variable '" + scan_node->var_name + "' not defined!");
        
        Result* res = traverse_tree(scan_node->next, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        
        if (res->err == ErrType::_ERR_VAR_ || res->err == ErrType::_ERR_ARR_) return res;
    }
    else if (statArrDecl_node) {
        auto it = arrs.find(statArrDecl_node->arr_name);
        if (it == arrs.end()) { arrs[statArrDecl_node->arr_name] = {var_counter, _STAT_}; }
        
        for (int i = 0; i < statArrDecl_node->arr_size; ++i) {
            Result* val = traverse_tree(statArrDecl_node->arr_vals[i], mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
            
            if (val->err == ErrType::_ERR_VAR_ || val->err == ErrType::_ERR_ARR_) return val;
            
            std::string elem_name = statArrDecl_node->arr_name + std::to_string(i);
            
            mp[elem_name] = var_counter;
            var_counter += VAR_STEP;
        }
        
        Result* res = traverse_tree(statArrDecl_node->next, mp, arrs, loop_counter, if_counter, cond_counter,
        main_counter, arrayDecl_loop);
        
        if (res->err == ErrType::_ERR_VAR_ || res->err == ErrType::_ERR_ARR_) return res;
    }
    else if (dynArrDecl_node) {
        dynArrDecl_node->arrayDecl_loop = *arrayDecl_loop;
        *arrayDecl_loop += 1;
        
        auto it = arrs.find(dynArrDecl_node->arr_name);
        if (it == arrs.end()) { arrs[dynArrDecl_node->arr_name] = {var_counter, _DYN_};}
        var_counter += VAR_STEP;
        
        Result* size = traverse_tree(dynArrDecl_node->arr_size, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        Result* val = traverse_tree(dynArrDecl_node->arr_val, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        Result* res = traverse_tree(dynArrDecl_node->next, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        
        if (size->err == ErrType::_ERR_VAR_) return size;
        else if (val->err == ErrType::_ERR_VAR_ || val->err == ErrType::_ERR_ARR_) return val;
        else if (res->err == ErrType::_ERR_VAR_ || res->err == ErrType::_ERR_ARR_) return res;
    }
    else if (if_else_node) {
        int n = if_else_node->conds.size();
        
        if (n == 1) {
            if_else_node->if_num = *if_counter;
            if_else_node->main_num = *main_counter;
            if_else_node->cond_num.push_back(*cond_counter);
            
            *if_counter += 1;
            *main_counter += 1;
            *cond_counter += 1;
            
            Result* cond = traverse_tree(if_else_node->conds[0].first, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
            Result* stmts = traverse_tree(if_else_node->conds[0].second, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
            Result* res = traverse_tree(if_else_node->next, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
            
            if (cond->err == ErrType::_ERR_VAR_ || cond->err == ErrType::_ERR_ARR_) return cond;
            else if (stmts->err == ErrType::_ERR_VAR_ || stmts->err == ErrType::_ERR_ARR_) return stmts;
            else if (res->err == ErrType::_ERR_VAR_ || res->err == ErrType::_ERR_ARR_) return res;
        }
        else {
            if_else_node->if_num = *if_counter;
            if_else_node->main_num = *main_counter;
            for (int i = 1; i < n; ++i) {
                if_else_node->cond_num.push_back(*cond_counter + i - 1);
                Result* cond = traverse_tree(if_else_node->conds[i].first, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
                
                if (cond->err == ErrType::_ERR_VAR_ || cond->err == ErrType::_ERR_ARR_) return cond;
            }
            if_else_node->cond_num.push_back(*cond_counter + n - 1);
            *cond_counter += n;
            *main_counter += n;
            
            for (int i = 1; i < n; ++i) {
                *if_counter += 1;
                Result* stmts = traverse_tree(if_else_node->conds[i].second, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
                
                if (stmts->err == ErrType::_ERR_VAR_ || stmts->err == ErrType::_ERR_ARR_) return stmts;
            }
            Result* stmts = traverse_tree(if_else_node->conds[0].second, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
            Result* res = traverse_tree(if_else_node->next, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
            
            if (stmts->err == ErrType::_ERR_VAR_ || stmts->err == ErrType::_ERR_ARR_) return stmts;
            else if (res->err == ErrType::_ERR_VAR_ || res->err == ErrType::_ERR_ARR_) return res;
        }
    }
    else if (while_node) {
        while_node->while_num = *loop_counter;
        while_node->main_num = *main_counter;
        *loop_counter += 1;    
        *main_counter += 1;
        
        Result* cond = traverse_tree(while_node->cond, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        Result* stmts = traverse_tree(while_node->stmts, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        Result* res = traverse_tree(while_node->next, mp, arrs, loop_counter, if_counter, cond_counter, main_counter, arrayDecl_loop);
        
        if (cond->err == ErrType::_ERR_VAR_ || cond->err == ErrType::_ERR_ARR_) return cond;
        else if (stmts->err == ErrType::_ERR_VAR_ || stmts->err == ErrType::_ERR_ARR_) return stmts;
        else if (res->err == ErrType::_ERR_VAR_ || res->err == ErrType::_ERR_ARR_) return res;
    }
    
    return new Result(ErrType::_OK_, -1, "");
};


void num_of_scans(ASTNode* ptr, int* num) {
    auto* num_node = dynamic_cast<NumNode*>(ptr);
    auto* var_node = dynamic_cast<VarNode*>(ptr);
    auto* arrElem_node = dynamic_cast<ArrayElemNode*>(ptr);
    auto* bin_op_node = dynamic_cast<BinaryNode*>(ptr);
    auto* main_node = dynamic_cast<MainNode*>(ptr);
    auto* assign_node = dynamic_cast<AssignNode*>(ptr);
    auto* print_node = dynamic_cast<PrintNode*>(ptr);
    auto* scan_node = dynamic_cast<ScanNode*>(ptr);
    auto* statArrDecl_node = dynamic_cast<StatArrayDeclNode*>(ptr);
    auto* dynArrDecl_node = dynamic_cast<DynArrayDeclNode*>(ptr);
    auto* arrElemAssign_node = dynamic_cast<ArrayElemAssignNode*>(ptr);
    auto* if_else_node = dynamic_cast<IfElseNode*>(ptr);
    auto* while_node = dynamic_cast<WhileNode*>(ptr);
    
    if (num_node) { return; }
    else if (var_node) { return; }
    else if (arrElem_node) { return; }
    else if (bin_op_node) {
        num_of_scans(bin_op_node->left, num);
        num_of_scans(bin_op_node->right, num);
    }
    else if (main_node) {
        num_of_scans(main_node->next, num);
    }
    else if (assign_node) {
        num_of_scans(assign_node->next, num);
    }
    else if (arrElemAssign_node) {
        num_of_scans(arrElemAssign_node->next, num);
    }
    else if (print_node) {
        num_of_scans(print_node->next, num);
    }
    else if (scan_node) {
        *num += 1;
        num_of_scans(scan_node->next, num);
    }
    else if (statArrDecl_node) {
        num_of_scans(statArrDecl_node->next, num);
    }
    else if (dynArrDecl_node) {
        num_of_scans(dynArrDecl_node->next, num);
    }
    else if (if_else_node) {
        int n = if_else_node->conds.size();
        
        if (n == 1) {
            num_of_scans(if_else_node->conds[0].second, num);
            num_of_scans(if_else_node->next, num);
        }
        else {
            for (int i = 0; i < n; ++i) {
                num_of_scans(if_else_node->conds[i].second, num);
            }
            
            num_of_scans(if_else_node->next, num);
        }
    }
    else if (while_node) {
        num_of_scans(while_node->stmts, num);
        num_of_scans(while_node->next, num); 
    }
    else { return; }
};

int num_vars(ASTNode* ptr, int res) {
    auto* num_node = dynamic_cast<NumNode*>(ptr);
    auto* var_node = dynamic_cast<VarNode*>(ptr);
    auto* bin_op_node = dynamic_cast<BinaryNode*>(ptr);
    
    if (num_node) { res = 0; }
    else if (var_node) { res = 1; }
    else if (bin_op_node) {
        res = num_vars(bin_op_node->left, res) + num_vars(bin_op_node->right, res);
    }
    else { return 0; }
    
    return res;
}

int num_expr_eval(ASTNode* ptr, int res) {
    auto* num_node = dynamic_cast<NumNode*>(ptr);
    auto* bin_op_node = dynamic_cast<BinaryNode*>(ptr);
    
    if (num_node) { res = num_node->num; }
    else if (bin_op_node) {
        switch (bin_op_node->tag) {
            case _ADD_:
                res = num_expr_eval(bin_op_node->left, res) + num_expr_eval(bin_op_node->right, res);
                break;
            case _SUB_:
                res = num_expr_eval(bin_op_node->left, res) - num_expr_eval(bin_op_node->right, res);
                break;
            case _MUL_:
                res = num_expr_eval(bin_op_node->left, res) * num_expr_eval(bin_op_node->right, res);
                break;
            case _DIV_:
                res = num_expr_eval(bin_op_node->left, res) / num_expr_eval(bin_op_node->right, res);
                break;
            case _MOD_:
                res = num_expr_eval(bin_op_node->left, res) % num_expr_eval(bin_op_node->right, res);
                break;
            case _SHL_:
                res = num_expr_eval(bin_op_node->left, res) << num_expr_eval(bin_op_node->right, res);
                break;
            case _SHR_:
                res = num_expr_eval(bin_op_node->left, res) % num_expr_eval(bin_op_node->right, res);
                break;
            case _LESS_:
                res = num_expr_eval(bin_op_node->left, res) < num_expr_eval(bin_op_node->right, res);
                break;
            case _GREAT_:
                res = num_expr_eval(bin_op_node->left, res) > num_expr_eval(bin_op_node->right, res);
                break;
            case _EQ_:
                res = num_expr_eval(bin_op_node->left, res) == num_expr_eval(bin_op_node->right, res);
                break;
            case _NEQ_:
                res = num_expr_eval(bin_op_node->left, res) != num_expr_eval(bin_op_node->right, res);
                break;
            case _GEQ_:
                res = num_expr_eval(bin_op_node->left, res) >= num_expr_eval(bin_op_node->right, res);
                break;
            case _LEQ_:
                res = num_expr_eval(bin_op_node->left, res) <= num_expr_eval(bin_op_node->right, res);
                break;
            case _AND_:
                res = num_expr_eval(bin_op_node->left, res) && num_expr_eval(bin_op_node->right, res);
                break;
            case _OR_:
                res = num_expr_eval(bin_op_node->left, res) || num_expr_eval(bin_op_node->right, res);
                break;
            case _NOT_:
                res = !num_expr_eval(bin_op_node->right, res);
                break;
            case _NEG_:
                res = -num_expr_eval(bin_op_node->right, res);
                break;
        }
    }
    
    return res;
}

int get_size(int n) {
    int i = 16;    
    while (i < n) i += 16;
    return i;
}

void print_asm(ASTNode* ptr, std::map<std::string, int>& mp, std::map<std::string, std::pair<int, ArrayType>>& arrs) {
    auto* num_node = dynamic_cast<NumNode*>(ptr);
    auto* var_node = dynamic_cast<VarNode*>(ptr);
    auto* arrElem_node = dynamic_cast<ArrayElemNode*>(ptr);
    auto* bin_op_node = dynamic_cast<BinaryNode*>(ptr);
    auto* main_node = dynamic_cast<MainNode*>(ptr);
    auto* assign_node = dynamic_cast<AssignNode*>(ptr);
    auto* print_node = dynamic_cast<PrintNode*>(ptr);
    auto* scan_node = dynamic_cast<ScanNode*>(ptr);
    auto* statArrDecl_node = dynamic_cast<StatArrayDeclNode*>(ptr);
    auto* dynArrDecl_node = dynamic_cast<DynArrayDeclNode*>(ptr);
    auto* arrElemAssign_node = dynamic_cast<ArrayElemAssignNode*>(ptr);
    auto* if_else_node = dynamic_cast<IfElseNode*>(ptr);
    auto* while_node = dynamic_cast<WhileNode*>(ptr);
    
    if (num_node) {
        std::cout << "  mov rax, " << num_node->num << std::endl;
    }
    else if (var_node) {
        std::cout << "  mov rax, QWORD PTR [rbp-" << 2 * mp[var_node->var_name] << "]" << std::endl;
    }
    else if (arrElem_node) {
        print_asm(arrElem_node->elem_index, mp, arrs);
        
        auto it = arrs.find(arrElem_node->arr_name);
        if (it != arrs.end()) {
            ArrayType ty = it->second.second;
            if (ty == ArrayType::_STAT_) {
                std::cout << "  mov r8, -8" << std::endl;
                std::cout << "  mul r8" << std::endl;
                std::cout << "  sub rax, " << 2*arrs[arrElem_node->arr_name].first << std::endl;
                std::cout << "  mov r9, rax" << std::endl;
                std::cout << "  mov rax, QWORD PTR [rbp+r9]" << std::endl;
            }
            else if (ty == ArrayType::_DYN_) {
                std::cout << "  mov r8, 8" << std::endl;
                std::cout << "  mul r8" << std::endl;
                std::cout << "  mov rdx, rax" << std::endl;
                std::cout << "  mov rax, QWORD PTR [rbp-" << 2*arrs[arrElem_node->arr_name].first << "]" << std::endl;
                std::cout << "  add rax, rdx" << std::endl;
                std::cout << "  mov rdi, rax" << std::endl;
                std::cout << "  mov rax, QWORD PTR [rdi]" << std::endl;
            }
        }
    }
    else if (bin_op_node) {
        switch (bin_op_node->tag) {
            case _ADD_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  add rax, rbx" << std::endl;
                break;
            }
            case _SUB_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  push rax" << std::endl;
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  pop rax" << std::endl;
                std::cout << "  sub rax, rbx" << std::endl;
                break;
            }
            case _MUL_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  imul rax, rbx" << std::endl;
                break;
            }
            case _DIV_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  push rax" << std::endl;
                std::cout << "  mov rax, rbx" << std::endl;
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  cqo" << std::endl;
                std::cout << "  xor rdx, rdx" << std::endl;
                std::cout << "  idiv rbx" << std::endl;
                break;
            }
            case _MOD_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  push rax" << std::endl;
                std::cout << "  mov rax, rbx" << std::endl;
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  cqo" << std::endl;
                std::cout << "  xor rdx, rdx" << std::endl;
                std::cout << "  idiv rbx" << std::endl;
                std::cout << "  mov rax, rdx" << std::endl;
                break;
            }
            case _SHL_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  push rax" << std::endl;
                std::cout << "  mov rax, rbx" << std::endl;
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  mov rdi, rax" << std::endl;
                std::cout << "  mov rsi, rbx" << std::endl;
                std::cout << "  call shlf" << std::endl;
                break;
            }
            case _SHR_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  push rax" << std::endl;
                std::cout << "  mov rax, rbx" << std::endl;
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  mov rdi, rax" << std::endl;
                std::cout << "  mov rsi, rbx" << std::endl;
                std::cout << "  call shrf" << std::endl;
                break;
            }
            case _AND_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  and rax, rbx" << std::endl;
                break;
            }
            case _OR_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  or rax, rbx" << std::endl;
                break;
            }
            case _NOT_ : {
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  not rax" << std::endl;
                break;
            }
            case _LESS_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  push rax" << std::endl;
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  pop rax" << std::endl;
                std::cout << "  mov rdi, rax" << std::endl;
                std::cout << "  mov rsi, rbx" << std::endl;
                std::cout << "  call cmp_less" << std::endl;  
                break;
            }
            case _GREAT_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  push rax" << std::endl;
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  pop rax" << std::endl;
                std::cout << "  mov rdi, rax" << std::endl;
                std::cout << "  mov rsi, rbx" << std::endl;
                std::cout << "  call cmp_great" << std::endl; 
                break;
            }
            case _EQ_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  push rax" << std::endl;
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  pop rax" << std::endl;
                std::cout << "  mov rdi, rax" << std::endl;
                std::cout << "  mov rsi, rbx" << std::endl;
                std::cout << "  call cmp_eq" << std::endl; 
                break;
            }
            case _NEQ_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  push rax" << std::endl;
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  pop rax" << std::endl;
                std::cout << "  mov rdi, rax" << std::endl;
                std::cout << "  mov rsi, rbx" << std::endl;
                std::cout << "  call cmp_neq" << std::endl; 
                break;
            }
            case _LEQ_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  push rax" << std::endl;
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  pop rax" << std::endl;
                std::cout << "  mov rdi, rax" << std::endl;
                std::cout << "  mov rsi, rbx" << std::endl;
                std::cout << "  call cmp_leq" << std::endl; 
                break;
            }
            case _GEQ_ : {
                print_asm(bin_op_node->left, mp, arrs);
                std::cout << "  push rax" << std::endl;
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  push rax" << std::endl;
                std::cout << "  pop rbx" << std::endl;
                std::cout << "  pop rax" << std::endl;
                std::cout << "  mov rdi, rax" << std::endl;
                std::cout << "  mov rsi, rbx" << std::endl;
                std::cout << "  call cmp_geq" << std::endl; 
                break;
            }
            case _NEG_ : {
                print_asm(bin_op_node->right, mp, arrs);
                std::cout << "  mov r8, -1" << std::endl;
                std::cout << "  mul r8" << std::endl;
                break;
            }
            default: break;
        }
    }
    else if (main_node) {
        std::cout << ".intel_syntax noprefix\n" << std::endl;

        std::cout << ".data" << std::endl;
        std::cout << "  print_format: .asciz \"\%ld\\n\"" << std::endl;
        std::cout << "  scan_format: .asciz \"\%ld\"" << std::endl;
        
        std::cout << "\n.text\n" << std::endl;
        std::cout << ".global main" << std::endl;
        std::cout << "main:" << std::endl;
        std::cout << "  push rbp" << std::endl;
        std::cout << "  mov rbp, rsp" << std::endl;
        
        int scans = 0;
        num_of_scans(main_node, &scans);
        scans *= 16;
        int vars = 0;
        for (auto it : mp) {
            if (vars < it.second) { vars = it.second; }
        }
        for (auto it : arrs) {
            if (vars < it.second.first) { vars = it.second.first; }
        }
        vars *= 2;
        
        std::cout << "#SCANS: " << scans << std::endl;
        std::cout << "#VARS: " << vars << std::endl;
        
        if (scans >= vars) {
            std::cout << "  sub rsp, " << scans << std::endl;
        }
        else {
            std::cout << "  sub rsp, " << get_size(vars) << std::endl;
        }

        print_asm(main_node->next, mp, arrs);
        std::cout << "  leave" << std::endl;
        std::cout << "  ret\n\n" << std::endl;
    }
    else if (assign_node) {
        print_asm(assign_node->assign_val, mp, arrs);
        std::cout << "  mov QWORD PTR [rbp-" << 2 * mp[assign_node->var_name] << "], rax" << std::endl;
        print_asm(assign_node->next, mp, arrs);
    }
    else if (arrElemAssign_node) {
        print_asm(arrElemAssign_node->elem_index, mp, arrs);
        
        auto it = arrs.find(arrElemAssign_node->arr_name);
        if (it != arrs.end()) { 
            ArrayType ty = it->second.second;
            if (ty == ArrayType::_STAT_) {
                std::cout << "  mov r8, -8" << std::endl;
                std::cout << "  mul r8" << std::endl;
                std::cout << "  sub rax, " << 2*arrs[arrElemAssign_node->arr_name].first << std::endl;
                std::cout << "  mov r9, rax" << std::endl;
                print_asm(arrElemAssign_node->assign_val, mp, arrs);
                std::cout << "  mov QWORD PTR [rbp+r9], rax" << std::endl;
            }
            else if (ty == ArrayType::_DYN_) {
                std::cout << "  mov r8, 8" << std::endl;
                std::cout << "  mul r8" << std::endl;
                std::cout << "  mov rdx, rax" << std::endl;
                std::cout << "  mov rax, QWORD PTR [rbp-" << 2*arrs[arrElemAssign_node->arr_name].first << "]" << std::endl;
                std::cout << "  add rax, rdx" << std::endl;
                std::cout << "  mov r9, rax" << std::endl;
                print_asm(arrElemAssign_node->assign_val, mp, arrs);
                std::cout << "  mov QWORD PTR [r9], rax" << std::endl;
                
            }
            else { return; }
        }
        
        print_asm(arrElemAssign_node->next, mp, arrs);
    }
    else if (print_node) {
        print_asm(print_node->print_val, mp, arrs);
        std::cout << "  lea rdi, print_format" << std::endl;
        std::cout << "  mov rsi, rax" << std::endl;
        std::cout << "  xor rax, rax" << std::endl;
        std::cout << "  call printf" << std::endl;
        print_asm(print_node->next, mp, arrs);
    }
    else if (scan_node) {
        std::cout << "  lea rdi, scan_format" << std::endl;
        std::cout << "  lea rsi, [rbp-" << 2 * mp[scan_node->var_name] << "]" << std::endl;
        std::cout << "  xor rax, rax" << std::endl;
        std::cout << "  call scanf" << std::endl;
        print_asm(scan_node->next, mp, arrs);
    }
    else if (statArrDecl_node) {
        for (int i = 0; i < statArrDecl_node->arr_size; ++i) {
            print_asm(statArrDecl_node->arr_vals[i], mp, arrs);
            
            std::string elem_name = statArrDecl_node->arr_name + std::to_string(i);
            std::cout << "  mov QWORD PTR [rbp-" << 2 * mp[elem_name] << "], rax" << std::endl;
        }
        
        print_asm(statArrDecl_node->next, mp, arrs);
    }
    else if (dynArrDecl_node) {
        int num = num_vars(dynArrDecl_node->arr_size, 0);
        if (num == 0) {
            int res = num_expr_eval(dynArrDecl_node->arr_size, 0);
            std::cout << "  mov rax, " << std::to_string(res) << std::endl;
        }
        else {
            print_asm(dynArrDecl_node->arr_size, mp, arrs);
        }
        
        std::cout << "  mov rbx, rax" << std::endl;
        std::cout << "  sal rax, 3" << std::endl;
        std::cout << "  mov r12, 0" << std::endl;
        std::cout << "  mov rdi, rax" << std::endl;
        std::cout << "  call malloc" << std::endl;
        std::cout << "  mov QWORD PTR [rbp-" << 2*arrs[dynArrDecl_node->arr_name].first << "], rax" << std::endl; 
        std::cout << "arr_loop" << dynArrDecl_node->arrayDecl_loop << ":" << std::endl;
        std::cout << "  cmp r12, rbx" << std::endl;
        std::cout << "  je arr_next" << dynArrDecl_node->arrayDecl_loop << std::endl;
        std::cout << "  mov rax, r12" << std::endl;
        std::cout << "  mov r8, 8" << std::endl;
        std::cout << "  mul r8" << std::endl;
        std::cout << "  mov rdx, rax" << std::endl;
        std::cout << "  mov rax, QWORD PTR [rbp-" << 2*arrs[dynArrDecl_node->arr_name].first << "]" << std::endl;
        std::cout << "  add rax, rdx" << std::endl;
        std::cout << "  mov rdi, rax" << std::endl;
        print_asm(dynArrDecl_node->arr_val, mp, arrs);
        std::cout << "  mov QWORD PTR [rdi], rax" << std::endl;
        std::cout << "  inc r12" << std::endl;
        std::cout << "  jmp arr_loop" << dynArrDecl_node->arrayDecl_loop << std::endl;
        std::cout << "arr_next" << dynArrDecl_node->arrayDecl_loop << ":" << std::endl;
            
        print_asm(dynArrDecl_node->next, mp, arrs);
    }
    else if (if_else_node) {
        auto* next_if_else = dynamic_cast<IfElseNode*>(if_else_node->next);
        auto* next_while = dynamic_cast<WhileNode*>(if_else_node->next);
        int n = if_else_node->conds.size();
        
        if (n == 1) {
            std::cout << "if" << if_else_node->if_num << ":" << std::endl;
            print_asm(if_else_node->conds[0].first, mp, arrs);
            std::cout << "  cmp rax, 1" << std::endl;
            std::cout << "  je cond" << if_else_node->cond_num[0] << std::endl;
            
            if (next_if_else) {
                std::cout << "  jmp if" << next_if_else->if_num << std::endl;
            }
            else if (next_while) {
                std::cout << "  jmp loop" << next_while->while_num << std::endl;
            }
            else {
                std::cout << "  jmp main" << if_else_node->main_num << std::endl;
            }
            
            std::cout << "cond" << if_else_node->cond_num[0] << ":" << std::endl;
            print_asm(if_else_node->conds[0].second, mp, arrs);
                
            if (next_if_else) {
                std::cout << "  jmp if" << next_if_else->if_num << std::endl; 
            }
            else if (next_while) {
                std::cout << "  jmp loop" << next_while->while_num << std::endl;
            }
            else {
                std::cout << "  jmp main" << if_else_node->main_num << std::endl;
            }
            
            if (!(next_if_else || next_while)) {
                std::cout << "main" << if_else_node->main_num << ":" << std::endl;
            }
            
            print_asm(if_else_node->next, mp, arrs);
        }
        else {
            std::cout << "if" << if_else_node->if_num << ":" << std::endl;
            int i;
            for (i = 1; i < n; ++i) {
                print_asm(if_else_node->conds[i].first, mp, arrs);
                std::cout << "  cmp rax, 1" << std::endl;
                std::cout << "  je cond" << if_else_node->cond_num[i-1] << std::endl;
            }
            std::cout << "  jmp cond" << if_else_node->cond_num[i-1] << std::endl;
            
            for (i = 1; i < n; ++i) {
                std::cout << "cond" << if_else_node->cond_num[i-1] << ":" << std::endl;
                print_asm(if_else_node->conds[i].second, mp, arrs);
                
                if (next_if_else) {
                    std::cout << "  jmp if" << next_if_else->if_num << std::endl; 
                }
                else if (next_while) {
                    std::cout << "  jmp loop" << next_while->while_num << std::endl;
                }
                else {
                    std::cout << "  jmp main" << if_else_node->main_num << std::endl;
                }
            }
            std::cout << "cond" << if_else_node->cond_num[i-1] << ":" << std::endl;
            print_asm(if_else_node->conds[0].second, mp, arrs);
                
            if (next_if_else) {
                std::cout << "  jmp if" << next_if_else->if_num << std::endl; 
            }
            else if (next_while) {
                std::cout << "  jmp loop" << next_while->while_num << std::endl;
            }
            else {
                std::cout << "  jmp main" << if_else_node->main_num << std::endl;
                std::cout << "main" << if_else_node->main_num << ":" << std::endl;
            }
            print_asm(if_else_node->next, mp, arrs);
        }
        
    }
    else if (while_node) {
        auto* next_while = dynamic_cast<WhileNode*>(while_node->next);
        auto* next_if_else = dynamic_cast<IfElseNode*>(while_node->next);
        
        std::cout << "loop" << while_node->while_num << ":" << std::endl;
        print_asm(while_node->cond, mp, arrs);
        std::cout << "  cmp rax, 0" << std::endl;
        if (next_while) {
            std::cout << "  je loop" << next_while->while_num << std::endl;
        }
        else if (next_if_else) {
            std::cout << "  je if" << next_if_else->if_num << std::endl;
        }
        else {
            std::cout << "  je main" << while_node->main_num << std::endl;
        }
        print_asm(while_node->stmts, mp, arrs);
        std::cout << "  jmp loop" << while_node->while_num << std::endl;
        
        if (!(next_while || next_if_else)) {
            std::cout << "main" << while_node->main_num << ":" << std::endl;
        }
        print_asm(while_node->next, mp, arrs);
    }
};

std::string get_err_line(int err_index, std::string filename) {
    std::ifstream file(filename);
    std::string result;
    
    int i = 1;
    
    if (file.is_open()) {
        std::string line;
        
        while (std::getline(file, line) && i++ < err_index);
        
        result = line;
        file.close();
    }
    
    return result;
};