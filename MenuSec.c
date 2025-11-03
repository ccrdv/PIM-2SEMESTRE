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

// Menu Secretária
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
        init_pair(3, COLOR_CYAN,     COLOR_BLACK);
	    init_pair(4, COLOR_YELLOW,   COLOR_BLACK);
	    init_pair(5, COLOR_MAGENTA,  COLOR_BLACK);
	    init_pair(6, COLOR_WHITE,    COLOR_BLACK);
	    init_pair(7, COLOR_BLUE,     COLOR_BLACK); 
	

	    attron(COLOR_PAIR(3) | A_BOLD);
	    printw(" :'######::'########::'######::'########::'########:'########::::'###::::'########::'####::::'###::::\n");
	    attroff(COLOR_PAIR(3) | A_BOLD);
	
	    attron(COLOR_PAIR(4));
	    printw(" '##... ##: ##.....::'##... ##: ##.... ##: ##.....::... ##..::::'## ##::: ##.... ##:. ##::::'## ##:::\n");
	    printw(" '##:::..:: ##::::::: ##:::..:: ##:::: ##: ##:::::::::: ##:::::'##:. ##:: ##:::: ##:: ##:::'##:. ##::\n");
	    attroff(COLOR_PAIR(4));
	
	    attron(COLOR_PAIR(5) | A_BOLD);
	    printw(" . ######:: ######::: ##::::::: ########:: ######:::::: ##::::'##:::. ##: ########::: ##::'##:::. ##:\n");
	    attroff(COLOR_PAIR(5) | A_BOLD);
	
	    attron(COLOR_PAIR(3) | A_BOLD);
	    printw(" :..... ##: ##...:::: ##::::::: ##.. ##::: ##...::::::: ##:::: #########: ##.. ##:::: ##:: #########:\n");
	    attroff(COLOR_PAIR(3) | A_BOLD);
	
	    attron(COLOR_PAIR(7) | A_BOLD);
	    printw(" '##::: ##: ##::::::: ##::: ##: ##::. ##:: ##:::::::::: ##:::: ##.... ##: ##::. ##::: ##:: ##.... ##:\n");
	    attroff(COLOR_PAIR(7) | A_BOLD);
	
	    attron(COLOR_PAIR(4));
	    printw(" . ######:: ########:. ######:: ##:::. ##: ########:::: ##:::: ##:::: ##: ##:::. ##:'####: ##:::: ##:\n");
	    attroff(COLOR_PAIR(4));
	
	    attron(COLOR_PAIR(5));
	    printw(" :......:::........:::......:::..:::::..::........:::::..:::::..:::::..::..:::::..::....::..:::::..::\n");
	    attroff(COLOR_PAIR(5));
		linha();
	    printw("\n");
        printw_utf8("=== Sistema de Matrículas - Secretaria ===");
        printw("\n");
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
                        menuCadastroMatricula();
                        break;
                    case 1: // Pesquisar por Matrícula
                        menuBuscarMatricula();
                        break;
                    case 2: // Pesquisar por Documento
                        menuBuscarDocumento();
                        break;
                    case 3: // Editar Dados
                        menuEditarMatricula();
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
            default:
                break;
        }
    }
}

// Menu Cadastro de Matrícula
void menuCadastroMatricula() {
    limparTela();
    linha();
    printw_utf8("=== Criar Matrícula ===\n");
    linha();
    
    if (pythonCriarMatricula()) {
        mostrarMensagem("Matrícula criada com sucesso!", 1);
    } else {
        mostrarMensagem("Erro ao criar matrícula!", 2);
    }
    aguardarEnter();
}

// Menu Buscar por Matrícula
void menuBuscarMatricula() {
    limparTela();
    linha();
    printw_utf8("=== Buscar Aluno por Matrícula ===\n");
    linha();
    
    if (pythonBuscarPorMatricula()) {
        // Sucesso - a mensagem é mostrada pela função Python
    } else {
        mostrarMensagem("Matrícula não encontrada!", 2);
    }
    aguardarEnter();
}

