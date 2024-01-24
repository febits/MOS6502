jmp start

load:
  ldx #128  
  rts

start:
  ldy #10
  jsr load
  nop
