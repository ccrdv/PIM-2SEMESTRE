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

// Função para imprimir texto UTF-8 no PDCurses
void printw_utf8();

// Funções principais (declaradas no main2.c)
void limparTela();
void linha();

void mostrarMensagem(const char* mensagem, int cor) {
    attron(COLOR_PAIR(cor));
    printw_utf8(mensagem);
    printw("\n");
    attroff(COLOR_PAIR(cor));
    refresh();
}

void aguardarEnter() {
    printw_utf8("\nPressione Enter para continuar...");
    refresh();
    noecho();
    getch();
    echo();
}

// Menu Administrador
void menuAdmin() {
    const char *opcoes[] = {
        "Cadastrar Usuário", 
        "Remover Usuário", 
        "Criar Turma",
        "Gerenciar Turmas",
        "Gerenciar Matrículas", 
        "Sair"
    };
    int num_opcoes = 6;
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
	    printw(":::'###::::'########::'##::::'##:'####:'##::: ##::::::\n");
	    attroff(COLOR_PAIR(3) | A_BOLD);
	
	    attron(COLOR_PAIR(4));
	    printw("::'## ##::: ##.... ##: ###::'###:. ##:: ###:: ##::::::\n");
	    printw(":'##:. ##:: ##:::: ##: ####'####:: ##:: ####: ##::::::\n");
	    attroff(COLOR_PAIR(4));
	
	    attron(COLOR_PAIR(5) | A_BOLD);
	    printw("'##:::. ##: ##:::: ##: ## ### ##:: ##:: ## ## ##::::::\n");
	    attroff(COLOR_PAIR(5) | A_BOLD);
	
	    attron(COLOR_PAIR(3) | A_BOLD);
	    printw(" #########: ##:::: ##: ##. #: ##:: ##:: ##. ####::::::\n");
	    attroff(COLOR_PAIR(3) | A_BOLD);
	
	    attron(COLOR_PAIR(7) | A_BOLD);
	    printw(" ##.... ##: ##:::: ##: ##:.:: ##:: ##:: ##:. ###:'###:\n");
	    attroff(COLOR_PAIR(7) | A_BOLD);
	
	    attron(COLOR_PAIR(4));
	    printw(" ##:::: ##: ########:: ##:::: ##:'####: ##::. ##: ###:\n");
	    attroff(COLOR_PAIR(4));
	
	    attron(COLOR_PAIR(5));
	    printw("..:::::..::........:::..:::::..::....::..::::..::...::\n");
	    attroff(COLOR_PAIR(5));
	
	    printw("\n");
	    linha();
        printw_utf8("=== Sistema de Gerenciamento - Administrador ===");
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
                    case 0: // Cadastrar Usuário
                        if (pythonRegister()) {
                            mostrarMensagem("Usuário cadastrado com sucesso!", 1);
                        } else {
                            mostrarMensagem("Erro ao cadastrar usuário ou operação cancelada!", 2);
                        }
                        aguardarEnter();
                        break;
                    case 1: // Remover Usuário
                        menuRemoverUsuario();
                        break;
                    case 2:
                    	if (pythonCriarTurma()) {
                            mostrarMensagem("Turma criada com sucesso!", 1);
                        } else {
                            mostrarMensagem("Erro ao criar turma ou operação cancelada!", 2);
                        }
                        aguardarEnter();
                        break;
                    case 3:
                        if (pythonGerenciarTurmas()) {
                            mostrarMensagem("Alterações nas turmas salvas com sucesso!", 1);
                        } else {
                            mostrarMensagem("Operação cancelada ou sem alterações!", 2);
                        }
                        aguardarEnter();
                        break;
					case 4: // Gerenciar Matrículas
                        menuGerenciarMatriculas();
                        break;
                    case 5: // Sair
                        limparTela();
                        linha();
                        mostrarMensagem("Saindo do menu administrador...", 2);
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

int pythonCriarTurma() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    PyObject *pModule, *pFunc, *pValue;
    
    pModule = PyImport_ImportModule("ADM");
    if (pModule == NULL) {
        PyErr_Print();
        initscr();
        return 0;
    }

    pFunc = PyObject_GetAttrString(pModule, "criar_turma");
    if (pFunc == NULL || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        initscr();
        return 0;
    }

    // Chama a função criar_turma do Python
    pValue = PyObject_CallObject(pFunc, NULL);

    int success = 0;
    if (pValue != NULL) {
        success = PyObject_IsTrue(pValue);
        Py_DECREF(pValue);
        
        // Feedback imediato no console
        if (success) {
            printf("\n\033[32mTurma criada com sucesso!\033[m\n");
        } else {
            printf("\n\033[33mOperação cancelada ou falhou!\033[m\n");
        }
        fflush(stdout);
        
    } else {
        PyErr_Print();
        printf("\n\033[31mErro durante a criação da turma!\033[m\n");
        fflush(stdout);
    }

    Py_DECREF(pFunc);
    Py_DECREF(pModule);
    
    // Aguarda um pouco para o usuário ver a mensagem
    Sleep(2000);
    
    // Restaura a tela do curses
    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    
    return success;
}

