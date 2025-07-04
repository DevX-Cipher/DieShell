@echo off

:: BatchGotAdmin
:-------------------------------------
REM  --> Check for permissions
IF "%PROCESSOR_ARCHITECTURE%" EQU "amd64" (
>nul 2>&1 "%SYSTEMROOT%\SysWOW64\cacls.exe" "%SYSTEMROOT%\SysWOW64\config\system"
) ELSE (
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
)

REM --> If error flag set, we do not have admin.
if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getadmin.vbs"
    "%temp%\getadmin.vbs"
    exit /B

:gotAdmin
    if exist "%temp%\getadmin.vbs" ( del "%temp%\getadmin.vbs" )
    pushd "%CD%"
    CD /D "%~dp0"
:--------------------------------------

:: Step 1: Compile the DLL
:: Set variables for DLL compilation
set SOURCE_FILE=src\dllmain.cpp
set OUTPUT_DIR=bin
set OBJ_DIR=obj
set DLL_NAME=DieShell.dll

:: Ensure output directories exist
if not exist %OUTPUT_DIR% mkdir %OUTPUT_DIR%
if not exist %OBJ_DIR% mkdir %OBJ_DIR%

:: Find vswhere.exe
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% (
    echo Error: vswhere.exe not found. Ensure Visual Studio 2017 or later is installed.
    exit /b 1
)

:: Find Visual Studio installation
for /f "tokens=*" %%i in ('%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath') do set VSINSTALL=%%i
if not defined VSINSTALL (
    echo Error: No Visual Studio installation found.
    exit /b 1
)

set VCVARSALL="%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat"
if not exist %VCVARSALL% (
    echo Error: vcvarsall.bat not found at %VCVARSALL%.
    exit /b 1
)

:: Set up Visual Studio environment for x64
echo Setting up Visual Studio environment...
call %VCVARSALL% x64
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to set up Visual Studio environment.
    exit /b %ERRORLEVEL%
)

:: Compile the DLL with Unicode definitions, required libraries, and object file output to obj folder
echo Compiling %SOURCE_FILE% to %OUTPUT_DIR%\%DLL_NAME% with object file in %OBJ_DIR%...
cl /nologo /LD /DUNICODE /D_UNICODE /Fo:%OBJ_DIR%\ /Fe:%OUTPUT_DIR%\%DLL_NAME% %SOURCE_FILE% /link /MACHINE:X64 user32.lib shell32.lib shlwapi.lib runtimeobject.lib
if %ERRORLEVEL% neq 0 (
    echo Error: Compilation failed.
    exit /b %ERRORLEVEL%
)

echo Build successful: %OUTPUT_DIR%\%DLL_NAME%
echo Object file generated in: %OBJ_DIR%

:: Move the compiled DLL to the Die\Die directory
set EXTRACT_FOLDER=Die
set INNER_FOLDER=Die
if not exist %EXTRACT_FOLDER%\%INNER_FOLDER% mkdir %EXTRACT_FOLDER%\%INNER_FOLDER%
echo Moving %DLL_NAME% to %EXTRACT_FOLDER%\%INNER_FOLDER%...
move %OUTPUT_DIR%\%DLL_NAME% %EXTRACT_FOLDER%\%INNER_FOLDER%\%DLL_NAME%
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to move %DLL_NAME% to %EXTRACT_FOLDER%\%INNER_FOLDER%.
    exit /b %ERRORLEVEL%
)

:: Delete the source directory if empty
echo Deleting source directory %OUTPUT_DIR%...
rd /s /q %OUTPUT_DIR%
if %ERRORLEVEL% neq 0 (
    echo Warning: Failed to delete %OUTPUT_DIR%. It may not be empty or in use.
) else (
    echo Successfully deleted %OUTPUT_DIR%.
)

:: Step 2: Get Windows Kits root path
for /f "tokens=3*" %%A in ('reg query "HKLM\SOFTWARE\Microsoft\Windows Kits\Installed Roots" /v KitsRoot10 2^>nul') do (
    set "kitsroot=%%A %%B"
)

if not defined kitsroot (
    echo [ERROR] Windows 10 SDK not found.
    exit /b 1
)

:: Step 3: Find latest version with both tools
set "MAKEAPPX_PATH="
set "SIGNTOOL_PATH="

for /f "delims=" %%D in ('dir /b /ad "%kitsroot%\bin" ^| sort /r') do (
    if exist "%kitsroot%\bin\%%D\x64\makeappx.exe" if exist "%kitsroot%\bin\%%D\x64\signtool.exe" (
        set "MAKEAPPX_PATH=%kitsroot%\bin\%%D\x64\makeappx.exe"
        set "SIGNTOOL_PATH=%kitsroot%\bin\%%D\x64\signtool.exe"
        goto :found_tools
    )
)

