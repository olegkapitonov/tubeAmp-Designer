; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

  !include "MUI2.nsh"

; The name of the installer
Name "tubeAmp Designer"

; The file to write
OutFile "tubeAmp-Designer-x64-win.exe"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

; Build Unicode installer
Unicode True

; The default installation directory
InstallDir "$PROGRAMFILES64\tubeAmp Designer"

;--------------------------------

  !define MUI_ABORTWARNING

; Pages


  !insertmacro MUI_PAGE_LICENSE LICENSE.txt
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  
  Var /GLOBAL SmDir
  
  !insertmacro MUI_PAGE_STARTMENU 0 $SmDir
  
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_LANGUAGE "English"
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------

; The stuff to install
Section "tubeAmp Designer" SecDummy

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "tAD.exe"
  File "*.dll"
  File "*.qm"
  File logo.ico
  SetOutPath $INSTDIR\platforms
  File "platforms\*.*"
  SetOutPath $INSTDIR\styles
  File "styles\*.*"
  SetOutPath $INSTDIR\profiles\v1.0
  File "profiles\v1.0\*.*"
  SetOutPath $INSTDIR\profiles\v1.2
  File "profiles\v1.2\*.*"
  SetOutPath $INSTDIR\data
  File "data\*.*"
  
  SetOutPath $INSTDIR
  
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN 0
    
  CreateDirectory "$SMPROGRAMS\$SmDir"
  CreateShortcut "$SMPROGRAMS\$SmDir\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortcut "$SMPROGRAMS\$SmDir\tubeAmp Designer.lnk" "$INSTDIR\tAD.exe" "" "$INSTDIR\logo.ico" 0
  
  !insertmacro MUI_STARTMENU_WRITE_END
  
SectionEnd

;--------------------------------

;Descriptions

  ;Language strings
  LangString DESC_SecDesigner ${LANG_ENGLISH} "tubeAmp Designer"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDummy} $(DESC_SecDesigner)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END
 
;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove files and uninstaller
  Delete $INSTDIR\tAD.exe
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\tAD.exe
  Delete $INSTDIR\*.dll
  Delete $INSTDIR\*.qm
  Delete $INSTDIR\platforms\*.*
  Delete $INSTDIR\styles\*.*
  Delete $INSTDIR\profiles\v1.0\*.*
  Delete $INSTDIR\profiles\v1.2\*.*
  Delete $INSTDIR\data\*.*
  RMDir $INSTDIR\platforms
  RMDir $INSTDIR\styles
  RMDir $INSTDIR\profiles\v1.0
  RMDir $INSTDIR\profiles\v1.2
  RMDir $INSTDIR\profiles
  RMDir $INSTDIR\data

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\tubeAmp Designer\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\tubeAmp Designer"
  RMDir "$INSTDIR"

SectionEnd
