# Microsoft Developer Studio Project File - Name="patterns" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=patterns - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "patterns.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "patterns.mak" CFG="patterns - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "patterns - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "patterns - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "patterns - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".." /I "..\engine" /D "NDEBUG" /D "HAVE_CONFIG_H" /D "WIN32" /D "_MBCS" /D "_LIB" /D "BUILDING_GNUGO_ENGINE" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "patterns - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".." /I "..\engine" /D "_DEBUG" /D "HAVE_CONFIG_H" /D "WIN32" /D "_MBCS" /D "_LIB" /D "BUILDING_GNUGO_ENGINE" /YX /FD /GZ  /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "patterns - Win32 Release"
# Name "patterns - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\conn.c
# End Source File
# Begin Source File

SOURCE=.\conn.db

!IF  "$(CFG)" == "patterns - Win32 Release"

USERDEP__CONN_="Release\mkpat.exe"	"conn.db"	
# Begin Custom Build
InputPath=.\conn.db

"conn.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Release\mkpat -c conn <conn.db >conn.c

# End Custom Build

!ELSEIF  "$(CFG)" == "patterns - Win32 Debug"

USERDEP__CONN_="Debug\mkpat.exe"	"conn.db"	
# Begin Custom Build
InputPath=.\conn.db

"conn.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Debug\mkpat -c conn <conn.db >conn.c

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\connections.c
# End Source File
# Begin Source File

SOURCE=.\eyes.c
# End Source File
# Begin Source File

SOURCE=.\eyes.db

!IF  "$(CFG)" == "patterns - Win32 Release"

USERDEP__EYES_="Release\mkeyes.exe"	"eyes.db"	
# Begin Custom Build
InputPath=.\eyes.db

"eyes.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Release\mkeyes <eyes.db >eyes.c

# End Custom Build

!ELSEIF  "$(CFG)" == "patterns - Win32 Debug"

USERDEP__EYES_="Debug\mkeyes.exe"	"eyes.db"	
# Begin Custom Build
InputPath=.\eyes.db

"eyes.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Debug\mkeyes <eyes.db >eyes.c

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\helpers.c
# End Source File
# Begin Source File

SOURCE=.\hoshi.sgf

!IF  "$(CFG)" == "patterns - Win32 Release"

USERDEP__HOSHI="Release\joseki.exe"	"hoshi.sgf"	
# Begin Custom Build
InputPath=.\hoshi.sgf

"hoshi.db" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Release\joseki JH <hoshi.sgf >hoshi.db

# End Custom Build

!ELSEIF  "$(CFG)" == "patterns - Win32 Debug"

USERDEP__HOSHI="Debug\joseki.exe"	"hoshi.sgf"	
# Begin Custom Build
InputPath=.\hoshi.sgf

"hoshi.db" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Debug\joseki JH <hoshi.sgf >hoshi.db

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\komoku.sgf

!IF  "$(CFG)" == "patterns - Win32 Release"

USERDEP__KOMOK="Release\joseki.exe"	"komoku.sgf"	
# Begin Custom Build
InputPath=.\komoku.sgf

"komoku.db" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Release\joseki JK <komoku.sgf >komoku.db

# End Custom Build

!ELSEIF  "$(CFG)" == "patterns - Win32 Debug"

USERDEP__KOMOK="Debug\joseki.exe"	"komoku.sgf"	
# Begin Custom Build
InputPath=.\komoku.sgf

"komoku.db" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Debug\joseki JK <komoku.sgf >komoku.db

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mokuhazushi.sgf

!IF  "$(CFG)" == "patterns - Win32 Release"

USERDEP__MOKUH="Release\joseki.exe"	"mokuhazushi.sgf"	
# Begin Custom Build
InputPath=.\mokuhazushi.sgf

"mokuhazushi.db" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Release\joseki JM <mokuhazushi.sgf >mokuhazushi.db

# End Custom Build

!ELSEIF  "$(CFG)" == "patterns - Win32 Debug"

USERDEP__MOKUH="Debug\joseki.exe"	"mokuhazushi.sgf"	
# Begin Custom Build
InputPath=.\mokuhazushi.sgf

"mokuhazushi.db" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Debug\joseki JM <mokuhazushi.sgf >mokuhazushi.db

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\patterns.c
# End Source File
# Begin Source File

SOURCE=.\patterns.db

!IF  "$(CFG)" == "patterns - Win32 Release"

USERDEP__PATTE="Release\mkpat.exe"	"patterns.db"	"hoshi.db"	"komoku.db"	"sansan.db"	"mokuhazushi.db"	"takamoku.db"	
# Begin Custom Build
InputPath=.\patterns.db

"patterns.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cat patterns.db hoshi.db komoku.db sansan.db mokuhazushi.db takamoku.db >temp.db 
	Release\mkpat pat <temp.db >patterns.c 
	del temp.db 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "patterns - Win32 Debug"

USERDEP__PATTE="Debug\mkpat.exe"	"patterns.db"	"hoshi.db"	"komoku.db"	"sansan.db"	"mokuhazushi.db"	"takamoku.db"	
# Begin Custom Build
InputPath=.\patterns.db

"patterns.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy patterns.db +  hoshi.db + komoku.db + sansan.db + mokuhazushi.db + takamoku.db temp.db 
	Debug\mkpat pat <temp.db >patterns.c 
	del temp.db 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sansan.sgf

!IF  "$(CFG)" == "patterns - Win32 Release"

USERDEP__SANSA="Release\joseki.exe"	"sansan.sgf"	
# Begin Custom Build
InputPath=.\sansan.sgf

"sansan.db" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Release\joseki JS <sansan.sgf >sansan.db

# End Custom Build

!ELSEIF  "$(CFG)" == "patterns - Win32 Debug"

USERDEP__SANSA="Debug\joseki.exe"	"sansan.sgf"	
# Begin Custom Build
InputPath=.\sansan.sgf

"sansan.db" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Debug\joseki JS <sansan.sgf >sansan.db

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\takamoku.sgf

!IF  "$(CFG)" == "patterns - Win32 Release"

USERDEP__TAKAM="Release\joseki.exe"	"takamoku.sgf"	
# Begin Custom Build
InputPath=.\takamoku.sgf

"takamoku.db" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Release\joseki JT <takamoku.sgf >takamoku.db

# End Custom Build

!ELSEIF  "$(CFG)" == "patterns - Win32 Debug"

USERDEP__TAKAM="Debug\joseki.exe"	"takamoku.sgf"	
# Begin Custom Build
InputPath=.\takamoku.sgf

"takamoku.db" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Debug\joseki JT <takamoku.sgf >takamoku.db

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
