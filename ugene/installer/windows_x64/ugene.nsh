# NSIS Script For Unipro UGENE

################################################################
# Modern UI
!include "MUI.nsh"
!verbose 4

# Compressor
    SetCompressor /SOLID /FINAL lzma
    SetCompressorDictSize 64

# Interface Settings
    !define MUI_ABORTWARNING
    !define MUI_HEADERIMAGE
    !define MUI_HEADERIMAGE_BITMAP "images\header.bmp"
    !define MUI_SPECIALIMAGE
    !define MUI_WELCOMEFINISHPAGE_BITMAP "images\welcome.bmp"
    !define MUI_FINISHPAGE_RUN "$INSTDIR\ugeneui.exe"

# Pages
    !insertmacro MUI_PAGE_WELCOME
    !insertmacro MUI_PAGE_LICENSE ../source/LICENSE
    !define MUI_PAGE_CUSTOMFUNCTION_LEAVE checkInstDir
    !insertmacro MUI_PAGE_DIRECTORY
    !insertmacro MUI_PAGE_INSTFILES
    !insertmacro MUI_PAGE_FINISH

    !insertmacro MUI_UNPAGE_CONFIRM
    !insertmacro MUI_UNPAGE_INSTFILES

# Languages
    !insertmacro MUI_LANGUAGE "English"

Function "checkInstDir"
   CreateDirectory $INSTDIR
   Iferrors 0 +3
   MessageBox MB_ICONEXCLAMATION 'The directory "$INSTDIR" is not available for writing. Please choose another directory.'
   abort
FunctionEnd

!include ugene_extensions.nsh

################################################################
# Installer options
    !define CompanyName "Unipro"
    !define ProductName "UGENE"
    !define FullProductName "${CompanyName} ${ProductName}"
    !define TargetPlatform x64

    !define ReleaseBuildDir "..\..\src\_release"
    !include ${ReleaseBuildDir}\version.nsis
    !ifndef ProductVersion
    !define ProductVersion "unknown"
    !endif


    Name    ${FullProductName}
    InstallDir "$PROGRAMFILES64\${FullProductName}"
    InstallDirRegKey HKCU "Software\${CompanyName}\${ProductName}" ""
    DirText "Please select the folder below"
    BrandingText "${FullProductName}"
    UninstallText "This will uninstall ${FullProductName} from your system"
    Icon "images/install.ico"
    UninstallIcon "images/uninstall.ico"
    ;BGGradient 000000 400040 FFFFFF
    SetFont "Tahoma" 9
    CRCCheck On

    !ifndef UGENE_DISTRIB_FILE_NAME
        OutFile "build/ugene_${ProductVersion}_win_${TargetPlatform}.exe"
    !else
        OutFile "build/${UGENE_DISTRIB_FILE_NAME}"
    !endif


################################################################
Section "Build"
    !include "FileFunc.nsh"

SetRegView 64
    SectionIn 1 2 RO
    SetOutPath $INSTDIR
    
    # Remove old install
    RMDir /r "$INSTDIR\plugins"
    Delete "$INSTDIR\ugene.exe"

    SetOverwrite IfNewer
    File "${ReleaseBuildDir}\ugeneui.exe"
    File "${ReleaseBuildDir}\ugenecl.exe"
    File "${ReleaseBuildDir}\ugeneui.map"
    File "${ReleaseBuildDir}\ugenecl.map"
    File "${ReleaseBuildDir}\ugenem.exe"
    Rename ugenecl.exe ugene.exe
    File "${ReleaseBuildDir}\U2Algorithm.dll"
    File "${ReleaseBuildDir}\U2Core.dll"
    File "${ReleaseBuildDir}\U2Designer.dll"
    File "${ReleaseBuildDir}\U2Formats.dll"
    File "${ReleaseBuildDir}\U2Gui.dll"
    File "${ReleaseBuildDir}\U2Lang.dll"
    File "${ReleaseBuildDir}\U2Private.dll"
    File "${ReleaseBuildDir}\U2Remote.dll"
    File "${ReleaseBuildDir}\U2Test.dll"
    File "${ReleaseBuildDir}\U2View.dll"
    File "${ReleaseBuildDir}\ugenedb.dll"
    File "${ReleaseBuildDir}\U2Algorithm.map"
    File "${ReleaseBuildDir}\U2Core.map"
    File "${ReleaseBuildDir}\U2Designer.map"
    File "${ReleaseBuildDir}\U2Formats.map"
    File "${ReleaseBuildDir}\U2Gui.map"
    File "${ReleaseBuildDir}\U2Lang.map"
    File "${ReleaseBuildDir}\U2Private.map"
    File "${ReleaseBuildDir}\U2Remote.map"
    File "${ReleaseBuildDir}\U2Test.map"
    File "${ReleaseBuildDir}\U2View.map"
    File "${ReleaseBuildDir}\ugenedb.map"
    File "${ReleaseBuildDir}\transl_en.qm"
    File "${ReleaseBuildDir}\transl_ru.qm"
    File "${ReleaseBuildDir}\transl_cs.qm"
    File "${ReleaseBuildDir}\transl_zh.qm"
    File "includes\*.*"

    SetOutPath $INSTDIR\data
    File /r /x .svn "..\..\data\*.*"

    SetOutPath $INSTDIR\styles
    File "includes\styles\qtdotnet2.dll"

    SetOutPath $INSTDIR\plugins
    File /r /x .svn "includes\plugins\*.*"

    !insertmacro AddPlugin annotator
    !insertmacro AddPlugin ball
    !insertmacro AddPlugin biostruct3d_view
    !insertmacro AddPlugin chroma_view
    !insertmacro AddPlugin circular_view
    !insertmacro AddPlugin remote_service
    !insertmacro AddPlugin cuda_support
    !insertmacro AddPlugin opencl_support
    !insertmacro AddPlugin dna_export
    !insertmacro AddPlugin dna_graphpack
    !insertmacro AddPlugin dna_stat
    !insertmacro AddPlugin dotplot
    !insertmacro AddPlugin enzymes
    !insertmacro AddPlugin external_tool_support
    !insertmacro AddPlugin genome_aligner
    !insertmacro AddPlugin gor4
    !insertmacro AddPlugin hmm2
    !insertmacro AddPlugin hmm3
    !insertmacro AddPlugin kalign
    !insertmacro AddPlugin orf_marker
    !insertmacro AddPlugin phylip
    !insertmacro AddPlugin primer3
    !insertmacro AddPlugin psipred
    !insertmacro AddPlugin query_designer
    !insertmacro AddPlugin remote_blast
    !insertmacro AddPlugin repeat_finder
    !insertmacro AddPlugin sitecon
    !insertmacro AddPlugin smith_waterman
