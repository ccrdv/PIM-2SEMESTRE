#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <Python.h>
#ifndef PyConfig_InitIsolatedConfig
#define PyConfig_InitIsolatedConfig PyConfig_InitPythonConfig
#endif
#include <curses.h>
#include <io.h>
#include <direct.h>
#include "cJSON.h"
#include "utils.h"

void printw_utf8(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    int wide_len = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, NULL, 0);
    if (wide_len <= 0) { printw("%s", buffer); return; }

    wchar_t* wide_str = (wchar_t*)malloc(wide_len * sizeof(wchar_t));
    if (!wide_str) { printw("%s", buffer); return; }
    MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wide_str, wide_len);

    int ansi_len = WideCharToMultiByte(CP_ACP, 0, wide_str, -1, NULL, 0, NULL, NULL);
    if (ansi_len <= 0) { free(wide_str); printw("%s", buffer); return; }

    char* ansi_str = (char*)malloc(ansi_len);
    if (!ansi_str) { free(wide_str); printw("%s", buffer); return; }
    WideCharToMultiByte(CP_ACP, 0, wide_str, -1, ansi_str, ansi_len, NULL, NULL);

    printw("%s", ansi_str);
    free(ansi_str);
    free(wide_str);
}

void limparTela(void) { clear(); refresh(); }
void linha(void) { for (int i = 0; i < COLS; i++) printw("="); refresh(); }

char* nome_professor_logado = NULL;
char* aluno_matricula_logado = NULL;
char* ROOT_PATH = NULL;

int load_config(void) {
    FILE* fp = fopen("config.txt", "r");
    if (!fp) {
        if (_access("Z:\\python_embedded", 0) == 0) {
            ROOT_PATH = _strdup("Z:\\python_embedded");
        }
        return ROOT_PATH != NULL;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        char* key = strtok(line, "=");
        char* value = strtok(NULL, "\n\r");
        if (!key || !value) continue;
        while (*value == ' ' || *value == '\t') value++;

        if (strcmp(key, "ROOT_PATH") == 0 && _access(value, 0) == 0) {
            ROOT_PATH = _strdup(value);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);

    // fallback final
    if (_access("Z:\\python_embedded", 0) == 0) {
        ROOT_PATH = _strdup("Z:\\python_embedded");
    }
    return ROOT_PATH != NULL;
}

wchar_t* wcs_concat(const wchar_t* a, const wchar_t* b) {
    size_t la = wcslen(a), lb = wcslen(b);
    wchar_t* r = (wchar_t*)malloc((la + lb + 1) * sizeof(wchar_t));
    if (r) { wcscpy(r, a); wcscat(r, b); }
    return r;
}

void print_banner_acad(){
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_WHITE, COLOR_BLACK);

    char *banner_lines[] = {
        ":::'###:::::'######:::::'###::::'########::'########:'##::::'##:'####::'######:::'#######::",
        "::'## ##:::'##... ##:::'## ##::: ##.... ##: ##.....:: ###::'###:. ##::'##... ##:'##.... ##:",
        ":'##:. ##:: ##:::..:::'##:. ##:: ##:::: ##: ##::::::: ####'####:: ##:: ##:::..:: ##:::: ##:",
        "'##:::. ##: ##:::::::'##:::. ##: ##:::: ##: ######::: ## ### ##:: ##:: ##::::::: ##:::: ##:",
        " #########: ##::::::: #########: ##:::: ##: ##...:::: ##. #: ##:: ##:: ##::::::: ##:::: ##:",
        " ##.... ##: ##::: ##: ##.... ##: ##:::: ##: ##::::::: ##:.:: ##:: ##:: ##::: ##: ##:::: ##:",
        " ##:::: ##:. ######:: ##:::: ##: ########:: ########: ##:::: ##:'####:. ######::. #######::",
        "..:::::..:::......:::..:::::..::........:::........::..:::::..::....:::......::::.......:::"
    };

    int line_colors[] = {3,4,3,5,4,3,5,4};

    for (int i = 0; i < 8; i++) {
        attron(COLOR_PAIR(6));
        for (int j = 0; banner_lines[i][j]; j++) {
            if (banner_lines[i][j] == '#') {
                attroff(COLOR_PAIR(6));
                attron(COLOR_PAIR(line_colors[i]));
                addch('#');
                attroff(COLOR_PAIR(line_colors[i]));
                attron(COLOR_PAIR(6));
            } else addch(banner_lines[i][j]);
        }
        attroff(COLOR_PAIR(6));
        addch('\n');
    }
}

