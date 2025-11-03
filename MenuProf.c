#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include <locale.h>
#include <Python.h>
#include <curses.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include "cJSON.h"
#include "utils.h"

// Declarações das funções do arquivos.c
extern void menu_atividades_professor();
extern void menu_criar_atividade();
extern void listar_submissoes(int atividade_id);
extern char* nome_professor_logado;

char* cp_to_utf8(const char* src) {
    if (!src || !*src) return strdup("");

    // CP_ACP → UTF-16
    int wide_len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
    if (wide_len <= 0) return strdup(src);

    wchar_t* wide = (wchar_t*)malloc(wide_len * sizeof(wchar_t));
    if (!wide) return strdup(src);
    MultiByteToWideChar(CP_ACP, 0, src, -1, wide, wide_len);

    // UTF-16 → UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0) { free(wide); return strdup(src); }

    char* utf8 = (char*)malloc(utf8_len);
    if (!utf8) { free(wide); return strdup(src); }
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, utf8_len, NULL, NULL);

    free(wide);
    return utf8;
}

void menuLogadoProf() {
    const char *opcoes[] = {"Menu de Atividades", "Postar Anúncio", "Registrar Frequência", "Atribuir Nota", "Sair"};
    int num_opcoes = 5;
    int selecionado = 0;
    int tecla;

    while (1) {
        limparTela();
        
        linha();
        printw_utf8("Bem-vindo à área do professor!\n");
        linha();
        for (int i = 0; i < num_opcoes; i++) {
            if (i == selecionado) {
                attron(COLOR_PAIR(1));
                printw_utf8("> %s\n", opcoes[i]);
                attroff(COLOR_PAIR(1));
            } else {
                printw_utf8("  %s\n", opcoes[i]);
            }
        }
        linha();
        refresh();

        tecla = getch();
        switch (tecla) {
            case KEY_UP:
                selecionado = (selecionado - 1 + num_opcoes) % num_opcoes;
                break;
            case KEY_DOWN:
                selecionado = (selecionado + 1) % num_opcoes;
                break;
            case 10: // Enter
                if (selecionado == 0) {
                    // Menu de Atividades - Agora chama o menu completo de atividades
                    menu_atividades_professor();
                } else if (selecionado == 1) { // Postar Anúncio
                    limparTela();
                    linha();
                    printw_utf8("Postar Anúncio\n");
                    linha();
                    printw_utf8("Digite a mensagem do anúncio:\n");
                    refresh();
                    echo();
                    char mensagem_raw[512];
                    getnstr(mensagem_raw, 510);
                    noecho();
                
                    // === CONVERTE CP_ACP → UTF-8 ===
                    char* mensagem_utf8 = cp_to_utf8(mensagem_raw);
                
                    limparTela();
                    linha();
                    if (strlen(mensagem_utf8) == 0) {
                        free(mensagem_utf8);
                        attron(COLOR_PAIR(2));
                        printw_utf8("Anúncio vazio! Cancelado.\n");
                        attroff(COLOR_PAIR(2));
                    } else if (postar_anuncio(mensagem_utf8) == 0) {
                        attron(COLOR_PAIR(1));
                        printw_utf8("Anúncio postado com sucesso!\n");
                        attroff(COLOR_PAIR(1));
                    } else {
                        attron(COLOR_PAIR(2));
                        printw_utf8("Erro ao salvar anúncio.\n");
                        attroff(COLOR_PAIR(2));
                    }
                    free(mensagem_utf8);  // ← Libera
                    linha();
                    printw_utf8("Pressione qualquer tecla para continuar...\n");
                    refresh();
                    getch();
                
                } else if (selecionado == 2) {
                    limparTela();
                    printw_utf8("Funcionalidade de registrar frequência ainda não implementada.\n");
                    refresh();
                    napms(2000);
                } else if (selecionado == 3) {
                    limparTela();
                    printw_utf8("Funcionalidade de atribuir nota ainda não implementada.\n");
                    refresh();
                    napms(2000);
                } else if (selecionado == 4) {
                    limparTela();
                    return;
                }
                break;
            default:
                attron(COLOR_PAIR(2));
                printw_utf8("\nTecla inválida. Use setas ou Enter. Tente novamente...\n");
                attroff(COLOR_PAIR(2));
                refresh();
                napms(2000);
                break;
        }
    }
}

// Função para postar anúncio
int postar_anuncio(const char* mensagem) {
    const char* prof_nome = nome_professor_logado ? nome_professor_logado : "Professor";

    char* caminho_anuncios = encontrar_arquivo_json("anuncios.json");
	HANDLE hFile = CreateFileA(
	    caminho_anuncios,
	    GENERIC_READ | GENERIC_WRITE,
	    0,
	    NULL,
	    OPEN_ALWAYS,
	    FILE_ATTRIBUTE_NORMAL,
	    NULL
	);

    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    // Lê conteúdo atual
    DWORD fileSize = GetFileSize(hFile, NULL);
    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer) {
        CloseHandle(hFile);
        return -1;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
        free(buffer);
        CloseHandle(hFile);
        return -1;
    }
    buffer[fileSize] = '\0';

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);

    // Se falhou ou arquivo vazio, cria estrutura básica
    if (!root) {
        root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "anuncios", cJSON_CreateArray());
    }

    cJSON* anuncios = cJSON_GetObjectItem(root, "anuncios");
    if (!cJSON_IsArray(anuncios)) {
        cJSON_Delete(root);
        CloseHandle(hFile);
        return -1;
    }

    // Cria novo anúncio
    cJSON* novo = cJSON_CreateObject();
    time_t now = time(NULL);
    char data[20];
    strftime(data, sizeof(data), "%Y-%m-%d %H:%M", localtime(&now));
    cJSON_AddStringToObject(novo, "data", data);
    cJSON_AddStringToObject(novo, "professor", prof_nome);
    cJSON_AddStringToObject(novo, "mensagem", mensagem);

    cJSON_AddItemToArray(anuncios, novo);

    // Gera JSON em UTF-8
    char* json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if (!json_str) {
        CloseHandle(hFile);
        return -1;
    }

    // === ESCREVE UTF-8 PURO COM WriteFile ===
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    DWORD bytesWritten;
    WriteFile(hFile, json_str, (DWORD)strlen(json_str), &bytesWritten, NULL);
    SetEndOfFile(hFile);  // Trunca o resto

    free(json_str);
    CloseHandle(hFile);

    return 0;
} //M