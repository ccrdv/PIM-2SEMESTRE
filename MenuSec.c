#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#undef MOUSE_MOVED
#include <Python.h>
#include <direct.h>
#include "MenuAdm.h"
#include "MenuSec.h"

#define MAX_INPUT 256

// Funções principais (declaradas no main2.c)
void limparTela();
void linha();
void printw_utf8();

/* Exibe o banner colorido específico do menu da Secretaria */
void print_banner_sec(){
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_pair(6, COLOR_WHITE, COLOR_BLACK);
    init_pair(7, COLOR_BLUE, COLOR_BLACK);
   
    char *banner_lines[] = {
        " :'######::'########::'######::'########::'########:'########::::'###::::'########::'####::::'###::::",
        " '##... ##: ##.....::'##... ##: ##.... ##: ##.....::... ##..::::'## ##::: ##.... ##:. ##::::'## ##:::",
        " '##:::..:: ##::::::: ##:::..:: ##:::: ##: ##:::::::::: ##:::::'##:. ##:: ##:::: ##:: ##:::'##:. ##::",
        " . ######:: ######::: ##::::::: ########:: ######:::::: ##::::'##:::. ##: ########::: ##::'##:::. ##:",
        " :..... ##: ##...:::: ##::::::: ##.. ##::: ##...::::::: ##:::: #########: ##.. ##:::: ##:: #########:",
        " '##::: ##: ##::::::: ##::: ##: ##::. ##:: ##:::::::::: ##:::: ##.... ##: ##::. ##::: ##:: ##.... ##:",
        " . ######:: ########:. ######:: ##:::. ##: ########:::: ##:::: ##:::: ##: ##:::. ##:'####: ##:::: ##:",
        " :......:::........:::......:::..:::::..::........:::::..:::::..:::::..::..:::::..::....::..:::::..::"
    };
   
    int line_colors[] = {3, 4, 4, 5, 3, 7, 4, 5};
    int bold_lines[] = {1, 0, 0, 1, 1, 1, 0, 0};
   
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
}

/* Exibe e controla o menu principal da Secretaria com navegação por setas */
void menuSecretaria() {
    const char *opcoes[] = {
        "Criar Matrícula",
        "Pesquisar por Matrícula",
        "Pesquisar por Documento",
        "Editar Dados do Aluno",
        "Sair"
    };
    int num_opcoes = 5;
    int selecionado = 0;
    int tecla;
    while (1) {
        limparTela();
        linha();
        print_banner_sec();
        linha();
        printw_utf8("=== Sistema de Matrículas - Secretaria ===\n");
        linha();
       
        for (int i = 0; i < num_opcoes; i++) {
            if (i == selecionado) {
                attron(COLOR_PAIR(1));
                printw("> ");
                printw_utf8(opcoes[i]);
                printw("\n");
                attroff(COLOR_PAIR(1));
            } else {
                printw("  ");
                printw_utf8(opcoes[i]);
                printw("\n");
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
                switch (selecionado) {
                    case 0: // Criar Matrícula
                        if (pythonCriarMatricula()) {
                            mostrarMensagem("Matrícula criada com sucesso!", 1);
                        } else {
                            mostrarMensagem("Erro ao criar matrícula!", 2);
                        }
                        aguardarEnter();
                        break;
                    case 1: // Pesquisar por Matrícula
                        pythonBuscarPorMatricula();
                        aguardarEnter();
                        break;
                    case 2: // Pesquisar por Documento
                        pythonBuscarPorDocumento();
                        aguardarEnter();
                        break;
                    case 3: // Editar Dados
                        if (pythonEditarMatricula()) {
                            mostrarMensagem("Dados atualizados com sucesso!", 1);
                        } else {
                            mostrarMensagem("Erro ao editar matrícula!", 2);
                        }
                        aguardarEnter();
                        break;
                    case 4: // Sair
                        limparTela();
                        linha();
                        mostrarMensagem("Saindo do menu secretaria...", 2);
                        linha();
                        refresh();
                        napms(1000);
                        return;
                }
                break;
        }
    }
}

/* Chama a função Python criar_matricula() e retorna sucesso (1) ou falha (0) */
int pythonCriarMatricula() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    PyObject *pModule = PyImport_ImportModule("cadastrosecretaria");
    if (!pModule) { PyErr_Print(); initscr(); return 0; }
    PyObject *pFunc = PyObject_GetAttrString(pModule, "criar_matricula");
    if (!pFunc || !PyCallable_Check(pFunc)) { Py_DECREF(pModule); initscr(); return 0; }
    PyObject *pValue = PyObject_CallObject(pFunc, NULL);
    int success = 0;
    if (pValue) {
        PyObject *pSuccess = PyTuple_GetItem(pValue, 0);
        success = PyObject_IsTrue(pSuccess);
        Py_DECREF(pValue);
    }
    Py_DECREF(pFunc);
    Py_DECREF(pModule);
   
    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
   
    return success;
}

/* Chama a função Python editar_matricula() e retorna sucesso (1) ou falha (0) */
int pythonEditarMatricula() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    PyObject *pModule = PyImport_ImportModule("cadastrosecretaria");
    if (!pModule) { PyErr_Print(); initscr(); return 0; }
    PyObject *pFunc = PyObject_GetAttrString(pModule, "editar_matricula");
    if (!pFunc || !PyCallable_Check(pFunc)) { Py_DECREF(pModule); initscr(); return 0; }
    PyObject *pValue = PyObject_CallObject(pFunc, NULL);
    int success = pValue ? PyObject_IsTrue(pValue) : 0;
    Py_XDECREF(pValue);
    Py_DECREF(pFunc);
    Py_DECREF(pModule);
   
    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
   
    return success;
}

/* Chama a função Python buscar_aluno_por_matricula() e exibe o resultado */
int pythonBuscarPorMatricula() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    PyObject *pModule = PyImport_ImportModule("cadastrosecretaria");
    if (!pModule) { PyErr_Print(); initscr(); return 0; }
    PyObject *pFunc = PyObject_GetAttrString(pModule, "buscar_aluno_por_matricula");
    if (!pFunc || !PyCallable_Check(pFunc)) { Py_DECREF(pModule); initscr(); return 0; }
    PyObject *pValue = PyObject_CallObject(pFunc, NULL);
    int success = 0;
    if (pValue) {
        PyObject *pSuccess = PyTuple_GetItem(pValue, 0);
        success = PyObject_IsTrue(pSuccess);
        Py_DECREF(pValue);
    }
    Py_DECREF(pFunc);
    Py_DECREF(pModule);
   
    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
   
    return success;
}

/* Chama a função Python buscar_aluno_por_documento() e exibe o resultado */
int pythonBuscarPorDocumento() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    PyObject *pModule = PyImport_ImportModule("cadastrosecretaria");
    if (!pModule) { PyErr_Print(); initscr(); return 0; }
    PyObject *pFunc = PyObject_GetAttrString(pModule, "buscar_aluno_por_documento");
    if (!pFunc || !PyCallable_Check(pFunc)) { Py_DECREF(pModule); initscr(); return 0; }
    PyObject *pValue = PyObject_CallObject(pFunc, NULL);
    int success = 0;
    if (pValue) {
        PyObject *pSuccess = PyTuple_GetItem(pValue, 0);
        success = PyObject_IsTrue(pSuccess);
        Py_DECREF(pValue);
    }
    Py_DECREF(pFunc);
    Py_DECREF(pModule);
   
    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
   
    return success;
}