int pythonGerenciarTurmas() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    PyObject *pModule, *pFunc, *pValue;
    
    pModule = PyImport_ImportModule("ADM");
    if (pModule == NULL) {
        PyErr_Print();
        initscr();
        return 0;
    }

    pFunc = PyObject_GetAttrString(pModule, "gerenciar_turmas");
    if (pFunc == NULL || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        initscr();
        return 0;
    }

    // Chama a função gerenciar_turmas do Python
    pValue = PyObject_CallObject(pFunc, NULL);

    int success = 0;
    if (pValue != NULL) {
        success = PyObject_IsTrue(pValue);
        Py_DECREF(pValue);
        
        // Feedback imediato no console
        if (success) {
            printf("\n\033[32mAlterações salvas com sucesso!\033[m\n");
        } else {
            printf("\n\033[33mOperação cancelada ou sem alterações!\033[m\n");
        }
        fflush(stdout);
        
    } else {
        PyErr_Print();
        printf("\n\033[31mErro durante o gerenciamento das turmas!\033[m\n");
        fflush(stdout);
    }

    Py_DECREF(pFunc);
    Py_DECREF(pModule);
    
    // Aguarda um pouco para o usuário ver a mensagem
    Sleep(2000);
    
    // Restaura a tela do curses
    initscr();
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    
    return success;
}

// Menu Remover Usuário
void menuRemoverUsuario() {
    limparTela();
    linha();
    printw_utf8("=== Remover Usuário ===");
    printw("\n");
    linha();
    
    printw_utf8("Digite o CPF do usuário (Somente Números): ");
    echo();
    
    char documento[MAX_INPUT];
    getnstr(documento, MAX_INPUT - 1);
    noecho();
    linha();
    
    if (pythonRemoverUsuario(documento)) {
        mostrarMensagem("Usuário removido com sucesso!", 1);
    } else {
        mostrarMensagem("CPF não encontrado!", 2);
    }
    aguardarEnter();
}

// Menu Gerenciar Matrículas (Admin)
void menuGerenciarMatriculas() {
    menuSecretaria(); // Reutiliza o mesmo menu da secretária
}

// Wrapper para função Python de cadastro
// Wrapper para função Python de cadastro
int pythonRegister() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    PyObject *pModule, *pFunc, *pValue;
    
    pModule = PyImport_ImportModule("ADM");
    if (pModule == NULL) {
        PyErr_Print();
        initscr();
        return 0;
    }

    pFunc = PyObject_GetAttrString(pModule, "register");
    if (pFunc == NULL || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        initscr();
        return 0;
    }

    // Chama a função sem argumentos
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
// Wrapper para função Python de remover usuário
int pythonRemoverUsuario(const char* documento) {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    PyObject *pModule, *pFunc, *pArgs, *pValue;
    
    pModule = PyImport_ImportModule("ADM");
    if (pModule == NULL) {
        PyErr_Print();
        initscr();
        return 0;
    }

    pFunc = PyObject_GetAttrString(pModule, "remover_usuario_por_documento");
    if (pFunc == NULL || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        Py_DECREF(pModule);
        initscr();
        return 0;
    }

    pArgs = PyTuple_Pack(1, PyUnicode_FromString(documento));
    pValue = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);

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