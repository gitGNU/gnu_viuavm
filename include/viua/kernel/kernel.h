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

#ifndef VIUA_CPU_H
#define VIUA_CPU_H

#pragma once

#include <dlfcn.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <viua/process.h>


class ForeignFunctionCallRequest {
    std::unique_ptr<Frame> frame;
    Process *caller_process;
    Kernel *kernel;

    public:
        std::string functionName() const;
        void call(ForeignFunction*);
        void registerException(Type*);
        void wakeup();

        ForeignFunctionCallRequest(Frame *fr, Process *cp, Kernel *c): frame(fr), caller_process(cp), kernel(c) {}
        ~ForeignFunctionCallRequest() {}
};


void ff_call_processor(std::vector<std::unique_ptr<ForeignFunctionCallRequest>> *requests, std::map<std::string, ForeignFunction*> *foreign_functions, std::mutex *ff_map_mtx, std::mutex *mtx, std::condition_variable *cv);


class Kernel {
#ifdef AS_DEBUG_HEADER
    public:
#endif
    /*  Bytecode pointer is a pointer to program's code.
     *  Size and executable offset are metadata exported from bytecode dump.
     */
    byte* bytecode;
    uint64_t bytecode_size;
    uint64_t executable_offset;

    // Map of the typesystem currently existing inside the VM.
    std::map<std::string, Prototype*> typesystem;

    /*  Function and block names mapped to bytecode addresses.
     */
    std::map<std::string, uint64_t> function_addresses;
    std::map<std::string, uint64_t> block_addresses;

    std::map<std::string, std::pair<std::string, byte*>> linked_functions;
    std::map<std::string, std::pair<std::string, byte*>> linked_blocks;
    std::map<std::string, std::pair<unsigned, byte*> > linked_modules;

    int return_code;

    /*
     *  VIRTUAL PROCESSES SCHEDULING
     *
     *  List of virtual processes that do not belong to any scheduler, and
     *  are waiting to be adopted, along with means of synchronization of
     *  concurrent accesses to said list.
     *  Schedulers can post their spawned processes to the Kernel to let
     *  other schedulers execute them (sometimes, a scheduler may fetch
     *  its own process back).
     *
     *  Also, a list of spawned VP schedulers.
     */
    // list of virtual processes not associated with any VP scheduler
    std::vector<std::unique_ptr<Process>> free_virtual_processes;
    std::mutex free_virtual_processes_mutex;
    std::condition_variable free_virtual_processes_cv;
    // list of running VP schedulers, pairs of {scheduler-pointer, thread}
    std::vector<std::pair<viua::scheduler::VirtualProcessScheduler*, std::thread>> virtual_process_schedulers;
    // list of idle VP schedulers
    std::vector<viua::scheduler::VirtualProcessScheduler*> idle_virtual_process_schedulers;

    static const long unsigned default_vp_schedulers_limit = 2UL;
    long unsigned vp_schedulers_limit;

    /*  This is the interface between programs compiled to VM bytecode and
     *  extension libraries written in C++.
     */
    std::map<std::string, ForeignFunction*> foreign_functions;
    std::mutex foreign_functions_mutex;

    /** This is the mapping Viua uses to dispatch methods on pure-C++ classes.
     */
    std::map<std::string, ForeignMethod> foreign_methods;

    // Foreign function call requests are placed here to be executed later.
    std::vector<std::unique_ptr<ForeignFunctionCallRequest>> foreign_call_queue;
    std::mutex foreign_call_queue_mutex;
    std::condition_variable foreign_call_queue_condition;
    static const long unsigned default_ffi_schedulers_limit = 2UL;
    long unsigned ffi_schedulers_limit;
    std::vector<std::thread*> foreign_call_workers;

    std::vector<void*> cxx_dynamic_lib_handles;

    std::map<PID, std::vector<std::unique_ptr<Type>>> mailboxes;
    std::mutex mailbox_mutex;

    public:
        /*  Methods dealing with dynamic library loading.
         */
        void loadNativeLibrary(const std::string&);
        void loadForeignLibrary(const std::string&);

        // debug and error reporting flags
        bool debug, errors;

        std::vector<std::string> commandline_arguments;

        /*  Public API of the Kernel provides basic actions:
         *
         *      * load bytecode,
         *      * set its size,
         *      * tell the Kernel where to start execution,
         *      * kick the Kernel so it starts running,
         */
        Kernel& load(byte*);
        Kernel& bytes(uint64_t);

        Kernel& mapfunction(const std::string&, uint64_t);
        Kernel& mapblock(const std::string&, uint64_t);

        Kernel& registerExternalFunction(const std::string&, ForeignFunction*);
        Kernel& removeExternalFunction(std::string);

        /*  Methods dealing with typesystem related tasks.
         */
        bool isClass(const std::string&) const;
        bool classAccepts(const std::string&, const std::string&) const;
        std::vector<std::string> inheritanceChainOf(const std::string&) const;
        bool isLocalFunction(const std::string&) const;
        bool isLinkedFunction(const std::string&) const;
        bool isNativeFunction(const std::string&) const;
        bool isForeignMethod(const std::string&) const;
        bool isForeignFunction(const std::string&) const;

        bool isBlock(const std::string&) const;
        bool isLocalBlock(const std::string&) const;
        bool isLinkedBlock(const std::string&) const;
        std::pair<byte*, byte*> getEntryPointOfBlock(const std::string&) const;

        std::string resolveMethodName(const std::string&, const std::string&) const;
        std::pair<byte*, byte*> getEntryPointOf(const std::string&) const;

        void registerPrototype(Prototype*);

        /// These two methods are used to inject pure-C++ classes into machine's typesystem.
        Kernel& registerForeignPrototype(const std::string&, Prototype*);
        Kernel& registerForeignMethod(const std::string&, ForeignMethod);

        void requestForeignFunctionCall(Frame*, Process*);
        void requestForeignMethodCall(const std::string&, Type*, Frame*, RegisterSet*, RegisterSet*, Process*);

        void postFreeProcess(std::unique_ptr<Process>);

        void createMailbox(const PID);
        void deleteMailbox(const PID);
        void send(const PID, std::unique_ptr<Type>);
        void receive(const PID, std::queue<std::unique_ptr<Type>>&);

        auto static no_of_vp_schedulers() -> decltype(default_vp_schedulers_limit);
        auto static no_of_ffi_schedulers() -> decltype(default_ffi_schedulers_limit);

        int run();

        int exit() const;

        Kernel();
        ~Kernel();
};

#endif