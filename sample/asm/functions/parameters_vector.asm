;
;   Copyright (C) 2015, 2016 Marek Marecki
;
;   This file is part of Viua VM.
;
;   Viua VM is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   Viua VM is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
;

.bsignature: misc::argsvector

.function: foo/4
    link misc

    ; FIXME: try will not be needed when ENTER instruction is implemented
    try
    enter misc::argsvector

    print 1
    return
.end

.function: main/1
    frame ^[(param 0 (istore 1 0)) (param 1 (istore 2 1)) (param 2 (istore 3 2)) (param 3 (istore 4 3))]
    call ^(izero 0) foo/4
    return
.end
