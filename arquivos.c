#define _CRT_SECURE_NO_WARNINGS
#define MAX_INPUT 256


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include <locale.h>
#include <Python.h>
#include <curses.h>
#include <direct.h>
#include <ctype.h>
#include "cJSON.h"
#include "utils.h"

extern char *ROOT_PATH;
extern char* nome_professor_logado;
extern char* aluno_matricula_logado;


void remover_aspas(char *str) {
    char *start = str;
    char *end = str + strlen(str) - 1;

    while (*start && (*start == ' ' || *start == '\t' || *start == '"' || *start == '\'')) start++;
    while (end >= start && (*end == ' ' || *end == '\t' || *end == '"' || *end == '\'' || *end == '\n' || *end == '\r')) end--;
    *(end + 1) = '\0';
    if (start != str) memmove(str, start, end - start + 2);
}

char* obter_nome_turma_por_id(int turma_id) {
    static char nome_turma[256] = "Turma Desconhecida";
    
    char* caminho_turmas = encontrar_arquivo_json("turmas.json");
    FILE* fp = fopen(caminho_turmas, "r");
    if (!fp) {
        return nome_turma;
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
        return nome_turma;
    }

    cJSON* turmas = cJSON_GetObjectItem(root, "turmas");
    cJSON* turma = NULL;
    
    cJSON_ArrayForEach(turma, turmas) {
        cJSON* id = cJSON_GetObjectItem(turma, "id");
        if (cJSON_IsNumber(id) && id->valueint == turma_id) {
            cJSON* nome = cJSON_GetObjectItem(turma, "nome_turma");
            if (!nome) {
                nome = cJSON_GetObjectItem(turma, "nome");
            }
            if (cJSON_IsString(nome)) {
                strncpy(nome_turma, nome->valuestring, sizeof(nome_turma) - 1);
                nome_turma[sizeof(nome_turma) - 1] = '\0';
                
                // Sanitizar nome para uso em caminhos de pasta
                for (char* p = nome_turma; *p; p++) {
                    if (*p == '\\' || *p == '/' || *p == ':' || *p == '*' || 
                        *p == '?' || *p == '"' || *p == '<' || *p == '>' || *p == '|') {
                        *p = '_';
                    }
                }
            }
            break;
        }
    }

    cJSON_Delete(root);
    return nome_turma;
}

char* encontrar_arquivo_json(const char* filename) {
    static char path[512];
    
    const char *base = ROOT_PATH ? ROOT_PATH : "python_embedded";
    snprintf(path, sizeof(path), "%s\\%s", base, filename);
    
    if (_access(path, 0) == 0) {
        return path;
    }
    
    snprintf(path, sizeof(path), "Z:\\python_embedded\\%s", filename);
    if (_access(path, 0) == 0) {
        return path;
    }
    
    snprintf(path, sizeof(path), "python_embedded\\%s", filename);
    return path;
}

char* get_caminho_embedded(const char* filename) {
    return encontrar_arquivo_json(filename);
}

// === FUNÇÃO MODIFICADA: Agora recebe turma_id ===
int enviar_arquivo(const char* caminho_origem, const char* aluno_matricula, int atividade_id, int turma_id) {
    const char* ext = strrchr(caminho_origem, '.');
    if (!ext) {
        return -3;
    }
    ext++;

    char ext_lower[5];
    strncpy(ext_lower, ext, 4);
    ext_lower[4] = '\0';
    for (int i = 0; ext_lower[i]; i++) ext_lower[i] = tolower(ext_lower[i]);

    if (strcmp(ext_lower, "txt") != 0 && strcmp(ext_lower, "pdf") != 0) {
        return -4;
    }

    const char* base_dir = ROOT_PATH ? ROOT_PATH : "Z:\\python_embedded";
    
    // Obter nome da turma para usar na estrutura de pastas
    char* nome_turma = obter_nome_turma_por_id(turma_id);
    
    // Criar estrutura: submissoes/turma_nome/matricula/atividade_id/
    char pasta_sub[512];
    snprintf(pasta_sub, sizeof(pasta_sub), "%s\\submissoes\\%s\\%s\\%d\\", 
             base_dir, nome_turma, aluno_matricula, atividade_id);

    char temp[512];
    
    // Criar diretórios recursivamente
    snprintf(temp, sizeof(temp), "%s\\submissoes", base_dir);
    _mkdir(temp);
    
    snprintf(temp, sizeof(temp), "%s\\submissoes\\%s", base_dir, nome_turma);
    _mkdir(temp);
    
    snprintf(temp, sizeof(temp), "%s\\submissoes\\%s\\%s", base_dir, nome_turma, aluno_matricula);
    _mkdir(temp);
    
    _mkdir(pasta_sub);

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char nome_dest[512];
    snprintf(nome_dest, sizeof(nome_dest), "%sresposta_%04d%02d%02d_%02d%02d.%s",
             pasta_sub, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, ext_lower);

    FILE* src = fopen(caminho_origem, "rb");
    if (!src) {
        return -1;
    }
    FILE* dest = fopen(nome_dest, "wb");
    if (!dest) {
        fclose(src);
        return -2;
    }

    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }
    fclose(src);
    fclose(dest);

    if (atualizar_json_submissao(atividade_id, aluno_matricula, nome_dest, turma_id) != 0) {
        return -5;
    }

    return 0;
}

