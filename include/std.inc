;============================================================================
; std.inc
; written by Alexandre Savelli Bencz, 2013
; Startup code for assembly programs written for General Assembler
;  Comments may be a bit lacking, but I have other things to work on
;============================================================================

STACKSIZE  equ  4096

; This is where the program will begin  
  mov   ax,[002ch]          ;get environment segment
  mov   [@envseg],ax        ;save it
  mov   si,0080h            ;get command line index
  lodsb                     ;get byte count
  mov   cl,al               ;put byte count in cx
  xor   ch,ch               ;clear ch
  cmp   cx,080h             ;only have 128 bytes
  ja    @skipcommandline    ;to large?
  mov   [@commcount],cx     ;save the count
  push  cs                  ;es=cs
  pop   es
  mov   di,@commline        ;es:di = @commline
  rep   movsb               ;copy it over
@skipcommandline:

;----------------------------------------------------------------------------
; save interrupt vectors 0 and 6

  mov   al,0                ;al<-0 for interrupt 0
  mov   ah,035h             ;ah<-35, DOS get interrupt vector
  int   021h                ;call DOS
  mov   ds:[@@SaveInt0],bx  ;save interrupt vector offset
  mov   bx,es               ;bx<-es, interrupt vector segment
  mov   ds:[@@SaveInt0+2],bx;save interrupt vector segment

  mov   al,6                ;al<-0 for interrupt 6
  mov   ah,035h             ;ah<-35, DOS get interrupt vector
  int   021h                ;call DOS
  mov   ds:[@@SaveInt6],bx  ;save interrupt vector offset
  mov   bx,es               ;bx<-es, interrupt vector segment
  mov   ds:[@@SaveInt6+2],bx;save interrupt vector segment

;----------------------------------------------------------------------------
; reset vectors to handle errors

  push  cs                  ;push cs
  pop   ds                  ;ds<-cs
  mov   al,0                ;al<-0 for interrupt 0
  mov   dx,@@DivisionByZero ;dx<-@@DivisionByZero offset
  mov   ah,025h             ;ah<-25, DOS set interrupt vector
  int   021h                ;call DOS

  push  cs                  ;push cs
  pop   ds                  ;ds<-cs
  mov   al,6                ;al<-6 for interrupt 6
  mov   dx,@@InvalidOpCode  ;dx<-@@InvalidOpCode offset
  mov   ah,025h             ;ah<-25, DOS set interrupt vector
  int   021h                ;call DOS

;----------------------------------------------------------------------------
; Expand the program's block size to allow for dynamic allocation
; So long as there is nothing allocated in the segment directly after the
; program was loaded this should work

  push  cs                  ;push cs
  pop   es                  ;es<-cs
  mov   bx,@@FinalOffset/16+2;bx<-@@FinalOffset(size of program block)
  mov   ah,04ah             ;ah<-4a, DOS resize block
  int   021h                ;call DOS
  jnc   @@BlockSizeOk       ;did resize work?

  mov   dx,@@BlockSizeErrorMessage
  jmp   @@ErrorExit         ;had an error, so exit
@@BlockSizeOk:

;----------------------------------------------------------------------------
;create more stack space

  mov   ax,STACKSIZE        ;ax<-4K
  call  @AllocateMem        ;allocate the mem
  mov   ss,ax               ;ss<-ax, new stack segment
  mov   sp,STACKSIZE        ;sp<-4K new stack pointer


;----------------------------------------------------------------------------
; call to the main code
;----------------------------------------------------------------------------
  mov   ax,cs               ;ax<-cs
  mov   ds,ax               ;ds=cs
  mov   es,ax               ;es=cs
  call  @start              ;call main program
;----------------------------------------------------------------------------

@exit:

;----------------------------------------------------------------------------
; get interrupt vectors back

  lds   dx,[@@SaveInt0]     ;ds:dx = old int 0
  mov   al,0                ;al<-0, int 0
  mov   ah,025h             ;ah<-25, DOS set Interrupt
  int   021h                ;call DOS
  lds   dx,[@@SaveInt6]     ;ds:dx = old int 6
  mov   al,6                ;al<-6, int 6
  mov   ah,025h             ;ah<-25, DOS set interrupt
  int   021h                ;call DOS

; get stack back

  mov   ax,ss               ;ax<-ss, get stack segment
  mov   bx,0100h            ;bx<-100, new stack size
  mov   sp,bx               ;sp<-bx=100, new stack size
  mov   bx,cs               ;bx<-cs
  mov   ss,bx               ;ss<-bx=cs,get back old stack segment
  call  @FreeMem            ;free allocated stack space

; go back to DOS
  mov   ah,04ch             ;ah<-4c, DOS terminate
  int   021h                ;call DOS

@@ErrorExit:
  push  cs                  ;push cs
  pop   ds                  ;ds = cs
  mov   ah,09h              ;ah<-9, DOS print string
  int   021h                ;call DOS
  jmp   @exit               ;quit program

