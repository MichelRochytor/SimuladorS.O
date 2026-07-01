#!/bin/bash
# tools/package_linux.sh

# ================= CONFIGURAÇÃO DINÂMICA (SDK KOSMOS) =================
PROJETO_NOME=$1

# Fallback caso o script seja chamado isoladamente
if [ -z "$PROJETO_NOME" ]; then
    echo "⚠️  Aviso: Nome do projeto não foi passado. Usando 'KosmosApp' como fallback."
    PROJETO_NOME="KosmosApp"
fi

APP_NAME="$PROJETO_NOME"
EXE_PATH="output/${PROJETO_NOME}.exe"
ICON_PATH="resource/icon.png"
OUTPUT_DIR="output/linux"
APP_DIR="$OUTPUT_DIR/$APP_NAME.AppDir"

# Wine 10.0 Staging Portátil (Kron4ek Builds)
WINE_URL="https://github.com/Kron4ek/Wine-Builds/releases/download/10.0/wine-10.0-staging-amd64.tar.xz"

# Runtime "Continuous"
RUNTIME_URL="https://github.com/AppImage/AppImageKit/releases/download/continuous/runtime-x86_64"
# ======================================================================

# Função para verificar erros
check_error() {
    if [ $? -ne 0 ]; then
        echo "[ERRO] $1"
        exit 1
    fi
}

echo "[1/7] Limpando build anterior..."
# Remove apenas o AppDir específico para evitar concorrência em builds concorrentes
rm -rf "$APP_DIR"
mkdir -p "$APP_DIR/usr/bin"
mkdir -p "$APP_DIR/usr/lib"

echo "[2/7] Copiando executável Windows do projeto ($EXE_PATH)..."
if [ ! -f "$EXE_PATH" ]; then
    echo "ERRO: Ficheiro $EXE_PATH não encontrado! Execute a compilação primeiro."
    exit 1
fi
cp "$EXE_PATH" "$APP_DIR/usr/bin/$APP_NAME.exe"

# Copia os assets adicionais e fontes do projeto (como Audiowide.ttf e imagens de teste)
if [ -d "./resource" ]; then
    cp -r ./resource "$APP_DIR/usr/bin/"
fi

# Copia ícone do pacote
if [ -f "$ICON_PATH" ]; then
    cp "$ICON_PATH" "$APP_DIR/$APP_NAME.png"
else
    # Fallback caso não possua ícone customizado
    wget -q https://upload.wikimedia.org/wikipedia/commons/8/83/Circle-icons-dev.svg -O "$APP_DIR/$APP_NAME.svg"
fi

# Garante que a pasta tools/ local existe para armazenar os downloads pesados e reutilizá-los
mkdir -p tools

echo "[3/7] Verificando Wine-Staging 10.0 Portátil em tools/..."
if [ ! -f "tools/wine-portable.tar.xz" ] || [ ! -s "tools/wine-portable.tar.xz" ]; then
    echo "Baixando Wine 10.0 (Isso pode demorar um pouco)..."
    wget -q --show-progress "$WINE_URL" -O "tools/wine-portable.tar.xz"
    check_error "Falha ao baixar o Wine."
fi

echo "Extraindo Wine para dentro do pacote..."
tar -xf "tools/wine-portable.tar.xz" -C "$APP_DIR/usr/" --strip-components=1
check_error "Falha ao extrair o Wine."

echo "[4/7] Criando AppRun Dinâmico (Auto-DPI Baseado no Monitor do Usuário)..."

cat > "$APP_DIR/AppRun" <<EOF
#!/bin/bash

HERE="\$(dirname "\$(readlink -f "\${0}")")"
export PATH="\$HERE/usr/bin:\$PATH"
export LD_LIBRARY_PATH="\$HERE/usr/lib:\$HERE/usr/lib64:\$LD_LIBRARY_PATH"
export WINEPREFIX="\$HOME/.wine_kosmos_${PROJETO_NOME,,}"
mkdir -p "\$WINEPREFIX"

export WINEDEBUG=-all
export WINEARCH=win64

# Inicialização pesada (apenas na primeira vez que o usuário roda o app)
if [ ! -f "\$WINEPREFIX/configured" ]; then
    echo "Inicializando prefixo Wine isolado para $PROJETO_NOME..."
    "\$HERE/usr/bin/wineboot" -u
    touch "\$WINEPREFIX/configured"
fi

# --- DETECÇÃO DINÂMICA DE DPI DO MONITOR DO USUÁRIO ---
LINUX_SCALE=\$(gsettings get org.gnome.desktop.interface scaling-factor 2>/dev/null | awk '{print \$2}')

if [ -z "\$LINUX_SCALE" ] || [ "\$LINUX_SCALE" = "0" ]; then
    X_DPI=\$(xrdb -query 2>/dev/null | grep dpi | awk '{print \$2}' | cut -d. -f1)
    if [ -z "\$X_DPI" ]; then
        X_DPI=130 
    fi
else
    X_DPI=\$((LINUX_SCALE * 192))
fi

if [ "\$X_DPI" -eq 96 ]; then
    TEXT_SCALE=\$(gsettings get org.gnome.desktop.interface text-scaling-factor 2>/dev/null)
    if [ ! -z "\$TEXT_SCALE" ] && [ "\$TEXT_SCALE" != "1.0" ]; then
        if (( \$(echo "\$TEXT_SCALE == 1.25" | bc -l 2>/dev/null || echo 0) )); then X_DPI=120; fi
        if (( \$(echo "\$TEXT_SCALE == 1.5" | bc -l 2>/dev/null || echo 0) )); then X_DPI=144; fi
    fi
