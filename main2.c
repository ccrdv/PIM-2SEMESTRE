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

// === PROTÓTIPOS ===
void menuPrincipal();
void limparTela();
void linha();
void garantir_arquivos_json();
int rodarPythonLogin();
wchar_t* get_exe_dir();
wchar_t* wcs_concat(const wchar_t* a, const wchar_t* b);
int init_python_portable();
void force_python_path();

// === FUNÇÃO: Imprime UTF-8 no PDCurses ===
void printw_utf8(const char* str) {
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (wide_len <= 0) { printw("%s", str); return; }

    wchar_t* wide_str = (wchar_t*)malloc(wide_len * sizeof(wchar_t));
    if (!wide_str) { printw("%s", str); return; }
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wide_str, wide_len);

    int ansi_len = WideCharToMultiByte(CP_ACP, 0, wide_str, -1, NULL, 0, NULL, NULL);
    if (ansi_len <= 0) { free(wide_str); printw("%s", str); return; }

    char* ansi_str = (char*)malloc(ansi_len);
    if (!ansi_str) { free(wide_str); printw("%s", str); return; }
    WideCharToMultiByte(CP_ACP, 0, wide_str, -1, ansi_str, ansi_len, NULL, NULL);

    printw("%s", ansi_str);
    free(wide_str);
    free(ansi_str);
}

void garantir_arquivos_json() {
    PyRun_SimpleString(
        "import os\n"
        "import json\n"
        "import shutil\n"
        "\n"
        "BASE_DIR = os.path.dirname(os.path.abspath(__file__))\n"
        "PYTHON_EMBEDDED_DIR = os.path.join(BASE_DIR, 'python_embedded')\n"
        "\n"
        "# DEBUG\n"
        "print(f'DEBUG garantir_arquivos_json:')\n"
        "print(f'  BASE_DIR: {BASE_DIR}')\n"
        "print(f'  PYTHON_EMBEDDED_DIR: {PYTHON_EMBEDDED_DIR}')\n"
        "print(f'  Python embedded existe: {os.path.exists(PYTHON_EMBEDDED_DIR)}')\n"
        "\n"
        "# Garante que a pasta python_embedded existe\n"
        "os.makedirs(PYTHON_EMBEDDED_DIR, exist_ok=True)\n"
        "\n"
        "# Lista de arquivos JSON necessários\n"
        "arquivos_json = ['users.json', 'turmas.json', 'alunos.json', 'matriculas.json', 'atividades.json', 'anuncios.json']\n"
        "\n"
        "for arquivo in arquivos_json:\n"
        "    caminho_original = os.path.join(BASE_DIR, arquivo)\n"
        "    caminho_destino = os.path.join(PYTHON_EMBEDDED_DIR, arquivo)\n"
        "    \n"
        "    print(f'\\\\nProcessando {arquivo}:')\n"
        "    print(f'  Origem: {caminho_original}')\n"
        "    print(f'  Destino: {caminho_destino}')\n"
        "    \n"
        "    # Se existe no diretório principal E não existe no destino, copia\n"
        "    if os.path.exists(caminho_original) and not os.path.exists(caminho_destino):\n"
        "        try:\n"
        "            shutil.copy2(caminho_original, caminho_destino)\n"
        "            print(f'  ✓ Copiado de {caminho_original}')\n"
        "        except Exception as e:\n"
        "            print(f'  ✗ Erro ao copiar: {e}')\n"
        "    # Se já existe no destino\n"
        "    elif os.path.exists(caminho_destino):\n"
        "        print(f'  ✓ Já existe em python_embedded')\n"
        "    # Se não existe em nenhum lugar, cria vazio\n"
        "    else:\n"
        "        try:\n"
        "            if arquivo == 'users.json':\n"
        "                with open(caminho_destino, 'w', encoding='utf-8') as f:\n"
        "                    json.dump({'users': []}, f, indent=4, ensure_ascii=False)\n"
        "            elif arquivo == 'turmas.json':\n"
        "                with open(caminho_destino, 'w', encoding='utf-8') as f:\n"
        "                    json.dump({'turmas': []}, f, indent=4, ensure_ascii=False)\n"
        "            elif arquivo == 'alunos.json':\n"
        "                with open(caminho_destino, 'w', encoding='utf-8') as f:\n"
        "                    json.dump({}, f, indent=4, ensure_ascii=False)\n"
        "            elif arquivo == 'matriculas.json':\n"
        "                with open(caminho_destino, 'w', encoding='utf-8') as f:\n"
        "                    json.dump({}, f, indent=4, ensure_ascii=False)\n"
        "            elif arquivo == 'atividades.json':\n"
        "                with open(caminho_destino, 'w', encoding='utf-8') as f:\n"
        "                    json.dump({'atividades': []}, f, indent=4, ensure_ascii=False)\n"
        "            elif arquivo == 'anuncios.json':\n"
        "                with open(caminho_destino, 'w', encoding='utf-8') as f:\n"
        "                    json.dump({'anuncios': []}, f, indent=4, ensure_ascii=False)\n"
        "            print(f'  ✓ Criado vazio em python_embedded')\n"
        "        except Exception as e:\n"
        "            print(f'  ✗ Erro ao criar: {e}')\n"
        "\n"
        "print('\\\\n=== VERIFICAÇÃO FINAL ===')\n"
        "for arquivo in arquivos_json:\n"
        "    caminho_destino = os.path.join(PYTHON_EMBEDDED_DIR, arquivo)\n"
        "    if os.path.exists(caminho_destino):\n"
        "        print(f'✓ {arquivo} - OK')\n"
        "    else:\n"
        "        print(f'✗ {arquivo} - FALTA')\n"
    );
}

