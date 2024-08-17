add_llvm_library( LLVMOurTRE MODULE BUILDTREE_ONLY
  OurTailRecursionElimination.cpp

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
