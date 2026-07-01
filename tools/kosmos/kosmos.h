#ifndef KOSMOS_H
#define KOSMOS_H

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <richedit.h>   
#include <mmsystem.h>
#include <stdarg.h>
#include <wingdi.h>
#include <gdiplus.h> 
#include <gdiplus/gdiplusflat.h> 

// Tipo amigável que representa um nó (Item) dentro da TreeView
typedef HTREEITEM KTreeItem;

// Constante para indicar que o nó não tem pai (é uma raiz)
#define KTREE_ROOT TVI_ROOT

// Estruturas
typedef struct {
    char texto[256]; 
    HBITMAP icone;
} DadosItemMenu;

typedef struct {
    COLORREF corFundo;
    COLORREF corTexto;
    COLORREF corDestaque;
    COLORREF corDesativado;
    char fonteNome[32]; 
    int fonteTamanho;
    int fontePeso;
} EstiloVisual;

typedef enum {
    K_RECTANGLE,
    K_ELLIPSE,
    K_CLEAR
} KShapeType;

// Definições de tipos
typedef INT_PTR (CALLBACK* KosmosWindowProc)(HWND, UINT, WPARAM, LPARAM);

typedef HWND KWidget;
typedef UINT KEvent;
typedef WPARAM Kid;
typedef LPARAM KData;
typedef INT_PTR KResult;

typedef HFONT KFONT;

#define kcontroller INT_PTR CALLBACK

KWidget kGetWidget(HWND janela, int id);
#define MsgWindow() switch (msg_param)

// Macros
#define KosmosWindow(nome) INT_PTR CALLBACK nome(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)

int KosmosMain(void); // Ponto de entrada limpo do usuário

#define kevents(msg) switch(msg)

// --- EXTRAÇÃO DE DADOS ---
#define kGetId(dado)     LOWORD(dado)
#define kGetAction(dado) HIWORD(dado)

#define KInit          WM_INITDIALOG
#define KCommand       WM_COMMAND
#define KResize        WM_SIZE
#define KResizing      WM_SIZING
#define KPreResize     WM_WINDOWPOSCHANGING
#define KErase         WM_ERASEBKGND
#define KPaint         WM_PAINT
#define KClose         WM_CLOSE
#define KNotify        WM_NOTIFY 
#define KDestroy       WM_DESTROY

#define KColors        WM_CTLCOLORBTN: \
                       WM_CTLCOLORLISTBOX: \
                       WM_CTLCOLORDLG: \
                       WM_CTLCOLOREDIT: \
                       WM_CTLCOLORSTATIC

#define WINDOW_INIT 1
#define KOSMOS_COMMAND 2

typedef struct {
    KShapeType type;
    int x, y, largura, altura;
    COLORREF cor;
} KShape;

typedef struct {
    int day;
    int month;
    int year;
} KDateValue;

// --- COMANDOS PARA TREEVIEW ---
// Adiciona um item na árvore. Se 'pai' for KTREE_ROOT, vira um nó principal.
KTreeItem kTreeAdd(KWidget treeview, KTreeItem pai, const char* texto);

// Remove todos os nós e limpa a TreeView por completo
void kTreeClear(KWidget treeview);

// Retorna o nó que foi clicado/selecionado pelo usuário dentro do KNotify
KTreeItem kTreeViewGetSelected(KWidget treeview, KData data);

// Pega o texto de um nó específico e joga no seu buffer
void kTreeGetText(KWidget treeview, KTreeItem item, char* buffer, int tamanhoMaximo);


// --- SISTEMA DE ABAS (Tab Control) ---
typedef struct {
    KWidget tabWidget;          // O Tab Control principal
    KWidget paineis[10];        // Array com os widgets/painéis de cada aba
    int totalAbas;              // Quantidade de abas configuradas
} KTabGroup;

void kTabAdd(KWidget tab, int index, const char* titulo);

// Retorna o índice da aba atualmente selecionada (0 para a primeira, 1 para a segunda, etc.)
int kTabGetIndex(KWidget tab);

// Altera programaticamente a aba ativa
void kTabSetIndex(KWidget tab, int index);
// Vincula um Tab Control aos seus respectivos painéis e esconde os inativos automaticamente
void kTabBindGroups(KTabGroup* grupo, KWidget tab, int totalAbas, ...);

// Verifica o evento de clique nas abas dentro do KNotify e faz o switch visual sozinho
// Retorna TRUE se a mensagem foi processada por este grupo de abas
BOOL kTabManageEvents(KTabGroup* grupo, KData data);

