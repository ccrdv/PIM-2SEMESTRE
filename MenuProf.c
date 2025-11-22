// MenuProf.c
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

// === GLOBALS ===
extern char* nome_professor_logado;

// === UTF-8 CONVERSION ===
char* cp_to_utf8(const char* src) {
    if (!src || !*src) return strdup("");
    int wide_len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
    if (wide_len <= 0) return strdup(src);
    wchar_t* wide = (wchar_t*)malloc(wide_len * sizeof(wchar_t));
    if (!wide) return strdup(src);
    MultiByteToWideChar(CP_ACP, 0, src, -1, wide, wide_len);
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0) { free(wide); return strdup(src); }
    char* utf8 = (char*)malloc(utf8_len);
    if (!utf8) { free(wide); return strdup(src); }
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, utf8_len, NULL, NULL);
    free(wide);
    return utf8;
}

void print_banner_prof(){
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_WHITE, COLOR_BLACK);
    
    char *banner_lines[] = {
        "'########::'########:::'#######::'########:'########::'######:::'######:::'#######::'########::",
        " ##.... ##: ##.... ##:'##.... ##: ##.....:: ##.....::'##... ##:'##... ##:'##.... ##: ##.... ##:",
        " ##:::: ##: ##:::: ##: ##:::: ##: ##::::::: ##::::::: ##:::..:: ##:::..:: ##:::: ##: ##:::: ##:",
        " ########:: ########:: ##:::: ##: ######::: ######:::. ######::. ######:: ##:::: ##: ########::",
        " ##.....::: ##.. ##::: ##:::: ##: ##...:::: ##...:::::..... ##::..... ##: ##:::: ##: ##.. ##:::",
        " ##:::::::: ##::. ##:: ##:::: ##: ##::::::: ##:::::::'##::: ##:'##::: ##: ##:::: ##: ##::. ##::",
        " ##:::::::: ##:::. ##:. #######:: ##::::::: ########:. ######::. ######::. #######:: ##:::. ##::",
        "..:::::::::..:::::..:::.......:::..::::::::........:::......::::......::::.......:::..:::::..::"
    };
    
    // Cores focadas em ciano/azul e amarelo
    int line_colors[] = {3, 4, 3, 5, 4, 3, 5, 4}; // Ciano(3), Amarelo(4), Azul(5)
    
    for (int i = 0; i < 8; i++) {
        attron(COLOR_PAIR(6));  // Branco como cor base
        
        for (int j = 0; banner_lines[i][j] != '\0'; j++) {
            if (banner_lines[i][j] == '#') {
                attroff(COLOR_PAIR(6));
                attron(COLOR_PAIR(line_colors[i]));
                addch('#');
                attroff(COLOR_PAIR(line_colors[i]));
                attron(COLOR_PAIR(6));
            } else {
                addch(banner_lines[i][j]);
            }
        }
        
        attroff(COLOR_PAIR(6));
        addch('\n');
    }
}

// === PYTHON CALL HELPER ===
static int call_professor_func(const char* func_name, const char* username) {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    PyObject *pMod = NULL, *pFunc = NULL, *pArgs = NULL, *pResult = NULL;
    int ok = 0;

    pMod = PyImport_ImportModule("LoginProfessor");
    if (!pMod) { PyErr_Print(); goto cleanup; }

    pFunc = PyObject_GetAttrString(pMod, func_name);
    if (!pFunc || !PyCallable_Check(pFunc)) { PyErr_Print(); goto cleanup; }

    pArgs = PyTuple_Pack(1, PyUnicode_FromString(username));
    pResult = PyObject_CallObject(pFunc, pArgs);
    if (!pResult) { PyErr_Print(); goto cleanup; }

    ok = PyObject_IsTrue(pResult);

cleanup:
    Py_XDECREF(pResult);
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunc);
    Py_XDECREF(pMod);

    initscr();
    keypad(stdscr, TRUE);
    cbreak(); noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    return ok;
}

int python_registrar_presenca(const char* username) {
    return call_professor_func("registrar_presenca", username);
}

int python_atribuir_notas(const char* username) {
    return call_professor_func("atribuir_notas", username);
}

// === POST ANNOUNCEMENT ===
int postar_anuncio(const char* mensagem) {
    const char* prof_nome = nome_professor_logado ? nome_professor_logado : "Professor";
    char* caminho_anuncios = encontrar_arquivo_json("anuncios.json");

    HANDLE hFile = CreateFileA(caminho_anuncios, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;

    DWORD fileSize = GetFileSize(hFile, NULL);
    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer) { CloseHandle(hFile); return -1; }

    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
        free(buffer); CloseHandle(hFile); return -1;
    }
    buffer[fileSize] = '\0';

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "anuncios", cJSON_CreateArray());
    }

    cJSON* anuncios = cJSON_GetObjectItem(root, "anuncios");
    if (!cJSON_IsArray(anuncios)) { cJSON_Delete(root); CloseHandle(hFile); return -1; }

    cJSON* novo = cJSON_CreateObject();
    time_t now = time(NULL);
    char data[20];
    strftime(data, sizeof(data), "%Y-%m-%d %H:%M", localtime(&now));
    cJSON_AddStringToObject(novo, "data", data);
    cJSON_AddStringToObject(novo, "professor", prof_nome);
    cJSON_AddStringToObject(novo, "mensagem", mensagem);
    cJSON_AddItemToArray(anuncios, novo);

    char* json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_str) { CloseHandle(hFile); return -1; }

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    DWORD bytesWritten;
    WriteFile(hFile, json_str, (DWORD)strlen(json_str), &bytesWritten, NULL);
    SetEndOfFile(hFile);
    free(json_str);
    CloseHandle(hFile);
    return 0;
}

