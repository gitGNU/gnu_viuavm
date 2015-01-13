#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::echo(byte* addr) {
    /*  Run echo instruction.
     */
    bool ref = false;
    int reg;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << "ECHO" << (ref ? " @" : " ") << reg;
        cout << endl;
    }

    if (ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    cout << fetch(reg)->str();

    if (debug) { cout << endl; }

    return addr;
}

byte* CPU::print(byte* addr) {
    /*  Run print instruction.
     */
    if (debug) {
        cout << "PRINT -> ";
    }
    addr = echo(addr);
    cout << '\n';
    return addr;
}


byte* CPU::move(byte* addr) {
    /** Run move instruction.
     *  Move an object from one register into another.
     */
    int a, b;
    bool a_ref = false, b_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    b_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    b = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << "COPY";
        cout << (a_ref ? " @" : " ") << a;
        cout << (b_ref ? " @" : " ") << b;
        cout << endl;
    }

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    registers[b] = registers[a];    // copy pointer from first-operand register to second-operand register
    registers[a] = 0;               // zero first-operand register

    return addr;
}
byte* CPU::copy(byte* addr) {
    /** Run move instruction.
     *  Copy an object from one register into another.
     */
    int a, b;
    bool a_ref = false, b_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    b_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    b = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << "COPY";
        cout << (a_ref ? " @" : " ") << a;
        cout << (b_ref ? " @" : " ") << b;
        cout << endl;
    }

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    place(b, fetch(a)->copy());

    return addr;
}
byte* CPU::ref(byte* addr) {
    /** Run ref instruction.
     *  Create a reference (implementation detail: copy a pointer) of an object in one register in
     *  another register.
     */
    int a, b;
    bool a_ref = false, b_ref = false;

    a_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    b_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    b = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << "REF";
        cout << (a_ref ? " @" : " ") << a;
        cout << (b_ref ? " @" : " ") << b;
        cout << endl;
    }

    if (a_ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }
    if (b_ref) {
        b = static_cast<Integer*>(fetch(b))->value();
    }

    registers[b] = registers[a];    // copy pointer
    references[b] = true;

    return addr;
}
byte* CPU::swap(byte* addr) {
    return addr;
}
byte* CPU::del(byte* addr) {
    return addr;
}
byte* CPU::isnull(byte* addr) {
    return addr;
}


byte* CPU::jump(byte* addr) {
    /*  Run jump instruction.
     */
    if (debug) {
        cout << "JUMP " << *(int*)addr << endl;
    }
    byte* target = bytecode+(*(int*)addr);
    if (target == addr) {
        throw "aborting: JUMP instruction pointing to itself";
    }
    return target;
}

byte* CPU::branch(byte* addr) {
    /*  Run branch instruction.
     */
    bool regcond_ref;
    int regcond_num;


    regcond_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    regcond_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    int addr_true = *((int*)addr);
    pointer::inc<int, byte>(addr);

    int addr_false = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << "BRANCH";
        cout << dec << (regcond_ref ? " @" : " ") << regcond_num;
        cout << " " << addr_true  << "::0x" << hex << (long)(bytecode+addr_true) << dec;
        cout << " " << addr_false << "::0x" << hex << (long)(bytecode+addr_false);
        cout << endl;
    }


    if (regcond_ref) {
        if (debug) { cout << "resolving reference to condition register" << endl; }
        regcond_num = static_cast<Integer*>(fetch(regcond_num))->value();
    }

    bool result = fetch(regcond_num)->boolean();

    addr = bytecode + (result ? addr_true : addr_false);

    return addr;
}