// === FUNÇÃO: Limpa tela e atualiza ===
void limparTela() {
    clear();
    refresh();
}

// === FUNÇÃO: Desenha linha horizontal ===
void linha() {
    for (int i = 0; i < COLS; i++) printw("=");
    refresh();
}

// === FUNÇÃO: Obtém diretório do .exe ===
wchar_t* get_exe_dir() {
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) return NULL;
    wchar_t* last = wcsrchr(path, L'\\');
    if (last) *last = L'\0';
    else return NULL;
    return _wcsdup(path);
}

// === FUNÇÃO: Concatena wide strings ===
wchar_t* wcs_concat(const wchar_t* a, const wchar_t* b) {
    size_t la = wcslen(a), lb = wcslen(b);
    wchar_t* r = (wchar_t*)malloc((la + lb + 1) * sizeof(wchar_t));
    if (r) { wcscpy(r, a); wcscat(r, b); }
    return r;
}

// === FUNÇÃO: Força python_embedded no sys.path ===
void force_python_path() {
    PyRun_SimpleString(
        "import sys\n"
        "import os\n"
        "exe_dir = os.path.dirname(sys.executable)\n"
        "embedded = os.path.join(exe_dir, 'python_embedded')\n"
        "if embedded not in sys.path:\n"
        "    sys.path.insert(0, embedded)\n"
    );
}

