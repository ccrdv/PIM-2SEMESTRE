#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#undef MOUSE_MOVED
#include <Python.h>
#include <curses.h>
#include "MenuAdm.h"
#include "MenuSec.h"

#define MAX_INPUT 256
#define PYTHON_EMBEDDED_ROOT L"Z:\\python_embedded"

/* Limpa a tela e atualiza o display */
void limparTela() {
    clear();
    refresh();
}

/* Desenha uma linha horizontal usando o caractere = na largura total da tela */
void linha() {
    for (int i = 0; i < COLS; i++) printw("=");
    refresh();
}

/* Converte e exibe uma string UTF-8 corretamente no PDCurses (Windows) */
void printw_utf8(const char* str) {
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (wide_len <= 0) { printw("%s", str); return; }
    wchar_t* wide = (wchar_t*)malloc(wide_len * sizeof(wchar_t));
    if (!wide) { printw("%s", str); return; }
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wide, wide_len);
    int ansi_len = WideCharToMultiByte(CP_ACP, 0, wide, -1, NULL, 0, NULL, NULL);
    if (ansi_len > 0) {
        char* ansi = (char*)malloc(ansi_len);
        if (ansi) {
            WideCharToMultiByte(CP_ACP, 0, wide, -1, ansi, ansi_len, NULL, NULL);
            printw("%s", ansi);
            free(ansi);
        }
    }
    free(wide);
}

/* Exibe o banner colorido de boas-vindas com o logo em ASCII art */
void print_banner() {
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_pair(6, COLOR_WHITE, COLOR_BLACK);
    init_pair(7, COLOR_BLUE, COLOR_BLACK);
    init_pair(8, COLOR_MAGENTA, COLOR_BLACK);
    char *banner_lines[] = {
        "'######:::'########::'######::'########::::'###:::::'#######::",
        "##...##:: '##.....::'##... ##:... ##..::::'## ##:::'##.... ##:",
        "##:::..::: ##::::::: ##:::..::::: ##:::::'##:. ##:: ##:::: ##:",
        "##::'####: ######:::. ######::::: ##::::'##:::. ##: ##:::: ##:",
        "##::: ##:: ##...:::::..... ##:::: ##:::: #########: ##:::: ##:",
        "##::: ##:: ##:::::::'##::: ##:::: ##:::: ##.... ##: ##:::: ##:",
        ".######::: ########:. ######::::: ##:::: ##:::: ##:. #######::",
        ":......::::........:::......::::::..:::::..:::::..:::.......::"
    };
    int line_colors[] = {3,4,4,5,3,7,4,5};
    int bold_lines[] = {1,0,0,1,1,1,0,0};
    for (int i = 0; i < 8; i++) {
        int attr = bold_lines[i] ? A_BOLD : A_NORMAL;
        attron(COLOR_PAIR(6) | attr);
        for (int j = 0; banner_lines[i][j] != '\0'; j++) {
            if (banner_lines[i][j] == '#') {
                attroff(COLOR_PAIR(6));
                attron(COLOR_PAIR(line_colors[i]) | attr);
                addch('#');
                attroff(COLOR_PAIR(line_colors[i]) | attr);
                attron(COLOR_PAIR(6) | attr);
            } else {
                addch(banner_lines[i][j]);
            }
        }
        attroff(COLOR_PAIR(6) | attr);
        addch('\n');
    }
    linha();
    attron(COLOR_PAIR(8) | A_BOLD);
    printw("Bem-vindo ao sistema de gerenciamento!\n");
    attroff(COLOR_PAIR(8) | A_BOLD);
    linha();
}

/* Cria os arquivos JSON necessários caso ainda não existam */
void garantir_arquivos_json() {
    PyRun_SimpleString(
        "import os, json\n"
        "dir = r'Z:\\python_embedded'\n"
        "os.makedirs(dir, exist_ok=True)\n"
        "arquivos = ['users.json','turmas.json','alunos.json','matriculas.json','atividades.json','anuncios.json']\n"
        "for a in arquivos:\n"
        " caminho = os.path.join(dir, a)\n"
        " if not os.path.exists(caminho):\n"
        " data = {'users':[]} if a=='users.json' else {'turmas':[]} if a=='turmas.json' else {'atividades':[]} if a=='atividades.json' else {'anuncios':[]} if a=='anuncios.json' else {}\n"
        " with open(caminho,'w',encoding='utf-8') as f: json.dump(data,f,indent=4,ensure_ascii=False)\n"
    );
}