int init_python_portable(void) {
    const char* base = ROOT_PATH ? ROOT_PATH : "python_embedded";
    char python_dll[MAX_PATH];
    char python_home[MAX_PATH];

    snprintf(python_dll, sizeof(python_dll), "%s\\python311.dll", base);
    if (_access(python_dll, 0) != 0) {
        char exe_dir[MAX_PATH];
        GetModuleFileNameA(NULL, exe_dir, MAX_PATH);
        char* slash = strrchr(exe_dir, '\\');
        if (slash) *slash = '\0';
        snprintf(python_dll, sizeof(python_dll), "%s\\python311.dll", exe_dir);
        if (_access(python_dll, 0) != 0) return 0;
    }

    snprintf(python_home, sizeof(python_home), "%s", base);
    if (_access(python_home, 0) != 0) return 0;

    HMODULE hPython = LoadLibraryA(python_dll);
    if (!hPython) return 0;

    PyStatus status;
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);

    wchar_t w_python_home[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, python_home, -1, w_python_home, MAX_PATH);
    PyConfig_SetString(&config, &config.home, w_python_home);

    config.site_import = 0;
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) return 0;

    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
        "import sys\n"
        "path = r'%s'.replace('\\\\','/')\n"
        "if path not in sys.path: sys.path.append(path)", base);
    PyRun_SimpleString(cmd);

    return 1;
}

void garantir_arquivos_json(void) {
    const char* root = ROOT_PATH ? ROOT_PATH : "python_embedded";
    char code[4096];
    snprintf(code, sizeof(code),
        "import os, json, sys\n"
        "root = r'%s'\n"
        "arquivos = ['users.json','turmas.json','alunos.json','matriculas.json','atividades.json','anuncios.json']\n"
        "for a in arquivos:\n"
        "    dest = os.path.join(root, a)\n"
        "    if not os.path.exists(dest):\n"
        "        data = {'users':[]} if a=='users.json' else \\\n"
        "               {'turmas':[]} if a=='turmas.json' else \\\n"
        "               {'atividades':[]} if a=='atividades.json' else \\\n"
        "               {'anuncios':[]} if a=='anuncios.json' else {}\n"
        "        with open(dest,'w',encoding='utf-8') as f:\n"
        "            json.dump(data,f,indent=4,ensure_ascii=False)\n",
        root);
    PyRun_SimpleString(code);
}

PyObject* safe_import_module(const char* module_name) {
    return PyImport_ImportModule(module_name);
}

/* ------------------------------------------------- */
/*  Funções que chamam os scripts Python (sem debug) */
/* ------------------------------------------------- */
int rodarPythonRegistrar(void) {
    endwin(); SetConsoleOutputCP(65001);
    PyObject* pModule = safe_import_module("registroaluno");
    if (!pModule) { initscr(); return 0; }

    PyObject* pFunc = PyObject_GetAttrString(pModule, "registrar");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        Py_DECREF(pModule);
        initscr(); return 0;
    }

    PyObject* pValue = PyObject_CallObject(pFunc, NULL);
    int result = (pValue && PyObject_IsTrue(pValue));
    Py_XDECREF(pValue); Py_DECREF(pFunc); Py_DECREF(pModule);

    initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
    start_color();
    return result;
}

int rodarPythonLogin(void) {
    endwin(); SetConsoleOutputCP(65001);
    PyObject* pModule = safe_import_module("registroaluno");
    if (!pModule) { initscr(); return 0; }

    PyObject* pFunc = PyObject_GetAttrString(pModule, "login_aluno");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        Py_DECREF(pModule);
        initscr(); return 0;
    }

    PyObject* pValue = PyObject_CallObject(pFunc, NULL);
    int result = 0;
    if (pValue && pValue != Py_None && PyUnicode_Check(pValue)) {
        const char* mat = PyUnicode_AsUTF8(pValue);
        if (mat) {
            if (aluno_matricula_logado) free(aluno_matricula_logado);
            aluno_matricula_logado = strdup(mat);
            result = 1;
        }
    }
    Py_XDECREF(pValue); Py_DECREF(pFunc); Py_DECREF(pModule);

    initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
    start_color();
    return result;
}

