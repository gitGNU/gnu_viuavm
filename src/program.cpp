#include <cstdint>
#include <iostream>
#include <sstream>
#include <viua/support/string.h>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/maps.h>
#include <viua/program.h>
using namespace std;


byte* Program::bytecode() {
    /*  Returns pointer to a copy of the bytecode.
     *  Each call produces new copy.
     *
     *  Calling code is responsible for proper destruction of the allocated memory.
     */
    byte* tmp = new byte[bytes];
    for (int i = 0; i < bytes; ++i) {
        tmp[i] = program[i];
    }
    return tmp;
}

Program& Program::fill(byte* code) {
    /** Fill generator with pre-generated bytecode.
     *
     *  Previous copy is deleted.
     *  Calling code must not delete passed bytecode.
     */
    delete[] program;
    program = code;
    addr_ptr = program;
    return (*this);
}


Program& Program::setdebug(bool d) {
    /** Sets debugging status.
     */
    debug = d;
    return (*this);
}

Program& Program::setscream(bool d) {
    /** Sets screaming status.
     */
    scream = d;
    return (*this);
}


int Program::size() {
    /*  Returns program size in bytes.
     */
    return bytes;
}

int Program::instructionCount() {
    /** Returns total number of instructions in the program.
     *
     *  This function should only be called after the program is fully constructed as the size is calculated by
     *  performing bytecode analysis.
     */
    int counter = 0;
    for (int i = 0; i < bytes; ++i) {
        switch (program[i]) {
            case IADD:
            case ISUB:
            case IMUL:
            case IDIV:
            case ILT:
            case ILTE:
            case IGT:
            case IGTE:
            case IEQ:
            case BRANCH:
                i += 3 * sizeof(int);
                break;
            case ISTORE:
                i += 2 * sizeof(int);
                break;
            case IINC:
            case IDEC:
            case PRINT:
            case JUMP:
                i += sizeof(int);
                break;
        }
        ++counter;
    }
    return counter;
}


uint16_t Program::countBytes(const vector<string>& lines) {
    /** Counts bytecode size required for a program.
     *
     *  Knowing how many instructions are in a program, and
     *  size of each instruction, required bytecode size can
     *  be calculated by a simple for loop adding sizes of
     *  instructions it encounters.
     *
     *  Passed lines must be sanitized, i.e. the comments and blanks must be removed.
     */
    uint16_t bytes = 0;
    int inc = 0;
    string instr, line;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);

        if (line[0] == '.') {
            /*  Assembler annotations must be skipped here or they would cause the code below to
             *  throw exceptions.
             */
            continue;
        }

        instr = "";
        inc = 0;

        instr = str::chunk(line);
        try {
            inc = OP_SIZES.at(instr);
            if (instr == "try") {
                // clear first chunk
                line = str::lstrip(str::sub(line, instr.size()));
                // get second chunk (function or block name)
                inc += str::chunk(line).size() + 1;
            } else if ((instr == "call") or (instr == "msg")) {
                // clear first chunk (opcode mnemonic)
                line = str::lstrip(str::sub(line, instr.size()));
                // get second chunk (optional register index)
                string first_op = str::chunk(line);
                line = str::lstrip(str::sub(line, first_op.size()));
                // get third chunk (function name if register index was given, empty otherwise)
                string second_op = str::chunk(line);
                inc += (second_op.size() ? second_op : first_op).size() + 1;
            } else if ((instr == "closure") or (instr == "function") or (instr == "class") or (instr == "prototype") or (instr == "derive") or (instr == "new")) {
                // clear first chunk (opcode mnemonic)
                line = str::lstrip(str::sub(line, instr.size()));
                // clear second chunk (register index)
                line = str::lstrip(str::sub(line, str::chunk(line).size()));
                // get third chunk (name)
                inc += str::chunk(line).size() + 1;
            } else if (instr == "attach") {
                // clear first chunk (opcode mnemonic)
                line = str::lstrip(str::sub(line, instr.size()));
                // clear second chunk (register index)
                line = str::lstrip(str::sub(line, str::chunk(line).size()));
                // get third chunk (function name)
                inc += str::chunk(line).size() + 1;
                line = str::lstrip(str::sub(line, str::chunk(line).size()));
                // get fourth chunk (method name)
                inc += str::chunk(line).size() + 1;
            } else if (instr == "import") {
                // clear first chunk
                line = str::lstrip(str::sub(line, instr.size()));
                // get second chunk (which is a string)
                inc += (str::extract(line).size() - 2 + 1); // +1: null-terminator, -2: quotes around module name
            } else if (instr == "link") {
                // clear first chunk
                line = str::lstrip(str::sub(line, instr.size()));
                // get second chunk (which is a module name)
                inc += (str::extract(line).size() + 1); // +1: null-terminator
            } else if (instr == "catch") {
                // clear first chunk (the opcode)
                line = str::lstrip(str::sub(line, instr.size()));
                // get second chunk (the type as a string)
                inc += (str::extract(line).size() - 2 + 1); // +1: null-terminator, -2: quotes
                line = str::lstrip(str::sub(line, str::extract(line).size()));
                // get third chunk (which is a block name)
                inc += str::chunk(line).size() + 1;
            } else if (instr == "strstore") {
                // clear first chunk
                line = str::lstrip(str::sub(line, instr.size()));
                line = str::lstrip(str::sub(line, str::chunk(line).size()));
                // get third chunk (which is a string)
                inc += (str::extract(line).size() - 2 + 1); // +1: null-terminator
            }
        } catch (const std::out_of_range &e) {
            throw ("unrecognised instruction: `" + instr + '`');
        }

        if (inc == 0) {
            throw ("fail: line is not empty and requires 0 bytes: " + line);
        }

        bytes += inc;
    }

    return bytes;
}


