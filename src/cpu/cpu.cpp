#include <dlfcn.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <functional>
#include <regex>
#include <chrono>
#include <viua/machine.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/maps.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/byte.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/exception.h>
#include <viua/types/reference.h>
#include <viua/types/object.h>
#include <viua/types/function.h>
#include <viua/support/pointer.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/loader.h>
#include <viua/include/module.h>
#include <viua/cpu/cpu.h>
#include <viua/scheduler/vps.h>
using namespace std;


CPU& CPU::load(byte* bc) {
    /*  Load bytecode into the CPU.
     *  CPU becomes owner of loaded bytecode - meaning it will consider itself responsible for proper
     *  destruction of it, so make sure you have a copy of the bytecode.
     *
     *  Any previously loaded bytecode is freed.
     *  To free bytecode without loading anything new it is possible to call .load(0).
     *
     *  :params:
     *
     *  bc:char*    - pointer to byte array containing bytecode with a program to run
     */
    if (bytecode) { delete[] bytecode; }
    bytecode = bc;
    jump_base = bytecode;
    return (*this);
}

CPU& CPU::bytes(uint64_t sz) {
    /*  Set bytecode size, so the CPU can stop execution even if it doesn't reach HALT instruction but reaches
     *  bytecode address out of bounds.
     */
    bytecode_size = sz;
    return (*this);
}

CPU& CPU::mapfunction(const string& name, uint64_t address) {
    /** Maps function name to bytecode address.
     */
    function_addresses[name] = address;
    return (*this);
}

CPU& CPU::mapblock(const string& name, uint64_t address) {
    /** Maps block name to bytecode address.
     */
    block_addresses[name] = address;
    return (*this);
}

CPU& CPU::registerExternalFunction(const string& name, ForeignFunction* function_ptr) {
    /** Registers external function in CPU.
     */
    unique_lock<mutex> lock(foreign_functions_mutex);
    foreign_functions[name] = function_ptr;
    return (*this);
}

CPU& CPU::registerForeignPrototype(const string& name, Prototype* proto) {
    /** Registers foreign prototype in CPU.
     */
    typesystem[name] = proto;
    return (*this);
}

CPU& CPU::registerForeignMethod(const string& name, ForeignMethod method) {
    /** Registers foreign prototype in CPU.
     */
    foreign_methods[name] = method;
    return (*this);
}


void CPU::loadNativeLibrary(const string& module) {
    regex double_colon("::");
    ostringstream oss;
    oss << regex_replace(module, double_colon, "/");
    string try_path = oss.str();
    string path = support::env::viua::getmodpath(try_path, "vlib", support::env::getpaths("VIUAPATH"));
    if (path.size() == 0) { path = support::env::viua::getmodpath(try_path, "vlib", VIUAPATH); }
    if (path.size() == 0) { path = support::env::viua::getmodpath(try_path, "vlib", support::env::getpaths("VIUAAFTERPATH")); }

    if (path.size()) {
        Loader loader(path);
        loader.load();

        byte* lnk_btcd = loader.getBytecode();
        linked_modules[module] = pair<unsigned, byte*>(static_cast<unsigned>(loader.getBytecodeSize()), lnk_btcd);

        vector<string> fn_names = loader.getFunctions();
        map<string, uint64_t> fn_addrs = loader.getFunctionAddresses();
        for (unsigned i = 0; i < fn_names.size(); ++i) {
            string fn_linkname = fn_names[i];
            linked_functions[fn_linkname] = pair<string, byte*>(module, (lnk_btcd+fn_addrs[fn_names[i]]));
        }

        vector<string> bl_names = loader.getBlocks();
        map<string, uint64_t> bl_addrs = loader.getBlockAddresses();
        for (unsigned i = 0; i < bl_names.size(); ++i) {
            string bl_linkname = bl_names[i];
            linked_blocks[bl_linkname] = pair<string, byte*>(module, (lnk_btcd+bl_addrs[bl_linkname]));
        }
    } else {
        throw new Exception("failed to link: " + module);
    }
}
void CPU::loadForeignLibrary(const string& module) {
    string path = "";
    path = support::env::viua::getmodpath(module, "so", support::env::getpaths("VIUAPATH"));
    if (path.size() == 0) { path = support::env::viua::getmodpath(module, "so", VIUAPATH); }
    if (path.size() == 0) { path = support::env::viua::getmodpath(module, "so", support::env::getpaths("VIUAAFTERPATH")); }

    if (path.size() == 0) {
        throw new Exception("LinkException", ("failed to link library: " + module));
    }

    void* handle = dlopen(path.c_str(), RTLD_LAZY);

    if (handle == nullptr) {
        throw new Exception("LinkException", ("failed to open handle: " + module));
    }

    ForeignFunctionSpec* (*exports)() = nullptr;
    if ((exports = reinterpret_cast<ForeignFunctionSpec*(*)()>(dlsym(handle, "exports"))) == nullptr) {
        throw new Exception("failed to extract interface from module: " + module);
    }

    ForeignFunctionSpec* exported = (*exports)();

    unsigned i = 0;
    while (exported[i].name != NULL) {
        registerExternalFunction(exported[i].name, exported[i].fpointer);
        ++i;
    }

    cxx_dynamic_lib_handles.push_back(handle);
}


