;============================================================================
; io.inc
; written by Alexandre Savelli Bencz
; A few file manipulation procedures for assembly programs written for the
; General Assembler
;
;
;============================================================================

;============================================================================
; Code
;============================================================================

Buffersize  equ  0100h      ;Text file buffer size

; ???????????????????????
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
@openFile