// --- COMANDOS PARA RICH EDIT ---

// Adiciona um texto no final do RichEdit com uma cor específica
// Ideal para criar consoles de log e saídas coloridas
void kRichAppendText(KWidget richedit, const char* texto, COLORREF cor);

// --- FUNÇÕES UTILITÁRIAS DE CONVERSÃO INTERNA ---
void kInternalCharToWide(const char* origem, WCHAR* destino, int tamanhoDestino);
void kInternalWideToChar(const WCHAR* origem, char* destino, int tamanhoDestino);

// --- COMANDOS PARA RADIO BUTTON ---

// Marca ou desmarca um Radio Button (TRUE para marcar, FALSE para desmarcar)
void kRadioSetCheck(KWidget radio, BOOL checado);

// Retorna TRUE se o Radio Button estiver selecionado, ou FALSE se não estiver
BOOL kRadioGetCheck(KWidget radio);

// --- CAIXA DE MENSAGEM ---
void kMessageBox(KWidget janela, const char* mensagem, const char* titulo);

// --- COMANDOS PARA DATA E CALENDÁRIO ---
KDateValue kGetDatePicker(KWidget picker);
KDateValue kGetMonthCalendar(KWidget calendar);

// --- COMANDOS PARA PROGRESS BAR ---
void kProgressSetColor(KWidget progress, COLORREF cor);
void kProgressSetRange(KWidget progress, int minimo, int maximo);
void kProgressSetPos(KWidget progress, int posicao);
void kProgressStep(KWidget progress, int incremento);

// --- COMANDOS PARA IMAGENS ---
void kInitGraphEngine();
void kPictureSetImageFile(KWidget picture, const char* caminhoArquivo); 

// --- COMANDOS PARA LISTVIEW ---
void kListColAdd(KWidget listview, int index, const char* titulo, int largura); 
int kListRowAdd(KWidget listview, int row, const char* texto); 
void kListSubSetText(KWidget listview, int row, int col, const char* texto); 
void kListRowClear(KWidget listview);
int kListViewGetIndex(KWidget listview, KData data);

// --- COMANDOS PARA LISTBOX ---
void kListAdd(KWidget listbox, const char* texto); 
void kListRemove(KWidget listbox, int index);
void kListClear(KWidget listbox);
int kListGetIndex(KWidget listbox, Kid id);
void kListGetText(KWidget listbox, Kid id, char* buffer, int tamanhoMaximo); 
int kListGetCount(KWidget listbox);

// --- SISTEMA DE ARQUIVOS ---
void KosmosListFiles(const char* pasta, HWND list_arquivos, char path[260]);

// --- COMANDOS DE CANVAS ---
void kCanvasClear(KWidget widget, COLORREF corFundo);
void kCanvasDrawRect(KWidget widget, int x, int y, int largura, int altura, COLORREF cor);
void kCanvasDrawCircle(KWidget widget, int x, int y, int raio, COLORREF cor);

// --- COMANDOS DE COMMAND LINK ---
void kCommandLinkSetTitle(KWidget widget, const char* titulo); 
void kCommandLinkSetNote(KWidget widget, const char* nota); 

// --- CONTROLE DE TEXTO E FONTES ---
void kSetWidgetFont(KWidget widget, const char* fontName, int size, BOOL bBold, BOOL bItalic, BOOL bUnderline); 
BOOL KCustomFont(const char* fontPath); 
int kGetText(HWND widget, char* buffer, int tamanho); 
WINBOOL kSetText(HWND widget, const char* texto); 

// --- COMBOBOX ---
void kComboAdd(KWidget combo, const char* texto); 
void kComboClear(KWidget combo);
int kComboGetIndex(KWidget combo);
void kComboGetText(KWidget combo, char* buffer); 

// --- ROTINAS DO MOTOR ---
void ConfigurarDPI();
KFONT CriarFontePersonalizada(const char* nomeFonte, int tamanho, int peso); 
HWND KCreateWindow(int idDialogo, KosmosWindowProc procedimento);
void LoopMsg();
void DefinirFonteDialogo(HWND dialogo, HFONT fonte);
void ConfigurarMenuOwnerDraw(HWND hwnd, HMENU menu, const EstiloVisual* estilo);
INT_PTR TratarMensagemMenu(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, const EstiloVisual* estilo);
void EndKosmos(HWND dialogo);

#endif // KOSMOS_H