int criar_atividade(const char* caminho_txt, const char* descricao, int turma_id) {
    const char* ext = strrchr(caminho_txt, '.');
    if (!ext || _stricmp(ext, ".txt") != 0) {
        return -1;
    }

    if (_access(caminho_txt, 0) != 0) {
        return -2;
    }

    int atividade_id = get_next_atividade_id();

    const char* base_dir = ROOT_PATH ? ROOT_PATH : "Z:\\python_embedded";
    
    // Obter nome da turma para organização
    char* nome_turma = obter_nome_turma_por_id(turma_id);
    
    char pasta[512];
    snprintf(pasta, sizeof(pasta), "%s\\atividades\\%s\\%d", base_dir, nome_turma, atividade_id);

    char temp[512];
    snprintf(temp, sizeof(temp), "%s\\atividades", base_dir);
    _mkdir(temp);
    
    snprintf(temp, sizeof(temp), "%s\\atividades\\%s", base_dir, nome_turma);
    _mkdir(temp);
    
    _mkdir(pasta);

    char destino[512];
    snprintf(destino, sizeof(destino), "%s\\enunciado.txt", pasta);
    if (!CopyFile(caminho_txt, destino, FALSE)) {
        return -3;
    }

    char* caminho_json = encontrar_arquivo_json("atividades.json");
    cJSON* root = NULL;
    FILE* fp = fopen(caminho_json, "r");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        long len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char* data = malloc(len + 1);
        fread(data, 1, len, fp);
        data[len] = '\0';
        fclose(fp);
        root = cJSON_Parse(data);
        free(data);
    }
    if (!root) {
        root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "atividades", cJSON_CreateArray());
    }

    cJSON* atividades = cJSON_GetObjectItem(root, "atividades");
    cJSON* nova = cJSON_CreateObject();
    cJSON_AddNumberToObject(nova, "atividade_id", atividade_id);
    cJSON_AddNumberToObject(nova, "turma_id", turma_id);
    cJSON_AddStringToObject(nova, "descricao", descricao[0] ? descricao : "Sem descrição");
    cJSON_AddStringToObject(nova, "enunciado", destino);
    cJSON_AddItemToObject(nova, "submissoes", cJSON_CreateArray());
    cJSON_AddItemToArray(atividades, nova);

    char* json_str = cJSON_Print(root);
    fp = fopen(caminho_json, "w");
    if (fp) {
        fprintf(fp, "%s", json_str);
        fclose(fp);
    } else {
        free(json_str);
        cJSON_Delete(root);
        return -4;
    }

    free(json_str);
    cJSON_Delete(root);
    return 0;
}

// === FUNÇÃO MODIFICADA: Agora recebe turma_id ===
int atualizar_json_submissao(int atividade_id, const char* aluno_matricula, const char* caminho_arquivo, int turma_id) {
    char* caminho_ativ = encontrar_arquivo_json("atividades.json");
    FILE* fp = fopen(caminho_ativ, "r");
    if (!fp) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(fp);
        return -2;
    }

    fread(buffer, 1, file_size, fp);
    buffer[file_size] = '\0';
    fclose(fp);

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        return -3;
    }

    cJSON* atividades = cJSON_GetObjectItem(root, "atividades");
    if (!cJSON_IsArray(atividades)) {
        cJSON_Delete(root);
        return -4;
    }

    cJSON* atividade = NULL;
    int found = 0;
    cJSON_ArrayForEach(atividade, atividades) {
        cJSON* id = cJSON_GetObjectItem(atividade, "atividade_id");
        cJSON* turma = cJSON_GetObjectItem(atividade, "turma_id");
        if (cJSON_IsNumber(id) && id->valueint == atividade_id && 
            cJSON_IsNumber(turma) && turma->valueint == turma_id) {
            found = 1;
            cJSON* submissoes = cJSON_GetObjectItem(atividade, "submissoes");
            if (!cJSON_IsArray(submissoes)) {
                cJSON_Delete(root);
                return -5;
            }

            cJSON* submissao = cJSON_CreateObject();
            cJSON_AddStringToObject(submissao, "aluno_id", aluno_matricula);
            cJSON_AddStringToObject(submissao, "arquivo", caminho_arquivo);
            cJSON_AddNumberToObject(submissao, "turma_id", turma_id);

            time_t now = time(NULL);
            char data[20];
            strftime(data, sizeof(data), "%Y-%m-%d %H:%M", localtime(&now));
            cJSON_AddStringToObject(submissao, "data_envio", data);
            cJSON_AddStringToObject(submissao, "status", "enviado");

            cJSON_AddItemToArray(submissoes, submissao);
            break;
        }
    }

    if (!found) {
        cJSON_Delete(root);
        return -6;
    }

    char* json_str = cJSON_Print(root);
    cJSON_Delete(root);

    fp = fopen(caminho_ativ, "w");
    if (!fp) return -7;
    fprintf(fp, "%s", json_str);
    free(json_str);
    fclose(fp);

    return 0;
}

int get_next_atividade_id() {
    char* caminho_ativ = encontrar_arquivo_json("atividades.json");
    FILE* fp = fopen(caminho_ativ, "r");
    if (!fp) return 1;

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(fp);
        return 1;
    }

    fread(buffer, 1, file_size, fp);
    buffer[file_size] = '\0';
    fclose(fp);

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) return 1;

    cJSON* atividades = cJSON_GetObjectItem(root, "atividades");
    if (!cJSON_IsArray(atividades)) {
        cJSON_Delete(root);
        return 1;
    }

    int max_id = 0;
    cJSON* atividade = NULL;
    cJSON_ArrayForEach(atividade, atividades) {
        cJSON* id = cJSON_GetObjectItem(atividade, "atividade_id");
        if (cJSON_IsNumber(id) && id->valueint > max_id) {
            max_id = id->valueint;
        }
    }

    cJSON_Delete(root);
    return max_id + 1;
}


