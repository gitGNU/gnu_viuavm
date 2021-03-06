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

#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/types/boolean.h>
#include <viua/cpu/opex.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Process::opnot(byte* addr) {
    auto target = viua::operand::extract(addr);

    place(viua::operand::getRegisterIndex(target.get(), this), new Boolean(not target->resolve(this)->boolean()));

    return addr;
}

byte* Process::opand(byte* addr) {
    auto target = viua::operand::extract(addr);
    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);

    place(viua::operand::getRegisterIndex(target.get(), this), new Boolean(first->resolve(this)->boolean() and second->resolve(this)->boolean()));

    return addr;
}

byte* Process::opor(byte* addr) {
    auto target = viua::operand::extract(addr);
    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);

    place(viua::operand::getRegisterIndex(target.get(), this), new Boolean(first->resolve(this)->boolean() or second->resolve(this)->boolean()));

    return addr;
}
