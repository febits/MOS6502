.equ $00, 00
.equ $4e20, 20000

lda #50
adc #50
adc $00
lda #01
ldx #01
adc $4e20, X

lda #120
adc #10
