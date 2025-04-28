#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Instruction.h"
#include <fstream>
#include <streambuf>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace llvm;
using namespace std;


enum Token {
    tok_error = 0,
    tok_eof = 1,

    tok_ident = 2,
    tok_number = 3,

    tok_if = 4,
    tok_for = 5,
    tok_return = 6,

    tok_lparen = 7,
    tok_rparen = 8,
    tok_lfig = 9,
    tok_rfig = 10,
    tok_comma = 11,
    
    tok_eq = 12,
    tok_plus = 13, 
    tok_minus = 14,
    tok_else = 15

};

static string sequ;
static int cur_pos = 0;

static string StrVal;
static int NumVal;

static char next_char()
{
    if (cur_pos >= sequ.length())
        return '$';
    char c = sequ[cur_pos++];
    return c;
}

static Token gettok() {
    static char LastChar = ' ';
   
    while (isspace(LastChar))
        LastChar = next_char();


    if (isalpha(LastChar)) { 
        StrVal = LastChar;
        while (isalnum((LastChar = next_char())))
            StrVal += LastChar;
        if (StrVal == "if")
            return tok_if;
        if (StrVal == "for")
            return tok_for;
        if (StrVal == "return")
            return tok_return;
        if (StrVal == "else")
            return tok_else;
        return tok_ident;
    }

    if (isdigit(LastChar)) { 
    string NumStr;
    do {
        NumStr += LastChar;
        LastChar = next_char();
    } while (isdigit(LastChar));

    NumVal = strtol(NumStr.c_str(), nullptr, 10);
    return tok_number;
    }

    Token t = tok_error;
    switch (LastChar){
        case '$': t = tok_eof; break;
        case '(': t = tok_lparen; break;
        case ')': t = tok_rparen; break;
        case '{': t = tok_lfig; break;
        case '}': t = tok_rfig; break;
        case ',': t = tok_comma; break;
        case '=': t = tok_eq; break;
        case '+': t = tok_plus; break;
        case '-': t =  tok_minus; break;
    }
    LastChar = next_char();
    
    return t;
}

static Token cur_tok = tok_eof;
static Token getNextToken() { return cur_tok = gettok(); }

void init_scanner(const char* filepath)
{
    cur_pos = 0;
    ifstream t(filepath);
    sequ = string((istreambuf_iterator<char>(t)),
                 istreambuf_iterator<char>());
}

// LLVM

static unique_ptr<LLVMContext> context;
static unique_ptr<Module> mod;
static unique_ptr<IRBuilder<>> builder; 
static map<string, AllocaInst *> named_values;


static void initialize_module() {
  context = make_unique<LLVMContext>();
  mod = make_unique<Module>("lab3", *context);

  builder = make_unique<IRBuilder<>>(*context);
}


//Abstract Syntax Tree(AST)


class ExprAST {
public:
  virtual ~ExprAST() = default;

  virtual Value *codegen() = 0;
};


class NumberExprAST : public ExprAST {
  int Val;

public:
  NumberExprAST(int Val) : Val(Val) {}

  Value *codegen() override;
};


class VariableExprAST : public ExprAST {
  string Name;

public:
    VariableExprAST(const string &Name) : Name(Name) {}
    const string &getName() const { return Name; }

    Value *codegen() override;
};


class BinaryExprAST : public ExprAST {
  char Op;
  unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char Op, unique_ptr<ExprAST> LHS,
                unique_ptr<ExprAST> RHS)
      : Op(Op), LHS(move(LHS)), RHS(move(RHS)) {}

  Value *codegen() override;
};


class IfExprAST : public ExprAST {
public:
  unique_ptr<ExprAST> Cond;
  vector<unique_ptr<ExprAST> > Then, Else;

public:
  IfExprAST(unique_ptr<ExprAST> Cond, vector<unique_ptr<ExprAST> > Then,
            vector<unique_ptr<ExprAST> > Else)
      : Cond(move(Cond)), Then(move(Then)), Else(move(Else)) {}

