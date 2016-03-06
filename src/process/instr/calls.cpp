#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Process::opframe(byte* addr) {
    /** Create new frame for function calls.
     */
    int arguments = viua::operand::getInteger(viua::operand::extract(addr).get(), this);
    int local_registers = viua::operand::getInteger(viua::operand::extract(addr).get(), this);

    requestNewFrame(arguments, local_registers);

    return addr;
}

byte* Process::opparam(byte* addr) {
    /** Run param instruction.
     */
    int parameter_no_operand_index = viua::operand::getInteger(viua::operand::extract(addr).get(), this);
    int source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (unsigned(parameter_no_operand_index) >= frame_new->args->size()) {
        throw new Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter");
    }
    uregset->flag(source, PASSED);
    frame_new->args->set(parameter_no_operand_index, fetch(source));
    frame_new->args->clear(parameter_no_operand_index);

    return addr;
}

byte* Process::oppamv(byte* addr) {
    /** Run pamv instruction.
     */
    int parameter_no_operand_index = viua::operand::getInteger(viua::operand::extract(addr).get(), this);
    int source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (unsigned(parameter_no_operand_index) >= frame_new->args->size()) {
        throw new Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter");
    }
    // FIXME: memory leak when PAMV'ed objects are not extracted inside function
    frame_new->args->set(parameter_no_operand_index, uregset->pop(source));
    frame_new->args->clear(parameter_no_operand_index);
    frame_new->args->flag(parameter_no_operand_index, MOVED);

    return addr;
}

byte* Process::opparef(byte* addr) {
    /** Run paref instruction.
     */
    int parameter_no_operand_index = viua::operand::getInteger(viua::operand::extract(addr).get(), this);

    if (unsigned(parameter_no_operand_index) >= frame_new->args->size()) {
        throw new Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter");
    }

    int object_operand_index = viua::operand::getInteger(viua::operand::extract(addr).get(), this);

    Type* object = uregset->at(object_operand_index);
    Reference* rf = dynamic_cast<Reference*>(object);
    if (rf == nullptr) {
        rf = new Reference(object);
        uregset->empty(object_operand_index);
        uregset->set(object_operand_index, rf);
    }
    frame_new->args->set(parameter_no_operand_index, rf);

    return addr;
}

byte* Process::oparg(byte* addr) {
    /** Run arg instruction.
     */
    int destination_register_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    int parameter_no_operand_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (unsigned(parameter_no_operand_index) >= frames.back()->args->size()) {
        ostringstream oss;
        oss << "invalid read: read from argument register out of bounds: " << parameter_no_operand_index;
        throw new Exception(oss.str());
    }

    if (frames.back()->args->isflagged(parameter_no_operand_index, MOVED)) {
        uregset->set(destination_register_index, frames.back()->args->pop(parameter_no_operand_index));
    } else {
        uregset->set(destination_register_index, frames.back()->args->get(parameter_no_operand_index)->copy());
    }

    return addr;
}

byte* Process::opargc(byte* addr) {
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    uregset->set(target, new Integer(static_cast<int>(frames.back()->args->size())));

    return addr;
}

byte* Process::opcall(byte* addr) {
    /*  Run call instruction.
     */
    bool return_register_ref = false;
    int return_register_index = 0;
    viua::cpu::util::extractIntegerOperand(addr, return_register_ref, return_register_index);

    string call_name = viua::operand::extractString(addr);

    // clear PASSED flag
    // since function calls are blocking, we can be sure that after the function returns
    // we can safely overwrite all registers
    for (unsigned i = 0; i < uregset->size(); ++i) {
        if (uregset->at(i) != nullptr) {
            uregset->unflag(i, PASSED);
        }
    }

    bool is_native = (cpu->function_addresses.count(call_name) or cpu->linked_functions.count(call_name));
    bool is_foreign = cpu->foreign_functions.count(call_name);
    bool is_foreign_method = cpu->foreign_methods.count(call_name);

    if (not (is_native or is_foreign or is_foreign_method)) {
        throw new Exception("call to undefined function: " + call_name);
    }

    if (is_foreign_method) {
        if (frame_new == nullptr) {
            throw new Exception("cannot call foreign method without a frame");
        }
        if (frame_new->args->size() == 0) {
            throw new Exception("cannot call foreign method using empty frame");
        }
        if (frame_new->args->at(0) == nullptr) {
            throw new Exception("frame must have at least one argument when used to call a foreign method");
        }
        Type* obj = frame_new->args->at(0);
        return callForeignMethod(addr, obj, call_name, return_register_ref, return_register_index, call_name);
    }

    auto caller = (is_native ? &Process::callNative : &Process::callForeign);
    return (this->*caller)(addr, call_name, return_register_ref, return_register_index, "");
}

byte* Process::opreturn(byte* addr) {
    if (frames.size() == 0) {
        throw new Exception("no frame on stack: no call to return from");
    }
    addr = frames.back()->ret_address();

    Type* returned = nullptr;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == nullptr) {
            throw new Exception("return value requested by frame but function did not set return register");
        }
        returned = uregset->pop(0);
    }

    dropFrame();

    // place return value
    if (returned and frames.size() > 0) {
        if (resolve_return_value_register) {
            return_value_register = static_cast<Integer*>(fetch(return_value_register))->value();
        }
        place(return_value_register, returned);
    }

    if (frames.size() > 0) {
        if (cpu->function_addresses.count(frames.back()->function_name)) {
            jump_base = cpu->bytecode;
        } else {
            jump_base = cpu->linked_modules.at(cpu->linked_functions.at(frames.back()->function_name).first).second;
        }
    }

    return addr;
}