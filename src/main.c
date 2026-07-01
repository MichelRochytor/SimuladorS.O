#include "kosmos.h"
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <commdlg.h>

#define MAX_PROCESSOS 100
#define TAM_PAGINA_MB 4

// -----------------------------------------------------------------------
// ESTRUTURAS
// -----------------------------------------------------------------------
typedef struct {
    char id[10];
    int tempo_chegada;
    int burst_total;
    int burst_restante;
    int prioridade;
    int memoria_mb;
    int paginas_necessarias;

    int tempo_inicio;
    int tempo_fim;
    int tempo_espera;
    int tempo_resposta;
    bool iniciado;
    bool concluido;
} Processo;

typedef struct {
    int numero_pagina;
    char proc_id[10];
    int ultimo_acesso;
} FrameMemoria;

// -----------------------------------------------------------------------
// REGISTRO DE EXECUCAO: guarda qual processo rodou em cada instante
// -----------------------------------------------------------------------
#define MAX_INSTANTES 2000

typedef struct {
    int instante;
    char proc_id[10]; // "" = ocioso
} RegistroExecucao;

// -----------------------------------------------------------------------
// GLOBAIS
// -----------------------------------------------------------------------
Processo processos[MAX_PROCESSOS];
int total_processos = 0;
FrameMemoria* ram = NULL;
int total_frames_ram = 0;

RegistroExecucao historico[MAX_INSTANTES];
int total_instantes = 0;

// -----------------------------------------------------------------------
// PALETA DE CORES POR PROCESSO
// -----------------------------------------------------------------------
static COLORREF CORES_PROC_BG[8] = {
    RGB(60,  180, 90),
    RGB(220, 80,  80),
    RGB(70,  140, 220),
    RGB(220, 200, 50),
    RGB(180, 80,  210),
    RGB(240, 140, 40),
    RGB(60,  200, 200),
    RGB(220, 100, 160),
};
#define COR_OCIOSO_BG  RGB(70,  70,  70)
#define COR_OCIOSO_FG  RGB(180, 180, 180)
#define COR_LABEL_BG   RGB(25,  25,  25)
#define COR_LABEL_FG   RGB(200, 200, 200)
#define COR_BARRA_FG   RGB(10,  10,  10)

