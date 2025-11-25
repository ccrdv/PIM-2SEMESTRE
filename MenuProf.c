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


extern char* nome_professor_logado;


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
    cbreak(); noecho(); reiniciar_cores();
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

/* MENU DE ATIVIDADES DO PROFESSOR - VERSÃO INTERATIVA E PROFISSIONAL */
void menu_atividades_professor() {
    while (1) {
        // === MENU PRINCIPAL ===
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
            printw_utf8("^v Navegar | ENTER Selecionar | ESC Voltar\n");
            refresh();

            tecla = getch();
            if (tecla == KEY_UP)   selecionado = (selecionado - 1 + num_opcoes) % num_opcoes;
            if (tecla == KEY_DOWN) selecionado = (selecionado + 1) % num_opcoes;
            if (tecla == 27) return; // ESC
            if (tecla == 10) break;
        }

        // === AÇÕES ===
        if (selecionado == 0) {
            menu_criar_atividade();  // ← chama do arquivos.c (com seleção de turma!)
            continue;
        }
        if (selecionado == 2) {
            return; // Voltar
        }

        // === VER SUBMISSÕES - LISTA INTERATIVA ===
        char* caminho = encontrar_arquivo_json("atividades.json");
        FILE* fp = fopen(caminho, "r");
        if (!fp) {
            limparTela();
            linha();
            attron(COLOR_PAIR(2));
            printw_utf8("Erro: atividades.json não encontrado!\n");
            attroff(COLOR_PAIR(2));
            linha();
            printw_utf8("Pressione qualquer tecla...\n");
            getch();
            continue;
        }

        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char* buffer = malloc(size + 1);
        fread(buffer, 1, size, fp);
        buffer[size] = '\0';
        fclose(fp);

        cJSON* root = cJSON_Parse(buffer);
        free(buffer);
        if (!root) {
            limparTela();
            attron(COLOR_PAIR(2));
            printw_utf8("Erro ao ler atividades.json\n");
            attroff(COLOR_PAIR(2));
            getch();
            continue;
        }

        typedef struct {
            int id;
            int turma_id;
            char descricao[256];
            int submissoes;
        } Ativ;

        Ativ lista[100];
        int total = 0;

        cJSON* array = cJSON_GetObjectItem(root, "atividades");
        cJSON* item = NULL;
        cJSON_ArrayForEach(item, array) {
            cJSON* id_obj = cJSON_GetObjectItem(item, "atividade_id");
            cJSON* turma_obj = cJSON_GetObjectItem(item, "turma_id");
            cJSON* desc_obj = cJSON_GetObjectItem(item, "descricao");
            cJSON* subs = cJSON_GetObjectItem(item, "submissoes");

            if (!id_obj || !turma_obj) continue;

            char* nome_turma = obter_nome_turma_por_id(turma_obj->valueint);
            if (strstr(nome_turma, "Desconhecida") != NULL) continue; // não é dele

            if (total >= 100) break;

            lista[total].id = id_obj->valueint;
            lista[total].turma_id = turma_obj->valueint;
            strncpy(lista[total].descricao,
                    desc_obj && cJSON_IsString(desc_obj) ? desc_obj->valuestring : "Sem descrição", 255);
            lista[total].submissoes = cJSON_GetArraySize(subs);
            total++;
        }
        cJSON_Delete(root);

        if (total == 0) {
            limparTela();
            linha();
            printw_utf8("Você ainda não criou nenhuma atividade.\n");
            linha();
            printw_utf8("Pressione qualquer tecla para voltar...\n");
            getch();
            continue;
        }

        // === SELEÇÃO DA ATIVIDADE ===
        int sel = 0;
        while (1) {
            limparTela();
            linha();
            printw_utf8("Suas Atividades (Selecione para ver submissões)\n");
            linha();
            printw_utf8(" ID  | Turma                   | Subs | Descrição\n");
            linha();

            for (int i = 0; i < total; i++) {
                char* turma = obter_nome_turma_por_id(lista[i].turma_id);
                if (i == sel) attron(COLOR_PAIR(1));
                printw_utf8("%s%3d | %-23s | %4d | %s\n",
                           i == sel ? "> " : "  ",
                           lista[i].id, turma, lista[i].submissoes, lista[i].descricao);
                if (i == sel) attroff(COLOR_PAIR(1));
            }

            linha();
            printw_utf8("^v Navegar | ENTER Ver submissões | ESC Voltar\n");
            refresh();

            tecla = getch();
            if (tecla == KEY_UP)   sel = (sel - 1 + total) % total;
            if (tecla == KEY_DOWN) sel = (sel + 1) % total;
            if (tecla == 27) break;
            if (tecla == 10) {
                limparTela();
                linha();
                char* turma_nome = obter_nome_turma_por_id(lista[sel].turma_id);
                printw_utf8("Submissões - Atividade ID: %d\n", lista[sel].id);
                printw_utf8("Turma: %s\n", turma_nome);
                printw_utf8("Descrição: %s\n", lista[sel].descricao);
                linha();

                listar_submissoes(lista[sel].id);

                linha();
                printw_utf8("Pressione qualquer tecla para voltar...\n");
                getch();
                break;
            }
        }
    }
}
void menuLogadoProf() {
    const char* opcoes[] = {"Menu de Atividades", "Postar Anúncio", "Registrar Presença", "Atribuir Nota", "Sair"};
    int num_opcoes = 5;
    int selecionado = 0;
    int tecla;
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
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
                    atribuir_notas_interativo();
                } else if (selecionado == 4) {
                    limparTela(); return;
                }
                break;
            default:
                break;
        }
    }
}