int Program::getInstructionBytecodeOffset(int instr, int count) {
    /** Returns bytecode offset for given instruction index.
     *
     *  The "count" parameter is there to pass assumed instruction count to
     *  avoid recalculating it on every call to this function.
     *  It can be passed as 0 to trigger calculation.
     */

    // check if instruction count was passed, and calculate it if not
    count = (count >= 0 ? count : instructionCount());

    int offset = 0;
    int inc;
    for (int i = 0; i < (instr >= 0 ? instr : count+instr); ++i) {
        /*  This loop iterates over so many instructions as needed to find bytecode offset for requested instruction.
         *
         *  Each time, the offset is increased by `inc` - which is equal to *1 plus size of operands of instructions at current index*.
         */
        string opcode_name;
        try {
            opcode_name = OP_NAMES.at(OPCODE(program[offset]));
        } catch (const std::out_of_range& e) {
            ostringstream oss;
            oss << "instruction not found in OP_NAMES: " << OPCODE(program[offset]);
            throw oss.str();
        }


        try {
            inc = OP_SIZES.at(opcode_name);
        } catch (const std::out_of_range& e) {
            throw ("instruction " + opcode_name + " not found in OP_SIZES");
        }

        if (scream) {
            cout << "[asm] debug: offsetting: " << opcode_name << ": +" << inc;
        }

        OPCODE opcode = OPCODE(program[offset]);
        if ((opcode == IMPORT) or (opcode == TRY) or (opcode == LINK)) {
            string s(program+offset+1);
            if (scream) {
                cout << '+' << s.size() << " (function/module name at byte " << offset+1 << ": `" << s << "`)";
            }
            inc += s.size()+1;
        }
        if ((opcode == CALL) or (opcode == CLOSURE) or (opcode == FUNCTION) or
            (opcode == CLASS) or (opcode == PROTOTYPE) or (opcode == DERIVE) or (opcode == NEW) or (opcode == MSG)) {
            string s(program+offset+sizeof(bool)+sizeof(int)+1);
            if (scream) {
                cout << '+' << s.size() << " (function/module/class name at byte " << offset+1 << ": `" << s << "`)";
            }
            inc += s.size()+1;
        }
        if (opcode == ATTACH) {
            string f(program+offset+sizeof(bool)+sizeof(int)+1);
            inc += f.size()+1;
            string m(program+offset+sizeof(bool)+sizeof(int)+1+f.size()+1);
            inc += m.size()+1;
        }
        if (opcode == STRSTORE) {
            string s(program+offset+inc);
            if (scream) {
                cout << '+' << s.size()+1 << " (string at byte " << offset+inc << ": `" << s << "`)";
            }
            inc += s.size()+1;
        }

        if (scream) {
            cout << " bytes" << endl;
        }

        offset += inc;
        if (offset+1 > bytes) {
            cout << "instruction offset out of bounds: check your branches: ";
            cout << "offset/bytecode size: ";
            cout << offset << '/' << bytes << endl;
            throw "instruction offset out of bounds: check your branches";
        }
    }
    return offset;
}

// FIXME: is unused, scheduled for removal
Program& Program::calculateBranches(unsigned offset) {
    /*  This function should be called after program is constructed
     *  to calculate correct bytecode offsets for BRANCH and JUMP instructions.
     */
    int instruction_count = instructionCount();
    int* ptr;

    for (unsigned i = 0; i < branches.size(); ++i) {
        ptr = (int*)(branches[i]);
        cout << "[brch] calculating jump at " << (int)(branches[i]-program) << ", " << hex << (long)branches[i] << dec << " (target: " << *ptr << ") with offset " << offset << " = ";
        (*ptr) = offset + getInstructionBytecodeOffset(*ptr, instruction_count);
        cout << *ptr << endl;
    }

    for (unsigned i = 0; i < branches_absolute.size(); ++i) {
        ptr = (int*)(branches_absolute[i]);
        cout << "[brch] calculating jump at " << (int)(branches_absolute[i]-program) << ", " << hex << (long)branches_absolute[i] << dec << " (target: " << *ptr << ") with offset " << 0 << " = ";
        (*ptr) = getInstructionBytecodeOffset(*ptr, instruction_count);
        cout << *ptr << endl;
    }

    return (*this);
}

Program& Program::calculateJumps(vector<tuple<int, int> > jump_positions) {
    /** Calculate jump targets in given bytecode.
     */
    int instruction_count = instructionCount();
    int* ptr;

    int position, offset, adjustment;
    for (tuple<int, int> jmp : jump_positions) {
        tie(position, offset) = jmp;
        ptr = (int*)(program+position);
        if (debug) {
            cout << "[bcgen:jump] calculating jump at " << position << " (target: " << *ptr << ") with offset " << offset << endl;
        }
        adjustment = getInstructionBytecodeOffset(*ptr, instruction_count);
        (*ptr) = offset + adjustment;
        if (debug) {
            cout << "[bcgen:jump] calculated jump at " << position << " (total: " << adjustment << ") with offset " << offset << " = ";
            cout << *ptr << endl;
        }
    }

    return (*this);
}

vector<unsigned> Program::jumps() {
    /** Returns vector if bytecode points which contain jumps.
     */
    vector<unsigned> jmps;
    for (byte* jmp : branches) { jmps.push_back( (unsigned)(jmp-program) ); }
    return jmps;
}

vector<unsigned> Program::jumpsAbsolute() {
    /** Returns vector if bytecode points which contain absolute jumps.
     */
    vector<unsigned> jmps;
    for (byte* jmp : branches_absolute) { jmps.push_back( (unsigned)(jmp-program) ); }
    return jmps;
}