  Value *codegen() override;
};


class ForExprAST : public ExprAST {
    unique_ptr<ExprAST> Start, End, Step;
    vector<unique_ptr<ExprAST> > Body;

public:
  ForExprAST(unique_ptr<ExprAST> Start,
             unique_ptr<ExprAST> End, unique_ptr<ExprAST> Step,
             vector<unique_ptr<ExprAST> > Body)
      : Start(move(Start)), End(move(End)),
        Step(move(Step)), Body(move(Body)) {}

  Value *codegen() override;
};


class PrototypeAST { 
  string Name;
  vector<string> Args;

public:
  PrototypeAST(const string &Name, vector<string> Args)
      : Name(Name), Args(move(Args)) {}

  Function *codegen();
  const string &getName() const { return Name; }
};


class FunctionAST {
public:
  unique_ptr<PrototypeAST> proto;
  vector<unique_ptr<ExprAST>> body;

public:
  FunctionAST(unique_ptr<PrototypeAST>& proto,
              vector<unique_ptr<ExprAST>>& body)
      : proto(move(proto)), body(move(body)) {}

  Function *codegen();
};

// Code generation

static AllocaInst *CreateEntryBlockAlloca(Function  *TheFunction,
                                          StringRef VarName) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                   TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(Type::getInt32Ty(*context), nullptr, VarName);
}

Value *NumberExprAST::codegen() {
    return ConstantInt::get(*context, APInt(32, Val, false));
}

Value *VariableExprAST::codegen() {

  AllocaInst *A = named_values[Name];
  if (!A)
    A = CreateEntryBlockAlloca(builder->GetInsertBlock()->getParent(), Name);
    named_values[Name] = A;
  return builder->CreateLoad(A->getAllocatedType(), A, Name.c_str());
}

Value *BinaryExprAST::codegen() {
    if (Op == '='){
        VariableExprAST *LHSE = static_cast<VariableExprAST *>(LHS.get());
        if (!LHSE)
            return nullptr;

        Value *Val = RHS->codegen();
        if (!Val)
            return nullptr;
        
        Value *Variable = named_values[LHSE->getName()];
        if (!Variable){
            Variable = CreateEntryBlockAlloca(builder->GetInsertBlock()->getParent(), LHSE->getName());
            named_values[LHSE->getName()] = static_cast<AllocaInst*>(Variable);
        }
            
        builder->CreateStore(Val, Variable);
        return Val;
    }

    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    if (!L || !R)
        return nullptr;

    switch (Op) {
        case '+':
            return builder->CreateAdd(L, R, "addtmp");
        case '-':
            return builder->CreateSub(L, R, "subtmp");
        default:
            return nullptr;
    }
}


Value *IfExprAST::codegen() {
    Value *CondV = Cond->codegen();
    if (!CondV)
        return nullptr;

    CondV = builder->CreateICmpNE(
        CondV, ConstantInt::get(*context, APInt(32, 0, false)), "ifcond");

    Function *TheFunction = builder->GetInsertBlock()->getParent();

    BasicBlock *ThenBB = BasicBlock::Create(*context, "then", TheFunction);
    BasicBlock *ElseBB = BasicBlock::Create(*context, "else");
    BasicBlock *MergeBB = BasicBlock::Create(*context, "ifcont");

    builder->CreateCondBr(CondV, ThenBB, ElseBB);

    builder->SetInsertPoint(ThenBB);

    for(auto &expr : Then){
        expr->codegen();
    }

    builder->CreateBr(MergeBB);
    ThenBB = builder->GetInsertBlock();

    TheFunction->getBasicBlockList().push_back(ElseBB);
    builder->SetInsertPoint(ElseBB);

    for(auto &expr : Else){
        expr->codegen();
    }

    builder->CreateBr(MergeBB);
    ElseBB = builder->GetInsertBlock();

    TheFunction->getBasicBlockList().push_back(MergeBB);
    builder->SetInsertPoint(MergeBB);

    return nullptr;
}

