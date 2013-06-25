@exit:
  mov  ax,04c00h              ; ax<-4C00 DOS Exit
  int  021h                   ; call DOS interrupt