fi

DPI_HEX=\$(printf "%08x" \$X_DPI)
echo "[KOSMOS] Monitor detectado: Aplicando \$X_DPI DPI dinamicamente (Hex: \$DPI_HEX)..."

# --- APLICAR TEMA WINDOWS 11 FLAT + AUTO DPI ---
cat <<REG > "\$WINEPREFIX/config.reg"
REGEDIT4

[HKEY_CURRENT_USER\Control Panel\Desktop]
"LogPixels"=dword:\$DPI_HEX
"FontSmoothing"="2"
"FontSmoothingType"=dword:00000002
"FontSmoothingOrientation"=dword:00000001
"FontSmoothingGamma"=dword:000004b0
"ForegroundLockTimeout"=dword:00000000

[HKEY_CURRENT_USER\Software\Wine\X11 Driver]
"Decorated"="Y"
"Managed"="Y"
"UseTakeFocus"="N"

[HKEY_CURRENT_USER\Software\Wine\Fonts]
"Antialias"="Y"

[HKEY_CURRENT_USER\Control Panel\Colors]
"ActiveBorder"="200 200 200"
"ActiveTitle"="255 255 255"
"AppWorkspace"="255 255 255"
"Background"="255 255 255"
"ButtonAlternateFace"="255 255 255"
"ButtonDkShadow"="160 160 160"
"ButtonFace"="243 243 243"
"ButtonHilight"="255 255 255"
"ButtonLight"="243 243 243"
"ButtonShadow"="200 200 200"
"ButtonText"="0 0 0"
"GradientActiveTitle"="255 255 255"
"GradientInactiveTitle"="243 243 243"
"GrayText"="120 120 120"
"Hilight"="0 120 215"
"HilightText"="255 255 255"
"HotTrackingColor"="0 102 204"
"InactiveBorder"="243 243 243"
"InactiveTitle"="243 243 243"
"InactiveTitleText"="120 120 120"
"InfoText"="0 0 0"
"InfoWindow"="255 255 255"
"Menu"="255 255 255"
"MenuBar"="243 243 243"
"MenuHilight"="230 230 230"
"MenuText"="0 0 0"
"Scrollbar"="243 243 243"
"TitleText"="0 0 0"
"Window"="255 255 255"
"WindowFrame"="200 200 200"
"WindowText"="0 0 0"

[HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\FontSubstitutes]
"MS Shell Dlg"="Segoe UI"
"MS Shell Dlg 2"="Segoe UI"
"Tahoma"="Segoe UI"
"Arial"="Segoe UI"
REG

"\$HERE/usr/bin/regedit" /S "\$WINEPREFIX/config.reg"
# ----------------------------------------------------

# Executa o executável passando também os argumentos extras passados ao AppImage (\$@)
cd "\$HERE/usr/bin"
exec "\$HERE/usr/bin/wine" "\$HERE/usr/bin/$APP_NAME.exe" "\$@"
EOF

chmod +x "$APP_DIR/AppRun"

# 5. Criar arquivo de metadados .desktop interno
cat > "$APP_DIR/$APP_NAME.desktop" <<EOF
[Desktop Entry]
Name=$APP_NAME
Exec=AppRun
Icon=$APP_NAME
Type=Application
Categories=Development;
EOF

echo "[6/7] Verificando Runtime do AppImage em tools/..."
if [ ! -f "tools/runtime-x86_64" ] || [ ! -s "tools/runtime-x86_64" ]; then
    echo "Baixando Runtime do AppImageKit..."
    rm -f "tools/runtime-x86_64"
    wget --no-check-certificate --show-progress -L "$RUNTIME_URL" -O "tools/runtime-x86_64"
    if [ $? -ne 0 ]; then
        echo "Wget falhou. Tentando Curl..."
        curl -L "$RUNTIME_URL" -o "tools/runtime-x86_64"
    fi
    if [ ! -s "tools/runtime-x86_64" ]; then
        echo "[ERRO] Falha fatal ao baixar o Runtime."
        exit 1
    fi
fi
chmod +x "tools/runtime-x86_64"

echo "[7/7] Empacotando árvore completa SquashFS..."
if ! command -v mksquashfs &> /dev/null; then
    echo "ERRO: mksquashfs não encontrado. Execute: sudo apt install squashfs-tools"
    exit 1
fi

mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_DIR/$APP_NAME-x86_64.AppImage"

mksquashfs "$APP_DIR" "$OUTPUT_DIR/fs.squashfs" -root-owned -noappend -comp xz -b 1M
check_error "Falha ao criar SquashFS bruto."

# Mescla o runtime binário com o sistema de arquivos SquashFS gerado
cat "tools/runtime-x86_64" "$OUTPUT_DIR/fs.squashfs" > "$OUTPUT_DIR/$APP_NAME-x86_64.AppImage"
check_error "Falha na fusão final do AppImage executável."

chmod +x "$OUTPUT_DIR/$APP_NAME-x86_64.AppImage"
rm -f "$OUTPUT_DIR/fs.squashfs"
rm -rf "$APP_DIR" # Limpa a pasta AppDir para economizar espaço em disco

echo "======================================================"
echo " ✅ SUCESSO! AppImage COM AUTO-DPI DINÂMICO CRIADO:"
echo " 📍 Localização: $OUTPUT_DIR/$APP_NAME-x86_64.AppImage"
echo "======================================================"