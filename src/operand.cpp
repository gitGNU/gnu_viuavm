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
#include <viua/assert.h>
#include <viua/operand.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/process.h>
#include <viua/exceptions.h>
using namespace std;


Type* viua::operand::Atom::resolve(Process*) {
    throw new UnresolvedAtomException(atom);
    // just to satisfy the compiler, after the exception is not thrown unconditionally
    // return real object
    return nullptr;
}


Type* viua::operand::RegisterIndex::resolve(Process* t) {
    return t->obtain(index);
}
unsigned viua::operand::RegisterIndex::get(Process*) const {
    return index;
}


Type* viua::operand::RegisterReference::resolve(Process* t) {
    return t->obtain(static_cast<Integer*>(t->obtain(index))->as_unsigned());
}
unsigned viua::operand::RegisterReference::get(Process* cpu) const {
    Type* o = cpu->obtain(index);
    viua::assertions::assert_typeof(o, "Integer");
    return static_cast<Integer*>(o)->as_unsigned();
}


unique_ptr<viua::operand::Operand> viua::operand::extract(byte*& ip) {
    /** Extract operand from given address.
     *
     *  After the operand is extracted, the address pointer is increased by the size
     *  of the operand.
     *  Address is modified in-place.
     */
    OperandType ot = *reinterpret_cast<OperandType*>(ip);
    ++ip;

    unique_ptr<viua::operand::Operand> operand;
    switch (ot) {
        case OT_REGISTER_INDEX:
            operand.reset(new RegisterIndex(static_cast<unsigned>(*reinterpret_cast<int*>(ip))));
            ip += sizeof(int);
            break;
        case OT_REGISTER_REFERENCE:
            operand.reset(new RegisterReference(static_cast<unsigned>(*reinterpret_cast<int*>(ip))));
            ip += sizeof(int);
            break;
        case OT_ATOM:
            operand.reset(new Atom(string(reinterpret_cast<char*>(ip))));
            ip += (static_cast<Atom*>(operand.get())->get().size() + 1);
            break;
        default:
            throw OperandTypeException();
    }

    return operand;
}

string viua::operand::extractString(byte*& ip) {
    string s = string(reinterpret_cast<char*>(ip));
    ip += (s.size()+1);
    return s;
}

unsigned viua::operand::getRegisterIndex(viua::operand::Operand* o, Process* t) {
    unsigned index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(o)) {
        index = ri->get(t);
    } else {
        throw new Exception("invalid operand type");
    }
    return index;
}
