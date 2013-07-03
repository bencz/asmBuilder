;============================================================================
; io.inc
; written by Alexandre Savelli Bencz, 2013
; A few file manipulation procedures for assembly programs written for the
; General Assembler
;
;============================================================================

;============================================================================
; Code
;============================================================================

Buffersize  equ  0100h      ;Text file buffer size

;----------------------------------------------------------------------------
; @OpenFile
; params  segment:offset of ascii NULL string containing file name 4 bytes
;         read/write  00 = read, 01 = write 02 = read/write        2 bytes
; returns AX = handle
;
;   Opens a file for reading or writing, if writing to a file then the
; file will be created or if an existing file of the same name exists,
; it will be overwritten
;----------------------------------------------------------------------------

filename  equ  [bp+6]
rwflag    equ  [bp+4]

@OpenFile:
  push  bp                  ;setup stack frame
  mov   bp,sp
  push  ds                  ;save ds
  push  dx                  ;save dx
  push  bx                  ;save bx
  lds   dx,filename         ;load file name
  mov   ax,rwflag           ;ax<-read/write flag
  cmp   ax,1                ;write a file?
  jne   @@OpenFile2         ;if not jump
@@OpenFile1:
  xor   cx,cx               ;clear cx
  mov   ah,03Ch             ;ah<-03ch DOS create file
  int   21h                 ;call DOS interrupt
  jc    @@CreateFileError   ;if error occured quit
  mov   bx,ax               ;Close the opened file
  mov   ah,03eh
  int   021h
  lds   dx,filename         ;reload the file name
@@OpenFile2:
  mov   al,0C0h             ;al<-0C0h DOS read/write flag
  or    al,rwflag           ;or in flag
  mov   ah,03dh             ;ah<-03dh DOS open file
  int   021h                ;call DOS
  jc    @@OpenFileError     ;jump on error
@@OpenFile3:
  pop   bx                  ;restore bx
  pop   dx                  ;restore dx
  pop   ds                  ;restore ds
  pop   bp                  ;restore bp
  ret   6                   ;return

@@OpenFileError:
  mov   ax,rwflag           ;check flag
  cmp   ax,2
  je    @@OpenFile1         ;is r/w flag? then truncate file
  push  cs                  ;ds<-cs
  pop   ds
  mov   dx,offset OpenFileErrorMessage
@@OpenFileError1:
  mov   ah,09h              ;ah<-09h DOS print string
  int   021h                ;call DOS interrupt
  lds   dx,filename         ;get the file name
  push  ds                  ;put ds on stack
  push  dx                  ;put dx on stack
  call  @WritePchar         ;display file name
  jmp   @exit               ;quit
@@CreateFileError:
  push  cs                  ;ds<-cs
  pop   ds
  mov   dx,offset CreateFileErrorMessage
  jmp   @@OpenFileError1    ;let code above finish this

;----------------------------------------------------------------------------
; @CloseFile
; params  filehandle  2 bytes
;----------------------------------------------------------------------------

filehandle   equ [bp+4]

@CloseFile:
  push  bp                  ;setup stack frame
  mov   bp,sp
  push  bx                  ;save bx
  push  ax                  ;save ax
  mov   bx,filehandle       ;bx<-filehandle
  mov   ah,03eh             ;ax<-03eh DOS close file
  int   021h                ;call DOS interrupt
  pop   ax                  ;restore ax
  pop   bx                  ;restore bx
  pop   bp                  ;restore bp
  ret   2                   ;return

;----------------------------------------------------------------------------
; @ReadFile
; params  seg:ofs of buffer to read to    4 bytes
;         number bytes to read            2 bytes
;         filehandle                      2 bytes
; returns AX=number bytes read
;----------------------------------------------------------------------------

buffer   equ  [bp+8]
count    equ  [bp+6]
handle   equ  [bp+4]
@ReadFile:
  push  bp                  ;setup stack frame
  mov   bp,sp
  push  cx                  ;save cx
  push  dx                  ;save dx
  push  bx                  ;save bx
  push  ds                  ;save ds

  lds   dx,buffer           ;ds:dx = address at bp+8
  mov   cx,count            ;cx<-[bp+6]
  mov   bx,handle           ;bx<-[bp+4]
  mov   ah,03fh             ;ah<-03fh
  int   021h                ;call dos
  jc    @@ReadFile2         ;read error occured
@@ReadFile1:

  pop   ds                  ;restore ds
  pop   bx                  ;restore bx
  pop   dx                  ;restore dx
  pop   cx                  ;restore cx
  pop   bp                  ;restore bp
  ret   8                   ;

@@ReadFile2:                ;had a read error
  xor   ax,ax               ;null out ax
  jmp   @@ReadFile1         ;return

;----------------------------------------------------------------------------
; @WriteFile
; params  seg:ofs of buffer to write from 4 bytes
;         number bytes to write           2 bytes
;         filehandle                      2 bytes
; returns AX=Number bytes written
;----------------------------------------------------------------------------