void menu_criar_atividade(void) {
    limparTela();
    linha();
    printw_utf8("=== Criar Nova Atividade ===\n");
    linha();

    if (!nome_professor_logado) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Nenhum professor logado.\n");
        attroff(COLOR_PAIR(2));
        getch();
        return;
    }

    // === 1. Carregar turmas.json ===
    char* caminho_turmas = encontrar_arquivo_json("turmas.json");
    FILE* fp = fopen(caminho_turmas, "r");
    if (!fp) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: turmas.json não encontrado!\n");
        attroff(COLOR_PAIR(2));
        getch();
        return;
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
    if (!root || !cJSON_GetObjectItem(root, "turmas")) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro ao ler turmas.json\n");
        attroff(COLOR_PAIR(2));
        getch();
        cJSON_Delete(root);
        return;
    }


    typedef struct { int id; char nome[128]; } TurmaInfo;
    TurmaInfo turmas[50];
    int count = 0;

    cJSON* array = cJSON_GetObjectItem(root, "turmas");
    cJSON* item = NULL;

    cJSON_ArrayForEach(item, array) {
        cJSON* prof_obj = cJSON_GetObjectItem(item, "professor");
        if (!prof_obj) continue;

        cJSON* nome_prof = cJSON_GetObjectItem(prof_obj, "nome");
        cJSON* username = cJSON_GetObjectItem(prof_obj, "username");

        int pertence = 0;
        if (nome_prof && cJSON_IsString(nome_prof)) {
            if (_stricmp(nome_prof->valuestring, nome_professor_logado) == 0 ||
                strstr(nome_prof->valuestring, nome_professor_logado)) {
                pertence = 1;
            }
        }
        if (!pertence && username && cJSON_IsString(username)) {
            if (_stricmp(username->valuestring, nome_professor_logado) == 0) {
                pertence = 1;
            }
        }

        if (pertence) {
            cJSON* id_obj = cJSON_GetObjectItem(item, "id");
            cJSON* nome_turma = cJSON_GetObjectItem(item, "nome_turma");
            if (!nome_turma) nome_turma = cJSON_GetObjectItem(item, "nome");

            if (cJSON_IsNumber(id_obj) && cJSON_IsString(nome_turma) && count < 50) {
                turmas[count].id = id_obj->valueint;
                strncpy(turmas[count].nome, nome_turma->valuestring, 127);
                turmas[count].nome[127] = '\0';
                count++;
            }
        }
    }
    cJSON_Delete(root);

    if (count == 0) {
        attron(COLOR_PAIR(2));
        printw_utf8("Você não tem turmas cadastradas.\n");
        attroff(COLOR_PAIR(2));
        linha();
        printw_utf8("Pressione qualquer tecla...\n");
        getch();
        return;
    }

   
    int selecionado = 0;
    int tecla;

    while (1) {
        limparTela();
        linha();
        printw_utf8("Selecione a turma para a atividade:\n");
        linha();

        for (int i = 0; i < count; i++) {
            if (i == selecionado) {
                attron(COLOR_PAIR(1));
                printw_utf8("> %s\n", turmas[i].nome);
                attroff(COLOR_PAIR(1));
            } else {
                printw_utf8("  %s\n", turmas[i].nome);
            }
        }

        linha();
        printw_utf8("^v Navegar | ENTER Confirmar | ESC Cancelar\n");
        refresh();

        tecla = getch();
        if (tecla == KEY_UP)   selecionado = (selecionado - 1 + count) % count;
        if (tecla == KEY_DOWN) selecionado = (selecionado + 1) % count;
        if (tecla == 10) break;  // ENTER
        if (tecla == 27) return; // ESC
    }

    int turma_id = turmas[selecionado].id;
    char* nome_turma = turmas[selecionado].nome;

    // === 4. Descrição da atividade ===
    limparTela();
    linha();
    printw_utf8("Turma: %s (ID: %d)\n", nome_turma, turma_id);
    linha();
    printw_utf8("Descrição da atividade (opcional):\n> ");
    echo();
    char descricao[1024] = {0};
    getnstr(descricao, 1020);
    noecho();

    // === 5. Caminho do enunciado (.txt) ===
    printw_utf8("\nCaminho completo do enunciado (.txt):\n> ");
    echo();
    char caminho_txt[512] = {0};
    getnstr(caminho_txt, 510);
    noecho();

    remover_aspas(caminho_txt);  // Remove aspas automaticamente

    if (_access(caminho_txt, 0) != 0) {
        attron(COLOR_PAIR(2));
        printw_utf8("\nArquivo não encontrado: %s\n", caminho_txt);
        attroff(COLOR_PAIR(2));
        getch();
        return;
    }

    // === 6. Criar atividade ===
    char* descricao_utf8 = cp_to_utf8(descricao);
    int resultado = criar_atividade(caminho_txt, descricao_utf8 ? descricao_utf8 : "Sem descrição", turma_id);
    free(descricao_utf8);

    limparTela();
    linha();
    if (resultado == 0) {
        attron(COLOR_PAIR(1));
        printw_utf8("ATIVIDADE CRIADA COM SUCESSO!\n");
        printw_utf8("Turma: %s\n", nome_turma);
        printw_utf8("ID da atividade: %d\n", get_next_atividade_id() - 1);
        attroff(COLOR_PAIR(1));
    } else {
        attron(COLOR_PAIR(2));
        printw_utf8("ERRO AO CRIAR ATIVIDADE (código: %d)\n", resultado);
        printw_utf8("Verifique se o arquivo .txt existe e se a turma está correta.\n");
        attroff(COLOR_PAIR(2));
    }
    linha();
    printw_utf8("Pressione qualquer tecla para continuar...\n");
    getch();
}

