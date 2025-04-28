#include <iostream>
#include <sstream>
#include <string>

#include <gcc-plugin.h>
#include <plugin-version.h>

#include <coretypes.h>

#include <tree-pass.h>
#include <context.h>
#include <basic-block.h>

#include <tree.h>
#include <tree-ssa-alias.h>
#include <gimple-expr.h>
#include <gimple.h>
#include <gimple-ssa.h>
#include <tree-phinodes.h>
#include <tree-ssa-operands.h>

#include <ssa-iterators.h>
#include <gimple-iterator.h>

#define PREFIX_UNUSED(variable) ((void)variable)

using namespace std;

// Функция для отладки, выводит информацию о базовом блоке
static unsigned int lab1_bb_id(basic_block bb)
{
    // Выводим метку базового блока
    cout << "\t" << "bb: ";

    edge e;
    edge_iterator it;

    // Выводим исходящие ребра из базового блока
    cout << "(";
    stringstream src_stream;
    FOR_EACH_EDGE(e, it, bb->preds) {
        src_stream << e->src->index << ", ";
    }
    string src = src_stream.str();
    cout << src.substr(0, src.size() - 2);
    cout << ")";

    // Выводим индекс базового блока
    cout << " -> " << "(" << bb->index << ")";

    // Выводим входящие ребра в базовый блок
    cout << " -> ";
    cout << "(";
    stringstream dst_stream;
    FOR_EACH_EDGE(e, it, bb->succs) {
        dst_stream << e->dest->index << ", ";
    }
    string dst = dst_stream.str();
    cout << dst.substr(0, dst.size() - 2);
    cout << ")";

    return 0;
}

// Функция для отладки, выводит информацию о дереве
static unsigned int lab1_tree(tree t)
{   
    switch (TREE_CODE(t)) {
        // Обработка констант.
    case INTEGER_CST:
        cout << TREE_INT_CST_LOW(t);
        break;
    case STRING_CST:
        cout << "\"" << TREE_STRING_POINTER(t) << "\"";
        break;
        // Обработка объявлений.
    case LABEL_DECL: 
        cout << (DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "unk_label_decl") << ":";
        break;
    case FIELD_DECL:
        cout << (DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "unk_field_decl");
        break;
    case VAR_DECL:
        cout << (DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "unk_var_decl");
        break;
    case CONST_DECL:
        cout << (DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "unk_const_decl");
        break;
        // Обработка ссылок на память.
    case COMPONENT_REF:
        lab1_tree(TREE_OPERAND(t, 0));
        cout << "->";
        lab1_tree(TREE_OPERAND(t, 1));
        break;
    case BIT_FIELD_REF:
        lab1_tree(TREE_OPERAND(t, 0));
        cout << "->";
        cout << "(";
        lab1_tree(TREE_OPERAND(t, 1));
        cout << " : ";
        lab1_tree(TREE_OPERAND(t, 2));
        cout << ")";
        break;
    case ARRAY_REF:
        lab1_tree(TREE_OPERAND(t, 0));
        cout << "[";
        lab1_tree(TREE_OPERAND(t, 1));
        cout << "]";
        break;
    case ARRAY_RANGE_REF:
        lab1_tree(TREE_OPERAND(t, 0));
        cout  << "[";
        lab1_tree(TREE_OPERAND(t, 1));
        cout << ":";
        lab1_tree(TREE_OPERAND(t, 2));
        cout << "]";
        break;
    case INDIRECT_REF:
        cout << "*";
        lab1_tree(TREE_OPERAND(t, 0));
        break;
    case CONSTRUCTOR:
        cout << "constructor";
        break;
    case ADDR_EXPR:
        cout << "&";
        lab1_tree(TREE_OPERAND(t, 0));
        break;
    case TARGET_MEM_REF:
        cout << "TMR(";
        cout << "BASE: ";
        lab1_tree(TREE_OPERAND(t, 0));
        cout << ", ";
        cout << "OFFSET: ";
        lab1_tree(TREE_OPERAND(t, 1));
        cout << ", ";
        cout << "STEP: ";
        lab1_tree(TREE_OPERAND(t, 2));
        cout << ", ";
        cout << "INDEX1: ";
        lab1_tree(TREE_OPERAND(t, 3));
        cout << ", ";
        cout << "INDEX2: ";
        lab1_tree(TREE_OPERAND(t, 4));
        cout << " )";
        break;
    case MEM_REF:
        cout << "((typeof(";
        lab1_tree(TREE_OPERAND(t, 1));
        cout << "))";
        lab1_tree(TREE_OPERAND(t, 0));
        cout << ")";
        break;
        // Обработка SSA-имен.
    case SSA_NAME: {
        gimple* stmt = SSA_NAME_DEF_STMT(t);
        if (gimple_code(stmt) == GIMPLE_PHI) {
            cout << "(" << (SSA_NAME_IDENTIFIER(t) ? IDENTIFIER_POINTER(SSA_NAME_IDENTIFIER(t)) : "unk_ssa_name") <<
            "__v" << SSA_NAME_VERSION(t);
            cout << " = GIMPLE_PHI(";
            for (unsigned int i = 0; i < gimple_phi_num_args(stmt); i++) {
                lab1_tree(gimple_phi_arg(stmt, i)->def);
                if (i != gimple_phi_num_args(stmt) - 1) {
                    cout << ", ";
                }
            }
            cout << "))";
        } else {
            cout << (SSA_NAME_IDENTIFIER(t) ? IDENTIFIER_POINTER(SSA_NAME_IDENTIFIER(t)) : "unk_ssa_name") <<
                "__v" << SSA_NAME_VERSION(t);
        }
        
        break;
    }
    default:
        cout << "unk_tree_code(" << TREE_CODE(t) << ")";
        break;
    }

    return 0;
}


