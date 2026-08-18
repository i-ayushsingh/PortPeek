; ==============================================================================
; PortPeek NSIS Installer Script
; Lightweight, Modern UI 2, Per-User (No Admin Rights Required)
; ==============================================================================

Unicode True
SetCompressor /SOLID lzma
RequestExecutionLevel user

!define PRODUCT_NAME "PortPeek"
!define PRODUCT_VERSION "0.2.0"
!define PRODUCT_PUBLISHER "PortPeek"
!define PRODUCT_WEB_SITE "https://i-ayushsingh.github.io/PortPeek/"
!define PRODUCT_EXE "PortPeek.exe"
!define PRODUCT_DIR_REGKEY "Software\PortPeek"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\PortPeek"
!define PRODUCT_UNINST_ROOT_KEY HKCU

; Output file name
OutFile "PortPeek-Setup-v0.2.0.exe"

; Default install directory ($LOCALAPPDATA\Programs\PortPeek)
InstallDir "$LOCALAPPDATA\Programs\PortPeek"
InstallDirRegKey HKCU "${PRODUCT_DIR_REGKEY}" ""

; Modern UI 2
!include "MUI2.nsh"
!include "FileFunc.nsh"

; Interface Settings
!define MUI_ABORTWARNING
!define MUI_ICON "res\appicon.ico"
!define MUI_UNICON "res\appicon.ico"

; ------------------------------------------------------------------------------
; Installer Pages
; ------------------------------------------------------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; Finish Page with "Launch PortPeek"
!define MUI_FINISHPAGE_RUN "$INSTDIR\${PRODUCT_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch PortPeek now"
!define MUI_FINISHPAGE_SHOWREADME ""
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Create Desktop Shortcut"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION CreateDesktopShortcutOpt
!insertmacro MUI_PAGE_FINISH

; ------------------------------------------------------------------------------
; Uninstaller Pages
; ------------------------------------------------------------------------------
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Language
!insertmacro MUI_LANGUAGE "English"

; ------------------------------------------------------------------------------
; Installer Section
; ------------------------------------------------------------------------------
Section "MainSection" SEC01
    SetOutPath "$INSTDIR"
    SetOverwrite on

    ; Stop any running instance before overwriting
    nsExec::Exec 'powershell -Command "Stop-Process -Name PortPeek -Force -ErrorAction SilentlyContinue"'
    Sleep 300

    ; Copy files
    File "PortPeek.exe"
    File "LICENSE"
    File "README.md"
    File "res\appicon.ico"

    ; Create Start Menu Shortcuts
    CreateDirectory "$SMPROGRAMS\PortPeek"
    CreateShortcut "$SMPROGRAMS\PortPeek\PortPeek.lnk" "$INSTDIR\${PRODUCT_EXE}" "" "$INSTDIR\appicon.ico" 0
    CreateShortcut "$SMPROGRAMS\PortPeek\Uninstall PortPeek.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\appicon.ico" 0

    ; Write Uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Write Installation Registry Keys
    WriteRegStr HKCU "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR"
    WriteRegStr HKCU "${PRODUCT_DIR_REGKEY}" "Version" "${PRODUCT_VERSION}"

    ; Write Add/Remove Programs (Uninstall) Registry Keys
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\${PRODUCT_EXE},0"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1

    ; Calculate and write EstimatedSize
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
SectionEnd

Function CreateDesktopShortcutOpt
    CreateShortcut "$DESKTOP\PortPeek.lnk" "$INSTDIR\${PRODUCT_EXE}" "" "$INSTDIR\appicon.ico" 0
FunctionEnd

; ------------------------------------------------------------------------------
; Uninstaller Section
; ------------------------------------------------------------------------------
Section "Uninstall"
    ; Stop running instance
    nsExec::Exec 'powershell -Command "Stop-Process -Name PortPeek -Force -ErrorAction SilentlyContinue"'
    Sleep 300

    ; Remove Run on Startup registry key if set
    DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "PortPeek"

    ; Remove App registry keys
    DeleteRegKey HKCU "${PRODUCT_DIR_REGKEY}"
    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"

    ; Remove Shortcuts
    Delete "$SMPROGRAMS\PortPeek\PortPeek.lnk"
    Delete "$SMPROGRAMS\PortPeek\Uninstall PortPeek.lnk"
    RMDir "$SMPROGRAMS\PortPeek"
    Delete "$DESKTOP\PortPeek.lnk"

    ; Remove Files
    Delete "$INSTDIR\${PRODUCT_EXE}"
    Delete "$INSTDIR\LICENSE"
    Delete "$INSTDIR\README.md"
    Delete "$INSTDIR\appicon.ico"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"

    SetAutoClose false
SectionEnd

; ------------------------------------------------------------------------------
; Init Functions
; ------------------------------------------------------------------------------
Function .onInit
    ; Stop any existing instance
    nsExec::Exec 'powershell -Command "Stop-Process -Name PortPeek -Force -ErrorAction SilentlyContinue"'
FunctionEnd

Function un.onInit
    ; Stop any existing instance
    nsExec::Exec 'powershell -Command "Stop-Process -Name PortPeek -Force -ErrorAction SilentlyContinue"'
FunctionEnd
