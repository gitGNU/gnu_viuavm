.function: is_divisible_by_2
    arg 0 1

    istore 2 2

    .mark: loop_begin
    ilt 1 2 3
    branch 3 loop_end loop_body

    .mark: loop_body
    isub 1 2
    jump loop_begin

    .mark: loop_end

    move 1 0

    ; make zero "true" and
    ; non-zero values "false"
    not 0

    end
.end

.function: filter
    ; classic filter() function
    ; it takes two arguments:
    ;   * a filtering function,
    ;   * a vector with values to be filtered,
    arg 0 1
    arg 1 2

    ; vector for filtered values
    vec 3

    ; initial loop counter and
    ; loop termination variable
    izero 4
    vlen 2 5

    ; while (...) {
    .mark: loop_begin
    igte 4 5 6
    branch 6 loop_end loop_body

    .mark: loop_body

    ; call filtering function to determine whether current element
    ; is a valid value
    frame 1
    vat 2 7 @4
    param 0 7
    fcall 1 8

    ; if the result from filtering function was "true" - the element should be pushed onto result vector
    ; it it was "false" - skip to next iteration
    branch 8 element_ok next_iter

    .mark: element_ok
    vpush 3 7

    .mark: next_iter
    ; empty the register because vat instruction creates references
    empty 7

    ; increase the counter and go back to the beginning of the loop
    ;     ++i;
    ; }
    iinc 4
    jump loop_begin

    .mark: loop_end

    ; move result vector into return register
    move 3 0
    end
.end

.function: main
    vec 1

    istore 2 1
    vpush 1 2
    istore 2 2
    vpush 1 2
    istore 2 3
    vpush 1 2
    istore 2 4
    vpush 1 2
    istore 2 5
    vpush 1 2

    print 1

    function is_divisible_by_2 3

    frame 2
    param 0 3
    paref 1 1
    call filter 4

    print 4

    izero 0
    end
.end