// Функция для отладки, выводит информацию об операторе дерева
static unsigned int lab1_op(enum tree_code code)
{
    switch (code) {
        // Арифметические операторы.
    case POINTER_PLUS_EXPR:
    case PLUS_EXPR:
        cout << "+";
        break;
    case NEGATE_EXPR:
    case MINUS_EXPR:
        cout << "-";
        break;
    case MULT_EXPR:
        cout << "*";
        break;
    case TRUNC_DIV_EXPR:
    case CEIL_DIV_EXPR:
    case FLOOR_DIV_EXPR:
    case ROUND_DIV_EXPR:
    case EXACT_DIV_EXPR:
    case RDIV_EXPR:
        cout << "/";
        break;
        // Операторы битового сдвига.
    case LSHIFT_EXPR:
        cout << "<<";
        break;
    case RSHIFT_EXPR:
        cout << ">>";
        break;
        // Операторы битовой логики.
    case BIT_IOR_EXPR:
        cout << "|";
        break;
    case BIT_XOR_EXPR:
        cout << "^";
        break;
    case BIT_AND_EXPR:
        cout << "&";
        break;
    case BIT_NOT_EXPR:
        cout << "!";
        break;
        // Операторы логической истины.
    case TRUTH_ANDIF_EXPR:
    case TRUTH_AND_EXPR:
        cout << "&&";
        break;
    case TRUTH_ORIF_EXPR:
    case TRUTH_OR_EXPR:
        cout << "||";
        break;
    case TRUTH_XOR_EXPR:
        cout << "^^";
        break;
    case TRUTH_NOT_EXPR:
        cout << "!";
        break;
        // Операторы отношений.
    case LT_EXPR:
    case UNLT_EXPR:
        cout << "<";
        break;
    case LE_EXPR:
    case UNLE_EXPR:
        cout << "<=";
        break;
    case GT_EXPR:
    case UNGT_EXPR:
        cout << ">";
        break;
    case GE_EXPR:
    case UNGE_EXPR:
        cout << ">=";
        break;
    case EQ_EXPR:
    case UNEQ_EXPR:
        cout << "==";
        break;
    case NE_EXPR:
    case LTGT_EXPR:
        cout << "!=";
        break;
    case UNORDERED_EXPR:
        cout << "unord";
        break;
    case ORDERED_EXPR:
        cout << "ord";
        break;
        // Неизвестные операторы.
    default:
        cout << "?(" << code << ")?";
        break;
    }
    return 0;
}


// Функция для отладки, выводит информацию о GIMPLE_ASSIGN выражении
static unsigned int lab1_assign(gimple* stmt)
{
    cout << "\t\t" << "stmt: " << "\"GIMPLE_ASSIGN\"" << " " << "(" << GIMPLE_ASSIGN << ")" << " { ";
    // Проверяем количество операндов в выражении
    switch (gimple_num_ops(stmt)) {
    case 2:
        // Выводим информацию о левой части выражения
        lab1_tree(gimple_assign_lhs(stmt));
        cout << " = ";
        // Выводим информацию о правой части выражения
        lab1_tree(gimple_assign_rhs1(stmt));
        break;
    case 3:
        // Выводим информацию о левой части выражения
        lab1_tree(gimple_assign_lhs(stmt));
        cout << " = ";
        // Выводим информацию о первой правой части выражения
        lab1_tree(gimple_assign_rhs1(stmt));
        cout << " ";
        // Выводим информацию об операторе между двумя правыми частями выражения
        lab1_op(gimple_assign_rhs_code(stmt));
        cout << " ";
        // Выводим информацию о второй правой части выражения
        lab1_tree(gimple_assign_rhs2(stmt));
        break;
    }
    cout << " }" << endl;
    
    return 0;
}