int rodarPythonLoginProfessor(void) {
    endwin(); SetConsoleOutputCP(65001);
    PyObject* pModule = safe_import_module("LoginProfessor");
    if (!pModule) { initscr(); return 0; }

    PyObject* pFunc = PyObject_GetAttrString(pModule, "login_professor");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        Py_DECREF(pModule);
        initscr(); return 0;
    }

    PyObject* pValue = PyObject_CallObject(pFunc, NULL);
    int result = 0;
    if (pValue && pValue != Py_None) {
        const char* name = PyUnicode_AsUTF8(pValue);
        if (name) {
            if (nome_professor_logado) free(nome_professor_logado);
            nome_professor_logado = strdup(name);
            result = 1;
        }
        Py_DECREF(pValue);
    }
    Py_DECREF(pFunc); Py_DECREF(pModule);

    initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
    start_color();
    return result;
}

/* ------------------- Menus ------------------- */
void menuAluno(void) {
    const char* opcoes[] = {"Login", "Registrar", "Voltar"};
    int num_opcoes = 3, sel = 0, tecla;

    while (1) {
        limparTela(); linha();
        printw_utf8("Menu do Aluno\n");
        linha();
        for (int i = 0; i < num_opcoes; i++) {
            if (i == sel) { attron(COLOR_PAIR(1)); printw("> %s\n", opcoes[i]); attroff(COLOR_PAIR(1)); }
            else printw("  %s\n", opcoes[i]);
        }
        linha(); refresh();

        tecla = getch();
        switch (tecla) {
            case KEY_UP:   sel = (sel - 1 + num_opcoes) % num_opcoes; break;
            case KEY_DOWN: sel = (sel + 1) % num_opcoes; break;
            case 10:
                if (sel == 0) {
                    if (rodarPythonLogin()) {
                        menuLogadoAluno();
                    }
                } else if (sel == 1) {
                    rodarPythonRegistrar();
                } else if (sel == 2) {
                    return;
                }
                break;
        }
    }
}

void menuProfessor(void) {
    const char* opcoes[] = {"Login", "Voltar"};
    int num_opcoes = 2, sel = 0, tecla;

    while (1) {
        limparTela(); linha();
        printw_utf8("Menu do Professor\n");
        linha();
        for (int i = 0; i < num_opcoes; i++) {
            if (i == sel) { attron(COLOR_PAIR(1)); printw("> %s\n", opcoes[i]); attroff(COLOR_PAIR(1)); }
            else printw("  %s\n", opcoes[i]);
        }
        linha(); refresh();

        tecla = getch();
        switch (tecla) {
            case KEY_UP:   sel = (sel - 1 + num_opcoes) % num_opcoes; break;
            case KEY_DOWN: sel = (sel + 1) % num_opcoes; break;
            case 10:
                if (sel == 0) {
                    if (rodarPythonLoginProfessor()) {
                        menuLogadoProf();
                    }
                } else if (sel == 1) {
                    return;
                }
                break;
        }
    }
}

void menuLogadoAluno(void);
void menuLogadoProf(void);

void menuPrincipal(void) {
    const char* opcoes[] = {"Aluno", "Professor", "Sair"};
    int num_opcoes = 3, sel = 0, tecla;

    while (1) {
        limparTela(); linha();
        print_banner_acad();
        linha();
        for (int i = 0; i < num_opcoes; i++) {
            if (i == sel) { attron(COLOR_PAIR(1)); printw("> %s\n", opcoes[i]); attroff(COLOR_PAIR(1)); }
            else printw("  %s\n", opcoes[i]);
        }
        linha(); refresh();

        tecla = getch();
        switch (tecla) {
            case KEY_UP:   sel = (sel - 1 + num_opcoes) % num_opcoes; break;
            case KEY_DOWN: sel = (sel + 1) % num_opcoes; break;
            case 10:
                if (sel == 0) menuAluno();
                else if (sel == 1) menuProfessor();
                else if (sel == 2) { limparTela(); printw("Saindo...\n"); refresh(); napms(1000); return; }
                break;
        }
    }
}

int main(void) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    load_config();

    char exe_dir[MAX_PATH];
    GetModuleFileNameA(NULL, exe_dir, MAX_PATH);
    char* slash = strrchr(exe_dir, '\\');
    if (slash) *slash = '\0';
    _chdir(exe_dir);

    if (!init_python_portable()) return 1;
    garantir_arquivos_json();

    initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK);

    menuPrincipal();

    if (nome_professor_logado) free(nome_professor_logado);
    if (aluno_matricula_logado) free(aluno_matricula_logado);
    if (ROOT_PATH) free(ROOT_PATH);

    endwin();
    Py_Finalize();
    return 0;
}