#    !insertmacro AddPlugin umuscle
    !insertmacro AddPlugin weight_matrix
    !insertmacro AddPlugin workflow_designer
    !insertmacro AddPlugin dbi_bam
    !insertmacro AddPlugin expert_discovery
    !insertmacro AddPlugin ptools
    !insertmacro AddPlugin dna_flexibility
    !insertmacro AddPlugin spb
    !insertmacro AddPlugin variants
    
    SetOutPath $INSTDIR\tools
    File /r /x .svn "includes\tools\*.*"
    
    SetOutPath $INSTDIR\imageformats
    File /r /x .svn "includes\imageformats\*.*"

    SetOutPath $INSTDIR
    
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0

    var /GLOBAL warnText #collects warnings during the installation
    Iferrors 0 +2
    StrCpy $warnText "Warning: not all files were copied successfully!"
    
    # Write the uninstall keys for Windows
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FullProductName}" "DisplayName" "${FullProductName} ${PrintableVersion}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FullProductName}" "UninstallString" "$INSTDIR\Uninst.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FullProductName}" "Publisher" "Unipro"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FullProductName}" "DisplayVersion" "${PrintableVersion}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FullProductName}" "EstimatedSize" "$0"

    Iferrors 0 +2
    StrCpy $warnText "$warnText$\r$\nWarning: cannot create registry entries!"

    WriteUninstaller "$INSTDIR\Uninst.exe"


    Iferrors 0 +2
    StrCpy $warnText "$warnText$\r$\nWarning: cannot create uninstaller!"

    # Remove old config
    # Delete $APPDATA\${CompanyName}\${ProductName}.ini


SectionEnd

################################################################
; Register extensions
!insertmacro AssociateExtSectionGroup ''

################################################################
Section "Add Shortcuts"
    SectionIn 1
    SetShellVarContext all
    RMDir /r "$SMPROGRAMS\${FullProductName}"
    ClearErrors
    CreateDirectory "$SMPROGRAMS\${FullProductName}"
    CreateShortCut "$SMPROGRAMS\${FullProductName}\Launch UGENE.lnk" "$INSTDIR\ugeneui.exe" "" "$INSTDIR\ugeneui.exe" 0
    CreateShortCut "$SMPROGRAMS\${FullProductName}\Download User Manual.lnk" "$INSTDIR\download_manual.url" "" "$INSTDIR\download_manual.url" 0
    # make sure uninstall shortcut will be last item in Start menu
#    nsisStartMenu::RegenerateFolder "${FullProductName}"
    CreateShortCut "$SMPROGRAMS\${FullProductName}\Uninstall.lnk" "$INSTDIR\Uninst.exe" "" "$INSTDIR\Uninst.exe" 0
    CreateShortCut "$DESKTOP\${FullProductName}.lnk" "$INSTDIR\ugeneui.exe" "" "$INSTDIR\ugeneui.exe" 0

    Iferrors 0 +2
    StrCpy $warnText "$warnText$\r$\nWarning: cannot create program shortcuts!"
#Display all of collected warnings  
    var /GLOBAL warnTextLen
    StrLen $warnTextLen "$warnText"
    IntCmp $warnTextLen 0 +2
    MessageBox MB_ICONEXCLAMATION "$warnText"
SectionEnd

################################################################
Section Uninstall
    # Delete shortcuts
    SetShellVarContext all
    Delete "$DESKTOP\${FullProductName}.lnk"
    Delete "$SMPROGRAMS\${FullProductName}\*.*"
    RmDir /r "$SMPROGRAMS\${FullProductName}"

    # Delete Uninstaller And Unistall Registry Entries
    Delete "$INSTDIR\Uninst.exe"
    RMDir /r "$INSTDIR"
    DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${FullProductName}"
SectionEnd

################################################################
; Unregister extensions
!insertmacro AssociateExtSectionGroup 'un.'


################################################################