/* Inicializa o interpretador Python embutido no diretório portátil Z:\python_embedded */
int init_python_portable() {
    PyConfig config;
    PyStatus status;
    PyConfig_InitIsolatedConfig(&config);
    status = PyConfig_SetString(&config, &config.home, PYTHON_EMBEDDED_ROOT);
    if (PyStatus_Exception(status)) goto erro;
    wchar_t base[512];
    wcscpy(base, PYTHON_EMBEDDED_ROOT);
    wchar_t lib_path[512];
    wchar_t dlls_path[512];
    wcscpy(lib_path, base);
    wcscat(lib_path, L"\\Lib");
    wcscpy(dlls_path, base);
    wcscat(dlls_path, L"\\DLLs");
    status = PyWideStringList_Append(&config.module_search_paths, lib_path);
    if (PyStatus_Exception(status)) goto erro;
    status = PyWideStringList_Append(&config.module_search_paths, dlls_path);
    if (PyStatus_Exception(status)) goto erro;
    status = PyConfig_SetString(&config, &config.program_name, L"PIMSeceAdm.exe");
    if (PyStatus_Exception(status)) goto erro;
    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) goto erro;
    PyConfig_Clear(&config);
    PyRun_SimpleString(
        "import sys\n"
        "sys.path.insert(0, r'Z:\\python_embedded')\n"
    );
    return 1;
erro:
    PyConfig_Clear(&config);
    return 0;
}

/* Executa o processo completo de login chamando a função login() do módulo Python ADM */
int rodarPythonLogin() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    char username[MAX_INPUT] = {0};
    char password[MAX_INPUT] = {0};
    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    clear();
    printw("=== Sistema de Login ===\n");
    printw_utf8("Nome de usuário: ");
    refresh();
    echo();
    getnstr(username, MAX_INPUT - 1);
    printw("Senha: ");
    refresh();
    noecho();
    getnstr(password, MAX_INPUT - 1);
    echo();
    endwin();

    PyObject *pName = PyUnicode_DecodeFSDefault("ADM");
    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (!pModule) {
        PyErr_Print();
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        attron(COLOR_PAIR(2));
        printw_utf8("Erro ao importar módulo ADM\n");
        attroff(COLOR_PAIR(2));
        refresh(); getch(); endwin();
        return 0;
    }

    PyObject *pFunc = PyObject_GetAttrString(pModule, "login");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        if (PyErr_Occurred()) PyErr_Print();
        Py_DECREF(pModule);
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        attron(COLOR_PAIR(2));
        printw_utf8("Erro ao encontrar função login\n");
        attroff(COLOR_PAIR(2));
        refresh(); getch(); endwin();
        return 0;
    }

    PyObject *pArgs = PyTuple_Pack(2, PyUnicode_FromString(username), PyUnicode_FromString(password));
    PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);

    if (!pValue) {
        PyErr_Print();
        Py_DECREF(pFunc); Py_DECREF(pModule);
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        attron(COLOR_PAIR(2));
        printw_utf8("Erro ao chamar função login\n");
        attroff(COLOR_PAIR(2));
        refresh(); getch(); endwin();
        return 0;
    }

    int success = 0;
    char *role = NULL;
    if (!PyArg_ParseTuple(pValue, "is", &success, &role)) {
        PyErr_Print();
        success = 0;
    }

    Py_DECREF(pValue); Py_DECREF(pFunc); Py_DECREF(pModule);

    initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
    start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);

    if (success && role) {
        attron(COLOR_PAIR(1));
        printw("\nLogin bem-sucedido! Carregando menu...\n");
        attroff(COLOR_PAIR(1));
        refresh(); napms(2000);
        if (strcmp(role, "admin") == 0) {
            endwin();
            menuAdmin();
        } else if (strcmp(role, "secretaria") == 0) {
            endwin();
            menuSecretaria();
        }
    } else {
        attron(COLOR_PAIR(2));
        printw("\nLogin falhou!\n");
        attroff(COLOR_PAIR(2));
        refresh(); getch();
    }
    endwin();
    return success;
}

/* Exibe o menu principal com navegação por setas e seleção com Enter */
void menuPrincipal() {
    const char *opcoes[] = {"Login", "Sair"};
    int num_opcoes = 2, selecionado = 0, tecla;
    while (1) {
        limparTela(); linha();
        print_banner();
        for (int i = 0; i < num_opcoes; i++) {
            if (i == selecionado) {
                attron(COLOR_PAIR(1));
                printw("> %s\n", opcoes[i]);
                attroff(COLOR_PAIR(1));
            } else {
                printw(" %s\n", opcoes[i]);
            }
        }
        linha(); refresh();
        tecla = getch();
        switch (tecla) {
            case KEY_UP:
                selecionado = (selecionado - 1 + num_opcoes) % num_opcoes;
                break;
            case KEY_DOWN:
                selecionado = (selecionado + 1) % num_opcoes;
                break;
            case 10:  // Enter
                if (selecionado == 0) {
                    limparTela();
                    rodarPythonLogin();
                } else if (selecionado == 1) {
                    limparTela(); linha();
                    attron(COLOR_PAIR(2));
                    printw("Saindo....\n");
                    attroff(COLOR_PAIR(2));
                    linha(); refresh(); napms(1000);
                    return;
                }
                break;
        }
    }
}

/* Função principal: inicia Python, garante arquivos JSON, inicia ncurses e abre o menu */
int main() {
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    if (!init_python_portable()) {
        system("pause");
        return 1;
    }

    garantir_arquivos_json();

    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);

    menuPrincipal();

    endwin();
    Py_Finalize();
    return 0;
}