Value *ForExprAST::codegen() {
    Function *TheFunction = builder->GetInsertBlock()->getParent();


    Value *StartVal = Start->codegen();
    if (!StartVal)
        return nullptr;

    BasicBlock *LoopBB = BasicBlock::Create(*context, "loop", TheFunction);

    builder->CreateBr(LoopBB);

    builder->SetInsertPoint(LoopBB);

    for(auto &expr : Body){
        expr->codegen();
    }

    Value *StepVal = nullptr;
    if (Step) {
        StepVal = Step->codegen();
        if (!StepVal)
        return nullptr;
    } else {
        StepVal = ConstantInt::get(*context, APInt(32, 1, false));
    }

    Value *EndCond = End->codegen();
    if (!EndCond)
        return nullptr;

    EndCond = builder->CreateICmpNE(
        EndCond, ConstantInt::get(*context, APInt(32, 0, false)), "loopcond");

    BasicBlock *AfterBB =
        BasicBlock::Create(*context, "afterloop", TheFunction);

    builder->CreateCondBr(EndCond, LoopBB, AfterBB);

    builder->SetInsertPoint(AfterBB);

    return Constant::getNullValue(Type::getInt32Ty(*context));
}


Function *PrototypeAST::codegen() {
  vector<Type *> Ints(Args.size(), Type::getInt32Ty(*context));
  FunctionType *FT =
      FunctionType::get(Type::getInt32Ty(*context), Ints, false);

  Function *F =
      Function::Create(FT, Function::ExternalLinkage, Name, mod.get());

  unsigned Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(Args[Idx++]);

  return F;
}

Function *FunctionAST::codegen() {
  
    Function *TheFunction = proto->codegen();

    if (!TheFunction)
        return nullptr;

    BasicBlock *BB = BasicBlock::Create(*context, "entry", TheFunction);
    builder->SetInsertPoint(BB);

    named_values.clear();
    for (auto &Arg : TheFunction->args())
    {
        AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName());
        builder->CreateStore(&Arg, Alloca);
        named_values[string(Arg.getName())] = Alloca;

    }
    Value *RetVal;
    for(unique_ptr<ExprAST> &expr: body)
    {
        RetVal = expr->codegen();
    }
    if (!RetVal){
        return nullptr;
    }
    builder->CreateRet(RetVal);

    verifyFunction(*TheFunction);

    return TheFunction;
  
}

void assertion(Token t)
{
    if (cur_tok != t){
        throw "Parsing error";
    }
}

vector<string> ParseVars();

vector<unique_ptr<ExprAST>> ParseBody();

unique_ptr<ExprAST> ParseExpr();

unique_ptr<ExprAST> ParseBinOp();

unique_ptr<ExprAST> ParseEq();

FunctionAST* Parse()
{
    unique_ptr<PrototypeAST> proto;
    getNextToken();
    
    assertion(tok_ident);
    string name = StrVal;
    
    getNextToken();
    vector<string> args = ParseVars();
    
    proto = make_unique<PrototypeAST>(name, args);

    FunctionAST *f;
    vector<unique_ptr<ExprAST>> body = ParseBody();
    f = new FunctionAST(proto, body);
    assertion(tok_eof);
    return f;
}


vector<string> ParseVars()
{
    vector<string> args;
    assertion(tok_lparen);
    getNextToken();
    while(cur_tok != tok_rparen && cur_tok != tok_eof)
    {
        assertion(tok_ident);
        args.emplace_back(StrVal);
        getNextToken();
        if (cur_tok == tok_comma)
            getNextToken();
    }
    getNextToken();
    return args;
}

