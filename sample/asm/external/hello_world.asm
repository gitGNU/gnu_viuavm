.signature: World::print_hello/0

.function: main
    import "build/test/World"

    frame 0
    call 0 World::print_hello/0

    izero 0
    return
.end
