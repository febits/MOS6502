.equ X, 10
.equ Y, 12

jmp init

check:
  cmp #233 ; Max Fibonacci to unsigned 8-bits integer
  beq end
  rts

swap:
  ldx Y
  tay
  sty Y
  rts

sum:
  txa
  clc
  adc Y
  rts

main:
  jsr swap
  jsr sum
  jsr check
  
  jmp main

end:
  nop

init:
  ldx #00
  ldy #01
  stx X
  sty Y

  txa
  adc Y

  jmp main