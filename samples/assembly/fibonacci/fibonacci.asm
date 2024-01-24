.equ Y, 06
.equ X, 05

; Init
ldx #00
ldy #01
stx X
sty Y

; add = 1
txa
adc Y

; swapping
ldx Y
tay
sty Y

; add = 2
txa
adc Y

; swapping
ldx Y
tay
sty Y

; add = 3
txa
adc Y

; swapping
ldx Y
tay
sty Y

; add = 5
txa
adc Y

; swapping
ldx Y
tay
sty Y

; add = 8
txa
adc Y

; swapping
ldx Y
tay
sty Y

; add = 13
txa
adc Y

; swapping
ldx Y
tay
sty Y

; add = 21
txa
adc Y

; swapping
ldx Y
tay
sty Y

; add = 34
txa
adc Y

; swapping
ldx Y
tay
sty Y

; add = 55
txa
adc Y

; swapping
ldx Y
tay
sty Y

; add = 89
txa
adc Y

; swapping
ldx Y
tay
sty Y

;add = 144
txa
adc Y