void atribuir_notas_interativo(void) {
    if (!nome_professor_logado || !strlen(nome_professor_logado)) {
        limparTela(); linha();
        attron(COLOR_PAIR(2)); printw_utf8("Professor não logado!\n"); attroff(COLOR_PAIR(2));
        linha(); printw_utf8("Pressione qualquer tecla...\n"); getch();
        return;
    }

    // === Carregar turmas do professor (só para exibir) ===
    char* caminho = encontrar_arquivo_json("turmas.json");
    FILE* f = fopen(caminho, "r");
    if (!f) { printw_utf8("Erro ao abrir turmas.json\n"); getch(); return; }

    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char* buf = malloc(sz + 1); fread(buf, 1, sz, f); buf[sz] = 0; fclose(f);
    cJSON* root = cJSON_Parse(buf); free(buf);
    if (!root) { fclose(f); return; }

    typedef struct { int id; char nome[128]; } Turma;
    Turma turmas[50]; int n = 0;

    cJSON* array = cJSON_GetObjectItem(root, "turmas");
    cJSON* t;
    cJSON_ArrayForEach(t, array) {
        cJSON* prof = cJSON_GetObjectItem(t, "professor");
        cJSON* user = prof ? cJSON_GetObjectItem(prof, "username") : NULL;
        cJSON* id = cJSON_GetObjectItem(t, "id");
        cJSON* nome = cJSON_GetObjectItem(t, "nome_turma");
        if (user && strcmp(user->valuestring, nome_professor_logado) == 0 &&
            id && nome && n < 50) {
            turmas[n].id = id->valueint;
            strncpy(turmas[n].nome, nome->valuestring, 127);
            n++;
        }
    }
    cJSON_Delete(root);
    if (n == 0) {
        limparTela(); linha();
        printw_utf8("Você não tem turmas cadastradas.\n");
        linha(); printw_utf8("Pressione qualquer tecla...\n"); getch();
        return;
    }

    // === Menu de turmas ===
    int sel = 0, tecla;
    while (1) {
        limparTela(); linha();
        printw_utf8("Selecione a Turma para Atribuir Notas\n");
        linha();
        for (int i = 0; i < n; i++) {
            if (i == sel) { attron(COLOR_PAIR(1)); printw_utf8("> "); attroff(COLOR_PAIR(1)); }
            else printw_utf8("  ");
            printw_utf8("%d. %s (ID: %d)\n", i+1, turmas[i].nome, turmas[i].id);
        }
        linha(); printw_utf8("^v Navegar | ENTER Selecionar | ESC Cancelar\n"); refresh();

        tecla = getch();
        if (tecla == KEY_UP) sel = (sel - 1 + n) % n;
        else if (tecla == KEY_DOWN) sel = (sel + 1) % n;
        else if (tecla == 10) break;
        else if (tecla == 27) return;
    }
    int turma_id = turmas[sel].id;

    // === Carregar atividades da turma selecionada ===
    caminho = encontrar_arquivo_json("atividades.json");
    f = fopen(caminho, "r");
    if (!f) return;
    fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
    buf = malloc(sz + 1); fread(buf, 1, sz, f); buf[sz] = 0; fclose(f);
    root = cJSON_Parse(buf); free(buf);

    typedef struct { int id; char desc[256]; } Atv;
    Atv atividades[100]; int na = 0;
    cJSON* array_atv = cJSON_GetObjectItem(root, "atividades");
    cJSON* a;
    cJSON_ArrayForEach(a, array_atv) {
        cJSON* tid = cJSON_GetObjectItem(a, "turma_id");
        cJSON* aid = cJSON_GetObjectItem(a, "atividade_id");
        cJSON* desc = cJSON_GetObjectItem(a, "descricao");
        if (tid && tid->valueint == turma_id && aid && desc && na < 100) {
            atividades[na].id = aid->valueint;
            strncpy(atividades[na].desc, desc->valuestring, 255);
            na++;
        }
    }
    cJSON_Delete(root);

    if (na == 0) {
        limparTela(); linha();
        printw_utf8("Nenhuma atividade nesta turma.\n");
        linha(); printw_utf8("Pressione qualquer tecla...\n"); getch();
        return;
    }

    // === Menu de atividades ===
    sel = 0;
    while (1) {
        limparTela(); linha();
        printw_utf8("Selecione a Atividade\n");
        linha();
        for (int i = 0; i < na; i++) {
            if (i == sel) { attron(COLOR_PAIR(1)); printw_utf8("> "); attroff(COLOR_PAIR(1)); }
            else printw_utf8("  ");
            printw_utf8("%d. %s (ID: %d)\n", i+1, atividades[i].desc, atividades[i].id);
        }
        linha(); printw_utf8("^v Navegar | ENTER Selecionar | ESC Cancelar\n"); refresh();

        tecla = getch();
        if (tecla == KEY_UP) sel = (sel - 1 + na) % na;
        else if (tecla == KEY_DOWN) sel = (sel + 1) % na;
        else if (tecla == 10) break;
        else if (tecla == 27) return;
    }
    int atividade_id = atividades[sel].id;

    // === CHAMA O PYTHON COM OS DOIS IDs JÁ VALIDADOS ===
    endwin();
    SetConsoleOutputCP(65001);

    PyObject *pModule = PyImport_ImportModule("LoginProfessor");
    if (pModule) {
        PyObject *pFunc = PyObject_GetAttrString(pModule, "atribuir_notas_com_ids");
        if (pFunc && PyCallable_Check(pFunc)) {
            PyObject *pArgs = PyTuple_New(2);
            PyTuple_SetItem(pArgs, 0, PyLong_FromLong(turma_id));
            PyTuple_SetItem(pArgs, 1, PyLong_FromLong(atividade_id));
            PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }

    initscr(); keypad(stdscr, TRUE); cbreak(); noecho(); reiniciar_cores();
}

void visualizador_interativo(const char* caminho_arquivo, const char* titulo) {
    FILE* fp = fopen(caminho_arquivo, "r");
    if (!fp) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Não foi possível abrir o arquivo.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        getch();
        return;
    }

    // Ler todas as linhas do arquivo
    char** linhas = NULL;
    int num_linhas = 0;
    char buffer[256];
    
    while (fgets(buffer, sizeof(buffer), fp)) {
        linhas = realloc(linhas, (num_linhas + 1) * sizeof(char*));
        linhas[num_linhas] = strdup(buffer);
        num_linhas++;
    }
    fclose(fp);

    if (num_linhas == 0) {
        printw_utf8("Arquivo vazio.\n");
        refresh();
        getch();
        return;
    }

    int linhas_por_pagina = LINES - 8; // Reserva espaço para header e footer
    if (linhas_por_pagina < 5) linhas_por_pagina = 5; // Mínimo de 5 linhas
    
    int total_paginas = (num_linhas + linhas_por_pagina - 1) / linhas_por_pagina;
    int pagina_atual = 0;
    int tecla;

    do {
        limparTela();
        linha();
        printw_utf8("%s - Página %d/%d\n", titulo, pagina_atual + 1, total_paginas);
        printw_utf8("Arquivo: %s\n", caminho_arquivo);
        linha();
        
        // Calcular range de linhas para a página atual
        int inicio = pagina_atual * linhas_por_pagina;
        int fim = inicio + linhas_por_pagina;
        if (fim > num_linhas) fim = num_linhas;
        
        // Exibir linhas da página atual
        for (int i = inicio; i < fim; i++) {
            printw_utf8("%s", linhas[i]);
        }
        
        // Preencher linhas restantes se necessário
        for (int i = fim - inicio; i < linhas_por_pagina; i++) {
            printw("\n");
        }
        
        linha();
        if (total_paginas > 1) {
            printw_utf8("< Anterior | > Próxima | ");
        }
        printw_utf8("A Abrir arquivo | ESC Voltar\n");
        linha();
        refresh();
        
        tecla = getch();
        switch (tecla) {
            case KEY_LEFT:
                if (pagina_atual > 0) pagina_atual--;
                break;
            case KEY_RIGHT:
                if (pagina_atual < total_paginas - 1) pagina_atual++;
                break;
            case 'a':
            case 'A':
                // Abrir arquivo no editor padrão
                endwin(); // Fecha ncurses temporariamente
                ShellExecuteA(NULL, "open", caminho_arquivo, NULL, NULL, SW_SHOW);
                initscr(); // Reabre ncurses
                keypad(stdscr, TRUE);
                cbreak();
                noecho();
                reiniciar_cores();
                init_pair(1, COLOR_GREEN, COLOR_BLACK);
                init_pair(2, COLOR_RED, COLOR_BLACK);
                break;
            case 27: // ESC
                break;
            default:
                break;
        }
    } while (tecla != 27);

    // Liberar memória
    for (int i = 0; i < num_linhas; i++) {
        free(linhas[i]);
    }
    free(linhas);
}

