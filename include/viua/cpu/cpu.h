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
#include <algorithm>
#include <stdexcept>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <viua/process.h>


class ForeignFunctionCallRequest {
    Frame *frame;
    Process *caller_process;
    CPU *cpu;

    public:
        std::string functionName() const;
        void call(ForeignFunction*);
        void registerException(Type*);
        void wakeup();

        ForeignFunctionCallRequest(Frame *fr, Process *cp, CPU *c): frame(fr), caller_process(cp), cpu(c) {}
        ~ForeignFunctionCallRequest() {
            delete frame;
        }
};


void ff_call_processor(std::vector<ForeignFunctionCallRequest*> *requests, std::map<std::string, ForeignFunction*>* foreign_functions, std::mutex *ff_map_mtx, std::mutex *mtx, std::condition_variable *cv);


class CPU {
    friend Process;
#ifdef AS_DEBUG_HEADER
    public:
#endif
    /*  Bytecode pointer is a pointer to program's code.
     *  Size and executable offset are metadata exported from bytecode dump.
     */
    byte* bytecode;
    uint64_t bytecode_size;
    uint64_t executable_offset;

    // vector of all processes machine is executing
    std::vector<Process*> processes;
    std::queue<Type*> watchdog_message_buffer;
    Process* watchdog_process;
    decltype(processes)::size_type current_process_index;
    std::mutex processes_mtx;

    // Global register set
    RegisterSet* regset;

    // Temporary register
    Type* tmp;

    // Map of the typesystem currently existing inside the VM.
    std::map<std::string, Prototype*> typesystem;

    /*  Function and block names mapped to bytecode addresses.
     */
    byte* jump_base;
    std::map<std::string, uint64_t> function_addresses;
    std::map<std::string, uint64_t> block_addresses;

    std::map<std::string, std::pair<std::string, byte*>> linked_functions;
    std::map<std::string, std::pair<std::string, byte*>> linked_blocks;
    std::map<std::string, std::pair<unsigned, byte*> > linked_modules;

    /*  Slot for thrown objects (typically exceptions).
     *  Can be set by user code and the CPU.
     */
    Type* thrown;
    Type* caught;

    /*  Variables set after CPU executed bytecode.
     *  They describe exit conditions of the bytecode that just stopped running.
     */
    int return_code;                // always set
    std::string return_exception;   // set if CPU stopped because of an exception
    std::string return_message;     // message set by exception

    uint64_t instruction_counter;

    /*  This is the interface between programs compiled to VM bytecode and
     *  extension libraries written in C++.
     */
    std::map<std::string, ForeignFunction*> foreign_functions;
    std::mutex foreign_functions_mutex;

    /** This is the mapping Viua uses to dispatch methods on pure-C++ classes.
     */
    std::map<std::string, ForeignMethod> foreign_methods;

    // Foreign function call requests are placed here to be executed later.
    std::vector<ForeignFunctionCallRequest*> foreign_call_queue;
    std::mutex foreign_call_queue_mutex;
    std::condition_variable foreign_call_queue_condition;
    std::vector<std::thread*> foreign_call_workers;

    // Final exception for stacktracing
    Type* terminating_exception;

    std::vector<void*> cxx_dynamic_lib_handles;

    /*  Methods dealing with typesystem related tasks.
     */
    std::vector<std::string> inheritanceChainOf(const std::string&);

    public:
        /*  Methods dealing with dynamic library loading.
         */
        void loadNativeLibrary(const std::string&);
        void loadForeignLibrary(const std::string&);

        // debug and error reporting flags
        bool debug, errors;

        std::vector<std::string> commandline_arguments;

        /*  Public API of the CPU provides basic actions:
         *
         *      * load bytecode,
         *      * set its size,
         *      * tell the CPU where to start execution,
         *      * kick the CPU so it starts running,
         */
        CPU& load(byte*);
        CPU& bytes(uint64_t);

        CPU& mapfunction(const std::string&, uint64_t);
        CPU& mapblock(const std::string&, uint64_t);

        CPU& registerExternalFunction(const std::string&, ForeignFunction*);
        CPU& removeExternalFunction(std::string);

        /// These two methods are used to inject pure-C++ classes into machine's typesystem.
        CPU& registerForeignPrototype(const std::string&, Prototype*);
        CPU& registerForeignMethod(const std::string&, ForeignMethod);

        CPU& iframe(Frame* frm = nullptr, unsigned r = DEFAULT_REGISTER_SIZE);

        Process* spawn(Frame*, Process* parent_process = nullptr);
        Process* spawnWatchdog(Frame*);
        void resurrectWatchdog();

        byte* tick(decltype(processes)::size_type process_index = 0);
        bool executeQuant(Process*, unsigned);
        bool burst();

        void requestForeignFunctionCall(Frame*, Process*);

        int run();
        inline decltype(instruction_counter) counter() {
            return processes[current_process_index]->counter();
        }

        inline std::tuple<int, std::string, std::string> exitcondition() {
            return std::tuple<int, std::string, std::string>(return_code, return_exception, return_message);
        }
        inline std::vector<Frame*> trace() { return processes[current_process_index]->trace(); }
        inline byte* executionAt() const { return processes[current_process_index]->executionAt(); }

        inline bool terminated() { return (terminating_exception != nullptr); }
        inline Type* terminatedBy() { return terminating_exception; }

        CPU():
            bytecode(nullptr), bytecode_size(0), executable_offset(0),
            watchdog_process(nullptr),
            current_process_index(0),
            regset(nullptr),
            tmp(nullptr),
            jump_base(nullptr),
            thrown(nullptr), caught(nullptr),
            return_code(0), return_exception(""), return_message(""),
            instruction_counter(0),
            terminating_exception(nullptr),
            debug(false), errors(false)
        {
            auto t = new std::thread(ff_call_processor, &foreign_call_queue, &foreign_functions, &foreign_functions_mutex, &foreign_call_queue_mutex, &foreign_call_queue_condition);
            foreign_call_workers.push_back(t);
        }

        ~CPU();
};

#endif
