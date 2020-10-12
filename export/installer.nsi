; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

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

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "tubeAmp Designer (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "tAD.exe"
  File "*.dll"
  File "*.qm"
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
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\tubeAmp Designer"
  CreateShortcut "$SMPROGRAMS\tubeAmp Designer\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortcut "$SMPROGRAMS\tubeAmp Designer\tubeAmp Designer.lnk" "$INSTDIR\tAD.exe" "" "$INSTDIR\tAD.exe" 0
  
SectionEnd

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