// === FUNÇÃO: Inicializa Python portavelmente ===
int init_python_portable() {
    wchar_t* exe_dir = get_exe_dir();
    if (!exe_dir) {
        fwprintf(stderr, L"Erro: não foi possível obter diretório.\n");
        return 0;
    }

    wchar_t* python_embedded = wcs_concat(exe_dir, L"\\python_embedded");
    wchar_t* lib_path = wcs_concat(python_embedded, L"\\Lib");
    wchar_t* dlls_path = wcs_concat(python_embedded, L"\\DLLs");

    if (!python_embedded || !lib_path || !dlls_path) {
        free(exe_dir); free(python_embedded); free(lib_path); free(dlls_path);
        return 0;
    }

    if (_waccess(python_embedded, 0) != 0) {
        fwprintf(stderr, L"Pasta 'python_embedded' não encontrada: %ls\n", python_embedded);
        free(exe_dir); free(python_embedded); free(lib_path); free(dlls_path);
        return 0;
    }

    PyStatus status;
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);

    status = PyConfig_SetString(&config, &config.home, python_embedded);
    if (PyStatus_Exception(status)) goto py_err;

    status = PyWideStringList_Append(&config.module_search_paths, python_embedded);
    if (PyStatus_Exception(status)) goto py_err;
    status = PyWideStringList_Append(&config.module_search_paths, lib_path);
    if (PyStatus_Exception(status)) goto py_err;
    status = PyWideStringList_Append(&config.module_search_paths, dlls_path);
    if (PyStatus_Exception(status)) goto py_err;

    wchar_t* prog_name = wcs_concat(exe_dir, L"\\PIMSeceAdm.exe");
    status = PyConfig_SetString(&config, &config.program_name, prog_name);
    free(prog_name);
    if (PyStatus_Exception(status)) goto py_err;

    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) goto py_err;

    PyConfig_Clear(&config);
    free(exe_dir); free(python_embedded); free(lib_path); free(dlls_path);
    force_python_path();
    return 1;

py_err:
    fwprintf(stderr, L"Erro Python: %ls\n", status.err_msg);
    PyConfig_Clear(&config);
    free(exe_dir); free(python_embedded); free(lib_path); free(dlls_path);
    return 0;
}

// === FUNÇÃO: Login com ADM.py ===
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

// === MENU PRINCIPAL ===
void menuPrincipal() {
    const char *opcoes[] = {"Login", "Sair"};
    int num_opcoes = 2, selecionado = 0, tecla;

    while (1) {
        limparTela(); linha();

        init_pair(1, COLOR_BLUE,    COLOR_BLACK);
        init_pair(2, COLOR_YELLOW,  COLOR_BLACK);
        init_pair(3, COLOR_GREEN,   COLOR_BLACK);
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);

        attron(COLOR_PAIR(1) | A_BOLD);
        printw("'######:::'########::'######::'########::::'###:::::'#######::\n");
        printw("##...##:: '##.....::'##... ##:... ##..::::'## ##:::'##.... ##:\n");
        attroff(COLOR_PAIR(1) | A_BOLD);

        attron(COLOR_PAIR(2));
        printw("##:::..::: ##::::::: ##:::..::::: ##:::::'##:. ##:: ##:::: ##:\n");
        printw("##::'####: ######:::. ######::::: ##::::'##:::. ##: ##:::: ##:\n");
        attroff(COLOR_PAIR(2));

        attron(COLOR_PAIR(1) | A_BOLD);
        printw("##::: ##:: ##...:::::..... ##:::: ##:::: #########: ##:::: ##:\n");
        printw("##::: ##:: ##:::::::'##::: ##:::: ##:::: ##.... ##: ##:::: ##:\n");
        attroff(COLOR_PAIR(1) | A_BOLD);

        attron(COLOR_PAIR(2));
        printw(".######::: ########:. ######::::: ##:::: ##:::: ##:. #######::\n");
        printw(":......::::........:::......::::::..:::::..:::::..:::.......::\n");
        attroff(COLOR_PAIR(2));

        linha();
        attron(COLOR_PAIR(4) | A_BOLD);
        printw("Bem-vindo ao sistema de gerenciamento!\n");
        attroff(COLOR_PAIR(4) | A_BOLD);
        linha();

        for (int i = 0; i < num_opcoes; i++) {
            if (i == selecionado) {
                attron(COLOR_PAIR(3));
                printw("> %s\n", opcoes[i]);
                attroff(COLOR_PAIR(3));
            } else {
                printw("  %s\n", opcoes[i]);
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
            case 10:
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
            default:
                attron(COLOR_PAIR(2));
                printw_utf8("\nTecla inválida. Use setas ou Enter. Tente novamente...\n");
                attroff(COLOR_PAIR(2));
                refresh(); napms(2000);
                break;
        }
    }
}

// === FUNÇÃO PRINCIPAL ===
int main() {
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    if (!init_python_portable()) {
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