// Функция для отладки, выводит информацию о GIMPLE_CALL выражении
static unsigned int lab1_call(gimple* stmt)
{
    cout << "\t\t" << "stmt: " << "\"GIMPLE_CALL\"" << " " << "(" << GIMPLE_CALL << ")" << " { ";
    // Получаем левую часть выражения
    tree lhs = gimple_call_lhs (stmt);
    if (lhs) {
        // Выводим информацию о левой части выражения
        lab1_tree(lhs);
        printf(" = ");
    }
    // Выводим информацию о вызываемой функции
    cout << fndecl_name(gimple_call_fndecl(stmt)) << "(";
    // Выводим информацию об аргументах вызова функции
    for (unsigned int i = 0; i < gimple_call_num_args(stmt); i++) {
        lab1_tree(gimple_call_arg(stmt, i));
        if (i != gimple_call_num_args(stmt) - 1) {
            cout << ", ";
        }
    }
    cout << ")";
    cout << " }" << endl;
    
    return 0;
}


// Функция для отладки, выводит информацию о GIMPLE_COND выражении
static unsigned int lab1_cond(gimple* stmt)
{
    cout << "\t\t" << "stmt: " << "\"GIMPLE_COND\"" << " " << "(" << GIMPLE_COND << ")" << " { ";
    // Выводим информацию о левой части условного выражения
    lab1_tree(gimple_cond_lhs(stmt));
    cout << " ";
    // Выводим информацию об операторе условного выражения
    lab1_op(gimple_assign_rhs_code(stmt));
    cout << " ";
    // Выводим информацию о правой части условного выражения
    lab1_tree(gimple_cond_rhs(stmt));
    cout << " }" << endl;
    
    return 0;
}

// Функция для отладки, выводит информацию о GIMPLE_LABEL выражении
static unsigned int lab1_label(gimple* stmt) {
    PREFIX_UNUSED(stmt);
    cout << "\t\t" << "stmt: " << "\"GIMPLE_LABEL\"" << " " << "(" << GIMPLE_LABEL << ")" << " {";
    cout << "}" << endl;
    
    return 0;
}

// Функция для отладки, выводит информацию о GIMPLE_RETURN выражении
static unsigned int lab1_return(gimple* stmt)
{
    PREFIX_UNUSED(stmt);
    cout << "\t\t" << "stmt: " << "\"GIMPLE_RETURN\"" << " " << "(" << GIMPLE_RETURN << ")" << " {";
    cout << "}" << endl;
        
    return 0;
}


// Функция для отладки, выводит информацию о выражениях внутри базового блока
static unsigned int lab1_statements(basic_block bb)
{
    // Итерируемся по выражениям в базовом блоке
    for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
        gimple* stmt = gsi_stmt(gsi);

        // Определяем тип выражения и вызываем соответствующую функцию для его отладки
        switch (gimple_code(stmt)) {
        case GIMPLE_ASSIGN:
            lab1_assign(stmt);
            break;
        case GIMPLE_CALL:
            lab1_call(stmt);
            break;
        case GIMPLE_COND:
            lab1_cond(stmt);
            break;
        case GIMPLE_LABEL:
            lab1_label(stmt);
            break;
        case GIMPLE_RETURN:
            lab1_return(stmt);
            break;
        }
    }
             
    return 0;
}

// Функция для отладки, выводит информацию о функции и ее базовых блоках
static unsigned int lab1_function(function* fn)
{
    cout << "func: " << "\"" << function_name(fn) << "\"" << " {" << endl;
    basic_block bb;
    // Итерируемся по базовым блокам в функции
    FOR_EACH_BB_FN(bb, fn) {
        // Выводим идентификатор базового блока
        lab1_bb_id(bb);
        cout << " {" <<  endl;
        // Выводим информацию о выражениях внутри базового блока
        lab1_statements(bb);
        cout << "\t" << "}" << endl;
    }
    cout << "}" << endl;
    cout << endl;
    return 0;
}


// make phi-debug
//make test
//gcc -fdump-tree-ssa-graph src/test/test.c

//dot -Tpng a-test.c.018t.ssa.dot > output.png

