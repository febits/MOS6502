.equ Y, 50
.equ MAX, 233

; Init
ldx #00
ldy #01
sty Y

txa
adc Y

fibonacci:
  ldx Y
  tay
  sty Y

  txa
  clc
  adc Y

  cmp MAX
  beq end

  jmp fibonacci

end:
  nop