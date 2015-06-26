; purpose of this program is to compute n-th power of a number

.function: dummy
    ; this function is here to stress jump calculation
    izero 0
    end
.end

.function: power_of
    .name: 1 base
    .name: 2 exponent
    .name: 3 zero
    .name: 6 result

    ; store operands of the power-of operation
    arg 0 base
    arg 1 exponent

    ; store zero - we need it to compare the exponent to it
    istore zero 0

    ; if the exponent is equal to zero, store 1 in first register and jump to print
    ieq exponent zero 4

    ; invert so we can use short form of branch instruction
    not 4
    branch 4 algorithm
    istore result 1
    jump final

    ; now, we multiply in a loop
    .mark: algorithm
    .name: 5 counter
    istore counter 1
    ; in register 6, store the base of power as
    ; we will need it for multiplication
    istore result @base

    .mark: loop
    ilt counter exponent 4
    branch 4 12 final
    imul result base
    nop
    iinc counter
    jump loop

    ; final instructions
    .mark: final
    ; return result
    move result 0
    end
.end

.function: main
    istore 1 4
    istore 2 3

    frame 2
    param 0 1
    param 1 2
    call power_of 1
    print 1

    izero 0
    end
.end
