/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <viua/bytecode/operand_types.h>
#include <viua/cpu/opex.h>


/** NOTICE
 *
 *  In this file there are lots of reinterpret_cast<>'s.
 *  It is in bad style, but C-style casts are even worse.
 *  We *know* that specified byte*'s really encode different datatypes.
 *  So, that leaves us with reinterpret_cast<> as it will allow the conversion.
 */

void viua::cpu::util::extractIntegerOperand(byte*& instruction_stream, bool& boolean, int& integer) {
    bool b = false;
    if (*reinterpret_cast<OperandType*>(instruction_stream) == OT_REGISTER_REFERENCE) {
        b = true;
    }
    boolean = b;
    pointer::inc<OperandType, byte>(instruction_stream);
    extractOperand<int>(instruction_stream, integer);
}

void viua::cpu::util::extractFloatingPointOperand(byte*& instruction_stream, float& fp) {
    extractOperand<float>(instruction_stream, fp);
}