// Menu Buscar por Documento
void menuBuscarDocumento() {
    limparTela();
    linha();
    printw_utf8("=== Buscar Aluno por Documento ===\n");
    linha();
    
    if (pythonBuscarPorDocumento()) {
        // Sucesso - a mensagem é mostrada pela função Python
    } else {
        mostrarMensagem("Documento não encontrado!", 2);
    }
    aguardarEnter();
}

// Menu Editar Matrícula
void menuEditarMatricula() {
    limparTela();
    linha();
    printw_utf8("=== Editar Matrícula ===\n");
    linha();
    
    if (pythonEditarMatricula()) {
        mostrarMensagem("Dados atualizados com sucesso!", 1);
    } else {
        mostrarMensagem("Erro ao editar matrícula!", 2);
    }
    aguardarEnter();
}

// Wrappers para funções de matrícula
int pythonCriarMatricula() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    PyObject *pModule, *pFunc, *pValue;
    
    pModule = PyImport_ImportModule("cadastrosecretaria");
    if (pModule == NULL) {
        PyErr_Print();
        initscr();
        return 0;
    }

    pFunc = PyObject_GetAttrString(pModule, "criar_matricula");
    if (pFunc == NULL || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        initscr();
        return 0;
    }

    pValue = PyObject_CallObject(pFunc, NULL);

    int success = 0;
    if (pValue != NULL) {
        PyObject *pSuccess = PyTuple_GetItem(pValue, 0);
        success = PyObject_IsTrue(pSuccess);
        Py_DECREF(pValue);
    } else {
        PyErr_Print();
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

int pythonEditarMatricula() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    PyObject *pModule, *pFunc, *pValue;
    
    pModule = PyImport_ImportModule("cadastrosecretaria");
    if (pModule == NULL) {
        PyErr_Print();
        initscr();
        return 0;
    }

    pFunc = PyObject_GetAttrString(pModule, "editar_matricula");
    if (pFunc == NULL || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        initscr();
        return 0;
    }

    pValue = PyObject_CallObject(pFunc, NULL);

    int success = 0;
    if (pValue != NULL) {
        success = PyObject_IsTrue(pValue);
        Py_DECREF(pValue);
    } else {
        PyErr_Print();
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

int pythonBuscarPorMatricula() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    PyObject *pModule, *pFunc, *pValue;
    
    pModule = PyImport_ImportModule("cadastrosecretaria");
    if (pModule == NULL) {
        PyErr_Print();
        initscr();
        return 0;
    }

    pFunc = PyObject_GetAttrString(pModule, "buscar_aluno_por_matricula");
    if (pFunc == NULL || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        initscr();
        return 0;
    }

    pValue = PyObject_CallObject(pFunc, NULL);

    int success = 0;
    if (pValue != NULL) {
        PyObject *pSuccess = PyTuple_GetItem(pValue, 0);
        success = PyObject_IsTrue(pSuccess);
        Py_DECREF(pValue);
    } else {
        PyErr_Print();
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

int pythonBuscarPorDocumento() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    PyObject *pModule, *pFunc, *pValue;
    
    pModule = PyImport_ImportModule("cadastrosecretaria");
    if (pModule == NULL) {
        PyErr_Print();
        initscr();
        return 0;
    }

    pFunc = PyObject_GetAttrString(pModule, "buscar_aluno_por_documento");
    if (pFunc == NULL || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        initscr();
        return 0;
    }

    pValue = PyObject_CallObject(pFunc, NULL);

    int success = 0;
    if (pValue != NULL) {
        PyObject *pSuccess = PyTuple_GetItem(pValue, 0);
        success = PyObject_IsTrue(pSuccess);
        Py_DECREF(pValue);
    } else {
        PyErr_Print();
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