;--------------------------------------------------------------------------- 
; Hello World program
; written by Alexandre Savelli Bencz
;--------------------------------------------------------------------------- 

;--------------------------------------------------------------------------- 
; here comes the code  
;--------------------------------------------------------------------------- 

  call @hello                 ; call hello world procedure
  mov  ax,04c00h              ; ax<-4C00 DOS Exit
  int  021h                   ; call DOS interrupt

@hello:
  push cs                     ; put cs onto the stack
  pop  ds                     ; retrieve cs into ds
  mov  dx,offset HelloWorld   ; move string offset into dx
  mov  ah,9                   ; ah<-9 DOS print string
  int  021h                   ; call DOS interrupt
  ret                         ; return back to caller

;---------------------------------------------------------------------------
;  now here's the data
;---------------------------------------------------------------------------

HelloWorld  db "Hello World!$"