void exibir_enunciado_atividade(int atividade_id) {
    char* caminho_ativ = encontrar_arquivo_json("atividades.json");
    FILE* fp_ativ = fopen(caminho_ativ, "r");
    if (!fp_ativ) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: atividades.json não encontrado.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        getch();
        return;
    }
    
    fseek(fp_ativ, 0, SEEK_END);
    long size_ativ = ftell(fp_ativ);
    fseek(fp_ativ, 0, SEEK_SET);
    char* buffer_ativ = malloc(size_ativ + 1);
    fread(buffer_ativ, 1, size_ativ, fp_ativ);
    buffer_ativ[size_ativ] = '\0';
    fclose(fp_ativ);
    
    cJSON* root_ativ = cJSON_Parse(buffer_ativ);
    free(buffer_ativ);
    if (!root_ativ) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro ao ler atividades.json.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        getch();
        return;
    }
    
    cJSON* atividades = cJSON_GetObjectItem(root_ativ, "atividades");
    cJSON* ativ = NULL;
    int encontrada = 0;
    
    cJSON_ArrayForEach(ativ, atividades) {
        cJSON* id = cJSON_GetObjectItem(ativ, "atividade_id");
        if (cJSON_IsNumber(id) && id->valueint == atividade_id) {
            encontrada = 1;
            
            cJSON* desc = cJSON_GetObjectItem(ativ, "descricao");
            cJSON* enunciado_path = cJSON_GetObjectItem(ativ, "enunciado");
            
            if (cJSON_IsString(enunciado_path)) {
                char titulo[256];
                snprintf(titulo, sizeof(titulo), "Atividade ID: %d - %s", 
                         atividade_id, 
                         desc && cJSON_IsString(desc) ? desc->valuestring : "Sem descrição");
                
                visualizador_interativo(enunciado_path->valuestring, titulo);
            } else {
                attron(COLOR_PAIR(2));
                printw_utf8("Erro: Enunciado não encontrado para esta atividade.\n");
                attroff(COLOR_PAIR(2));
                refresh();
                getch();
            }
            break;
        }
    }
    
    if (!encontrada) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Atividade não encontrada.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        getch();
    }
    
    cJSON_Delete(root_ativ);
}

char* username_para_nome_completo(const char* username){
    if (!username || username[0] == '\0') return NULL;

    char* caminho = encontrar_arquivo_json("turmas.json");
    FILE* f = fopen(caminho, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = malloc(size + 1);
    if (!buffer) { fclose(f); return NULL; }

    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) return NULL;

    char* nome_completo = NULL;

    if (cJSON_HasObjectItem(root, "turmas")) {
        cJSON* turmas = cJSON_GetObjectItem(root, "turmas");
        cJSON* turma;
        cJSON_ArrayForEach(turma, turmas) {
            cJSON* prof_obj = cJSON_GetObjectItem(turma, "professor");
            if (!prof_obj) continue;

            cJSON* user = cJSON_GetObjectItem(prof_obj, "username");
            cJSON* nome = cJSON_GetObjectItem(prof_obj, "nome");

            if (user && user->valuestring && nome && nome->valuestring) {
                if (strcmp(user->valuestring, username) == 0) {
                    nome_completo = _strdup(nome->valuestring);
                    break;
                }
            }
        }
    }

    cJSON_Delete(root);
    return nome_completo;  // pode ser NULL → caller deve verificar
}

void formatar_nome_proprio(char* str){
    if (!str || str[0] == '\0') return;

    int nova_palavra = 1;
    for (int i = 0; str[i]; i++) {
        if (str[i] == ' ') {
            nova_palavra = 1;
        } else if (nova_palavra) {
            str[i] = toupper((unsigned char)str[i]);
            nova_palavra = 0;
        } else {
            str[i] = tolower((unsigned char)str[i]);
        }
    }
}

// === FUNÇÃO MODIFICADA: Agora recebe turma_id ===
int menu_responder_atividade(int atividade_id, int turma_id) {
    const char *opcoes[] = {"Responder Atividade", "Voltar ao Menu Anterior"};
    int num_opcoes = 2;
    int selecionado = 0;
    int tecla;
    
    int menu_start_line = 15; 
    
    while (1) {
        move(menu_start_line, 0);
        clrtobot();
        
        linha();
        
        printw_utf8("O que você deseja fazer?\n");
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
            case KEY_UP:
                selecionado = (selecionado - 1 + num_opcoes) % num_opcoes;
                break;
            case KEY_DOWN:
                selecionado = (selecionado + 1) % num_opcoes;
                break;
            case 10:
                if (selecionado == 0) {
                    char caminho[256];
                    limparTela();
                    linha();
                    printw_utf8("Responder Atividade ID: %d\n", atividade_id);
                    linha();
                    printw_utf8("Digite o caminho do seu arquivo de resposta (TXT ou PDF):\n");
                    refresh();
                    echo();
                    getstr(caminho);
                    noecho();
                    remover_aspas(caminho);
                    int resultado = enviar_arquivo(caminho, aluno_matricula_logado, atividade_id, turma_id);
                    if (resultado == 0) {
                        attron(COLOR_PAIR(1));
                        printw_utf8("Resposta enviada com sucesso!\n");
                        attroff(COLOR_PAIR(1));
                    } else {
                        attron(COLOR_PAIR(2));
                        printw_utf8("Erro ao enviar resposta. Código: %d\n", resultado);
                        attroff(COLOR_PAIR(2));
                    }
                    refresh();
                    getch();
                    return 1;
                } else if (selecionado == 1) {
                    return 0;
                }
                break;
            default:
                break;
        }
    }
}

