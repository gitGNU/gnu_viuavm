.block: handler
    print (pull 2)
    leave
.end

.block: throws_derived
    throw (new 1 Derived)
    leave
.end

.function: main
    register (class 1 Base)

    register (derive (class 1 Derived) Base)

    tryframe
    catch "Base" handler
    try throws_derived

    izero 0
    end
.end