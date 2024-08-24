#include "llvm/ADT/Statistic.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

//std::unordered_map<Value *, Value *> ArgsMap;
std::vector<Value *> ArgsLoc;

/// Check if a function can be optimized if it contains tail recursion.
bool isCandidate(const Function &F) {
  // TODO implement
  return true;
}

/// Check if a BasicBlock can contain tail recursion.
bool isCandidate(const BasicBlock &BB) {
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
  CallInst *Recursion = nullptr;
  for (auto &I : BB)
    if (auto Call = dyn_cast<CallInst>(&I))
      if (Call->getCalledFunction() == BB.getParent())
        Recursion = Call;
  return Recursion;
}

/// Check if a given recursive call is a tail call.
bool isTail(const CallInst *Call) {
  const BasicBlock *BB = Call->getParent();
  const Instruction *Terminator = BB->getTerminator();
  bool isVoid = BB->getParent()->getReturnType()->isVoidTy();

  const Value *ReturnValueStore{};
  const Value *ReturnValueLoad{};

  BasicBlock::const_iterator It(Call);
  for (++It; &*It != Terminator; ++It) {
    if (It->isVolatile()) {
      return false;
    }
    if (auto Store = dyn_cast<StoreInst>(It)) {
      if (ReturnValueStore != nullptr || Store->getOperand(0) != Call) {
        errs() << "  Bad store!\n";
        return false;
      }
      ReturnValueStore = Store->getOperand(1);
    } else if (auto Load = dyn_cast<LoadInst>(It)) {
      if (ReturnValueLoad != nullptr ||
          Load->getOperand(0) != ReturnValueStore) {
        errs() << "  Bad load!\n";
        return false;
      }
      ReturnValueLoad = Load;
    } else {
      errs() << "  Bad instruction!\n";
      return false;
    }
  }

  if (auto Ret = dyn_cast<ReturnInst>(Terminator)) {
    if (isVoid || Ret->getReturnValue() == Call ||
        Ret->getReturnValue() == ReturnValueLoad)
      return true;
    errs() << "  Bad return!\n";
    return false;
  }

  if (auto Br = dyn_cast<BranchInst>(Terminator)) {
    if (Br->isConditional()) {
      errs() << "  Branch is conditional!\n";
      return false;
    }
    auto Succ = Br->getSuccessor(0);

    // TODO remove code duplication

    for (auto &Instr : *Succ) {
      auto I = &Instr;
      if (I->isVolatile()) {
        return false;
      }
      if (auto Store = dyn_cast<StoreInst>(I)) {
        if (ReturnValueStore != nullptr || Store->getOperand(0) != Call) {
          errs() << "  Bad store!\n";
          return false;
        }
        ReturnValueStore = Store->getOperand(1);
      } else if (auto Load = dyn_cast<LoadInst>(I)) {
        if (ReturnValueLoad != nullptr ||
            Load->getOperand(0) != ReturnValueStore) {
          errs() << "  Bad load!\n";
          return false;
        }
        ReturnValueLoad = Load;
      } else if (auto Ret = dyn_cast<ReturnInst>(I)) {
        if (isVoid || Ret->getReturnValue() == Call ||
            Ret->getReturnValue() == ReturnValueLoad)
          return true;
        errs() << "  Bad return!\n";
        return false;
      } else {
        errs() << "  Bad instruction!\n";
        return false;
      }
    }
  }
  errs() << "  Bad terminator!\n";
  return false;
}

/// Returns the first tail recursive call eligible for optimization, or nullptr.
CallInst *findTailRecursion(Function &F) {
  errs() << "Looking for tail recursion in function: ";
  errs().write_escaped(F.getName()) << '\n';

  if (!isCandidate(F)) {
    errs() << "Function can't be optimized.\n";
    return nullptr;
  }
  for (auto &BB : F) {
    if (!isCandidate(BB))
      continue;
    if (auto Call = findLastRecursion(BB)) {
      errs() << "Found a recursion.\n";
      if (isTail(Call)) {
        errs() << "Found a tail recursion in ";
        errs().write_escaped(F.getName()) << ".\n\n";
        return Call;
      } else {
        errs() << "Not a tail recursion.\n";
        return nullptr;
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
  
  for(Instruction &I : BB){
    if(!isa<StoreInst>(&I) && !isa<AllocaInst>(&I)){
      BasicBlock *newBB = BB.splitBasicBlock(&I, "start");
      break;
    }
  }

}

//removes call and everything after
void eliminateCall(Function &F){

  std::vector<Instruction *> InstructionsToRemove;
  auto Call = findTailRecursion(F);
  BasicBlock *BB = Call->getParent();

  bool remove = false;
  for(Instruction &I : *BB){
    
    if(&I == Call || remove){
      remove = true;
      InstructionsToRemove.push_back(&I);
    }
  }

  for(Instruction *I : InstructionsToRemove){
    I->eraseFromParent();
  }
}

//places function's argument's location in a map
void placeArgInMap(Function &F){

  BasicBlock &BB = F.getEntryBlock();

  Instruction *storeI;
  for(Instruction &I : BB){

    if(isa<StoreInst>(I)){
     storeI = &I;
    }
  }

  while(isa<StoreInst>(storeI)){
    ArgsLoc.push_back(storeI->getOperand(1)); 
    storeI = storeI->getNextNode();
  }

}

struct TRE : public FunctionPass {
  static char ID; // Pass identification, replacement for typeid
  TRE() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    
          //TODO 
          //     -insert start label +
          //     -Create instructions to remove collection +
          //     -eliminate call and instructions after call +
          //     -remember function arguments' locations +
          //     -insert store inst:
          //        -get call operands 
          //        -store operands in location of function arguments
          //     -insert goto start
      
    return true;
    
  }
};
} // namespace

char TRE::ID = 0;
static RegisterPass<TRE> X("our-tre", "Our Tail Recursion Elimination Pass");