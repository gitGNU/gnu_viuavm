#include <iostream>
#include <algorithm>
#include <map>
#include <string>
#include "src/bytecode/opcodes.h"
#include "src/bytecode/maps.h"
using namespace std;

int main() {
    for (pair<const OPCODE, string> i : OP_NAMES) {
        cout << i.second << ":\t";
        cout << i.first << " (0x" << hex << i.first << dec << "), size: ";
        cout << OP_SIZES.at(i.second) << '\n';
    }
    cout << flush;
    return 0;
}
