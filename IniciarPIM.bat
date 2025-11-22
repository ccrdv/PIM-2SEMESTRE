@echo off
:: ================================================================
:: IniciarPIM.bat - VERSÃO MÍNIMA
::   - Mapeia Z: para \\SERVIDOR\python_embedded
::   - Cria config.txt se não existir
::   - NÃO abre nenhum programa
:: ================================================================

set "SERVIDOR=SERVIDOR"
set "COMPARTILHAMENTO=python_embedded"
set "NETWORK_PATH=\\%SERVIDOR%\%COMPARTILHAMENTO%"

:: ---------------------------------------------------------------
:: 1) Verifica se o servidor está online
:: ---------------------------------------------------------------
echo.
echo Verificando conexão com %SERVIDOR%...
ping -n 1 %SERVIDOR% >nul 2>&1
if errorlevel 1 (
    echo.
    echo [Falha] Servidor offline ou inacessível.
    echo         Tente novamente mais tarde.
    echo.
    timeout /t 5 >nul
    exit /b 1
)

:: ---------------------------------------------------------------
:: 2) Remove mapeamento antigo
:: ---------------------------------------------------------------
net use Z: /delete /yes >nul 2>&1

:: ---------------------------------------------------------------
:: 3) Mapeia Z: (SEM persistência)
:: ---------------------------------------------------------------
net use Z: "%NETWORK_PATH%" /persistent:no >nul 2>&1
if errorlevel 1 (
    echo.
    echo [Falha] Não foi possível mapear Z:.
    echo         Verifique o nome do servidor ou permissões.
    echo.
    timeout /t 5 >nul
    exit /b 1
)

echo.
echo [Sucesso] Z: mapeado para:
echo           %NETWORK_PATH%
echo.

:: ---------------------------------------------------------------
:: 4) Cria config.txt se não existir
:: ---------------------------------------------------------------
if not exist "config.txt" (
    echo ROOT_PATH=Z:\python_embedded> "config.txt"
    echo [Info] config.txt criado.
) else (
    echo [Info] config.txt já existe.
)

:: ---------------------------------------------------------------
:: FIM
:: ---------------------------------------------------------------
echo.
echo Tudo pronto! Agora você pode executar o programa.
echo.
pause