Process* CPU::spawn(Frame* frm, Process* parent_process) {
    unique_lock<std::mutex> lck{processes_mtx};
    Process* thrd = new Process(frm, this, jump_base, parent_process);
    thrd->begin();
    processes.push_back(thrd);
    return thrd;
}
Process* CPU::spawnWatchdog(Frame* frm) {
    if (watchdog_process != nullptr) {
        throw new Exception("watchdog process already spawned");
    }
    unique_lock<std::mutex> lck{processes_mtx};
    watchdog_function = frm->function_name;
    Process* thrd = new Process(frm, this, jump_base, nullptr);
    thrd->begin();
    watchdog_process = thrd;
    return thrd;
}
void CPU::resurrectWatchdog() {
    auto active_exception = watchdog_process->getActiveException();
    if (active_exception) {
        cout << "watchdog process terminated by: " << active_exception->type() << ": '" << active_exception->str() << "'" << endl;
        delete active_exception;
    }

    delete watchdog_process;
    watchdog_process = nullptr;

    Frame *frm = nullptr;
    frm = new Frame(nullptr, 0, watchdog_process_register_count);
    frm->function_name = watchdog_function;

    spawnWatchdog(frm);
}

vector<string> CPU::inheritanceChainOf(const string& type_name) {
    /** This methods returns full inheritance chain of a type.
     */
    if (typesystem.count(type_name) == 0) {
        // FIXME: better exception message
        throw new Exception("unregistered type: " + type_name);
    }
    vector<string> ichain = typesystem.at(type_name)->getAncestors();
    for (unsigned i = 0; i < ichain.size(); ++i) {
        vector<string> sub_ichain = inheritanceChainOf(ichain[i]);
        for (unsigned j = 0; j < sub_ichain.size(); ++j) {
            ichain.push_back(sub_ichain[j]);
        }
    }

    vector<string> linearised_inheritance_chain;
    unordered_set<string> pushed;

    string element;
    for (unsigned i = 0; i < ichain.size(); ++i) {
        element = ichain[i];
        if (pushed.count(element)) {
            linearised_inheritance_chain.erase(remove(linearised_inheritance_chain.begin(), linearised_inheritance_chain.end(), element), linearised_inheritance_chain.end());
        } else {
            pushed.insert(element);
        }
        linearised_inheritance_chain.push_back(element);
    }

    return ichain;
}

byte* CPU::tick(decltype(processes)::size_type index) {
    byte* ip = processes[index]->tick();  // returns instruction pointer
    if (processes[index]->terminated()) {
        return nullptr;
    }
    return ip;
}