void menu_atividades_aluno() {
    if (!aluno_matricula_logado) {
        limparTela();
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Nenhum aluno logado.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        getch();
        return;
    }

    char* caminho_turmas = encontrar_arquivo_json("turmas.json");
    FILE* fp_turmas = fopen(caminho_turmas, "r");
    if (!fp_turmas) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: turmas.json não encontrado.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        getch();
        return;
    }
    fseek(fp_turmas, 0, SEEK_END);
    long size_turmas = ftell(fp_turmas);
    fseek(fp_turmas, 0, SEEK_SET);
    char* buffer_turmas = malloc(size_turmas + 1);
    fread(buffer_turmas, 1, size_turmas, fp_turmas);
    buffer_turmas[size_turmas] = '\0';
    fclose(fp_turmas);

    cJSON* root_turmas = cJSON_Parse(buffer_turmas);
    free(buffer_turmas);
    if (!root_turmas) return;

    cJSON* turmas = cJSON_GetObjectItem(root_turmas, "turmas");

    int my_turmas[100];
    int num_my_turmas = 0;
    cJSON* turma = NULL;
    cJSON_ArrayForEach(turma, turmas) {
        cJSON* alunos = cJSON_GetObjectItem(turma, "alunos");
        if (cJSON_IsArray(alunos)) {
            cJSON* aluno_item = NULL;
            cJSON_ArrayForEach(aluno_item, alunos) {
                const char* matricula_aluno = NULL;
                
                if (cJSON_IsString(aluno_item)) {
                    matricula_aluno = aluno_item->valuestring;
                }
                else if (cJSON_IsObject(aluno_item)) {
                    cJSON* matricula_obj = cJSON_GetObjectItem(aluno_item, "matricula");
                    if (cJSON_IsString(matricula_obj)) {
                        matricula_aluno = matricula_obj->valuestring;
                    }
                }
                
                if (matricula_aluno && _stricmp(matricula_aluno, aluno_matricula_logado) == 0) {
                    cJSON* id = cJSON_GetObjectItem(turma, "id");
                    if (cJSON_IsNumber(id)) {
                        my_turmas[num_my_turmas++] = id->valueint;
                    }
                    break;
                }
            }
        }
    }
    cJSON_Delete(root_turmas);

    if (num_my_turmas == 0) {
        limparTela();
        printw_utf8("Nenhuma turma encontrada para você.\n");
        refresh();
        getch();
        return;
    }

    char* caminho_ativ = encontrar_arquivo_json("atividades.json");
    FILE* fp_ativ = fopen(caminho_ativ, "r");
    if (!fp_ativ) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: atividades.json não encontrado.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        getch();
        return;
    }
    fseek(fp_ativ, 0, SEEK_END);
    long size_ativ = ftell(fp_ativ);
    fseek(fp_ativ, 0, SEEK_SET);
    char* buffer_ativ = malloc(size_ativ + 1);
    fread(buffer_ativ, 1, size_ativ, fp_ativ);
    buffer_ativ[size_ativ] = '\0';
    fclose(fp_ativ);

    cJSON* root_ativ = cJSON_Parse(buffer_ativ);
    free(buffer_ativ);
    if (!root_ativ) return;

    cJSON* atividades = cJSON_GetObjectItem(root_ativ, "atividades");

    int num_ativ = 0;
    int ativ_ids[100];
    int ativ_turmas[100];
    char* ativ_descs[100] = {0};
    cJSON* ativ = NULL;
    cJSON_ArrayForEach(ativ, atividades) {
        cJSON* turma_id = cJSON_GetObjectItem(ativ, "turma_id");
        if (cJSON_IsNumber(turma_id)) {
            for (int i = 0; i < num_my_turmas; i++) {
                if (turma_id->valueint == my_turmas[i]) {
                    cJSON* id = cJSON_GetObjectItem(ativ, "atividade_id");
                    cJSON* desc = cJSON_GetObjectItem(ativ, "descricao");
                    if (cJSON_IsNumber(id) && cJSON_IsString(desc)) {
                        ativ_ids[num_ativ] = id->valueint;
                        ativ_turmas[num_ativ] = turma_id->valueint;
                        ativ_descs[num_ativ] = strdup(desc->valuestring);
                        num_ativ++;
                    }
                    break;
                }
            }
        }
    }
    cJSON_Delete(root_ativ);

    if (num_ativ == 0) {
        limparTela();
        printw_utf8("Nenhuma atividade disponível.\n");
        refresh();
        getch();
        return;
    }

    int selecionado = 0;
    int tecla;
    while (1) {
        limparTela();
        linha();
        printw_utf8("Atividades Disponíveis:\n");
        linha();
        for (int i = 0; i < num_ativ; i++) {
            if (i == selecionado) {
                attron(COLOR_PAIR(1));
                printw_utf8("> ID %d: %s (Turma ID: %d)\n", ativ_ids[i], ativ_descs[i], ativ_turmas[i]);
                attroff(COLOR_PAIR(1));
            } else {
                printw_utf8("  ID %d: %s (Turma ID: %d)\n", ativ_ids[i], ativ_descs[i], ativ_turmas[i]);
            }
        }
        linha();
        printw_utf8("Pressione ENTER para visualizar a atividade selecionada\n");
        printw_utf8("Pressione ESC para voltar ao menu anterior\n");
        linha();
        refresh();
        tecla = getch();

        switch (tecla) {
            case KEY_UP: 
                selecionado = (selecionado - 1 + num_ativ) % num_ativ; 
                break;
            case KEY_DOWN: 
                selecionado = (selecionado + 1) % num_ativ; 
                break;
            case 10:
                {
                    int atividade_id = ativ_ids[selecionado];
                    int turma_id = ativ_turmas[selecionado];
                    exibir_enunciado_atividade(atividade_id);
                    int resultado = menu_responder_atividade(atividade_id, turma_id);
                    if (resultado == 0) {
                        continue;
                    } else {
                        for (int i = 0; i < num_ativ; i++) free(ativ_descs[i]);
                        return;
                    }
                }
                break;
            case 27: 
                for (int i = 0; i < num_ativ; i++) free(ativ_descs[i]);
                return;
            default: 
                break;
        }
    }
}

