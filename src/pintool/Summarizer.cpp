#include "pin.H"
#include <iostream>
#include <fstream>
using std::cerr;
using std::endl;
using std::string;

/* ================================================================== */
// Global variables
/* ================================================================== */

// number of dynamically executions per instruction
std::map<int, int> insCount = {}; 

std::ostream* out = &cerr;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */

KNOB< string > KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "summary.out", "specify file name for MyPinTool output");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

INT32 Usage() {
    cerr << "This tool prints out the number of dynamically executed " << endl
         << "instructions, basic blocks and threads in the application." << endl
         << endl;

    cerr << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */


VOID countInstruction(UINT32 opcode) {

}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

VOID Instruction(INS ins, VOID* v) {
    UINT32 opcode = INS_Opcode(ins);
    if (!insCount.count(opcode)) {
        insCount[opcode] = 0;
    }
    insCount[opcode]++;
    // INS_InsertCall(
    //     ins, 
    //     IPOINT_BEFORE,
    //     (AFUNPTR)countInstruction,
    //     IARG_UINT32,
    //     INS_Opcode(ins),
    //     IARG_END
    // );
}

VOID Fini(INT32 code, VOID* v) {
    for (const auto& pair : insCount) {
        *out << OPCODE_StringShort(pair.first) << "," << pair.second << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid
    if (PIN_Init(argc, argv))
    {
        return Usage();
    }

    string fileName = KnobOutputFile.Value();

    if (!fileName.empty())
    {
        out = new std::ofstream(fileName.c_str());
    }

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    cerr << "===============================================" << endl;
    cerr << "Running summarizer" << endl;
    if (!KnobOutputFile.Value().empty())
    {
        cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
    }
    cerr << "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
