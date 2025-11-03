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

void limparTela(void) {
    clear();
    refresh();
}

void linha(void) {
    for (int i = 0; i < COLS; i++) printw("=");
    refresh();
}


char* nome_professor_logado = NULL;
char* aluno_matricula_logado = NULL;
char* ROOT_PATH = NULL;


void menuPrincipal(void);
void menuAluno(void);
void menuProfessor(void);
void menuLogado(void);
void garantir_arquivos_json(void);
int rodarPythonRegistrar(void);
int rodarPythonLogin(void);
int rodarPythonLoginProfessor(void);
wchar_t* get_exe_dir(void);
wchar_t* wcs_concat(const wchar_t* a, const wchar_t* b);
int init_python_portable(void);
void debug_python_files(void);
static int load_config(void);
PyObject* safe_import_module(const char* module_name);


static int load_config(void) {
    FILE* fp = fopen("config.txt", "r");
    if (!fp) {
        printf("AVISO: config.txt nao encontrado – usando pasta local \"python_embedded\"\n");
        
    
        if (_access("Z:\\python_embedded", 0) == 0) {
            ROOT_PATH = _strdup("Z:\\python_embedded");
            printf("CONFIG: Usando Z:\\python_embedded (fallback)\n");
            return 1;
        }
        return 0;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        char* key = strtok(line, "=");
        char* value = strtok(NULL, "\n\r");
        if (!key || !value) continue;
        

        while (*value == ' ' || *value == '\t') value++;
        
        if (strcmp(key, "ROOT_PATH") == 0) {
          
            if (_access(value, 0) == 0) {
                ROOT_PATH = _strdup(value);
                printf("CONFIG: ROOT_PATH = %s (ACESSÍVEL)\n", ROOT_PATH);
                fclose(fp);
                return 1;
            } else {
                printf("AVISO: ROOT_PATH %s não acessível\n", value);
                
               
                if (_access("Z:\\python_embedded", 0) == 0) {
                    ROOT_PATH = _strdup("Z:\\python_embedded");
                    printf("CONFIG: Usando Z:\\python_embedded (fallback)\n");
                    fclose(fp);
                    return 1;
                }
            }
        }
    }
    fclose(fp);
    return 0;
}


wchar_t* get_exe_dir(void) {
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) return NULL;
    
    wchar_t* last = wcsrchr(path, L'\\');
    if (last) *last = L'\0';
    else return NULL;
    
    return _wcsdup(path);
}


wchar_t* wcs_concat(const wchar_t* a, const wchar_t* b) {
    size_t la = wcslen(a), lb = wcslen(b);
    wchar_t* r = (wchar_t*)malloc((la + lb + 1) * sizeof(wchar_t));
    if (r) {
        wcscpy(r, a);
        wcscat(r, b);
    }
    return r;
}

void debug_python_files(void) {
    printf("\n=== DEBUG DETALHADO DOS ARQUIVOS PYTHON ===\n");
    
    PyRun_SimpleString(
        "import os\n"
        "import sys\n"
        "\n"
        "print('1. DIRETÓRIO ATUAL:')\n"
        "print('   ', os.getcwd())\n"
        "\n"
        "print('2. CONTEÚDO DA PASTA ATUAL:')\n"
        "try:\n"
        "    files = os.listdir('.')\n"
        "    py_files = [f for f in files if f.endswith('.py')]\n"
        "    print('   Arquivos .py no diretório atual:')\n"
        "    for f in py_files:\n"
        "        print('     -', f)\n"
        "    if not py_files:\n"
        "        print('     NENHUM arquivo .py encontrado!')\n"
        "except Exception as e:\n"
        "    print('   ERRO:', e)\n"
        "\n"
        "print('3. CONTEÚDO DE python_embedded:')\n"
        "try:\n"
        "    embedded_files = os.listdir('python_embedded')\n"
        "    embedded_py = [f for f in embedded_files if f.endswith('.py')]\n"
        "    print('   Arquivos .py em python_embedded:')\n"
        "    for f in embedded_py:\n"
        "        print('     -', f)\n"
        "    if not embedded_py:\n"
        "        print('     NENHUM arquivo .py em python_embedded!')\n"
        "        print('   Todos os arquivos em python_embedded:')\n"
        "        for f in embedded_files:\n"
        "            print('     -', f)\n"
        "except Exception as e:\n"
        "    print('   ERRO ao acessar python_embedded:', e)\n"
        "\n"
        "print('4. TESTANDO IMPORTAÇÃO DOS MÓDULOS:')\n"
        "modules_to_test = ['registroaluno', 'LoginProfessor', 'ADM', 'cadastrosecretaria']\n"
        "for mod in modules_to_test:\n"
        "    try:\n"
        "        __import__(mod)\n"
        "        print(f'   ✓ {mod} - OK')\n"
        "    except ImportError as e:\n"
        "        print(f'   ✗ {mod} - FALHA: {e}')\n"
        "\n"
        "print('5. CAMINHOS DE BUSCA DO PYTHON:')\n"
        "for path in sys.path:\n"
        "    print('   ', path)\n"
        "\n"
        "print('========================================\\n')\n"
    );
}