/* LISTAR SUBMISSÕES COM VISUALIZADOR INTERATIVO - VERSÃO FINAL */
void listar_submissoes(int atividade_id) {
    limparTela();
    linha();
    printw_utf8("Carregando submissões da atividade %d...\n", atividade_id);
    refresh();
    napms(500);

    // === BUSCAR TURMA DA ATIVIDADE ===
    char* caminho_ativ = encontrar_arquivo_json("atividades.json");
    FILE* fp = fopen(caminho_ativ, "r");
    if (!fp) {
        attron(COLOR_PAIR(2)); printw_utf8("Erro: atividades.json não encontrado!\n"); attroff(COLOR_PAIR(2));
        getch(); return;
    }

    fseek(fp, 0, SEEK_END); long size = ftell(fp); fseek(fp, 0, SEEK_SET);
    char* buffer = malloc(size + 1); fread(buffer, 1, size, fp); buffer[size] = '\0'; fclose(fp);

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        attron(COLOR_PAIR(2)); printw_utf8("Erro ao ler atividades.json\n"); attroff(COLOR_PAIR(2));
        getch(); return;
    }

    int turma_id = -1;
    cJSON* atividades = cJSON_GetObjectItem(root, "atividades");
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, atividades) {
        cJSON* id_obj = cJSON_GetObjectItem(item, "atividade_id");
        if (cJSON_IsNumber(id_obj) && id_obj->valueint == atividade_id) {
            cJSON* t = cJSON_GetObjectItem(item, "turma_id");
            if (cJSON_IsNumber(t)) turma_id = t->valueint;
            break;
        }
    }
    cJSON_Delete(root);

    if (turma_id == -1) {
        attron(COLOR_PAIR(2)); printw_utf8("Atividade não encontrada!\n"); attroff(COLOR_PAIR(2));
        getch(); return;
    }

    char* nome_turma = obter_nome_turma_por_id(turma_id);
    const char* base = ROOT_PATH ? ROOT_PATH : "Z:\\python_embedded";
    char pasta_base[512];
    snprintf(pasta_base, sizeof(pasta_base), "%s\\submissoes\\%s", base, nome_turma);

    // === COLETAR TODAS AS SUBMISSÕES ===
    typedef struct {
        char matricula[32];
        char caminho_arquivo[512];
        char data_envio[20];
        char nome_arquivo[256];
    } Submissao;

    Submissao subs[200];
    int total = 0;

    WIN32_FIND_DATAA fd;
    char busca_alunos[512];
    snprintf(busca_alunos, sizeof(busca_alunos), "%s\\*", pasta_base);
    HANDLE hFind = FindFirstFileA(busca_alunos, &fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        limparTela(); linha();
        printw_utf8("Nenhuma submissão encontrada para esta atividade.\n");
        linha(); printw_utf8("Pressione qualquer tecla...\n"); getch();
        return;
    }

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;

        char pasta_aluno_ativ[512];
        snprintf(pasta_aluno_ativ, sizeof(pasta_aluno_ativ), "%s\\%s\\%d", pasta_base, fd.cFileName, atividade_id);

        if (_access(pasta_aluno_ativ, 0) != 0) continue;

        char busca_arquivos[512];
        snprintf(busca_arquivos, sizeof(busca_arquivos), "%s\\*", pasta_aluno_ativ);
        WIN32_FIND_DATAA fd2;
        HANDLE h2 = FindFirstFileA(busca_arquivos, &fd2);

        if (h2 == INVALID_HANDLE_VALUE) continue;

        do {
            if (fd2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

            if (total >= 200) break;

            strncpy(subs[total].matricula, fd.cFileName, 31);
            snprintf(subs[total].caminho_arquivo, sizeof(subs[total].caminho_arquivo), "%s\\%s", pasta_aluno_ativ, fd2.cFileName);
            strncpy(subs[total].nome_arquivo, fd2.cFileName, 255);

            // Extrair data do nome do arquivo
            char* p = strstr(fd2.cFileName, "resposta_");
            if (p) {
                p += 9;
                snprintf(subs[total].data_envio, 20, "%c%c/%c%c/%c%c %c%c:%c%c",
                         p[6], p[7], p[4], p[5], p[0], p[1], p[2], p[3], p[8], p[9], p[10], p[11]);
            } else {
                strcpy(subs[total].data_envio, "Desconhecida");
            }
            total++;
        } while (FindNextFileA(h2, &fd2));
        FindClose(h2);
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    if (total == 0) {
        limparTela(); linha();
        printw_utf8("Nenhuma submissão encontrada.\n");
        linha(); printw_utf8("Pressione qualquer tecla...\n"); getch();
        return;
    }

    // === MENU INTERATIVO DE SUBMISSÕES ===
    int selecionado = 0;
    int tecla;

    while (1) {
        limparTela();
        linha();
        printw_utf8("Submissões da Atividade %d - Turma: %s\n", atividade_id, nome_turma);
        linha();
        printw_utf8(" Matrícula      | Data/Hora        | Arquivo\n");
        linha();

        for (int i = 0; i < total; i++) {
            if (i == selecionado) attron(COLOR_PAIR(1));
            printw_utf8("%s %-14s | %-16s | %s\n",
                       i == selecionado ? "> " : "  ",
                       subs[i].matricula, subs[i].data_envio, subs[i].nome_arquivo);
            if (i == selecionado) attroff(COLOR_PAIR(1));
        }

        linha();
        printw_utf8("^v Navegar | ENTER Abrir arquivo | ESC Voltar\n");
        refresh();

        tecla = getch();
        if (tecla == KEY_UP)   selecionado = (selecionado - 1 + total) % total;
        if (tecla == KEY_DOWN) selecionado = (selecionado + 1) % total;
        if (tecla == 27) return; // ESC
        if (tecla == 10) { // ENTER
            limparTela();
            linha();
            printw_utf8("Visualizando: %s\n", subs[selecionado].nome_arquivo);
            printw_utf8("Aluno: %s\n", subs[selecionado].matricula);
            linha();

            // === ABRIR ARQUIVO (PDF ou TXT) ===
            char* ext = strrchr(subs[selecionado].nome_arquivo, '.');
            if (ext && (_stricmp(ext, ".pdf") == 0 || _stricmp(ext, ".txt") == 0)) {
                ShellExecuteA(NULL, "open", subs[selecionado].caminho_arquivo, NULL, NULL, SW_SHOWNORMAL);
                printw_utf8("Arquivo aberto no visualizador padrão!\n");
            } else {
                attron(COLOR_PAIR(2));
                printw_utf8("Formato não suportado para visualização.\n");
                attroff(COLOR_PAIR(2));
            }

            linha();
            printw_utf8("Pressione qualquer tecla para voltar à lista...\n");
            getch();
        }
    }
}

