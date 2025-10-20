#include "pin.H"
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>

using std::cerr;
using std::endl;
using std::string;

/* ================================================================== */
// Global variables
/* ================================================================== */

// number of dynamically executions per instruction (global)
static std::map<UINT32, UINT64> insCount = {};

// per-routine opcode counts: rtn id -> (opcode -> count)
static std::map<UINT32, std::map<UINT32, UINT64>> perRtnCounts;

// per-address opcode counts for instructions that don't belong to a known RTN
static std::map<ADDRINT, std::map<UINT32, UINT64>> perAddrCounts;

// map rtn id to name for printing
static std::map<UINT32, std::string> rtnNames;

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

// fast analysis function: increment global and per-routine counts
VOID PIN_FAST_ANALYSIS_CALL CountInstr(UINT32 rtnId, UINT32 opcode) {
    insCount[opcode]++;
    perRtnCounts[rtnId][opcode]++;
}

// fast analysis function: increment global and per-address counts
VOID PIN_FAST_ANALYSIS_CALL CountAddrInstr(ADDRINT addr, UINT32 opcode) {
    insCount[opcode]++;
    perAddrCounts[addr][opcode]++;
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

VOID Trace(TRACE trace, VOID* v) {
    ADDRINT addr = TRACE_Address(trace);
    // Only analyze instructions that belong to the main executable image
    IMG img = IMG_FindByAddress(addr);
    if (!IMG_Valid(img) || !IMG_IsMainExecutable(img)) return;

    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
            Instruction(ins, nullptr);
        }
    }
}

VOID Instruction(INS ins, VOID* v) {
    UINT32 opcode = INS_Opcode(ins);

    RTN rtn = INS_Rtn(ins);
    UINT32 rtnId = 0;
    if (RTN_Valid(rtn)) {
        rtnId = RTN_Id(rtn);
        // Record name if we haven't seen it for dynamically generated routines
        if (!rtnNames.count(rtnId)) {
            rtnNames[rtnId] = Demangle(RTN_Name(rtn).c_str());
        }
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CountInstr,
                       IARG_FAST_ANALYSIS_CALL, IARG_UINT32, rtnId, IARG_UINT32, opcode, IARG_END);
    } else {
        // No RTN available: instrument by address
        ADDRINT addr = INS_Address(ins);
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CountAddrInstr,
                       IARG_FAST_ANALYSIS_CALL, IARG_ADDRINT, addr, IARG_UINT32, opcode, IARG_END);
    }
}

VOID Fini(INT32 code, VOID* v) {
    // Per-routine summary
    for (const auto& rtnPair : perRtnCounts) {
        UINT32 rtnId = rtnPair.first;
        const auto& mapOpcode = rtnPair.second;

        std::string name = "[UNKNOWN]";
        if (rtnNames.count(rtnId)) name = rtnNames[rtnId];

        *out << "Function: " << name << std::endl;
        for (const auto& opPair : mapOpcode) {
            UINT32 opcode = opPair.first;
            UINT64 count = opPair.second;
            *out << "  " << OPCODE_StringShort(opcode) << ": " << count << std::endl;
        }
        *out << std::endl;
    }

    // Global summary
    *out << "Global opcode counts:" << std::endl;
    for (const auto& pair : insCount) {
        *out << OPCODE_StringShort(pair.first) << "," << pair.second << std::endl;
    }

        // Print per-address summaries for code that had no RTN
        if (!perAddrCounts.empty()) {
            *out << std::endl << "Per-address summaries (unnamed routines):" << std::endl;
            for (const auto& addrPair : perAddrCounts) {
                ADDRINT addr = addrPair.first;
                *out << "Address: 0x" << std::hex << addr << std::dec << std::endl;
                for (const auto& opPair : addrPair.second) {
                    UINT32 opcode = opPair.first;
                    UINT64 count = opPair.second;
                    *out << "  " << OPCODE_StringShort(opcode) << ": " << count << std::endl;
                }
                *out << std::endl;
            }
        }

    if (out != &cerr) {
        // flush and close file if we opened one
        std::ofstream* fout = static_cast<std::ofstream*>(out);
        fout->flush();
        fout->close();
    }
}

// Record routine names for routines in the main executable only
VOID ImageLoad(IMG img, VOID* v)
{
    if (!IMG_IsMainExecutable(img)) return;

    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
    {
        for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
        {
            UINT32 id = RTN_Id(rtn);
            if (!rtnNames.count(id)) {
                rtnNames[id] = Demangle(RTN_Name(rtn).c_str());
            }
        }
    }
}

int main(int argc, char* argv[]) {
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid
    if (PIN_Init(argc, argv))
    {
        return Usage();
    }
    PIN_InitSymbols();

    string fileName = KnobOutputFile.Value();

    if (!fileName.empty())
    {
        out = new std::ofstream(fileName.c_str());
    }

    TRACE_AddInstrumentFunction(Trace, 0);
    IMG_AddInstrumentFunction(ImageLoad, 0);
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
