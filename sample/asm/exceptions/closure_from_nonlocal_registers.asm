.function: foo
    print 1
    end
.end

.function: main
    ress global
    istore 1 42
    clbind 1
    closure foo 2

    izero 0
    end
.end
