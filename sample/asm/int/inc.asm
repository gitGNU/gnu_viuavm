; This script tests support for iinc instruction, i.e. i++.

.function: main
    istore 1 0
    iinc 1
    print 1
    izero 0
    end
.end
