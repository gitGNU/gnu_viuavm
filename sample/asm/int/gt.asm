; This script tests support for integer greater-than checking.
; Its expected output is "true".

.function: main
    istore 1 4
    istore 2 1
    igt 1 2
    print 1
    izero 0
    end
.end
