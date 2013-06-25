;============================================================================
; STD.INC
; written by Alexandre Savelli Bemcz 2013
; Startup code for assembly programs written for General Assembler
;============================================================================

STACKSIZE  equ  4096

@exit:
  mov   ah,04ch             ;ah<-4c, DOS terminate
  int   021h                ;call DOS 

;============================================================================
; Data
;============================================================================