// === MENU DE ATIVIDADES (EMBUTIDO) ===
static void menu_atividades_professor() {
    const char* opcoes[] = {"Criar Atividade", "Ver Submissões", "Voltar"};
    int num_opcoes = 3;
    int selecionado = 0;
    int tecla;

    while (1) {
        limparTela();
        linha();
        printw_utf8("Menu de Atividades - Professor\n");
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
            case KEY_UP:  selecionado = (selecionado - 1 + num_opcoes) % num_opcoes; break;
            case KEY_DOWN:selecionado = (selecionado + 1) % num_opcoes; break;
            case 10:
                if (selecionado == 0) {
                    // === CRIAR ATIVIDADE ===
                    limparTela();
                    linha();
                    printw_utf8("Criar Nova Atividade\n");
                    linha();

                    char caminho[512], desc[512];
                    printw_utf8("Caminho do enunciado, sem aspas (.txt): ");
                    echo(); getnstr(caminho, 510); noecho();
                    printw_utf8("Descrição (opcional): ");
                    echo(); getnstr(desc, 510); noecho();

                    int turma_id = 1; // Hardcoded for now
                    int result = criar_atividade(caminho, desc, turma_id);
                    if (result == 0) {
                        attron(COLOR_PAIR(1));
                        printw_utf8("Atividade criada com sucesso!\n");
                        attroff(COLOR_PAIR(1));
                    } else {
                        attron(COLOR_PAIR(2));
                        printw_utf8("Erro ao criar atividade (código: %d)\n", result);
                        attroff(COLOR_PAIR(2));
                    }
                    refresh(); getch();

                } else if (selecionado == 1) {
                    // === VER SUBMISSÕES ===
                    limparTela();
                    linha();
                    printw_utf8("Ver Submissões\n");
                    linha();
                    printw_utf8("ID da atividade: ");
                    int id;
                    echo();
                    if (scanw("%d", &id) != 1) {
                        noecho();
                        attron(COLOR_PAIR(2));
                        printw_utf8("Entrada inválida!\n");
                        attroff(COLOR_PAIR(2));
                        napms(1500);
                        continue;
                    }
                    noecho();
                    listar_submissoes(id);
                    printw_utf8("Pressione qualquer tecla...\n");
                    getch();
                } else if (selecionado == 2) {
                    return;
                }
                break;
            default: break;
        }
    }
}

// === MAIN PROFESSOR MENU ===
void menuLogadoProf() {
    const char* opcoes[] = {"Menu de Atividades", "Postar Anúncio", "Registrar Presença", "Atribuir Nota", "Sair"};
    int num_opcoes = 5;
    int selecionado = 0;
    int tecla;

    while (1) {
        limparTela();
        linha();
        print_banner_prof();
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
            case KEY_UP:  selecionado = (selecionado - 1 + num_opcoes) % num_opcoes; break;
            case KEY_DOWN:selecionado = (selecionado + 1) % num_opcoes; break;
            case 10:
                if (selecionado == 0) {
                    menu_atividades_professor();
                } else if (selecionado == 1) {
                    limparTela();
                    linha();
                    printw_utf8("Postar Anúncio\n");
                    linha();
                    printw_utf8("Mensagem: ");
                    echo();
                    char msg_raw[512];
                    getnstr(msg_raw, 510);
                    noecho();
                    char* msg_utf8 = cp_to_utf8(msg_raw);
                    if (strlen(msg_utf8) == 0) {
                        free(msg_utf8);
                        attron(COLOR_PAIR(2)); printw_utf8("Anúncio vazio!\n"); attroff(COLOR_PAIR(2));
                    } else if (postar_anuncio(msg_utf8) == 0) {
                        attron(COLOR_PAIR(1)); printw_utf8("Anúncio postado!\n"); attroff(COLOR_PAIR(1));
                    } else {
                        attron(COLOR_PAIR(2)); printw_utf8("Erro ao postar.\n"); attroff(COLOR_PAIR(2));
                    }
                    free(msg_utf8);
                    linha(); printw_utf8("Pressione qualquer tecla...\n"); getch();
                } else if (selecionado == 2) {
                    if (nome_professor_logado) python_registrar_presenca(nome_professor_logado);
                    else { attron(COLOR_PAIR(2)); printw_utf8("Professor não logado.\n"); attroff(COLOR_PAIR(2)); getch(); }
                } else if (selecionado == 3) {
                    if (nome_professor_logado) python_atribuir_notas(nome_professor_logado);
                    else { attron(COLOR_PAIR(2)); printw_utf8("Professor não logado.\n"); attroff(COLOR_PAIR(2)); getch(); }
                } else if (selecionado == 4) {
                    limparTela(); return;
                }
                break;
            default:
                break;
        }
    }
}