buffer   equ  [bp+8]
count    equ  [bp+6]
handle   equ  [bp+4]

@WriteFile:
  push  bp                  ;setup stack frame
  mov   bp,sp
  push  cx                  ;save cx
  push  dx                  ;save dx
  push  bx                  ;save bx
  push  ds                  ;save ds

  lds   dx,buffer           ;load buffer
  mov   cx,count            ;cx<-number of bytes to read from buffer
  mov   bx,handle           ;bx<-file handle
  mov   ah,040h             ;ah<-40h, DOS write to file from buffer
  int   021h                ;call DOS interrupt
  jc    @@writeerror        ;jump on error
@@WriteFile1:
  pop   ds                  ;restore ds
  pop   bx                  ;restore bx
  pop   dx                  ;restore dx
  pop   cx                  ;restore cx
  pop   bp                  ;restore bp
  ret   8                   ;return to call
@@writeerror:               ;
  xor   ax,ax               ;return zero bytes written if error
  jmp   @@WriteFile1        ;return


;----------------------------------------------------------------------------
; @OpenTextFile:
; params  seg:ofs of buffer to write from 4 bytes
;         read/write 00=read,01=write     2 bytes
; returns ax:dx = filestructure
;FILERECORD STRUCT
;    FR_handle dw     ?          read/write handle
;    FR_pos    dw     ?          position in buffer
;    FR_size   dw     ?          read/write buffer size
;    FR_flag   dw     ?          read/write buffer?
;    FR_buffer dw  0100h dup(?)  read/write buffer
;FILERECORD ENDS
;----------------------------------------------------------------------------

fname     equ  [bp+6]
rwflag    equ  [bp+4]

@OpenTextFile:
  push   bp                 ;setup stack frame
  mov    bp,sp
  push   es                 ;save es
  push   di                 ;save di
  push   bx                 ;save bx
  push   cx                 ;save cx

  les   si,fname            ;load in filename pointer
  push  es                  ;put in on the back stack
  push  si                  ;
  mov   ax,rwflag           ;now get the read write flag
  and   ax,01h              ;can only be read or write
  push  ax                  ;put flag onto the stack
  call  @OpenFile           ;now open the file

  push  ax                  ;save handle

  mov   ax,BufferSize + 8   ;File Record Size + BufferSize
  call  @AllocateMem        ;Allocate mem for file record
  mov   es,ax               ;es:di points to the read/write buffer
  mov   di,dx

  pop   ax                  ;restore handle

  mov   es:[di],ax          ;(file handle)
  xor   ax,ax               ;ax=0
  mov   es:[di+2],ax        ;(pos)
  mov   es:[di+4],ax        ;(length)
  mov   ax,rwflag           ;(read/write)
  mov   es:[di+6],ax        ;(r/w flag)
  cmp   ax,0                ;reading from text?
  jne   @@OpenTextFile1     ;if not then jump

  push  es                  ;put record segment on stack
  mov   ax,di               ;put record offset on stack
  add   ax,8                ;move up to buffer
  push  ax                  ;put offfset on stack
  mov   ax,BufferSize       ;ax<-Buffersize
  push  ax                  ;put buffersize on stack
  mov   ax,es:[di]          ;(file handle)
  push  ax                  ;put file handle onto the stack
  call  @ReadFile           ;read the file
  mov   es:[di+4],ax        ;save the number of bytes read

@@OpenTextFile1:
  mov   ax,es               ;ax<-record segment
  mov   dx,di               ;dx<-record offset

  pop   cx                  ;restore cx
  pop   bx                  ;restore bx
  pop   di                  ;restore di
  pop   es                  ;restore es
  pop   bp                  ;restore bp
  ret   6

;----------------------------------------------------------------------------
; @CloseTextFile
; params  seg:ofs to file record          4 bytes
;----------------------------------------------------------------------------

file   equ  [bp+4]

@CloseTextFile:
  push  bp                  ;setup stack frame
  mov   bp,sp
  push  ds                  ;save ds
  push  si                  ;save si
  push  dx                  ;save dx
  push  ax                  ;save ax

  lds   si,file             ;load file record
  mov   ax,[si+6]           ;check r/w flag
  cmp   ax,0                ;ax=0? read
  je    @@CloseTextFile1    ;jump if nothing to write

; dispose of write buffer

@@CloseTextFile1:
  mov   ax,[si]             ;get filehandle
  push  ax                  ;put it onto the stack
  call  @CloseFile          ;close the file
  mov   ax,ds               ;put record segment into ax
  mov   dx,si               ;put record offset into dx
  call  @FreeMem            ;get that memory back

  pop   ax                  ;restore ax
  pop   dx                  ;restore dx
  pop   si                  ;restore si
  pop   ds                  ;restore ds
  pop   bp                  ;restore bp
  ret   4