int verificar_unidade_z(void) {
    DWORD drives = GetLogicalDrives();
    if (drives & (1 << ('Z' - 'A'))) {
        if (_access("Z:\\python_embedded", 0) == 0) {
            printf("SUCESSO: Z:\\python_embedded está acessível\n");
            return 1;
        } else {
            printf("AVISO: Z: existe mas python_embedded não encontrado\n");
        }
    } else {
        printf("AVISO: Unidade Z: não encontrada\n");
    }
    return 0;
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
        if (_access(python_dll, 0) != 0) {
            printf("ERRO: python311.dll nao encontrado em:\n  - %s\n  - %s\n", base, exe_dir);
            return 0;
        }
    }

    snprintf(python_home, sizeof(python_home), "%s", base);
    if (_access(python_home, 0) != 0) {
        printf("ERRO: Pasta Python nao encontrada: %s\n", python_home);
        return 0;
    }

    HMODULE hPython = LoadLibraryA(python_dll);
    if (!hPython) {
        printf("ERRO: Falha ao carregar %s\n", python_dll);
        return 0;
    }

    PyStatus status;
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);

    wchar_t w_python_home[MAX_PATH];
    int len = MultiByteToWideChar(CP_UTF8, 0, python_home, -1, w_python_home, MAX_PATH);
    if (len == 0) {
        printf("ERRO: Falha ao converter caminho para Unicode\n");
        PyConfig_Clear(&config);
        return 0;
    }

    status = PyConfig_SetString(&config, &config.home, w_python_home);
    if (PyStatus_Exception(status)) {
        printf("ERRO: Falha ao definir Python home: %s\n", status.err_msg);
        PyConfig_Clear(&config);
        return 0;
    }

    config.site_import = 0;
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);

    if (PyStatus_Exception(status)) {
        printf("ERRO: Falha na inicializacao do Python: %s\n", status.err_msg);
        return 0;
    }

    char sys_path_cmd[4096];
    
    snprintf(sys_path_cmd, sizeof(sys_path_cmd),
             "import sys\n"
             "path = r'%s'.replace('\\\\', '/')\n"
             "if path not in sys.path:\n"
             "    sys.path.append(path)\n"
             "print('Adicionado ao sys.path:', path)",
             base);

    printf("Executando comando Python para sys.path...\n");
    
    if (PyRun_SimpleString(sys_path_cmd) != 0) {
        printf("AVISO: Falha ao adicionar %s ao sys.path\n", base);
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        
        printf("Tentando método alternativo...\n");
        
        char escaped_path[512];
        char *p = escaped_path;
        const char *s = base;
        while (*s && (p - escaped_path) < (sizeof(escaped_path) - 4)) {
            if (*s == '\\') {
                *p++ = '\\';
                *p++ = '\\';
            } else {
                *p++ = *s;
            }
            s++;
        }
        *p = '\0';
        
        char sys_path_cmd2[4096];
        snprintf(sys_path_cmd2, sizeof(sys_path_cmd2),
                 "import sys\n"
                 "path = '%s'\n"
                 "if path not in sys.path:\n"
                 "    sys.path.append(path)\n"
                 "print('Adicionado (método 2):', path)",
                 escaped_path);
                 
        if (PyRun_SimpleString(sys_path_cmd2) != 0) {
            printf("AVISO: Método alternativo também falhou\n");
            if (PyErr_Occurred()) PyErr_Print();
        } else {
            printf("SUCESSO com método alternativo\n");
        }
    } else {
        printf("SUCESSO ao adicionar ao sys.path\n");
    }

    return 1;
}

