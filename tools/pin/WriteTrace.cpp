/**
 * 本工具通过对指令的插桩追踪应用所有的写存操作
 *
 * 本工具的统计结果将输出在 "WriteTrace.out"中
 */
#include "pin.H"
#include <fstream>
#include <iostream>

INT32 Usage();
VOID InsInstruction(INS ins, VOID *v);
VOID MemWrite(ADDRINT addr);
VOID FiniFunction(INT32 code, VOID *v);

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
std::ofstream TraceFile;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o",
                            "WriteTrace.out", "specify trace file name");

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
int main(int argc, char *argv[]) {
  // Initialize pin
  if (PIN_Init(argc, argv))
    return Usage();

  // Write to a file since cout and cerr maybe closed by the application
  TraceFile.open(KnobOutputFile.Value().c_str());
  TraceFile << hex;
  TraceFile.setf(ios::showbase);

  // Register Instruction to be called to instrument instructions
  PIN_SetSyntaxIntel();
  INS_AddInstrumentFunction(InsInstruction, 0);

  // Register Function to be called when program finished.
  PIN_AddFiniFunction(FiniFunction, 0);

  // Start the program, never returns
  PIN_StartProgram();

  return 0;
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage() {
  cerr << "This tool produces a trace of calls to malloc." << endl;
  cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
  return -1;
}

/* ===================================================================== */
/* 对指令进行插桩：在执行对内存写操作时执行函数MemWrite，传递内存地址作为参数 */
/* ===================================================================== */
VOID InsInstruction(INS ins, VOID *v) {
  UINT32 memOperands = INS_MemoryOperandCount(ins);
  if (INS_MemoryOperandIsWritten(ins, 0)) {
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
      INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemWrite, IARG_MEMORYOP_EA,
                     memOp, IARG_END);
  }
}

/* ===================================================================== */
VOID MemWrite(ADDRINT addr) {
    TraceFile << "W:" << std::hex << addr << endl;
}

/* ===================================================================== */
VOID FiniFunction(INT32 code, VOID *v) {
  std::cout << "Program Fishished: Records will write to \"WriteTrace.out\"."
            << endl;
  TraceFile.close();
}