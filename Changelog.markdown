# Viua VM changelog

This file documents version-to-version changes in the machine; made to
both internal and external APIs, and to the assembly language closest to
machine's core.

Changes documented here are not the fullest representation of all changes.
For that you have to reference Git commit log.


----

### Change categories

There are several categories of change:

- **fix**: reserved for bugfixes
- **enhancement**: reserved for backward-compatible enhancements to machine's
  core (e.g. speed improvement, better memory management etc.)
- **bic**: an acronym for *Backwards Incompatible Change*, these changes (possibly
  also enhancements) are backwards incompatible and
  may break software written in machine's assembly or using it's public APIs,
  BICs in internal APIs may not be announced here
- **feature**: resereved for new features implemented in the VM, e.g. a new
  instruction, or a new feature in a standard library
- **misc**: various other changes not really fitting into any other category,


----


# From 0.8.2 to 0.8.3

- feature: the `--` string can now be used to begin comments,
  the same as `;` comments, `--` comments must appear on their own line and
  run only until the end of their line,
- feature: `vec` instruction can pack objects
- feature: external function and block signatures are included in disassembler output
- feature: `.info: <key> "<value>"` directive may be used to embed additional information in compiled files,
- feature: assembler is able to verify ranges of function- and block-local jumps and refuses to compile code that
  contains out-of-range jumps
- fix: removed race condition that would swallow exceptions thrown in FFI calls without registering them in watchdog, or
  priting stack trace
- feature: machine can spawn and utilise multiple FFI schedulers to execute several foreign calls in parallel
  sufficient hardware resources (CPU cores) are required to avoid overscheduling, currently the number of FFI schedulers
  spawned by default is 2


----


# From 0.8.1 to 0.8.2

CPU access from processes is now routed through the scheduler, which allows
decoupling process code from the CPU code and further parallelisation of the VM.
Only schedulers will "talk" with the heart of the VM, processes only access
information local to their scheduler - and since they run sequentially under
schedulers control the access can be lock-less even if there are myriads of
concurrent processes running.

- enhancement: VM is able to restart watchdog even with no stack trace available,
- bic: floats are stringified using the `std::fixed` modifier, which limits decimal digits,
- misc: `VIUA_TEST_SUITE_VALGRIND_CHECKS` environment variable is checked to see if the
  memory leak tests are to be performed,
- fix: processes can be spawned from dynamically linked functions,
- fix: machine does not segfault when an exception is passed between modules (i.e. when
  module A threw it, and module B caught it),
- bic: if an exception escapes from a process it does not bring down the whole VM - other
  processes continue to run uninterrupted,
- bic: stack traces for failed processes are printed during runtime, not in the "aftermath"
  inside CPU frontend,
- bic: machine exits with non-zero exit code only if the `main/` function fails,


----


# From 0.8.0 to 0.8.1

More errors are now detected at compile-time; assembler detects duplicate symbols during linking,
duplicate link requestes (i.e. the same module requested to be linked more than once in a single
compilation unit), and some arity errors during function calls.

- enhancement: assembler fails when duplicate symbols are found during linking phase, and produces
  appropriate error message informing the programmer which symbol was duplicated, linking which module
  triggered the error, and in which module the symbol appeared first,
- enhancement: assembler fails when a module is given more than once as a static link target in one
  compilation unit,
- enhancement: assembler is able to detect some arity errors during compile-time; errors are caught when
  number of call parameters is known at compile-time, and if the function specifies its arity - which
  it can do by appending `/N` suffix to its name (where `N` is a non-negative integer),
- enhancement: assembler may warn (or trigger an error) when it finds a function declared with undefined
  arity, this is controller by `--Wundefined-arity` and `--Eundefined-arity` options,
- bic: assembler option `-E` (*expand source code*)  renamed to `-e`,
- bic/feature: triggering all errors in assembler is enabled by `-E` option,
- bic: functions in `std::functional` module have defined arities,
- enhancement: assembler reports using the following format `<filename>:<line-number>: <class>: <message>`,
  for example `./foo.asm:18: call to undefined function foo/1`
- bic: assembler reports all errors it finds by default, in next versions some error *silencing* options
  may be introduced,
- fix: all machine-provided functions have theit arity specified (as either fixed or variable),
- bic: assembler enforces that main function has specified fixed arity
- enhancement: three variants of main functions are available: `main/0` that receives no arguments,
  `main/1` which receives a vector with all command line arguments sent to the program and the program name, and
  `main/2` which receives two parameters - first the name of the program, and second the rest of the command
  line parameters,
- bic: popping elements from vector with `VPOP` instruction to zero register does not drop them but
  places them in the register, popped elements can be then manually deleted,
- fix: VM does not block on FFI calls when run on one core (fixed race condition that caused the process issuing a
  FFI call to receive `wakeup()` before `suspend()`),
- bic: inspecting type of expired pointers returns "ExpiredPointer" instead of throwing an exception,
- bic: exiting watchdog process with "return" is a fatal VM exception,
- bic: functions must end with either `return` or `tailcall` instruction, the same rule applies to `main/` variants,
- bic: assembler no longer issues errors or warnings about `main/` function ending with `halt` instruction as
  this is not legal - an error about function not ending with `return` or `tailcalll` is issued instead,
