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

/* Exibe uma mensagem colorida (verde = 1, vermelho = 2) */
void mostrarMensagem(const char* mensagem, int cor) {
    attron(COLOR_PAIR(cor));
    printw_utf8(mensagem);
    printw("\n");
    attroff(COLOR_PAIR(cor));
    refresh();
}

/* Pausa a execução até o usuário pressionar Enter */
void aguardarEnter() {
    printw_utf8("\nPressione Enter para continuar...");
    refresh();
    noecho();
    getch();
    echo();
}

/* Exibe o banner colorido específico do menu Administrador */
void print_banner_adm() {
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_pair(6, COLOR_WHITE, COLOR_BLACK);
    init_pair(7, COLOR_BLUE, COLOR_BLACK);
    char *banner_lines[] = {
        ":::'###::::'########::'##::::'##:'####:'##::: ##::::::",
        "::'## ##::: ##.... ##: ###::'###:. ##:: ###:: ##::::::",
        ":'##:. ##:: ##:::: ##: ####'####:: ##:: ####: ##::::::",
        "'##:::. ##: ##:::: ##: ## ### ##:: ##:: ## ## ##::::::",
        " #########: ##:::: ##: ##. #: ##:: ##:: ##. ####::::::",
        " ##.... ##: ##:::: ##: ##:.:: ##:: ##:: ##:. ###:'###:",
        " ##:::: ##: ########:: ##:::: ##:'####: ##::. ##: ###:",
        "..:::::..::........:::..:::::..::....::..::::..::...::"
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

/* Exibe e controla o menu principal do Administrador com navegação por setas */
void menuAdmin() {
    const char *opcoes[] = {
        "Cadastrar Usuário",
        "Remover Usuário",
        "Criar Turma",
        "Gerenciar Turmas",
        "Gerenciar Matrículas",
        "Relatórios",
        "Sair"
    };
    int num_opcoes = 7;
    int selecionado = 0;
    int tecla;
    while (1) {
        limparTela();
        linha();
        print_banner_adm();
        printw("\n");
        linha();
        printw_utf8("=== Sistema de Gerenciamento - Administrador ===\n");
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
            case 10:
                switch (selecionado) {
                    case 0:
                        if (pythonRegister()) {
                            mostrarMensagem("Usuário cadastrado com sucesso!", 1);
                        } else {
                            mostrarMensagem("Erro ao cadastrar usuário ou operação cancelada!", 2);
                        }
                        aguardarEnter();
                        break;
                    case 1:
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
                    case 4:
                        menuGerenciarMatriculas();
                        break;
                    case 5: 
					    menuRelatorios();
					    break;
                    case 6:
                        limparTela();
                        linha();
                        mostrarMensagem("Saindo do menu administrador...", 2);
                        linha();
                        refresh();
                        napms(1000);
                        return;
                }
                break;
        }
    }
}

/* Chama a função Python criar_turma() e retorna sucesso (1) ou falha (0) */
int pythonCriarTurma() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    PyObject *pModule = PyImport_ImportModule("ADM");
    if (!pModule) { PyErr_Print(); initscr(); return 0; }
    PyObject *pFunc = PyObject_GetAttrString(pModule, "criar_turma");
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
void menuRelatorios(){
	const char *opcoes[] = {
        "Boletim por Turma",
        "Frequência por Turma",
        "Relatório Geral do Sistema",
        "Voltar ao Menu Principal"
    };
    int num_opcoes = 4;
    int selecionado = 0;
    int tecla;

    while (1) {
        limparTela();
        linha();
        print_banner_adm();  // mesmo banner do menu principal
        printw("\n");
        linha();
        printw_utf8("=== Relatórios Disponíveis ===\n");
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
            case 10:  // Enter
            case 13:
                if (selecionado == 3) {  // Voltar
                    return;  // sai do submenu e volta pro menuAdmin()
                }

                // === AQUI RODA O RELATÓRIO (sem quebrar o curses) ===
                endwin();  // só agora sai do curses

                if (selecionado == 0 || selecionado == 1) {
                    int turma_id = 0;
                    char input[20] = {0};

                    printf("\n\n");
                    printf("   DIGITE O ID DA TURMA: ");
                    fflush(stdout);
                    if (scanf("%19s", input) == 1) {
                        turma_id = atoi(input);
                    }
                    while(getchar() != '\n'); // limpa buffer

                    char cmd[600];
                    if (selecionado == 0) {
                        snprintf(cmd, sizeof(cmd),
                            "import relatorios, os\n"
                            "try:\n"
                            "    relatorios.boletim_turma(%d)\n"
                            "except Exception as e:\n"
                            "    print('Erro ao gerar boletim:', e)\n"
                            "input('\\n   BOLETIM GERADO!\\n   Aberto automaticamente.\\n   Enter para voltar...')\n", turma_id);
                    } else {
                        snprintf(cmd, sizeof(cmd),
                            "import relatorios, os\n"
                            "try:\n"
                            "    relatorios.frequencia_turma(%d)\n"
                            "except Exception as e:\n"
                            "    print('Erro ao gerar frequência:', e)\n"
                            "input('\\n   FREQUÊNCIA GERADA!\\n   Aberto automaticamente.\\n   Enter para voltar...')\n", turma_id);
                    }
                    PyRun_SimpleString(cmd);

                } else if (selecionado == 2) {
                    PyRun_SimpleString(
                        "import relatorios\n"
                        "try:\n"
                        "    relatorios.relatorio_geral()\n"
                        "except Exception as e:\n"
                        "    print('Erro ao gerar relatório geral:', e)\n"
                        "input('\\n   RELATÓRIO GERAL GERADO!\\n   Aberto automaticamente.\\n   Enter para voltar...')\n"
                    );
                }

                // VOLTA PRO CURSES EXATAMENTE COMO NO SEU SISTEMA
                initscr();
                keypad(stdscr, TRUE);
                cbreak();
                noecho();
                start_color();
                init_pair(1, COLOR_GREEN, COLOR_BLACK);
                init_pair(2, COLOR_RED, COLOR_BLACK);
                init_pair(7, COLOR_CYAN, COLOR_BLACK);
                clear();
                refresh();
                break;

            default:
                break;
        }
    }
}
/* Chama a função Python gerenciar_turmas() e retorna sucesso (1) ou cancelamento (0) */
int pythonGerenciarTurmas() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    PyObject *pModule = PyImport_ImportModule("ADM");
    if (!pModule) { PyErr_Print(); initscr(); return 0; }
    PyObject *pFunc = PyObject_GetAttrString(pModule, "gerenciar_turmas");
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