void garantir_arquivos_json(void) {
    const char* root = ROOT_PATH ? ROOT_PATH : "python_embedded";

    char code[4096];
    snprintf(code, sizeof(code),
        "import os, json, sys\n"
        "try:\n"
        "    root = r'%s'\n"
        "    if not os.path.exists(root):\n"
        "        print('ERRO: ROOT_PATH inacessivel: %%s' %% root)\n"
        "        sys.exit(1)\n"
        "    arquivos = ['users.json','turmas.json','alunos.json','matriculas.json','atividades.json','anuncios.json']\n"
        "    for a in arquivos:\n"
        "        dest = os.path.join(root, a)\n"
        "        if not os.path.exists(dest):\n"
        "            data = {'users':[]} if a=='users.json' else \\\n"
        "                   {'turmas':[]} if a=='turmas.json' else \\\n"
        "                   {'atividades':[]} if a=='atividades.json' else \\\n"
        "                   {'anuncios':[]} if a=='anuncios.json' else \\\n"
        "                   {} if a in ['alunos.json','matriculas.json'] else {}\n"
        "            with open(dest,'w',encoding='utf-8') as f:\n"
        "                json.dump(data,f,indent=4,ensure_ascii=False)\n"
        "            print('Criado: %%s' %% dest)\n"
        "        else:\n"
        "            print('OK: %%s' %% dest)\n"
        "except Exception as e:\n"
        "    print('ERRO Python: %%s' %% e, file=sys.stderr)\n",
        root);

    PyRun_SimpleString(code);
    if (PyErr_Occurred()) PyErr_Print();
}

PyObject* safe_import_module(const char* module_name) {
    printf("Tentando importar: %s\n", module_name);
    
    PyObject* pModule = PyImport_ImportModule(module_name);
    
    if (!pModule) {
        printf("FALHA na importação de %s\n", module_name);
        PyErr_Print();
        
        char debug_code[512];
        snprintf(debug_code, sizeof(debug_code),
            "import os\n"
            "print('=== DEBUG PARA MÓDULO: %%s ===')\n"
            "print('Arquivos em python_embedded:')\n"
            "try:\n"
            "    for f in os.listdir('python_embedded'):\n"
            "        if f.startswith('%%s') or f == '%%s.py':\n"
            "            print('  → ', f)\n"
            "except: pass\n"
            "print('========================')\n",
            module_name, module_name, module_name);
        
        PyRun_SimpleString(debug_code);
    } else {
        printf("SUCESSO na importação de %s\n", module_name);
    }
    
    return pModule;
}

int rodarPythonRegistrar(void) {
    endwin();
    SetConsoleOutputCP(65001);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    fflush(stdin); fflush(stdout);

    printf("\n--- Iniciando rodarPythonRegistrar ---\n");
    
    PyObject* pModule = safe_import_module("registroaluno");
    if (!pModule) {
        printf("ERRO CRÍTICO: Não foi possível importar registroaluno\n");
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        attron(COLOR_PAIR(2));
        printw("ERRO: Módulo 'registroaluno' não encontrado!\n");
        attroff(COLOR_PAIR(2));
        refresh(); getch();
        return 0;
    }

    PyObject* pFunc = PyObject_GetAttrString(pModule, "registrar");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        PyErr_Print(); Py_DECREF(pModule);
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        printw("ERRO: Função 'registrar' não encontrada.\n");
        refresh(); getch();
        return 0;
    }

    PyObject* pValue = PyObject_CallObject(pFunc, NULL);
    int result = (pValue && PyObject_IsTrue(pValue));
    Py_XDECREF(pValue); Py_DECREF(pFunc); Py_DECREF(pModule);

    initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
    start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
    return result;
}