void ver_anuncios(void) {
    limparTela();
    linha();
    printw_utf8("=== Anúncios ===\n");
    linha();

    // --- 1. Coletar usernames dos professores que o aluno tem ---
    char* professores_permitidos[32] = {0};
    int num_professores = 0;

    char* caminho_turmas = encontrar_arquivo_json("turmas.json");
    FILE* ft = fopen(caminho_turmas, "r");
    cJSON* turmas_root = NULL;

    if (ft) {
        fseek(ft, 0, SEEK_END);
        long size = ftell(ft);
        fseek(ft, 0, SEEK_SET);
        char* buffer = malloc(size + 1);
        if (buffer) {
            fread(buffer, 1, size, ft);
            buffer[size] = '\0';
            turmas_root = cJSON_Parse(buffer);
            free(buffer);
        }
        fclose(ft);
    }

    if (turmas_root && cJSON_HasObjectItem(turmas_root, "turmas")) {
        cJSON* turmas = cJSON_GetObjectItem(turmas_root, "turmas");
        cJSON* turma;
        cJSON_ArrayForEach(turma, turmas) {
            cJSON* alunos = cJSON_GetObjectItem(turma, "alunos");
            if (!alunos) continue;

            int aluno_na_turma = 0;
            cJSON* aluno;
            cJSON_ArrayForEach(aluno, alunos) {
                cJSON* mat = cJSON_GetObjectItem(aluno, "matricula");
                if (mat && strcmp(mat->valuestring, aluno_matricula_logado) == 0) {
                    aluno_na_turma = 1;
                    break;
                }
            }
            if (!aluno_na_turma) continue;

            cJSON* prof = cJSON_GetObjectItem(turma, "professor");
            if (prof) {
                cJSON* username = cJSON_GetObjectItem(prof, "username");
                if (username && username->valuestring && num_professores < 32) {
                    int duplicado = 0;
                    for (int i = 0; i < num_professores; i++) {
                        if (strcmp(professores_permitidos[i], username->valuestring) == 0) {
                            duplicado = 1;
                            break;
                        }
                    }
                    if (!duplicado) {
                        professores_permitidos[num_professores++] = _strdup(username->valuestring);
                    }
                }
            }
        }
    }

    // --- 2. Carregar anúncios ---
    char* caminho_anuncios = encontrar_arquivo_json("anuncios.json");
    FILE* f = fopen(caminho_anuncios, "r");
    if (!f) {
        printw_utf8("Nenhum anúncio disponível.\n");
        goto limpar_memoria;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        printw_utf8("Erro de memória.\n");
        goto limpar_memoria;
    }

    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);

    if (!root || !cJSON_HasObjectItem(root, "anuncios")) {
        printw_utf8("Nenhum anúncio disponível.\n");
        if (root) cJSON_Delete(root);
        goto limpar_memoria;
    }

    int count = 0;
    cJSON* anuncios = cJSON_GetObjectItem(root, "anuncios");
    cJSON* anuncio;

    cJSON_ArrayForEach(anuncio, anuncios) {
        cJSON* data = cJSON_GetObjectItem(anuncio, "data");
        cJSON* username_prof = cJSON_GetObjectItem(anuncio, "professor");  // agora é username
        cJSON* msg = cJSON_GetObjectItem(anuncio, "mensagem");

        if (!data || !username_prof || !msg) continue;

        // Verifica permissão
        int permitido = 0;
        for (int i = 0; i < num_professores; i++) {
            if (strcmp(professores_permitidos[i], username_prof->valuestring) == 0) {
                permitido = 1;
                break;
            }
        }
        if (!permitido) continue;

        // Converte username → nome completo bonito
        char* nome_completo = username_para_nome_completo(username_prof->valuestring);
        char nome_exibir[128];
        if (nome_completo) {
            strcpy(nome_exibir, nome_completo);
            free(nome_completo);
        } else {
            strncpy(nome_exibir, username_prof->valuestring, sizeof(nome_exibir)-1);
            nome_exibir[sizeof(nome_exibir)-1] = '\0';
        }
        formatar_nome_proprio(nome_exibir);

        printw_utf8("[%s] %s:\n", data->valuestring, nome_exibir);
        printw_utf8(" %s\n\n", msg->valuestring);
        count++;
    }

    if (count == 0) {
        printw_utf8("Nenhum anúncio disponível para suas turmas.\n");
    }

    cJSON_Delete(root);

limpar_memoria:
    for (int i = 0; i < num_professores; i++) {
        free(professores_permitidos[i]);
    }
    if (turmas_root) cJSON_Delete(turmas_root);

    linha();
    printw_utf8("Pressione qualquer tecla para voltar...\n");
    getch();
}