/* Solicita o CPF e remove o usuário via função Python */
void menuRemoverUsuario() {
    limparTela();
    linha();
    printw_utf8("=== Remover Usuário ===\n");
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

/* Abre o menu de gerenciamento de matrículas (reutiliza o menu da secretaria) */
void menuGerenciarMatriculas() {
    menuSecretaria();
}

/* Chama a função Python register() para cadastrar novo usuário */
int pythonRegister() {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    PyObject *pModule = PyImport_ImportModule("ADM");
    if (!pModule) { PyErr_Print(); initscr(); return 0; }
    PyObject *pFunc = PyObject_GetAttrString(pModule, "register");
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

/* Remove usuário pelo CPF usando a função Python remover_usuario_por_documento() */
int pythonRemoverUsuario(const char* documento) {
    endwin();
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    PyObject *pModule = PyImport_ImportModule("ADM");
    if (!pModule) { PyErr_Print(); initscr(); return 0; }
    PyObject *pFunc = PyObject_GetAttrString(pModule, "remover_usuario_por_documento");
    if (!pFunc || !PyCallable_Check(pFunc)) { Py_DECREF(pModule); initscr(); return 0; }
    PyObject *pArgs = PyTuple_Pack(1, PyUnicode_FromString(documento));
    PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);
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