int rodarPythonLogin(void) {
    endwin();
    SetConsoleOutputCP(65001);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    fflush(stdin); fflush(stdout);

    printf("\n--- Iniciando rodarPythonLogin ---\n");
    
    PyObject* pModule = safe_import_module("registroaluno");
    if (!pModule) {
        printf("ERRO CRÍTICO: Não foi possível importar registroaluno\n");
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        attron(COLOR_PAIR(2));
        printw("ERRO: Módulo 'registroaluno' não encontrado!\n");
        attroff(COLOR_PAIR(2));
        refresh(); getch();
        return 0;
    }

    PyObject* pFunc = PyObject_GetAttrString(pModule, "login_aluno");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        PyErr_Print(); Py_DECREF(pModule);
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        printw("ERRO: Função 'login_aluno' não encontrada.\n");
        refresh(); getch();
        return 0;
    }

    PyObject* pValue = PyObject_CallObject(pFunc, NULL);
    int result = 0;
    
    if (pValue != NULL && pValue != Py_None) {
        if (PyUnicode_Check(pValue)) {
            const char* matricula = PyUnicode_AsUTF8(pValue);
            if (matricula) {
                if (aluno_matricula_logado) free(aluno_matricula_logado);
                aluno_matricula_logado = strdup(matricula);
                result = 1;
                printf("Aluno logado com matrícula: %s\n", matricula);
            }
        } else {
            result = PyObject_IsTrue(pValue);
        }
        Py_DECREF(pValue);
    } else if (pValue == NULL) {
        PyErr_Print();
    }
    
    Py_DECREF(pFunc); Py_DECREF(pModule);

    initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
    start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
    return result;
}

int rodarPythonLoginProfessor(void) {
    endwin();
    SetConsoleOutputCP(65001);
    setlocale(LC_ALL, "pt_BR.UTF-8");
    fflush(stdin); fflush(stdout);

    printf("\n--- Iniciando rodarPythonLoginProfessor ---\n");
    
    PyObject* pModule = safe_import_module("LoginProfessor");
    if (!pModule) {
        printf("ERRO CRÍTICO: Não foi possível importar LoginProfessor\n");
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        attron(COLOR_PAIR(2));
        printw_utf8("ERRO: Módulo 'LoginProfessor' não encontrado!\n");
        attroff(COLOR_PAIR(2));
        refresh(); getch();
        return 0;
    }

    PyObject* pFunc = PyObject_GetAttrString(pModule, "login_professor");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        PyErr_Print(); Py_DECREF(pModule);
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        printw_utf8("ERRO: Função 'login_professor' não encontrada.\n");
        refresh(); getch();
        return 0;
    }

    PyObject* pValue = PyObject_CallObject(pFunc, NULL);
    int result = 0;
    if (pValue != NULL && pValue != Py_None) {
        const char* name = PyUnicode_AsUTF8(pValue);
        if (name) {
            if (nome_professor_logado) free(nome_professor_logado);
            nome_professor_logado = strdup(name);
            result = 1;
        }
        Py_DECREF(pValue);
    } else if (pValue == NULL) {
        PyErr_Print();
    }
    Py_DECREF(pFunc); Py_DECREF(pModule);

    initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
    start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
    return result;
}

void menuAluno(void) {
    const char* opcoes[] = {"Registrar", "Login", "Voltar"};
    int num_opcoes = 3, selecionado = 0, tecla;

    while (1) {
        limparTela(); linha(); printw("Menu Aluno\n"); linha();

        for (int i = 0; i < num_opcoes; i++) {
            if (i == selecionado) { attron(COLOR_PAIR(1)); printw("> %s\n", opcoes[i]); attroff(COLOR_PAIR(1)); }
            else printw(" %s\n", opcoes[i]);
        }

        linha(); refresh();
        tecla = getch();

        switch (tecla) {
            case KEY_UP:  selecionado = (selecionado - 1 + num_opcoes) % num_opcoes; break;
            case KEY_DOWN:selecionado = (selecionado + 1) % num_opcoes; break;
            case 10:
                if (selecionado == 0) {
                    limparTela();
                    if (rodarPythonRegistrar()) {
                        attron(COLOR_PAIR(1));
                        printw("Registro concluído! Retornando ao menu...\n");
                        attroff(COLOR_PAIR(1));
                        refresh(); napms(2000);
                    }
                }
                else if (selecionado == 1) {
                    limparTela();
                    if (rodarPythonLogin()) { limparTela(); menuLogado(); }
                }
                else if (selecionado == 2) { limparTela(); return; }
                break;
            default:
                attron(COLOR_PAIR(2));
                printw_utf8("\nTecla inválida. Use setas ou Enter. Tente novamente...\n");
                attroff(COLOR_PAIR(2));
                refresh(); napms(1000);
                break;
        }
    }
}

