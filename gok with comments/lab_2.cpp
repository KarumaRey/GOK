#include "llvm/ADT/APInt.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/IRBuilder.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
//obj dump
using namespace llvm;

int main() {

    LLVMContext context; // чисто по лекции
    Module *module = new Module("gen-opt-lab2", context); // название лабы ;)
    IRBuilder<> builder(context); // делает блоки
    FunctionType *funcType = FunctionType::get(builder.getInt32Ty(), false); 
    //определяем тип функции (возвращает инт значение)
//(надо реализовать сумму)
    
    Function *mainFunc = Function::Create(funcType, Function::ExternalLinkage, "main", module);
    // создаем функцию, название

    BasicBlock *entry = BasicBlock::Create( //базовый блок создаем энтри
        context, 
        "entrypoint", 
        mainFunc); // передали функциб мэйн

    builder.SetInsertPoint(entry); //устанавливаем точку входу и устанавливаем ее в энтри

    Value *a_const = ConstantInt::get(Type::getInt32Ty(context), 353); //тип,значение
    // создаем 2 переменные которые складывать будем 
        //через переменные результат будет в скобочках цифра + цифра была бы
        //потому что константы 
    Value *b_const = ConstantInt::get(Type::getInt32Ty(context), 48); 
    
     // создаем переменные которые будут возвращаться   
    Value *return_value = builder.CreateAdd(a_const, b_const, "retVal"); 
    //задает само значение которая функция вернет
    builder.CreateRet(return_value);  
    
	//вывод в консоль
    module->dump();
    return 0;
}

// 
// clang++ lab_2.cpp `llvm-config --cxxflags --ldflags --system-libs --libs engine interpreter` -lffi
//./a.out
// int main() {
//      return 353 + 48;
// }
