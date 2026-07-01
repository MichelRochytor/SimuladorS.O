#include "kosmos.h"
#include <stdio.h> // Necessário para o sprintf interno do Buscador de Arquivos

typedef enum KOSMOS_DPI_AWARENESS {
    KOSMOS_DPI_UNAWARE = 0,
    KOSMOS_SYSTEM_DPI_AWARE = 1,
    KOSMOS_PER_MONITOR_DPI_AWARE = 2
} KOSMOS_DPI_AWARENESS;

typedef HRESULT (STDAPICALLTYPE *SetProcessDpiAwarenessProc)(KOSMOS_DPI_AWARENESS);

// --- COMANDOS PARA TREEVIEW ---
KTreeItem kTreeAdd(KWidget treeview, KTreeItem pai, const char* texto) {
    if (treeview == NULL || texto == NULL) return NULL;

    WCHAR textoWide[256];
    kInternalCharToWide(texto, textoWide, 256);

    TVINSERTSTRUCTW tvis;
    ZeroMemory(&tvis, sizeof(tvis));
    
    // Configura o nó pai e define que o novo item vai para o final da lista daquele nível
    tvis.hParent = pai;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT;
    tvis.item.pszText = textoWide;

    // Envia a mensagem nativa e retorna o identificador do nó recém-criado
    return (KTreeItem)SendMessageW(treeview, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
}

void kTreeClear(KWidget treeview) {
    if (treeview != NULL) {
        // Envia a mensagem nativa de remoção passando o item raiz (deleta tudo para baixo)
        SendMessageW(treeview, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
    }
}

KTreeItem kTreeViewGetSelected(KWidget treeview, KData data) {
    if (treeview == NULL || data == 0) return NULL;

    LPNMHDR lpnmh = (LPNMHDR)data;
    
    // Verifica se a notificação veio da TreeView e se a seleção mudou (TVN_SELCHANGEDW)
    if (lpnmh->hwndFrom == treeview && lpnmh->code == TVN_SELCHANGEDW) {
        LPNMTREEVIEWW pnmtv = (LPNMTREEVIEWW)data;
        return (KTreeItem)pnmtv->itemNew.hItem; // Retorna o identificador do nó ativo
    }
    return NULL;
}

void kTreeGetText(KWidget treeview, KTreeItem item, char* buffer, int tamanhoMaximo) {
    if (buffer == NULL) return;
    buffer[0] = '\0';
    if (treeview == NULL || item == NULL || tamanhoMaximo <= 0) return;

    WCHAR textoWide[256] = {0};
    
    TVITEMW tvi;
    ZeroMemory(&tvi, sizeof(tvi));
    tvi.hItem = item;
    tvi.mask = TVIF_TEXT;
    tvi.pszText = textoWide;
    tvi.cchTextMax = 256;

    // Solicita os dados do item para a TreeView
    if (SendMessageW(treeview, TVM_GETITEMW, 0, (LPARAM)&tvi)) {
        kInternalWideToChar(textoWide, buffer, tamanhoMaximo);
    }
}

// --- TAB CONTROL ---
void kTabAdd(KWidget tab, int index, const char* titulo) {
    if (tab == NULL || titulo == NULL) return;

    WCHAR tituloWide[128];
    kInternalCharToWide(titulo, tituloWide, 128);

    TCITEMW tie;
    ZeroMemory(&tie, sizeof(tie));
    tie.mask = TCIF_TEXT;
    tie.pszText = tituloWide;

    // Envia a mensagem nativa para inserir a aba na posição especificada
    SendMessageW(tab, TCM_INSERTITEMW, (WPARAM)index, (LPARAM)&tie);
}

int kTabGetIndex(KWidget tab) {
    if (tab == NULL) return -1;
    // Pergunta ao Windows qual aba está selecionada no momento
    return (int)SendMessageW(tab, TCM_GETCURSEL, 0, 0);
}

void kTabSetIndex(KWidget tab, int index) {
    if (tab != NULL && index >= 0) {
        // Força a seleção de uma aba específica
        SendMessageW(tab, TCM_SETCURSEL, (WPARAM)index, 0);
    }
}

void kTabBindGroups(KTabGroup* grupo, KWidget tab, int totalAbas, ...) {
    if (grupo == NULL || tab == NULL || totalAbas <= 0) return;

    grupo->tabWidget = tab;
    grupo->totalAbas = totalAbas > 10 ? 10 : totalAbas; // Limita a 10 por segurança

    va_list args;
    va_start(args, totalAbas);

    for (int i = 0; i < grupo->totalAbas; i++) {
        grupo->paineis[i] = va_arg(args, KWidget);
        
        // Inicialização padrão: mostra a primeira aba (0) e esconde o resto
        if (i == 0) {
            SendMessageW(tab, TCM_SETCURSEL, 0, 0);
            ShowWindow(grupo->paineis[i], SW_SHOW);
        } else {
            ShowWindow(grupo->paineis[i], SW_HIDE);
        }
    }
    va_end(args);
}

BOOL kTabManageEvents(KTabGroup* grupo, KData data) {
    if (grupo == NULL || data == 0) return FALSE;

    LPNMHDR lpnmh = (LPNMHDR)data;

    // Se a notificação veio do Tab Control mapeado e a aba mudou:
    if (lpnmh->hwndFrom == grupo->tabWidget && lpnmh->code == TCN_SELCHANGE) {
        // Descobre qual aba foi clicada
        int abaAtiva = (int)SendMessageW(grupo->tabWidget, TCM_GETCURSEL, 0, 0);

        // Varre os painéis escondendo todos e exibindo apenas o ativo!
        for (int i = 0; i < grupo->totalAbas; i++) {
            if (i == abaAtiva) {
                ShowWindow(grupo->paineis[i], SW_SHOW);
            } else {
                ShowWindow(grupo->paineis[i], SW_HIDE);
            }
        }
        return TRUE; // Evento processado com sucesso!
    }
    return FALSE;
}



// --- IMPLEMENTAÇÃO SEGURO DE CONVERSÃO DE STRINGS ---
void kInternalCharToWide(const char* origem, WCHAR* destino, int tamanhoDestino) {
    if (origem == NULL || destino == NULL || tamanhoDestino <= 0) return;
    MultiByteToWideChar(CP_UTF8, 0, origem, -1, destino, tamanhoDestino);
}

void kInternalWideToChar(const WCHAR* origem, char* destino, int tamanhoDestino) {
    if (origem == NULL || destino == NULL || tamanhoDestino <= 0) return;
    WideCharToMultiByte(CP_UTF8, 0, origem, -1, destino, tamanhoDestino, NULL, NULL);
}

// --- CONTROLE DE RADIO BUTTONS ---
void kRadioSetCheck(KWidget radio, BOOL checado) {
    if (radio != NULL) {
        // Envia a mensagem para marcar (BST_CHECKED) ou desmarcar (BST_UNCHECKED)
        SendMessageW(radio, BM_SETCHECK, checado ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

BOOL kRadioGetCheck(KWidget radio) {
    if (radio != NULL) {
        // Pergunta para o Windows qual o estado atual do botão
        LRESULT estado = SendMessageW(radio, BM_GETCHECK, 0, 0);
        return (estado == BST_CHECKED);
    }
    return FALSE;
}

// --- CAIXA DE MENSAGEM GLOBAL ---
void kMessageBox(KWidget janela, const char* mensagem, const char* titulo) {
    WCHAR msgWide[512], titWide[256];
    kInternalCharToWide(mensagem, msgWide, 512);
    kInternalCharToWide(titulo, titWide, 256);
    MessageBoxW(janela, msgWide, titWide, MB_OK | MB_ICONINFORMATION);
}

// --- COMANDOS BÁSICOS TEXTUAIS ---
WINBOOL kSetText(HWND widget, const char* texto) {
    WCHAR bufferWide[512];
    kInternalCharToWide(texto, bufferWide, 512);
    return SetWindowTextW(widget, bufferWide);
}

int kGetText(HWND widget, char* buffer, int tamanho) {
    WCHAR bufferWide[512];
    int resultado = GetWindowTextW(widget, bufferWide, 512);
    kInternalWideToChar(bufferWide, buffer, tamanho);
    return resultado;
}

KWidget kGetWidget(HWND janela, int id) {
    return GetDlgItem(janela, id);
}

// Motor Gráfico GDI+ e Instância da Janela
ULONG_PTR gdiplusToken;
HINSTANCE hInst;

int WINAPI wWinMain(HINSTANCE hInst1, HINSTANCE hPrevInst, LPWSTR pCmdLine, int nCmdShow) 
{
    hInst = hInst1;
    LoadLibraryW(L"riched32.dll"); // RichEdit 1.0 (Versão antiga padrão do Windows)
    LoadLibraryW(L"riched20.dll"); // RichEdit 2.0 e 3.0 (Altamente estável e amplamente utilizada)
    LoadLibraryW(L"msftedit.dll"); // RichEdit 4.1+ (Moderna nativa do Windows 10/11)
    
    // Runtimes Alternativos (Trazidos frequentemente pelo Microsoft Office/SDKs)
    LoadLibraryW(L"riched64.dll"); // Variação de 64-bit em ambientes específicos
    LoadLibraryW(L"msftedit64.dll");// Variação moderna de 64-bit
    return KosmosMain();
}

// kosmos.c

void kRichAppendText(KWidget richedit, const char* texto, COLORREF cor) {
    if (richedit == NULL || texto == NULL) return;

    // 1. Converte o texto que o usuário digitou (char) para WCHAR interno
    int tam = strlen(texto) + 1;
    WCHAR* textoWide = (WCHAR*)malloc(tam * sizeof(WCHAR));
    kInternalCharToWide(texto, textoWide, tam);

    // 2. Coloca o cursor/seleção lá no final do texto existente
    int comprimento = GetWindowTextLengthW(richedit);
    CHARRANGE cr;
    cr.cpMin = comprimento;
    cr.cpMax = comprimento;
    SendMessageW(richedit, EM_EXSETSEL, 0, (LPARAM)&cr);

    // 3. Preenche a estrutura de formatação do Windows para aplicar a cor
    CHARFORMAT2W cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = cor;

    // Aplica a cor apenas na seleção atual (que está no final do arquivo)
    SendMessageW(richedit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    // 4. Injeta o texto formatado no controle
    SendMessageW(richedit, EM_REPLACESEL, FALSE, (LPARAM)textoWide);

    // Libera a memória temporária
    free(textoWide);
}

void kInitGraphEngine() {
    struct GdiplusStartupInput gdiplusStartupInput;
    gdiplusStartupInput.GdiplusVersion = 1;
    gdiplusStartupInput.DebugEventCallback = NULL;
    gdiplusStartupInput.SuppressBackgroundThread = FALSE;
    gdiplusStartupInput.SuppressExternalCodecs = FALSE;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void kPictureSetImageFile(KWidget picture, const char* caminhoArquivo) {
    if (picture == NULL || caminhoArquivo == NULL) return;

    WCHAR caminhoWide[MAX_PATH];
    kInternalCharToWide(caminhoArquivo, caminhoWide, MAX_PATH);

    GpBitmap* bitmapGDIPlus = NULL;
    HBITMAP hBitmapNativo = NULL;

    if (GdipCreateBitmapFromFile(caminhoWide, &bitmapGDIPlus) == 0) { 
        GdipCreateHBITMAPFromBitmap(bitmapGDIPlus, &hBitmapNativo, 0);
        SendMessageW(picture, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmapNativo);
        GdipDisposeImage((GpImage*)bitmapGDIPlus);
    }
}

// Cache de Desenho do Canvas
static KShape g_desenhos[100];
static int g_total_desenhos = 0;
static COLORREF g_cor_fundo_padrao = RGB(33, 33, 33);

LRESULT CALLBACK KosmosCustomControlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            HBRUSH hBtnFace = CreateSolidBrush(g_cor_fundo_padrao);
            FillRect(hdc, &rc, hBtnFace);
            DeleteObject(hBtnFace);
            
            for (int i = 0; i < g_total_desenhos; i++) {
                HBRUSH hBrush = CreateSolidBrush(g_desenhos[i].cor);
                HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);
                HPEN hNullPen = CreatePen(PS_NULL, 0, 0);
                HGDIOBJ hOldPen = SelectObject(hdc, hNullPen);

                if (g_desenhos[i].type == K_RECTANGLE) {
                    Rectangle(hdc, g_desenhos[i].x, g_desenhos[i].y, 
                              g_desenhos[i].x + g_desenhos[i].largura, 
                              g_desenhos[i].y + g_desenhos[i].altura);
                } 
                else if (g_desenhos[i].type == K_ELLIPSE) {
                    Ellipse(hdc, g_desenhos[i].x, g_desenhos[i].y, 
                            g_desenhos[i].x + g_desenhos[i].largura, 
                            g_desenhos[i].y + g_desenhos[i].altura);
                }

                SelectObject(hdc, hOldBrush);
                SelectObject(hdc, hOldPen);
                DeleteObject(hBrush);
                DeleteObject(hNullPen);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Progress Bar
void kProgressSetColor(KWidget progress, COLORREF cor) {
    if (progress != NULL) {
        SendMessageW(progress, PBM_SETBARCOLOR, 0, (LPARAM)cor);
    }
}

void kProgressSetRange(KWidget progress, int minimo, int maximo) {
    if (progress != NULL) {
        SendMessageW(progress, PBM_SETRANGE, 0, MAKELPARAM(minimo, maximo));
    }
}

void kProgressSetPos(KWidget progress, int posicao) {
    if (progress != NULL) {
        SendMessageW(progress, PBM_SETPOS, (WPARAM)posicao, 0);
    }
}

void kProgressStep(KWidget progress, int incremento) {
    if (progress != NULL) {
        SendMessageW(progress, PBM_SETSTEP, (WPARAM)incremento, 0);
        SendMessageW(progress, PBM_STEPIT, 0, 0);
    }
}

// Calendários
KDateValue kGetDatePicker(KWidget picker) {
    SYSTEMTIME st;
    KDateValue dataOutput = {0, 0, 0};
    if (SendMessageW(picker, DTM_GETSYSTEMTIME, 0, (LPARAM)&st) == GDT_VALID) {
        dataOutput.day = st.wDay;
        dataOutput.month = st.wMonth;
        dataOutput.year = st.wYear;
    }
    return dataOutput;
}

KDateValue kGetMonthCalendar(KWidget calendar) {
    SYSTEMTIME st;
    KDateValue dataOutput = {0, 0, 0};
    if (SendMessageW(calendar, MCM_GETCURSEL, 0, (LPARAM)&st)) {
        dataOutput.day = st.wDay;
        dataOutput.month = st.wMonth;
        dataOutput.year = st.wYear;
    }
    return dataOutput;
}

// ListBox
void kListAdd(KWidget listbox, const char* texto) {
    if (listbox != NULL) {
        WCHAR bufferWide[512];
        kInternalCharToWide(texto, bufferWide, 512);
        SendMessageW(listbox, LB_ADDSTRING, 0, (LPARAM)bufferWide);
    }
}

void kListRemove(KWidget listbox, int index) {
    if (listbox != NULL && index >= 0) {
        SendMessageW(listbox, LB_DELETESTRING, (WPARAM)index, 0);
    }
}

void kListClear(KWidget listbox) {
    if (listbox != NULL) {
        SendMessageW(listbox, LB_RESETCONTENT, 0, 0);
    }
}

int kListGetIndex(KWidget listbox, Kid id) {
    if (listbox == NULL) return -1;
    if (kGetAction(id) != LBN_SELCHANGE) return -1;
    return (int)SendMessageW(listbox, LB_GETCURSEL, 0, 0);
}

void kListGetText(KWidget listbox, Kid id, char* buffer, int tamanhoMaximo) {
    if (buffer == NULL) return;
    buffer[0] = '\0';
    if (listbox == NULL) return;
    if (kGetAction(id) != LBN_SELCHANGE) return;

    int index = (int)SendMessageW(listbox, LB_GETCURSEL, 0, 0);
    if (index != -1) {
        WCHAR bufferWide[512];
        SendMessageW(listbox, LB_GETTEXT, (WPARAM)index, (LPARAM)bufferWide);
        kInternalWideToChar(bufferWide, buffer, tamanhoMaximo);
    }
}

int kListGetCount(KWidget listbox) {
    if (listbox == NULL) return 0;
    return (int)SendMessageW(listbox, LB_GETCOUNT, 0, 0);
}

// Canvas Gráfico
void kCanvasClear(KWidget widget, COLORREF corFundo) {
    g_total_desenhos = 0;
    g_cor_fundo_padrao = corFundo;
    InvalidateRect(widget, NULL, TRUE);
}

void kCanvasDrawRect(KWidget widget, int x, int y, int largura, int altura, COLORREF cor) {
    if (g_total_desenhos < 100) {
        g_desenhos[g_total_desenhos].type = K_RECTANGLE;
        g_desenhos[g_total_desenhos].x = x;
        g_desenhos[g_total_desenhos].y = y;
        g_desenhos[g_total_desenhos].largura = largura;
        g_desenhos[g_total_desenhos].altura = altura;
        g_desenhos[g_total_desenhos].cor = cor;
        g_total_desenhos++;
        InvalidateRect(widget, NULL, FALSE);
    }
}

void kCanvasDrawCircle(KWidget widget, int x, int y, int raio, COLORREF cor) {
    if (g_total_desenhos < 100) {
        g_desenhos[g_total_desenhos].type = K_ELLIPSE;
        g_desenhos[g_total_desenhos].x = x - raio;
        g_desenhos[g_total_desenhos].y = y - raio;
        g_desenhos[g_total_desenhos].largura = raio * 2;
        g_desenhos[g_total_desenhos].altura = raio * 2;
        g_desenhos[g_total_desenhos].cor = cor;
        g_total_desenhos++;
        InvalidateRect(widget, NULL, FALSE);
    }
}

// 🌟 BUSCADOR DE ARQUIVOS (Totalmente adaptado para char externo!)
void KosmosListFiles(const char* pasta, HWND list_arquivos, char pathDestino[260]) {
    WIN32_FIND_DATAW FindFileData;
    HANDLE hfind;

    WCHAR pastaWide[260];
    WCHAR pathWide[260];
    
    kInternalCharToWide(pasta, pastaWide, 260);
    wcscpy(pathWide, pastaWide);
    wcscat(pathWide, L"\\*.*");

    hfind = FindFirstFileW(pathWide, &FindFileData);

    if (hfind != INVALID_HANDLE_VALUE) {
        do {
            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                const WCHAR* ext = wcsrchr(FindFileData.cFileName, L'.');
                if (ext != NULL && wcscmp(ext, L".s") == 0) {
                    char nameChar[260];
                    kInternalWideToChar(FindFileData.cFileName, nameChar, 260);

                    char formatar_arq[300];
                    sprintf(formatar_arq, "   >  %s", nameChar);

                    kListAdd(list_arquivos, formatar_arq);
                }
            }
        } while (FindNextFileW(hfind, &FindFileData) != 0);
        FindClose(hfind);
    }
    kInternalWideToChar(pathWide, pathDestino, 260);
}

// DPI e Fontes
void ConfigurarDPI() {
    HMODULE hShcore = LoadLibraryW(L"Shcore.dll");
    if (hShcore) {
        SetProcessDpiAwarenessProc setDpiAware = (SetProcessDpiAwarenessProc)GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (setDpiAware) setDpiAware(KOSMOS_PER_MONITOR_DPI_AWARE);
        FreeLibrary(hShcore);
    } else {
        HMODULE hUser32 = LoadLibraryW(L"user32.dll");
        if (hUser32) {
            typedef BOOL (STDAPICALLTYPE *SetProcessDPIAwareProc)(void);
            SetProcessDPIAwareProc setDpi = (SetProcessDPIAwareProc)GetProcAddress(hUser32, "SetProcessDPIAware");
            if (setDpi) setDpi();
            FreeLibrary(hUser32);
        }
    }
}

void kSetWidgetFont(KWidget widget, const char* fontName, int size, BOOL bBold, BOOL bItalic, BOOL bUnderline) {
    int fontWeight = bBold ? FW_BOLD : FW_NORMAL;
    WCHAR nameWide[64];
    kInternalCharToWide(fontName, nameWide, 64);

    HFONT hFont = CreateFontW(size, 0, 0, 0, fontWeight, bItalic, bUnderline, FALSE, 
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, nameWide);
    if (hFont != NULL) {
        SendMessageW(widget, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
}

BOOL KCustomFont(const char* fontPath) {
    WCHAR pathWide[MAX_PATH];
    kInternalCharToWide(fontPath, pathWide, MAX_PATH);
    int result = AddFontResourceExW(pathWide, FR_PRIVATE, NULL);
    if (result > 0) {
        PostMessageW(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
        return TRUE;
    }
    return FALSE;
}

// ComboBox
void kComboAdd(KWidget combo, const char* texto) {
    WCHAR bufferWide[512];
    kInternalCharToWide(texto, bufferWide, 512);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)bufferWide);
}

void kComboClear(KWidget combo) {
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
}

int kComboGetIndex(KWidget combo) {
    return (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
}

void kComboGetText(KWidget combo, char* buffer) {
    int index = kComboGetIndex(combo);
    if (index != CB_ERR) {
        WCHAR bufferWide[512];
        SendMessageW(combo, CB_GETLBTEXT, (WPARAM)index, (LPARAM)bufferWide);
        kInternalWideToChar(bufferWide, buffer, 512);
    } else {
        buffer[0] = '\0';
    }
}

// Command Link
void kCommandLinkSetTitle(KWidget widget, const char* titulo) {
    if (widget != NULL) {
        WCHAR titleWide[256];
        kInternalCharToWide(titulo, titleWide, 256);
        SetWindowTextW(widget, titleWide);
    }
}

void kCommandLinkSetNote(KWidget widget, const char* nota) {
    if (widget != NULL) {
        WCHAR noteWide[512];
        kInternalCharToWide(nota, noteWide, 512);
        SendMessageW(widget, BCM_SETNOTE, 0, (LPARAM)noteWide);
    }
}

// ListView
void kListColAdd(KWidget listview, int index, const char* titulo, int largura) {
    if (listview == NULL) return;
    WCHAR titleWide[256];
    kInternalCharToWide(titulo, titleWide, 256);
    
    LVCOLUMNW lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = largura;
    lvc.pszText = titleWide;
    lvc.iSubItem = index;
    SendMessageW(listview, LVM_INSERTCOLUMNW, (WPARAM)index, (LPARAM)&lvc);
}

int kListRowAdd(KWidget listview, int row, const char* texto) {
    if (listview == NULL) return -1;
    WCHAR textWide[512];
    kInternalCharToWide(texto, textWide, 512);
    
    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = row;
    lvi.iSubItem = 0;
    lvi.pszText = textWide;
    return (int)SendMessageW(listview, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
}

void kListSubSetText(KWidget listview, int row, int col, const char* texto) {
    if (listview == NULL) return;
    WCHAR textWide[512];
    kInternalCharToWide(texto, textWide, 512);
    
    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = row;
    lvi.iSubItem = col;
    lvi.pszText = textWide;
    SendMessageW(listview, LVM_SETITEMW, 0, (LPARAM)&lvi);
}

void kListRowClear(KWidget listview) {
    if (listview != NULL) {
        SendMessageW(listview, LVM_DELETEALLITEMS, 0, 0);
    }
}

int kListViewGetIndex(KWidget listview, KData data) {
    if (listview == NULL || data == 0) return -1;
    LPNMHDR lpnmh = (LPNMHDR)data;
    if (lpnmh->code == LVN_ITEMCHANGED) {
        LPNMLISTVIEW pnmv = (LPNMLISTVIEW)data;
        if ((pnmv->uNewState & LVIS_SELECTED) && !(pnmv->uOldState & LVIS_SELECTED)) {
            return pnmv->iItem;
        }
    }
    return -1;
}

// Rotinas do Sistema da Janela
KFONT CriarFontePersonalizada(const char* nomeFonte, int tamanho, int peso) {
    WCHAR nameWide[64];
    kInternalCharToWide(nomeFonte, nameWide, 64);
    return CreateFontW(-MulDiv(tamanho, GetDpiForSystem(), 96), 0, 0, 0, peso,
                       FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                       DEFAULT_PITCH | FF_DONTCARE, nameWide);
}

HWND KCreateWindow(int idDialogo, KosmosWindowProc procedimento) {
    ConfigurarDPI();
    kInitGraphEngine();
    return CreateDialogW(hInst, MAKEINTRESOURCE(idDialogo), NULL, (DLGPROC)procedimento);
}

void LoopMsg() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void EndKosmos(HWND dialogo) {
    EndDialog(dialogo, 0);
    PostQuitMessage(117);
}

void DefinirFonteDialogo(HWND dialogo, HFONT fonte) {
    SendMessage(dialogo, WM_SETFONT, (WPARAM)fonte, TRUE);
}

// OwnerDraw Menu (Processamento Interno)
void ConfigurarMenuOwnerDraw(HWND hwnd, HMENU menu, const EstiloVisual* estilo) {
    MENUINFO mi = { sizeof(MENUINFO) };
    mi.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
    mi.hbrBack = CreateSolidBrush(estilo->corFundo);
    SetMenuInfo(menu, &mi);

    int count = GetMenuItemCount(menu);
    for (int i = 0; i < count; i++) {
        MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
        wchar_t textWide[256];
        mii.fMask = MIIM_STRING | MIIM_SUBMENU;
        mii.dwTypeData = textWide;
        mii.cch = ARRAYSIZE(textWide);
        GetMenuItemInfo(menu, i, TRUE, &mii);

        DadosItemMenu* pData = (DadosItemMenu*)malloc(sizeof(DadosItemMenu));
        kInternalWideToChar(textWide, pData->texto, 256);
        pData->icone = NULL;

        mii.fMask = MIIM_FTYPE | MIIM_DATA;
        mii.fType = MFT_OWNERDRAW;
        mii.dwItemData = (ULONG_PTR)pData;
        SetMenuItemInfo(menu, i, TRUE, &mii);

        if (mii.hSubMenu) {
            ConfigurarMenuOwnerDraw(hwnd, mii.hSubMenu, estilo);
        }
    }
}
