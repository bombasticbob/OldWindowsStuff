@ECHO OFF
REPEAT
LET X1=INPUT("Enter file spec to search for, <ENTER> to quit: ")
LET X1=LTRIM(RTRIM(X1))
IF (LEN(X1)<>0)
  LET X2=NETVIEW()
  FOR I FROM 0 TO NROWS(X2)-1
    IF (LEN(X2[i,2])<>0)
      LET X3=NETVIEW(X2[i,2])
      FOR J FROM 0 TO NROWS(X3)-1
         ECHO.%(X3[J,0]++"\\"++X3[J,1]++"\\"++X1)%
         DIR "%(X3[J,0]++"\\"++X3[J,1]++"\\"++X1)%" /S/B
      NEXT J
    ENDIF
  NEXT I
ENDIF
UNTIL LEN(X1)=0
