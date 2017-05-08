/**
 * 本工具通过对函数的插桩获得程序malloc获得的内存地址区间
 * 通过对指令的插桩对程序对所申请内存的写进行计数统计
 *
 * 本工具通过预常量定义malloc名
 * 本工具的统计结果将输出在 "MemTrack.out"中
 */
#include "pin.H"
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

const char MALLOC[10] = "malloc";

INT32 Usage();
VOID InsInstruction(INS ins, VOID *v);
VOID InsFunction(IMG img, VOID *v);
VOID MemWrite(ADDRINT addr);
VOID BeforeMalloc(ADDRINT size);
VOID AfterMalloc(ADDRINT addr);
VOID FiniFunction(INT32 code, VOID *v);

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
std::ofstream TraceFile;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "MemTrack.out",
                            "specify trace file name");

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
int main(int argc, char *argv[]) {
  // Initialize pin
  PIN_InitSymbols();
  if (PIN_Init(argc, argv))
    return Usage();

  // Write to a file since cout and cerr maybe closed by the application
  TraceFile.open(KnobOutputFile.Value().c_str());
  TraceFile << hex;
  TraceFile.setf(ios::showbase);

  // Register Instruction to be called to instrument instructions
  PIN_SetSyntaxIntel();
  INS_AddInstrumentFunction(InsInstruction, 0);

  // Register Image to be called to instrument functions.
  IMG_AddInstrumentFunction(InsFunction, 0);

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
  // 记录写地址
  TraceFile << "Access: Write to " << std::hex << addr << endl;
}

/* ===================================================================== */
/* 对函数进行插桩：在执行malloc之前及之后插桩，以记录申请内存地址 */
/* ===================================================================== */

VOID InsFunction(IMG img, VOID *v) {
  //  寻找函数
  RTN mallocRtn = RTN_FindByName(img, MALLOC);
  if (RTN_Valid(mallocRtn)) {
    RTN_Open(mallocRtn);
    // 对函数插桩
    RTN_InsertCall(mallocRtn, IPOINT_BEFORE, (AFUNPTR)BeforeMalloc,
                   IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
    RTN_InsertCall(mallocRtn, IPOINT_AFTER, (AFUNPTR)AfterMalloc,
                   IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);

    RTN_Close(mallocRtn);
  }
}

/* ===================================================================== */
VOID BeforeMalloc(ADDRINT size) {
  // 记录申请内存大小
  TraceFile << "Malloc: Require size " << size << endl;
}

/* ===================================================================== */
VOID AfterMalloc(ADDRINT addr) {
  // 记录返回地址
  TraceFile << "Malloc: Return addr " << std::hex << addr << endl;
}

/* ===================================================================== */
VOID FiniFunction(INT32 code, VOID *v) {
  std::cout << "Program Fishished: Records will write to \"MemTrack.out\"."
            << endl;
  TraceFile.close();
}