// -----------------------------------------------------------------------
// FUNCOES AUXILIARES DO RICHEDIT
// -----------------------------------------------------------------------
void GanttAppendColorido(KWidget gantt, const char* texto, COLORREF fg, COLORREF bg) {
    CHARRANGE cr = { -1, -1 };
    SendMessageW(gantt, EM_EXSETSEL, 0, (LPARAM)&cr);

    CHARFORMAT2W cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize      = sizeof(cf);
    cf.dwMask      = CFM_COLOR | CFM_BACKCOLOR | CFM_FACE | CFM_SIZE | CFM_BOLD | CFM_CHARSET;
    cf.dwEffects   = 0;
    cf.crTextColor = fg;
    cf.crBackColor = bg;
    cf.yHeight     = 160; // 8pt
    cf.bCharSet    = DEFAULT_CHARSET;
    wcscpy(cf.szFaceName, L"Consolas");
    SendMessageW(gantt, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    WCHAR wbuf[512];
    MultiByteToWideChar(CP_UTF8, 0, texto, -1, wbuf, 512);
    SendMessageW(gantt, EM_REPLACESEL, FALSE, (LPARAM)wbuf);
}

void AdicionarLog(KWidget rich, const char* texto, COLORREF cor) {
    CHARRANGE cr = { -1, -1 };
    SendMessageW(rich, EM_EXSETSEL, 0, (LPARAM)&cr);

    CHARFORMAT2W cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize      = sizeof(cf);
    cf.dwMask      = CFM_COLOR | CFM_BACKCOLOR | CFM_FACE | CFM_SIZE;
    cf.dwEffects   = 0;
    cf.crTextColor = cor;
    cf.crBackColor = RGB(30, 30, 30);
    cf.yHeight     = 170;
    cf.bCharSet    = DEFAULT_CHARSET;
    wcscpy(cf.szFaceName, L"Consolas");
    SendMessageW(rich, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    char buffer[512];
    sprintf(buffer, "%s\n", texto);
    WCHAR wbuf[512];
    MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wbuf, 512);
    SendMessageW(rich, EM_REPLACESEL, FALSE, (LPARAM)wbuf);
}

// -----------------------------------------------------------------------
// RENDERIZAR GANTT NO RICHEDIT
//
// Formato:
//   rgua  00  01  02  03 ...
//   P1    ████    ████  ...
//   P2        ████      ...
//   Ocio              ...
// -----------------------------------------------------------------------
void RenderizarGantt(KWidget gantt) {
    if (total_instantes == 0) return;
    SendMessageW(gantt, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(25, 25, 25));
    SendMessageW(gantt, WM_SETREDRAW, FALSE, 0);
    SetWindowTextW(gantt, L"");

    // -- Regua de tempo --------------------------------------------------
    // Label de 5 chars + 2 chars por instante
    GanttAppendColorido(gantt, "     ", COR_LABEL_FG, COR_LABEL_BG);
    for (int t = 0; t < total_instantes; t++) {
        char cell[8];
        if (t % 5 == 0) {
            sprintf(cell, "%-2d", t);
            GanttAppendColorido(gantt, cell, RGB(130, 130, 130), COR_LABEL_BG);
        } else {
            GanttAppendColorido(gantt, "\xC2\xB7 ", RGB(55, 55, 55), COR_LABEL_BG);
        }
    }
    GanttAppendColorido(gantt, "\r\n", COR_LABEL_FG, COR_LABEL_BG);

    // -- Uma linha por processo ------------------------------------------
    for (int p = 0; p < total_processos; p++) {
        COLORREF bg = CORES_PROC_BG[p % 8];

        // Label "P1   " (5 chars)
        char label[8];
        sprintf(label, "%-4s ", processos[p].id);
        GanttAppendColorido(gantt, label, COR_LABEL_FG, COR_LABEL_BG);

        // Celulas
        for (int t = 0; t < total_instantes; t++) {
            bool rodou = (strcmp(historico[t].proc_id, processos[p].id) == 0);
            if (rodou) {
                // Bloco Unicode cheio (2 chars = visual quadrado em Consolas)
                GanttAppendColorido(gantt, "\xE2\x96\x88\xE2\x96\x88", COR_BARRA_FG, bg);
            } else {
                GanttAppendColorido(gantt, "  ", COR_LABEL_FG, COR_LABEL_BG);
            }
        }
        GanttAppendColorido(gantt, "\r\n", COR_LABEL_FG, COR_LABEL_BG);
    }

    // -- Linha de ocioso -------------------------------------------------
    GanttAppendColorido(gantt, "Ocio ", COR_OCIOSO_FG, COR_LABEL_BG);
    for (int t = 0; t < total_instantes; t++) {
        bool ocioso = (strcmp(historico[t].proc_id, "") == 0);
        if (ocioso) {
            GanttAppendColorido(gantt, "\xE2\x96\x92\xE2\x96\x92", COR_OCIOSO_FG, COR_OCIOSO_BG);
        } else {
            GanttAppendColorido(gantt, "  ", COR_LABEL_FG, COR_LABEL_BG);
        }
    }
    GanttAppendColorido(gantt, "\r\n", COR_LABEL_FG, COR_LABEL_BG);

    // Reativa redesenho e volta ao inicio
    SendMessageW(gantt, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(gantt, NULL, TRUE);
    SendMessageW(gantt, WM_HSCROLL, MAKEWPARAM(SB_LEFT, 0), 0);
    SendMessageW(gantt, EM_SETSEL, 0, 0);
}

// -----------------------------------------------------------------------
// MEMORIA
// -----------------------------------------------------------------------
// Coloque essa variável global junto com as outras no topo do arquivo
int ponteiro_fifo = 0;
int contador_acessos_memoria = 0; // Para o LRU ter precisão absoluta

bool AcessarPaginaMemoria(const char* proc_id, int pagina, int politica, int tempo_atual, int* page_faults) {
    contador_acessos_memoria++; // Incrementa a cada tentativa de leitura
    
    // 1. Verifica se a página já está na memória (HIT)
    for (int i = 0; i < total_frames_ram; i++) {
        if (ram[i].numero_pagina == pagina && strcmp(ram[i].proc_id, proc_id) == 0) {
            if (politica == 1) // Se for LRU, atualiza o momento do acesso
                ram[i].ultimo_acesso = contador_acessos_memoria;
            return true;
        }
    }

    (*page_faults)++; // MISS

    // 2. Procura um espaço vazio na RAM
    for (int i = 0; i < total_frames_ram; i++) {
        if (strcmp(ram[i].proc_id, "") == 0) {
            strcpy(ram[i].proc_id, proc_id);
            ram[i].numero_pagina  = pagina;
            ram[i].ultimo_acesso  = contador_acessos_memoria;
            return false;
        }
    }

    // 3. RAM cheia! Fazer a Substituição
    int fs = 0;
    if (politica == 0) { 
        // FIFO Real: Anda um quadro para a frente a cada substituição
        fs = ponteiro_fifo;
        ponteiro_fifo = (ponteiro_fifo + 1) % total_frames_ram;
    } 
    else if (politica == 1) { 
        // LRU
        int menor_tempo = ram[0].ultimo_acesso;
        for (int i = 1; i < total_frames_ram; i++) {
            if (ram[i].ultimo_acesso < menor_tempo) { 
                menor_tempo = ram[i].ultimo_acesso; 
                fs = i; 
            }
        }
    } 
    else { 
        // Política Random (Já que o Ótimo exigiria ler o array de processos futuros)
        fs = rand() % total_frames_ram;
    }

    // Substitui a página
    strcpy(ram[fs].proc_id, proc_id);
    ram[fs].numero_pagina = pagina;
    ram[fs].ultimo_acesso = contador_acessos_memoria;
    
    return false;
}

// -----------------------------------------------------------------------
// CARREGAR CSV
// -----------------------------------------------------------------------
bool CarregarCSV(const char* caminho, KWidget listview, KWidget rich) {
    FILE* arquivo = fopen(caminho, "r");
    if (!arquivo) {
        AdicionarLog(rich, "Erro: nao foi possivel abrir o CSV.", RGB(255, 85, 85));
        return false;
    }

    char linha[256];
    total_processos = 0;
    kListRowClear(listview);

    if (fgets(linha, sizeof(linha), arquivo)) {
        if (!(strstr(linha, "Tempo") || strstr(linha, "id") || strstr(linha, "Chegada")))
            fseek(arquivo, 0, SEEK_SET);
    }

    int index = 0;
    while (fgets(linha, sizeof(linha), arquivo) && index < MAX_PROCESSOS) {
        char* token = strtok(linha, ";,");
        if (!token) continue;

        sprintf(processos[index].id, "P%d", index + 1);
        processos[index].tempo_chegada = atoi(token);

        token = strtok(NULL, ";,"); if (!token) continue;
        processos[index].burst_total    = atoi(token);
        processos[index].burst_restante = processos[index].burst_total;

        token = strtok(NULL, ";,"); if (!token) continue;
        processos[index].prioridade = atoi(token);

        token = strtok(NULL, ";,"); if (!token) continue;
        processos[index].memoria_mb = atoi(token);

        processos[index].paginas_necessarias =
            (processos[index].memoria_mb + TAM_PAGINA_MB - 1) / TAM_PAGINA_MB;
        processos[index].iniciado     = false;
        processos[index].concluido    = false;
        processos[index].tempo_inicio = -1;

        char bc[12], bb[12], bp[12], bm[12];
        sprintf(bc, "%d",    processos[index].tempo_chegada);
        sprintf(bb, "%d",    processos[index].burst_total);
        sprintf(bp, "%d",    processos[index].prioridade);
        sprintf(bm, "%d MB", processos[index].memoria_mb);

        kListRowAdd(listview, index, processos[index].id);
        kListSubSetText(listview, index, 1, bc);
        kListSubSetText(listview, index, 2, bb);
        kListSubSetText(listview, index, 3, bp);
        kListSubSetText(listview, index, 4, bm);
        index++;
    }
    total_processos = index;
    fclose(arquivo);

    kSetText(rich, "");
    char msg[100];
    sprintf(msg, "Sucesso: %d processos carregados.", total_processos);
    AdicionarLog(rich, msg, RGB(85, 255, 85));
    return true;
}

// -----------------------------------------------------------------------
// SIMULACAO PRINCIPAL
// -----------------------------------------------------------------------
void ExecutarSimulacao(KWidget janela) {
    KWidget rich      = kGetWidget(janela, IDC_RICHEDIT_LOG);
    KWidget gantt     = kGetWidget(janela, IDC_RICHEDIT_GANTT);
    KWidget combo_esc = kGetWidget(janela, IDC_COMBO_ESCALONAMENTO);
    KWidget combo_sub = kGetWidget(janela, IDC_COMBO_SUBSTITUICAO);

    kSetText(rich, "");

    char buf_mf[16], buf_q[16];
    kGetText(kGetWidget(janela, IDC_EDIT_MEM_FISICA), buf_mf, 16);
    kGetText(kGetWidget(janela, IDC_EDIT_QUANTUM),    buf_q,  16);

    int mem_fisica        = atoi(buf_mf);
    int quantum           = atoi(buf_q);
    int alg_escalonamento = kComboGetIndex(combo_esc);
    int alg_substituicao  = kComboGetIndex(combo_sub);

    if (total_processos == 0) {
        kMessageBox(janela, "Carregue um arquivo de processos antes.", "Aviso");
        return;
    }
    if (mem_fisica <= 0) {
        kMessageBox(janela, "Configure a Memoria Fisica maior que 0 MB.", "Erro");
        return;
    }

    total_frames_ram = mem_fisica / TAM_PAGINA_MB;
    if (total_frames_ram <= 0) total_frames_ram = 1;
    ram = (FrameMemoria*)realloc(ram, total_frames_ram * sizeof(FrameMemoria));
    for (int i = 0; i < total_frames_ram; i++) {
        strcpy(ram[i].proc_id, "");
        ram[i].numero_pagina = -1;
        ram[i].ultimo_acesso = 0;
    }

    for (int i = 0; i < total_processos; i++) {
        processos[i].burst_restante = processos[i].burst_total;
        processos[i].iniciado       = false;
        processos[i].concluido      = false;
        processos[i].tempo_inicio   = -1;
    }

    total_instantes = 0;

    int tempo_atual          = 0;
    int processos_concluidos = 0;
    int page_faults          = 0;
    int index_rr             = 0;

    AdicionarLog(rich, "=== INICIANDO SIMULACAO ===", RGB(100, 200, 255));

    while (processos_concluidos < total_processos) {
        int p_escolhido = -1;

        if (alg_escalonamento == 0) { // Round-Robin
            int contados = 0;
            while (contados < total_processos) {
                int i = (index_rr + contados) % total_processos;
                if (!processos[i].concluido && processos[i].tempo_chegada <= tempo_atual) {
                    p_escolhido = i;
                    index_rr = (i + 1) % total_processos;
                    break;
                }
                contados++;
            }
            if (p_escolhido == -1)
                for (int i = 0; i < total_processos; i++)
                    if (!processos[i].concluido) { p_escolhido = i; break; }
        }
        else if (alg_escalonamento == 1) { // SJF Preemptivo
            int menor = 999999;
            for (int i = 0; i < total_processos; i++)
                if (!processos[i].concluido && processos[i].tempo_chegada <= tempo_atual)
                    if (processos[i].burst_restante < menor) {
                        menor = processos[i].burst_restante;
                        p_escolhido = i;
                    }
        }
        else { // Prioridade Preemptiva
            int maior = -999999;
            for (int i = 0; i < total_processos; i++)
                if (!processos[i].concluido && processos[i].tempo_chegada <= tempo_atual)
                    if (processos[i].prioridade > maior) {
                        maior = processos[i].prioridade;
                        p_escolhido = i;
                    }
        }

        // Instante ocioso
        if (p_escolhido == -1 || processos[p_escolhido].tempo_chegada > tempo_atual) {
            if (total_instantes < MAX_INSTANTES) {
                historico[total_instantes].instante = tempo_atual;
                strcpy(historico[total_instantes].proc_id, "");
                total_instantes++;
            }
            tempo_atual++;
            continue;
        }

        Processo* p = &processos[p_escolhido];

        if (!p->iniciado) {
            p->iniciado       = true;
            p->tempo_inicio   = tempo_atual;
            p->tempo_resposta = tempo_atual - p->tempo_chegada;
        }

        int passos = (alg_escalonamento == 0)
                     ? (p->burst_restante > quantum ? quantum : p->burst_restante)
                     : 1;

        // Registra cada u.t. individualmente
        for (int s = 0; s < passos && total_instantes < MAX_INSTANTES; s++) {
            historico[total_instantes].instante = tempo_atual + s;
            strcpy(historico[total_instantes].proc_id, p->id);
            total_instantes++;
        }

        char log_exec[128];
        sprintf(log_exec, "t=%02d: %s executa %d u.t.", tempo_atual, p->id, passos);
        AdicionarLog(rich, log_exec, RGB(240, 240, 100));

        for (int pag = 0; pag < p->paginas_necessarias; pag++) {
            bool hit = AcessarPaginaMemoria(p->id, pag, alg_substituicao, tempo_atual, &page_faults);
            if (!hit) {
                char log_pf[128];
                sprintf(log_pf, "   Page Fault: %s (pag %d)", p->id, pag);
                AdicionarLog(rich, log_pf, RGB(255, 120, 120));
            }
        }

        p->burst_restante -= passos;
        tempo_atual       += passos;

        if (p->burst_restante <= 0) {
            p->concluido    = true;
            p->tempo_fim    = tempo_atual;
            p->tempo_espera = (p->tempo_fim - p->tempo_chegada) - p->burst_total;
            if (p->tempo_espera < 0) p->tempo_espera = 0;
            processos_concluidos++;
        }
    }

    // Renderiza o Gantt no RichEdit
    RenderizarGantt(gantt);

    // Metricas
    double soma_espera = 0, soma_resposta = 0;
    for (int i = 0; i < total_processos; i++) {
        soma_espera   += processos[i].tempo_espera;
        soma_resposta += processos[i].tempo_resposta;
    }

    char out_resp[32], out_esp[32], out_faults[32];
    sprintf(out_resp,   "%.2f ms", soma_resposta / total_processos);
    sprintf(out_esp,    "%.2f ms", soma_espera   / total_processos);
    sprintf(out_faults, "%d",      page_faults);

    kSetText(kGetWidget(janela, IDC_STATIC_M_RESP_VAL),   out_resp);
    kSetText(kGetWidget(janela, IDC_STATIC_M_ESPERA_VAL), out_esp);
    kSetText(kGetWidget(janela, IDC_STATIC_FAULTS_VAL),   out_faults);
}

// -----------------------------------------------------------------------
// CALLBACK DA JANELA
// -----------------------------------------------------------------------
kcontroller SimuladorJanelaProc(KWidget janela, KEvent msg, Kid id, KData data) {
    switch (msg) {
        case KInit: {
            KWidget combo_esc = kGetWidget(janela, IDC_COMBO_ESCALONAMENTO);
            kComboAdd(combo_esc, "Round-Robin (RR)");
            kComboAdd(combo_esc, "SJF Preemptivo");
            kComboAdd(combo_esc, "Prioridade Preemptiva");
            SendMessageW(combo_esc, CB_SETCURSEL, 0, 0);

            KWidget combo_sub = kGetWidget(janela, IDC_COMBO_SUBSTITUICAO);
            kComboAdd(combo_sub, "FIFO (First-In, First-Out)");
            kComboAdd(combo_sub, "LRU (Least Recently Used)");
            kComboAdd(combo_sub, "Algoritmo Otimo");
            SendMessageW(combo_sub, CB_SETCURSEL, 0, 0);

            kSetText(kGetWidget(janela, IDC_EDIT_MEM_FISICA),  "32");
            kSetText(kGetWidget(janela, IDC_EDIT_MEM_VIRTUAL), "64");
            kSetText(kGetWidget(janela, IDC_EDIT_QUANTUM),     "2");

            KWidget lv = kGetWidget(janela, IDC_LISTVIEW_PROCESSOS);
            SendMessageW(lv, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                         LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            kListColAdd(lv, 0, "ID Proc",             75);
            kListColAdd(lv, 1, "T. Chegada",         100);
            kListColAdd(lv, 2, "Burst CPU",          100);
            kListColAdd(lv, 3, "Prioridade",         100);
            kListColAdd(lv, 4, "Memoria Requisitada", 130);

            // Fundo escuro
            KWidget rich  = kGetWidget(janela, IDC_RICHEDIT_LOG);
            KWidget gantt = kGetWidget(janela, IDC_RICHEDIT_GANTT);
            SendMessageW(rich,  EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(30, 30, 30));
            SendMessageW(gantt, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(25, 25, 25));

            // IMPORTANTE: desabilita word-wrap para o scroll horizontal funcionar
            SendMessageW(gantt, EM_SETTARGETDEVICE, 0, 1);

            return TRUE;
        }

        case WM_MOUSEWHEEL: {
            KWidget rich = kGetWidget(janela, IDC_RICHEDIT_LOG);
            short delta  = (short)HIWORD(id);
            SendMessageW(rich, WM_VSCROLL,
                         delta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
            return TRUE;
        }

        case KCommand: {
            int id_controle = kGetId(id);

            if (id_controle == IDC_BTN_PROCURAR) {
                OPENFILENAMEW ofn;
                WCHAR file_buffer[260];
                ZeroMemory(&file_buffer, sizeof(file_buffer));
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner   = janela;
                ofn.lpstrFile   = file_buffer;
                ofn.nMaxFile    = 260;
                ofn.lpstrFilter = L"Arquivos CSV (*.csv)\0*.csv\0Todos os Arquivos\0*.*\0";
                ofn.Flags       = 0x00000800 | 0x00001000;

                if (GetOpenFileNameW(&ofn)) {
                    char path_char[260];
                    kInternalWideToChar(file_buffer, path_char, 260);
                    kSetText(kGetWidget(janela, IDC_EDIT_CSV_PATH), path_char);
                    CarregarCSV(path_char,
                                kGetWidget(janela, IDC_LISTVIEW_PROCESSOS),
                                kGetWidget(janela, IDC_RICHEDIT_LOG));
                }
                return TRUE;
            }
            else if (id_controle == IDC_BTN_INICIAR) {
                ExecutarSimulacao(janela);
                return TRUE;
            }
            else if (id_controle == IDC_BTN_LIMPAR) {
                kSetText(kGetWidget(janela, IDC_EDIT_CSV_PATH), "");
                kListRowClear(kGetWidget(janela, IDC_LISTVIEW_PROCESSOS));
                kSetText(kGetWidget(janela, IDC_RICHEDIT_LOG), "");
                SetWindowTextW(kGetWidget(janela, IDC_RICHEDIT_GANTT), L"");
                total_processos = 0;
                total_instantes = 0;
                return TRUE;
            }
            break;
        }

        case KClose: {
            if (ram) free(ram);
            EndKosmos(janela);
            return TRUE;
        }
    }
    return FALSE;
}

// -----------------------------------------------------------------------
// ENTRY POINT
// -----------------------------------------------------------------------
int KosmosMain() {
    KWidget win = KCreateWindow(IDD_SIMULADOR_DIALOG, SimuladorJanelaProc);
    if (win) {
        ShowWindow(win, SW_SHOW);
        LoopMsg();
    }
    return 0;
}