;============================================================================
; Procedures
;============================================================================

;----------------------------------------------------------------------------
; New Interrupt 0
;----------------------------------------------------------------------------
@@DivisionByZero:
  mov   dx,@@DivideByZeroErrorMessage
  mov   ah,09h              ;write error message
  int   021h

  mov   bp,sp               ;write segment to screen
  mov   ax,[bp+2]
  mov   dx,4
  call  @WriteHexVal

  mov   dl,':'              ;write ':' to screen
  mov   ah,2
  int   021h

  mov   ax,[bp]             ;write offset to screen
  mov   dx,4
  call  @WriteHexVal
  jmp   @exit               ;exit program

;----------------------------------------------------------------------------
; New Interrupt 6
;----------------------------------------------------------------------------
@@InvalidOpCode:
  mov   dx,@@InvalidOpCodeErrorMessage
  mov   ah,09h              ;write error message
  int   021h

  mov   bp,sp               ;write segment to screen
  mov   ax,[bp+2]
  mov   dx,4
  call  @WriteHexVal

  mov   dl,':'              ;write ':' to screen
  mov   ah,2
  int   021h

  mov   ax,[bp]             ;write offset to screen
  mov   dx,4
  call  @WriteHexVal
  jmp   @exit               ;exit program

;----------------------------------------------------------------------------
; @AllocateMem
; AX = amount to allocate
; return AX:DX = memory location
;----------------------------------------------------------------------------
@AllocateMem:
  push  cx                  ;save cx
  push  bx                  ;save bx
  mov   bx,ax               ;bx<-ax
  mov   cl,4                ;cl<-4
  shr   bx,cl               ;bx = bx / 16
  inc   bx
  mov   ah,048h             ;ah<-48, allocate mem
  int   021h                ;call DOS
  jc    @@AllocMemError     ;error?
  xor   dx,dx               ;clear dx
  pop   bx                  ;restore bx
  pop   cx                  ;restore cx
  ret                       ;return
@@AllocMemError:
  mov   dx,@@NoMemMessage   ;display error message
  jmp   @@ErrorExit         ;quit

;----------------------------------------------------------------------------
; @FreeMem
; AX:DX = memory location
;----------------------------------------------------------------------------
@FreeMem:
  push  es                  ;save es
  mov   es,ax               ;es<-ax
  mov   ah,049h             ;ah<-49, free mem
  int   021h                ;call DOS
  pop   es                  ;restore es
  jc    @@FreeMemError      ;error?
  ret                       ;return
@@FreeMemError:
  mov   dx,@@FreeMemErrorMessage
  jmp   @@ErrorExit         ;display error message and quit

;----------------------------------------------------------------------------
; @WriteHexVal
; AX = Value
; Dx = digits
;----------------------------------------------------------------------------
@WriteHexVal:
  push  si                  ;save si
  push  ds                  ;save ds
  push  bx                  ;save bx
  push  cx                  ;save cx

  push  cs                  ;ds<-cs
  pop   ds
  mov   si,@@HexValues      ;si<-@@HexValues
  xor   dh,dh               ;clear dh
  mov   cx,dx               ;cx<-dx
@@WriteHexValLoop1:
  mov   bx,ax               ;bx<-ax
  and   bx,0fh              ;bx = bx & 0f
  mov   bx,[si+bx]          ;bx<-hex value
  push  bx                  ;push bx
  shr   ax                  ;ax = ax/16
  shr   ax
  shr   ax
  shr   ax
  loop  @@WriteHexValLoop1  ;go until no more values
  mov   cx,dx               ;cx<-dx
@@WriteHexValLoop2:
  pop   dx                  ;get value off stack
  mov   ah,02h              ;print character
  int   021h
  loop  @@WriteHexValLoop2

  pop   cx                  ;restore cx
  pop   bx                  ;restore bx
  pop   ds                  ;restore ds
  pop   si                  ;restore si
  ret

;============================================================================
; Data
;============================================================================
@envvars:   dw 0             ;environment vars
@envseg:    dw 0             ;environment vars segment
@commcount: dw 0             ;number of commands
@commline:  db 128 dup (0)   ;copy of the command line

@@SaveInt0: dw 0,0           ;Area used to save interrupts  int 0
@@SaveInt6: dw 0,0           ;                              int 6

@@HexValues: db "0123456789ABCDEF"
@@DivideByZeroErrorMessage:
  db 0dh,0ah,"Division by zero at: $"
@@InvalidOpCodeErrorMessage:
  db 0dh,0ah,"Invalid Opcode encountered at: $"
@@NoMemMessage:
  db "Error: Out of memory.$"
@@FreeMemErrorMessage:
  db "Error: Unable to free memory.$"
@@BlockSizeErrorMessage:
  db "Error: Unable to adjust program's block size.$"