:found_tools
if not defined MAKEAPPX_PATH (
    echo [ERROR] makeappx.exe not found.
    exit /b 1
)
if not defined SIGNTOOL_PATH (
    echo [ERROR] signtool.exe not found.
    exit /b 1
)

echo Found makeappx.exe: [%MAKEAPPX_PATH%]
echo Found signtool.exe: [%SIGNTOOL_PATH%]

:: Step 4: Download and extract DIE-engine
set DOWNLOAD_URL=https://github.com/horsicq/DIE-engine/releases/download/Beta/die_win64_portable_3.11_x64.zip
set DOWNLOAD_FILE=die_win64_portable_3.11_x64.zip

echo Creating folders %EXTRACT_FOLDER%...
if not exist %EXTRACT_FOLDER% mkdir %EXTRACT_FOLDER%

echo Downloading %DOWNLOAD_FILE%...
curl -LJO %DOWNLOAD_URL%

echo Extracting files...
tar -xf %DOWNLOAD_FILE% -C %EXTRACT_FOLDER%\%INNER_FOLDER%

echo Cleanup: Removing downloaded ZIP file...
del %DOWNLOAD_FILE%

:: Step 5: Verify DieShell.dll exists
:CheckFiles
echo Checking if DieShell.dll exists...
if exist %EXTRACT_FOLDER%\%INNER_FOLDER%\DieShell.dll (
    echo DieShell.dll found.
) else (
    echo DieShell.dll not found in %EXTRACT_FOLDER%\%INNER_FOLDER%.
    pause
    goto CheckFiles
)

:: Step 6: Create and sign certificate
powershell -Command "New-SelfSignedCertificate -Type Custom -Subject 'CN=Die' -KeyUsage DigitalSignature -FriendlyName 'SelfSignCert' -CertStoreLocation 'Cert:\CurrentUser\My' -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3', '2.5.29.19={text}')"
set /p password="Enter a password for the PFX file: "

for /f "tokens=*" %%a in ('powershell -Command "Get-ChildItem -Path Cert:\CurrentUser\My | Where-Object { $_.Subject -eq 'CN=Die' } | Select-Object -ExpandProperty Thumbprint"') do (
    set thumbprint=%%a
)

if not defined thumbprint (
    echo Certificate not found.
    exit /b 1
)

set "batch_dir=%~dp0"
powershell -Command "Export-PfxCertificate -Cert Cert:\CurrentUser\My\%thumbprint% -FilePath '%batch_dir%Die.pfx' -Password (ConvertTo-SecureString -String '%password%' -Force -AsPlainText)"

echo Removing certificate from CertMgr...
certutil -delstore My %thumbprint%
powershell -Command "Remove-Item -Path Cert:\CurrentUser\My\%thumbprint%"

:: Run the makeappx.exe command and redirect output to a log file
echo Packing MSIX...
"%MAKEAPPX_PATH%" pack /d "%EXTRACT_FOLDER%" /p Die.msix /nv
if errorlevel 1 (
    echo [ERROR] Failed to create MSIX.
    exit /b 1
)

set /p PASSWORD="Enter the password for the .pfx file: "

set DIRECTORY=%~dp0

for /R %DIRECTORY% %%F in (*.pfx) do (
    set PFX_PATH="%%F"
)

for /R %DIRECTORY% %%F in (*.msix) do (
    set MSIX_PATH="%%F"
)

echo Signing MSIX...
"%SIGNTOOL_PATH%" sign /fd SHA256 /a /f "Die.pfx" /p %password% "Die.msix"
if errorlevel 1 (
    echo [ERROR] Failed to sign MSIX.
    exit /b 1
)
:: Import the certificate
set "batch_dir=%~dp0"

:: Step 9: Import certificate to TrustedPeople store
for /f "tokens=*" %%a in ('powershell -Command "Get-ChildItem -Path Cert:\LocalMachine\TrustedPeople | Where-Object { $_.Subject -eq 'CN=Die' } | Select-Object -ExpandProperty Thumbprint"') do (
    certutil -delstore TrustedPeople %%a
)

powershell -Command "& { $msixFile = '%MSIX_PATH%'; if (!(Test-Path -Path $msixFile)) { Write-Host 'Error: MSIX file not found.'; exit }; $signature = Get-AuthenticodeSignature -FilePath $msixFile; $certificate = $signature.SignerCertificate; $certificate.Export('Cert') | Set-Content -Encoding Byte 'certificate.cer'; }"

certutil -addstore -f TrustedPeople certificate.cer

:: Clean up
del certificate.cer
del "%batch_dir%Die.pfx" /f /q

:: Step 10: Install MSIX
set /p user_input="Do you want to install the package manually? (yes/no): "
if /I "%user_input%" EQU "yes" (
    echo Please install the package manually.
) else (
    echo Installing...
    powershell -Command "Add-AppPackage -Path %MSIX_PATH%"
)

:: Wait for 10 seconds
timeout /t 10

exit