vector<unique_ptr<ExprAST>> ParseBody()
{
    assertion(tok_lfig);
    getNextToken();
    vector<unique_ptr<ExprAST>> exprs;
    while (cur_tok == tok_ident || cur_tok == tok_for || cur_tok == tok_if)
    {
        exprs.push_back(ParseExpr());
    }
    assertion(tok_return);
    getNextToken();
    exprs.push_back(ParseBinOp());
    assertion(tok_rfig);
    getNextToken();
    return exprs;
}

unique_ptr<ExprAST> ParseEq()
{
    assertion(tok_ident);
    unique_ptr<ExprAST> LHS =  make_unique<VariableExprAST>(StrVal);
    getNextToken();
    assertion(tok_eq);
    getNextToken();
    unique_ptr<ExprAST> RHS = ParseBinOp();
    return make_unique<BinaryExprAST>('=', move(LHS), move(RHS));
}

unique_ptr<ExprAST> ParseExpr()
{
    if (cur_tok == tok_ident){
        return ParseEq();
    }
    if (cur_tok == tok_for){
        getNextToken();
        assertion(tok_lparen);
        getNextToken();
        unique_ptr<ExprAST> start = ParseEq();
        assertion(tok_comma);
        getNextToken();
        unique_ptr<ExprAST> cond = ParseBinOp();
        assertion(tok_comma);
        getNextToken();
        unique_ptr<ExprAST> step = ParseEq();
        assertion(tok_rparen);
        getNextToken();
        assertion(tok_lfig);
        getNextToken();
        vector<unique_ptr<ExprAST> > body;
        while (cur_tok != tok_rfig && cur_tok != tok_eof)
        {
            body.push_back(move(ParseEq()));
        }
        assertion(tok_rfig);
        getNextToken();
        return make_unique<ForExprAST>(move(start), move(cond), move(step), move(body));
    }
    if (cur_tok == tok_if){
        getNextToken();
        assertion(tok_lparen);
        getNextToken();
        unique_ptr<ExprAST> cond = ParseBinOp();
        assertion(tok_rparen);
        getNextToken();
        assertion(tok_lfig);
        getNextToken();
        vector<unique_ptr<ExprAST> > body;
        while (cur_tok != tok_rfig && cur_tok != tok_eof)
        {
            body.push_back(move(ParseEq()));
        }
        assertion(tok_rfig);
        getNextToken();
        assertion(tok_else);
        getNextToken();
        assertion(tok_lfig);
        getNextToken();
        vector<unique_ptr<ExprAST> > else_body;
        while (cur_tok != tok_rfig && cur_tok != tok_eof)
        {
            else_body.push_back(move(ParseEq()));
        }
        assertion(tok_rfig);
        getNextToken();
        return make_unique<IfExprAST>(move(cond), move(body), move(else_body));
    }
    return nullptr;
}

unique_ptr<ExprAST> ParseBinOp()
{
    if (cur_tok == tok_ident){
        string var(StrVal);
        getNextToken();
        if (cur_tok != tok_plus && cur_tok != tok_minus){
            return make_unique<VariableExprAST>(var);
        }
        char op = cur_tok == tok_plus ? '+' : '-';
        getNextToken();
        return make_unique<BinaryExprAST>(op, make_unique<VariableExprAST>(var), ParseBinOp());
    }
    else if (cur_tok == tok_number)
    {
        int number = NumVal;
        getNextToken();
        if (cur_tok != tok_plus && cur_tok != tok_minus){
            return make_unique<NumberExprAST>(number);
        }
        char op = cur_tok == tok_plus ? '+' : '-';
        getNextToken();
        return make_unique<BinaryExprAST>(op, make_unique<NumberExprAST>(number), ParseBinOp());
    }
    return nullptr;
}

int main(int argc, char *argv[])
{
    using namespace std;
    if (argc == 2){
            init_scanner(argv[1]);
            initialize_module();
            FunctionAST* f = Parse();
            f->codegen();
            mod->print(errs(), nullptr); 
    } else {
        cout<<"error"<<endl;
    }
    return 0;
}


