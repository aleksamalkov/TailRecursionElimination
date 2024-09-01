#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

/// Check if a function is safe to be optimized if it contains tail recursion.
///
/// A function can't be optimized if its stack frame is not the same size in
/// every call or if its stack frame may be used by the callee.
bool isCandidate(const Function &F) {
  SmallSet<const Value *, 8> allocas;

  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto Alloca = dyn_cast<AllocaInst>(&I)) {
        if (!BB.isEntryBlock()) {
          errs() << "  Alloca outside of the entry block!\n";
          return false;
        }
        if (!Alloca->isStaticAlloca()) {
          errs() << "  Dynamic alloca!\n";
          return false;
        }
        allocas.insert(Alloca);
      } else {
        for (size_t i = 0; i != I.getNumOperands(); ++i) {
          // To avoid more complex analysis, we only optimize functions where
          // identifiers representing allocas are not used except in load and
          // store instructions (as a 2nd argument of store), so pointers to
          // caller's stack frame can't exist out of it.
          if (allocas.contains(I.getOperand(i)) &&
              !(I.getOpcode() == Instruction::Load ||
                (i == 1 && I.getOpcode() == Instruction::Store))) {
            errs() << "  Caller's stack frame may be used again!\n";
            return false;
          }
        }
      }
    }
  }
  return true;
}

/// Check if a BasicBlock can contain a tail recursion.
bool isCandidate(const BasicBlock &BB) {
  // We only look for a tail recursion if the block has either a ret
  // instruction or an unconditional branch to a block with a ret instruction.
  const Instruction *Terminator = BB.getTerminator();
  if (isa<ReturnInst>(Terminator))
    return true;

  if (auto BI = dyn_cast<BranchInst>(Terminator)) {
    if (BI->isConditional())
      return false;
    if (isa<ReturnInst>(BI->getSuccessor(0)->getTerminator()))
      return true;
  }
  return false;
}

/// Returns the last recursive call in a basic block, or nullptr.
CallInst *findLastRecursion(BasicBlock &BB) {
  BasicBlock::iterator It(BB.getTerminator());
  const auto Beg = BB.begin();
  while (It != Beg) {
    --It;
    if (auto Call = dyn_cast<CallInst>(It))
      if (Call->getCalledFunction() == BB.getParent())
        return Call;
  }
  return nullptr;
}

/// Check if a recursion can be eliminated even if this instruction is between
/// the call and the ret instruction, by adding an accumulator.
bool canAccumulate(const Instruction *I, const CallInst *Call) {
  return I->isAssociative() && I->isCommutative() && I->isBinaryOp() &&
         ((I->getOperand(0) == Call && I->getOperand(1) != Call) ||
          (I->getOperand(0) != Call && I->getOperand(1) == Call));
}

/// Finds calls which can be optimized.
class TailRecursionFinder {
public:
  /// Returns the first tail recursive call eligible for optimization, or
  /// nullptr.
  ///
  /// If `findAccInst` is true, finds functions which can be optimized by
  /// adding a variable as an accumulator.
  CallInst *find(Function &F, bool findAccInst = false);

  /// Get instruction which should be accumulated, if it exists.
  Instruction *accumlatorInstruction() { return AccumulatorInstruction; }

private:
  /// Check if a given recursive call is a tail call.
  ///
  /// A call is a tail call if there is nothing between it and the ret
  /// instruction. There can also be an unconditional jump between them. Load
  /// and store instructions can be there if they are loading and storing the
  /// result of the call. If the return type is not void, ret must return the
  /// result of the function. This function can also find functions which can
  /// become tail recursive by adding an accumulator.
  bool isTail(CallInst *Call, bool findAccInst = false);

  Instruction *AccumulatorInstruction{};
};

bool TailRecursionFinder::isTail(CallInst *Call, bool findAccInst) {
  BasicBlock *CallBB = Call->getParent();
  assert(isCandidate(CallBB));

  const Value *ReturnValueStore{};
  const Value *ReturnValueLoad{};
  assert(!AccumulatorInstruction);

  Instruction *Terminator = CallBB->getTerminator();
  BasicBlock::iterator It(Call);
  ++It;
  while (It->getOpcode() != Instruction::Ret) {
    if (&*It == Terminator) {
      assert(dyn_cast<BranchInst>(Terminator));
      assert(!(dyn_cast<BranchInst>(Terminator)->isConditional()));
      BasicBlock *BB = Terminator->getSuccessor(0);
      It = BB->begin();
      Terminator = BB->getTerminator();
      continue;
    }

    if (It->isVolatile()) {
      errs() << "  Volatile instruction!\n";
      return false;
    } else if (auto Store = dyn_cast<StoreInst>(It)) {
      if (ReturnValueStore == nullptr &&
          ((!AccumulatorInstruction && Store->getOperand(0) == Call) ||
           (AccumulatorInstruction &&
            Store->getOperand(0) == AccumulatorInstruction))) {
        ReturnValueStore = Store->getOperand(1);
      } else {
        errs() << "  Bad store!\n";
        return false;
      }
    } else if (auto Load = dyn_cast<LoadInst>(It)) {
      if (ReturnValueLoad != nullptr ||
          Load->getOperand(0) != ReturnValueStore) {
        errs() << "  Bad load!\n";
        return false;
      }
      ReturnValueLoad = Load;
    } else if (findAccInst && !AccumulatorInstruction &&
               canAccumulate(&*It, Call)) {
      AccumulatorInstruction = &*It;
      errs() << "  Instruction can be accumulated\n";
    } else {
      errs() << "  Bad instruction!\n";
      return false;
    }

    ++It;
  }

  auto Ret = dyn_cast<ReturnInst>(Terminator);
  assert(Ret && "Terminator should be a ret instruction.");

  if (CallBB->getParent()->getReturnType()->isVoidTy())
    return true;
  Value *ReturnValue = Ret->getReturnValue();
  if (ReturnValue == Call ||
      (AccumulatorInstruction && ReturnValue == AccumulatorInstruction) ||
      (ReturnValueLoad && ReturnValue == ReturnValueLoad)) {
    return true;
  }

  errs() << "  Bad return!\n";
  return false;
}