void menuProfessor(void) {
    const char* opcoes[] = {"Login", "Voltar"};
    int num_opcoes = 2, selecionado = 0, tecla;

    while (1) {
        limparTela(); linha(); printw_utf8("Área do Professor\n"); linha();

        for (int i = 0; i < num_opcoes; i++) {
            if (i == selecionado) { attron(COLOR_PAIR(1)); printw_utf8("> %s\n", opcoes[i]); attroff(COLOR_PAIR(1)); }
            else printw_utf8(" %s\n", opcoes[i]);
        }

        linha(); refresh();
        tecla = getch();

        switch (tecla) {
            case KEY_UP:  selecionado = (selecionado - 1 + num_opcoes) % num_opcoes; break;
            case KEY_DOWN:selecionado = (selecionado + 1) % num_opcoes; break;
            case 10:
                if (selecionado == 0) { if (rodarPythonLoginProfessor()) { menuLogadoProf(); return; } }
                else if (selecionado == 1) return;
                break;
            default:
                attron(COLOR_PAIR(2));
                printw_utf8("\nUse setas e Enter.\n");
                attroff(COLOR_PAIR(2));
                refresh(); napms(1000);
                break;
        }
    }
}

void menuPrincipal(void) {
    const char* opcoes[] = {"Aluno", "Professor", "Sair"};
    int num_opcoes = 3, selecionado = 0, tecla;

    while (1) {
        limparTela(); linha();
        printw("Bem-vindo ao sistema de registro e login!\n"); linha();

        for (int i = 0; i < num_opcoes; i++) {
            if (i == selecionado) { attron(COLOR_PAIR(1)); printw("> %s\n", opcoes[i]); attroff(COLOR_PAIR(1)); }
            else printw(" %s\n", opcoes[i]);
        }

        linha(); refresh();
        tecla = getch();

        switch (tecla) {
            case KEY_UP:  selecionado = (selecionado - 1 + num_opcoes) % num_opcoes; break;
            case KEY_DOWN:selecionado = (selecionado + 1) % num_opcoes; break;
            case 10:
                if (selecionado == 0) { limparTela(); menuAluno(); }
                else if (selecionado == 1) { limparTela(); menuProfessor(); }
                else if (selecionado == 2) {
                    limparTela(); linha();
                    attron(COLOR_PAIR(2)); printw("Saindo....\n"); attroff(COLOR_PAIR(2));
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

int main(void) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    setlocale(LC_ALL, "pt_BR.UTF-8");

    printf("Verificando conectividade com Z:\\...\n");
    verificar_unidade_z();
    load_config();
    
    
	if (!ROOT_PATH && _access("Z:\\python_embedded", 0) == 0) {
        ROOT_PATH = _strdup("Z:\\python_embedded");
        printf("CONFIG: Definindo ROOT_PATH para Z:\\python_embedded (automático)\n");
    }
    
    char exe_dir[MAX_PATH];
    GetModuleFileNameA(NULL, exe_dir, MAX_PATH);
    char* slash = strrchr(exe_dir, '\\');
    if (slash) *slash = '\0';
    _chdir(exe_dir);

    if (!init_python_portable()) {
        printf("ERRO: Falha na inicializacao do Python\n");
        return 1;
    }

    garantir_arquivos_json();

    initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);

    menuPrincipal();

    if (nome_professor_logado) free(nome_professor_logado);
    if (aluno_matricula_logado) free(aluno_matricula_logado);
    if (ROOT_PATH) free(ROOT_PATH);

    endwin();
    Py_Finalize();
    return 0;
} //G