bool CPU::burst(viua::scheduler::VirtualProcessScheduler *sched) {
    if (not processes.size()) {
        // make CPU stop if there are no processes to run
        return false;
    }

    current_process_index = 0;
    bool ticked = false;

    decltype(processes) running_processes;
    decltype(processes) dead_processes;
    bool abort_because_of_process_termination = false;
    for (decltype(processes)::size_type i = 0; i < processes.size(); ++i) {
        current_process_index = i;
        auto th = processes[i];

        ticked = (sched->executeQuant(th, th->priority()) or ticked);

        if (th->terminated() and not th->joinable() and th->parent() == nullptr) {
            if (watchdog_process == nullptr) {
                abort_because_of_process_termination = true;
            } else {
                Object* death_message = new Object("Object");
                Type* exc = th->transferActiveException();
                Vector *parameters = new Vector();
                RegisterSet *top_args = th->trace()[0]->args;
                for (unsigned long j = 0; j < top_args->size(); ++j) {
                    if (top_args->at(j)) {
                        parameters->push(top_args->at(j));
                    }
                }
                top_args->drop();
                death_message->set("function", new Function(th->trace()[0]->function_name));
                death_message->set("exception", exc);
                death_message->set("parameters", parameters);
                watchdog_process->pass(death_message);

                // push broken process to dead processes list to
                // erase it later
                dead_processes.push_back(th);

                // copy what is left after this broken process to running processes array to
                // prevent losing them
                // because after the loop the `processes` vector is reset
                for (decltype(processes)::size_type j = i+1; j < processes.size(); ++j) {
                    running_processes.push_back(processes[j]);
                }
            }
            break;
        }

        // if the process stopped and is not joinable declare it dead and
        // schedule for removal thus shortening the vector of running processes and
        // speeding up execution
        if (th->stopped() and (not th->joinable())) {
            dead_processes.push_back(th);
        } else {
            running_processes.push_back(th);
        }
    }

    if (abort_because_of_process_termination) {
        return false;
    }

    for (decltype(dead_processes)::size_type i = 0; i < dead_processes.size(); ++i) {
        delete dead_processes[i];
    }

    // if there were any dead processes we must rebuild the scheduled processes vector
    if (dead_processes.size()) {
        processes.erase(processes.begin(), processes.end());
        for (decltype(running_processes)::size_type i = 0; i < running_processes.size(); ++i) {
            processes.push_back(running_processes[i]);
        }
    }

    while (watchdog_process != nullptr and not watchdog_process->suspended()) {
        sched->executeQuant(watchdog_process, 0);
        if (watchdog_process->terminated() or watchdog_process->stopped()) {
            resurrectWatchdog();
        }
    }

    return ticked;
}

void CPU::requestForeignFunctionCall(Frame *frame, Process *requesting_process) {
    unique_lock<mutex> lock(foreign_call_queue_mutex);
    foreign_call_queue.push_back(new ForeignFunctionCallRequest(frame, requesting_process, this));

    // unlock before calling notify_one() to avoid waking the worker thread when it
    // cannot obtain the lock and
    // fetch the call request
    lock.unlock();
    foreign_call_queue_condition.notify_one();
}

int CPU::run() {
    /*  VM CPU implementation.
     */
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }

    viua::scheduler::VirtualProcessScheduler vps(this);
    Process *t = vps.bootstrap(commandline_arguments, jump_base);
    processes.push_back(t);

    while (burst(&vps));

    if (current_process_index < processes.size() and processes[current_process_index]->terminated()) {
        auto trace = processes[current_process_index]->trace();
        cout << "process " << current_process_index << " spawned using ";
        if (trace.size() > 1) {
            // if trace size if greater than one, detect if this is main process
            cout << trace[(trace[0]->function_name == ENTRY_FUNCTION_NAME)]->function_name;
        } else if (trace.size()) {
            // if trace size is equal to one, just print the top-most function
            cout << trace[0]->function_name;
        } else {
            // if we have no trace available, inform the user about it
            cout << "<function unavailable>";
        }

        cout << " has terminated" << endl;
        Type* e = processes[current_process_index]->getActiveException();

        return_code = 1;
        terminating_exception = e;
    }

    /*
    if (return_code == 0 and regset->at(0)) {
        // if return code if the default one and
        // return register is not unused
        // copy value of return register as return code
        try {
            return_code = static_cast<Integer*>(regset->get(0))->value();
        } catch (const Exception* e) {
            return_code = 1;
            return_exception = e->type();
            return_message = e->what();
        }
    }
    */

    // delete processes and global registers
    // otherwise we get huge memory leak
    // do not delete if execution was halted because of exception
    if (not terminated()) {
        while (processes.size()) {
            delete processes.back();
            processes.pop_back();
        }
    }

    return return_code;
}