- enhancement: assembler catches frames with gaps, i.e. frames that declare a number of parameter slots but leave
  some of the slots empty,
- enhancement: assembler catches parameters passed to slots with too high indexes, i.e. passing parameter to slot
  with index 3 when frame declares only 3 slots,
- enhancement: assembler catches double passes to parameter slotss,


----


# From 0.7.0 to 0.8.0

Better support for closures: users can control what registers to enclose and
how to enclose values in them.
Vector and object modifications have move semantics now (copy semantics must be
implemented in user code).
Improvements to standard library (in `std::vector`, `std::misc` and `std::functional` modules).
Inter-function tail calls.
FFI worker thread for non-blocking calls to foreign functions.

- bic: `PAREF` and `REF` instructions removed, references made an internal tool of the VM,
- feature: `ENCLOSECOPY` instruction for enclosing objects in closures by copying them,
- feature: `ENCLOSEMOVE` instruction for enclosing objects in closures by moving them
  inside the closure,
- bic: renamed `CLBIND` to `ENCLOSE`,
- bic: changed the way closures are created, i.e. now, the closure is created first and
  only then objects are enclosed (either by copy, by move or by reference),
- bic: renamed `THREAD` to `PROCESS`,
- bic: vector instructions `VINSERT`, `VPUSH` and `VPOP` have move semantics,
- bic: removed `Object::set` and `Object::get` methods from `Object`'s prototype (object
  modification instructions `INSERT` and `REMOVE` must be used instead),
- feature: `INSERT` and `REMOVE` instructions with move semantics for object modification,
- fix: `VPOP` allows popping all indexes, not only the last one,
- fix: `std::functional::apply` works correctly for functions that do not return anything,
- bic: renamed `THJOIN` to `JOIN` to reflect the name change from *threads* to *processes*,
- bic: renamed `THRECEIVE` to `RECEIVE` to reflect the name change from *threads* to *processes*,
- fix: process abort messages sent to watchdog include `parameters` attribute with a vector of
  parameters passed to top-most function of the aborted process,
- fix: watchdog executes after process queue cleanup during CPU burst phase to prevent processes restarted
  by watchdog to be immediately erased,
- misc: function `std::misc::cycle/1` (*running for at least N cycles*) added to standard library,
- feature: function `std::vector::of/2` (*create vector of N objects generated by supplied F function*) added to standard library,
- feature: function `std::vector::of_ints/2` (*create a vector of integers from 0 to N-1*) added to standard library,
- feature: function `std::vector::every/2` (*check if every element passes a test supplied in function F*) added to standard library,
- feature: function `std::vector::any/2` (*check if any element passes a test supplied in function F*) added to standard library,
- feature: functions `std::vector::reverse/1` and `std::vector::reverse_in_place/1` added to standard library,
- fix: function `std::functional::apply` correctly handles functions that do not return a value,
- fix: machine no longer crashes when exceptions thrown by foreign libraries enter watchdog process and
  are not manually deleted before the foreign library is closed - readonly resources (e.g. `vtable`s) of
  foreign library (`.so`) were being made unavailable but machine still wanted to access them when
  deleting objects,
- bic: `PARAM` immediately copies parameters into a frame, not when they are accessed;
- feature: new `TAILCALL` instruction,
- enhancement: foreign functions are not called immediately with `CALL` instruction, but are instead scheduled
  to run on a FFI worker thread - special thread that only executes foreign function calls,
  this way native Viua code is never blocked by an FFI call,


----


# From 0.6.1 to 0.7.0

- bic: `throw` instruction no longer leaves thrown object in its source register;
  thrown object is put in special *throw-register* and the source register in
  currently used instruction set is made empty
- bic: `free` instruction was renamed to `delete` to better match the naming convections
  of the other object-lifespan controlling instructions (i.e. the `new` instruction)
- enhancement: change `Thread::instruction_counter`'s type from `unsigned` to `uint64_t`
- fix: stack traces displayed after uncaught exceptions are generated for the thread that
  the exception originated from
- bic: machine reports the function that started a thread as orphaning thread's children,
  this means that old "main/1 orphaning threads" changes to "__entry/0 orphaning trhreads"
- misc: if stack is not available "<unavailable> (stack empty)" will be used when
  machine reports that threads were orphaned,
- enhancement: returning objects from functions has "move semantics" - there is no copy
  operation and the object is just moved to the caller frame,
- bic: `String::stringify()` foreign method takes pointer to an object as its second argument,
- bic: `String::represent()` foreign method takes pointer to an object as its second argument,
- bic: `std::string::stringify` takes pointer to object to stringify instead of a reference,
  this makes the objects stay/1 within VM's scope-based memory management system,
- bic: `std::string::represent/1` takes pointer to object to stringify instead of a reference,
- bic: `end` instruction renamed to `return`,
- feature: VM provides a mechanism to spawn an immortal watchdog thread to deal with deaths of
  other threads (the syntax is `watchdog <function-name>`, `watchdog` instruction requires a frame),
- enhancement: VM provides a mechanism to extract return values from functions running in threads,
- bic: syntax of `thjoin` instruction changed from `thjoin <thread-handle>`
  to `thjoin <target-register> <thread-handle>`,
- enhancement: slightly better messages for some exceptions,
- feature: threads can be suspended and woken-up,
- feature: `pamv` instruction added to instruction set, supports pass-by-move,