CallInst *TailRecursionFinder::find(Function &F, bool findAccInst) {
  errs() << "Looking for tail recursion in function: ";
  errs().write_escaped(F.getName()) << '\n';

  AccumulatorInstruction = nullptr;

  if (!isCandidate(F)) {
    errs() << "Function can't be optimized.\n";
    return nullptr;
  }
  for (auto &BB : F) {
    if (!isCandidate(BB))
      continue;
    if (auto Call = findLastRecursion(BB)) {
      errs() << "Found a recursion.\n";
      if (isTail(Call, findAccInst)) {
        errs() << "Found a tail recursion in ";
        errs().write_escaped(F.getName()) << ".\n\n";
        return Call;
      } else {
        errs() << "Not a tail recursion.\n";
      }
    }
  }

  errs() << "No tail recursion in ";
  errs().write_escaped(F.getName()) << ".\n\n";
  return nullptr;
}

//adds a label to jump to when call is eliminated
void addLabel(Function &F){

  BasicBlock &BB = F.getEntryBlock();
  
  int i = F.arg_size();
  errs() << "DBG: num of funcs arg: " << i << "\n";

  for(Instruction &I : BB){
    
    if(isa<AllocaInst>(&I) && i!=0){
      continue;
    }
    
    if(i==0){
      BasicBlock *newBB = BB.splitBasicBlock(&I, "start");
      break;
    }
    i--;

  }

  errs() << "DBG: addLabel comes to an end\n";
}

//removes call and everything after
void eliminateCall(Function &F, CallInst *Call){

  std::vector<Instruction *> InstructionsToRemove;
  BasicBlock *BB = Call->getParent();

  bool remove = false;
  for(Instruction &I : *BB){
    
    if(&I == Call || remove){
      remove = true;
      InstructionsToRemove.push_back(&I);
    }
    
  }

  for(Instruction *I : InstructionsToRemove){
    errs() << "\tDBG: erasing " << I->getOpcodeName() << "\n";
    I->eraseFromParent();
  }

  errs() << "DBG: eliminateCall comes to an end\n";
}

//inserts a branch to start before call
void insertBr(Function &F, CallInst *Call){

  BasicBlock *startBB = nullptr;
  for(BasicBlock &BB : F){
    if(BB.getName() == "start"){
      startBB = &BB;
    }
  }

  if(startBB == nullptr){
    errs() << "No start Basic Block found!\n";
    return;
  }

  //auto *newBr = new BranchInst(startBB, Call); ??????????????????????
  auto *newBr = BranchInst::Create(startBB);
  newBr->insertBefore(Call);

  errs() << "DBG: insertBr comes to an end\n";
}

struct TRE : public FunctionPass {

  std::vector<Value *> ArgsLoc;

  static char ID; // Pass identification, replacement for typeid
  TRE() : FunctionPass(ID) {}


  //places function arguments' location in a map
  void placeArgInMap(Function &F){  

    BasicBlock &BB = F.getEntryBlock(); 

    Instruction *storeI;
    for(Instruction &I : BB){ 

      if(isa<StoreInst>(I)){
       storeI = &I;
       break;
      }
    } 

    ArgsLoc.clear();
    while(isa<StoreInst>(storeI)){
      //errs() << "\tDBG: store from entry block: " << *storeI << "\n";
      ArgsLoc.push_back(storeI->getOperand(1)); 
      storeI = storeI->getNextNode();
    }  

    errs() << "DBG: placeArgInMap comes to an end\n";
  } 

  //takes values of call arguments and stores them in function arguments' locations
  void createStoreInst(Function &F, CallInst *Call){  

    //errs() << "\t\t DBG: call get num operands" << Call->arg_size() << "\n";

    for(int i=0; i < Call->arg_size(); i++){

      Value *arg = Call->getArgOperand(i);
      //errs() << "\t DBG: in for loop before creating store\n";
      auto *newStore = new StoreInst(arg, ArgsLoc[i], Call); // ?: inserting store before call?
      
      if(newStore == nullptr){
        errs() << "\tno store instruction made\n";
        return;
      }

      errs() << "\tDBG: newstore: " << *newStore << "\n";
    }

    errs() << "DBG: createStoreInst comes to an end\n";
  } 



  bool runOnFunction(Function &F) override {
      TailRecursionFinder Finder;

      if(auto *Call = Finder.find(F, true)){
        // TODO: ako ovo nije nullptr, treba napraviti akumulator
        Instruction *AccumulatorInstruction = Finder.accumlatorInstruction();
        
        placeArgInMap(F);
        addLabel(F);

        createStoreInst(F, Call);
        insertBr(F, Call);

        eliminateCall(F, Call);

        return true;
      }
      
      else return false;
    
  }
};
} // namespace

char TRE::ID = 0;
static RegisterPass<TRE> X("our-tre", "Our Tail Recursion Elimination Pass");