;----------------------------------------------------------------------------
; @ReadTextFile
; params  seg:ofs of buffer to write to   4 bytes
;         max number bytes to read        2 bytes
;         seg:ofs to file record          4 bytes
;
; Read text file into seg:ofs as a NULL terminated string when then end of
; a line has been met or max number of bytes-1 have been read.
;----------------------------------------------------------------------------

buffer  equ  [bp+10]
count   equ  [bp+8]
file    equ  [bp+4]
@ReadTextFile:
  push  bp                  ;setup stack frame
  mov   bp,sp
  push  es                  ;save es
  push  ds                  ;save ds
  push  si                  ;save si
  push  di                  ;save di
  push  ax                  ;save ax
  push  cx                  ;save cx
  push  dx                  ;save dx
  push  bx                  ;save bx

  cld                       ;clear decrement
  les   di,buffer           ;load buffer to write to
  lds   si,file             ;load file record
  mov   cx,count            ;cx<-Max number of bytes to read
  dec   cx                  ;
  mov   bx,[si+2]           ;
@@ReadTextFile1:            ;
  mov   al,[si+bx+8]        ;
  cmp   al,0dh              ;
  je    @@ReadTextFile2     ;
  cmp   al,0ah              ;
  je    @@ReadTextFile2     ;
  stosb                     ;
  dec   cx                  ;
@@ReadTextFile2:            ;
  inc   bx                  ;
  mov   dx,[si+2]           ;(position)
  inc   dx                  ;
  mov   [si+2],dx           ;(position)
  cmp   dx,[si+4]           ;(size)
  jne   @@ReadTextFile3     ;
  cmp   dx,BufferSize       ;end of file?
  jne   @@ReadTextFile4     ;

; read more into buffer
  push  ds                  ;
  mov   ax,si               ;
  add   ax,8                ;
  push  ax                  ;
  mov   ax,BufferSize       ;
  push  ax                  ;
  mov   ax,[si]             ;(file handle)
  push  ax                  ;
  call  @ReadFile           ;
  mov   [si+4],ax           ;(size)
  xor   ax,ax               ;
  mov   [si+2],ax           ;(position)
  xor   bx,bx               ;
@@ReadTextFile3:            ;
  cmp   al,0ah              ;
  je    @@ReadTextFile4     ;
  jcxz  @@ReadTextFile4     ;
  jmp   @@ReadTextFile1     ;
@@ReadTextFile4:            ;
  xor   al,al               ;
  stosb                     ;

  pop   bx                  ;restore bx
  pop   dx                  ;restore dx
  pop   cx                  ;restore cx
  pop   ax                  ;restore ax
  pop   di                  ;restore di
  pop   si                  ;restore si
  pop   ds                  ;restore ds
  pop   es                  ;restore es
  pop   bp                  ;restore bp
  ret   10

;----------------------------------------------------------------------------
; @TextEOF
; params  seg:ofs to file record          4 bytes
; returns ax=1 if EOF
;----------------------------------------------------------------------------

file   equ  [bp+4]
@TextEOF:
  push  bp                  ;setup stack frame
  mov   bp,sp
  push  ds                  ;save ds
  push  si                  ;save di

  xor   ax,ax               ;null out ax
  lds   si,file             ;ds:si=file record
  mov   bx,[si+2]           ;bx=buffer position
  cmp   bx,[si+4]           ;bx==number bytes read?
  jne   @@TextEOF1          ;no then quit
  cmp   bx,BufferSize       ;at end of read buffer?
  je    @@TextEOF1          ;yes then quit
  mov   ax,1                ;pos==size != fullsize
@@TextEOF1:

  pop   si                  ;restore di
  pop   ds                  ;restore ds
  pop   bp                  ;restore bp
  ret   4

;----------------------------------------------------------------------------
; @WritePchar
; params  seg:ofs of string               4 bytes
;----------------------------------------------------------------------------

string  equ  [bp+4]
@WritePchar:
  push  bp                  ;setup stack frame
  mov   bp,sp
  push  ds                  ;save ds
  push  si                  ;save si
  push  ax                  ;save ax
  push  dx                  ;save dx

  lds   si,string           ;load string pointer
@@WritePchar1:
  lodsb                     ;read a byte
  cmp   al,0                ;is it null?
  je    @@WritePchar2       ;yes? then done
  mov   ah,02h              ;ah<-02h DOS print char
  mov   dl,al               ;move character to dl
  int   021h                ;call DOS
  jmp   @@WritePchar1       ;keep going
@@WritePchar2:

  pop   dx                  ;restore dx
  pop   ax                  ;restore ax
  pop   si                  ;restore si
  pop   ds                  ;restore ds
  pop   bp                  ;restore bp
  ret   4                   ;return

;============================================================================
; Data
;============================================================================

OpenFileErrorMessage   db "Unable to open file: $"
CreateFileErrorMessage db "Unable to create file: $"