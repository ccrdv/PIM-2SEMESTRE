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

// === NOVA FUNÇÃO: Obter nome da turma por ID ===
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

void menu_criar_atividade() {
    limparTela();
    linha();
    printw_utf8("Criar Nova Atividade\n");
    linha();

    char* caminho_turmas = encontrar_arquivo_json("turmas.json");
    FILE* fp = fopen(caminho_turmas, "r");
    if (!fp) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: turmas.json não encontrado.\n");
        attroff(COLOR_PAIR(2));
        refresh();
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
    if (!root) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro ao ler turmas.json.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        getch();
        return;
    }

    cJSON* turmas = cJSON_GetObjectItem(root, "turmas");
    if (!cJSON_IsArray(turmas)) {
        cJSON_Delete(root);
        attron(COLOR_PAIR(2));
        printw_utf8("Formato inválido em turmas.json.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        getch();
        return;
    }

    int num_turmas = 0;
    int turma_ids[100];
    char* turma_nomes[100] = {0};
    cJSON* turma = NULL;
    cJSON_ArrayForEach(turma, turmas) {
        cJSON* prof = cJSON_GetObjectItem(turma, "professor");
        if (prof) {
            const char* professor_nome = NULL;
            
            if (cJSON_IsString(prof)) {
                professor_nome = prof->valuestring;
            } 
            else if (cJSON_IsObject(prof)) {
                cJSON* prof_nome = cJSON_GetObjectItem(prof, "nome");
                if (cJSON_IsString(prof_nome)) {
                    professor_nome = prof_nome->valuestring;
                }
            }
            
            if (professor_nome && _stricmp(professor_nome, nome_professor_logado) == 0) {
                cJSON* id = cJSON_GetObjectItem(turma, "id");
                cJSON* nome = cJSON_GetObjectItem(turma, "nome_turma");
                if (!nome) {
                    nome = cJSON_GetObjectItem(turma, "nome");
                }
                if (cJSON_IsNumber(id) && cJSON_IsString(nome)) {
                    turma_ids[num_turmas] = id->valueint;
                    turma_nomes[num_turmas] = strdup(nome->valuestring);
                    num_turmas++;
                }
            }
        }
    }
    cJSON_Delete(root);

    if (num_turmas == 0) {
        printw_utf8("Nenhuma turma encontrada para você.\n");
        refresh();
        getch();
        return;
    }

    int selecionado = 0;
    int tecla;
    while (1) {
        limparTela();
        linha();
        printw_utf8("Selecione a Turma:\n");
        linha();
        for (int i = 0; i < num_turmas; i++) {
            if (i == selecionado) {
                attron(COLOR_PAIR(1));
                printw_utf8("> %s (ID: %d)\n", turma_nomes[i], turma_ids[i]);
                attroff(COLOR_PAIR(1));
            } else {
                printw_utf8("  %s (ID: %d)\n", turma_nomes[i], turma_ids[i]);
            }
        }
        linha();
        refresh();
        tecla = getch();

        switch (tecla) {
            case KEY_UP: selecionado = (selecionado - 1 + num_turmas) % num_turmas; break;
            case KEY_DOWN: selecionado = (selecionado + 1) % num_turmas; break;
            case 10: 
                goto turma_selecionada;
            default: break;
        }
    }

turma_selecionada:
    ;
    int turma_id = turma_ids[selecionado];

    for (int i = 0; i < num_turmas; i++) free(turma_nomes[i]);

    char caminho[256];
    char desc[512];
    limparTela();
    linha();
    printw_utf8("Digite o caminho do arquivo TXT da atividade, sem aspas:\n");
    refresh();
    echo();
    getstr(caminho);
    printw_utf8("Digite a descrição (opcional):\n");
    getnstr(desc, 511);
    noecho();

    int resultado = criar_atividade(caminho, desc, turma_id);
    if (resultado == 0) {
        attron(COLOR_PAIR(1));
        printw_utf8("Atividade criada com sucesso!\n");
        attroff(COLOR_PAIR(1));
    } else {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro ao criar atividade. Código: %d\n", resultado);
        attroff(COLOR_PAIR(2));
    }
    refresh();
    getch();
}

// === NOVA FUNÇÃO: Visualizador interativo com paginação ===
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
            printw_utf8("← Anterior | → Próxima | ");
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
                start_color();
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
                    printw_utf8("Digite o caminho do seu arquivo de resposta, sem aspas (TXT ou PDF):\n");
                    refresh();
                    echo();
                    getstr(caminho);
                    noecho();
                    
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

void listar_submissoes(int atividade_id) {
    PyObject *pName = NULL, *pModule = NULL, *pFunc = NULL, *pArgs = NULL, *pValue = NULL;
    PyObject *pStdout = NULL, *pStringIO = NULL;
    PyObject *sys_module = NULL, *old_stdout = NULL;
    char *output = NULL;

    // === ADICIONA CAMINHO AO sys.path ===
    const char* base = ROOT_PATH ? ROOT_PATH : "Z:\\python_embedded";
    wchar_t w_base[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, base, -1, w_base, MAX_PATH);

    sys_module = PyImport_ImportModule("sys");
    if (!sys_module) {
        PyErr_Print();
        goto show_error;
    }

    PyObject* sys_path = PyObject_GetAttrString(sys_module, "path");
    if (!sys_path) goto show_error;

    // Verifica se já está no path
    int already_in_path = 0;
    Py_ssize_t path_len = PyList_Size(sys_path);
    for (Py_ssize_t i = 0; i < path_len; i++) {
        PyObject* item = PyList_GetItem(sys_path, i);
        const char* path_str = PyUnicode_AsUTF8(item);
        if (path_str && _stricmp(path_str, base) == 0) {
            already_in_path = 1;
            break;
        }
    }

    if (!already_in_path) {
        PyList_Append(sys_path, PyUnicode_FromWideChar(w_base, -1));
    }

    // === REDIRECIONA STDOUT ===
    pStdout = PyImport_ImportModule("io");
    if (!pStdout) goto show_error;

    pStringIO = PyObject_CallMethod(pStdout, "StringIO", NULL);
    if (!pStringIO) goto show_error;

    old_stdout = PyObject_GetAttrString(sys_module, "stdout");
    PyObject_SetAttrString(sys_module, "stdout", pStringIO);

    // === IMPORTA MÓDULO ===
    pName = PyUnicode_FromString("listarsubmissoes");
    pModule = PyImport_Import(pName);
    if (!pModule) {
        PyErr_Print();
        goto cleanup;
    }

    pFunc = PyObject_GetAttrString(pModule, "listar_submissoes");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        goto cleanup;
    }

    pArgs = PyTuple_Pack(1, PyLong_FromLong(atividade_id));
    pValue = PyObject_CallObject(pFunc, pArgs);
    if (!pValue) {
        PyErr_Print();
        goto cleanup;
    }
    Py_DECREF(pValue);

    // === CAPTURA SAÍDA ===
    PyObject* pGetValue = PyObject_CallMethod(pStringIO, "getvalue", NULL);
    if (pGetValue) {
        output = _strdup(PyUnicode_AsUTF8(pGetValue));
        Py_DECREF(pGetValue);
    }

cleanup:
    // Restaura stdout
    if (sys_module && old_stdout) {
        PyObject_SetAttrString(sys_module, "stdout", old_stdout);
    }

    Py_XDECREF(pFunc);
    Py_XDECREF(pModule);
    Py_XDECREF(pStringIO);
    Py_XDECREF(pStdout);
    Py_XDECREF(sys_module);
    Py_XDECREF(old_stdout);
    Py_XDECREF(pArgs);
    Py_XDECREF(pName);

    // === EXIBE RESULTADO ===
    limparTela();
    linha();
    printw_utf8("Submissões da Atividade %d\n", atividade_id);
    linha();

    if (output && strlen(output) > 0) {
        printw_utf8("%s", output);
    } else {
        printw_utf8("Nenhuma submissão encontrada.\n");
    }

    free(output);
    linha();
    printw_utf8("Pressione qualquer tecla para continuar...\n");
    refresh();
    getch();
    return;

show_error:
    limparTela();
    linha();
    printw_utf8("ERRO CRÍTICO: Python não inicializado corretamente.\n");
    linha();
    refresh();
    getch();
}

void ver_anuncios(void) {
    limparTela();
    linha();
    printw_utf8("=== Anúncios ===\n");
    linha();

    // --- 1. Carregar turmas.json para mapear aluno -> professores ---
    char* caminho_turmas = encontrar_arquivo_json("turmas.json");
    FILE* ft = fopen(caminho_turmas, "r");
    cJSON* turmas_root = NULL;
    char* professores_permitidos[32] = {0};
    int num_professores = 0;

    if (ft) {
        fseek(ft, 0, SEEK_END);
        long size = ftell(ft);
        fseek(ft, 0, SEEK_SET);
        char* buffer = (char*)malloc(size + 1);
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

            cJSON* aluno;
            cJSON_ArrayForEach(aluno, alunos) {
                cJSON* mat = cJSON_GetObjectItem(aluno, "matricula");
                if (mat && strcmp(mat->valuestring, aluno_matricula_logado) == 0) {
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
                                professores_permitidos[num_professores++] = username->valuestring;
                            }
                        }
                    }
                    break;
                }
            }
        }
    }

    // --- 2. Carregar e filtrar anuncios.json ---
    char* caminho = encontrar_arquivo_json("anuncios.json");
    FILE* f = fopen(caminho, "r");
    if (!f) {
        printw_utf8("Nenhum anúncio disponível.\n");
        if (turmas_root) cJSON_Delete(turmas_root);
        linha();
        printw_utf8("Pressione qualquer tecla para voltar...\n");
        getch();
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(f);
        if (turmas_root) cJSON_Delete(turmas_root);
        printw_utf8("Erro de memória.\n");
        linha();
        printw_utf8("Pressione qualquer tecla...\n");
        getch();
        return;
    }

    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);

    if (!root || !cJSON_HasObjectItem(root, "anuncios")) {
        printw_utf8("Nenhum anúncio disponível.\n");
        if (root) cJSON_Delete(root);
        if (turmas_root) cJSON_Delete(turmas_root);
        linha();
        printw_utf8("Pressione qualquer tecla para voltar...\n");
        getch();
        return;
    }

    cJSON* anuncios = cJSON_GetObjectItem(root, "anuncios");
    cJSON* anuncio;
    int count = 0;

    char* nomes_professores[32] = {0};
    num_professores = 0;

    if (turmas_root) {
        cJSON* turmas = cJSON_GetObjectItem(turmas_root, "turmas");
        cJSON* turma;
        cJSON_ArrayForEach(turma, turmas) {
            cJSON* alunos = cJSON_GetObjectItem(turma, "alunos");
            if (!alunos) continue;

            int aluno_encontrado = 0;
            cJSON* aluno;
            cJSON_ArrayForEach(aluno, alunos) {
                cJSON* mat = cJSON_GetObjectItem(aluno, "matricula");
                if (mat && strcmp(mat->valuestring, aluno_matricula_logado) == 0) {
                    aluno_encontrado = 1;
                    break;
                }
            }
            if (!aluno_encontrado) continue;

            cJSON* prof = cJSON_GetObjectItem(turma, "professor");
            if (prof) {
                cJSON* nome_prof = cJSON_GetObjectItem(prof, "nome");
                if (nome_prof && nome_prof->valuestring && num_professores < 32) {
                    int duplicado = 0;
                    for (int i = 0; i < num_professores; i++) {
                        if (strcmp(nomes_professores[i], nome_prof->valuestring) == 0) {
                            duplicado = 1;
                            break;
                        }
                    }
                    if (!duplicado) {
                        nomes_professores[num_professores++] = _strdup(nome_prof->valuestring);
                    }
                }
            }
        }
    }

    count = 0;
    cJSON_ArrayForEach(anuncio, anuncios) {
        cJSON* data = cJSON_GetObjectItem(anuncio, "data");
        cJSON* prof_nome = cJSON_GetObjectItem(anuncio, "professor");
        cJSON* msg = cJSON_GetObjectItem(anuncio, "mensagem");

        if (!data || !prof_nome || !msg) continue;

        int permitido = 0;
        for (int i = 0; i < num_professores; i++) {
            if (strcmp(nomes_professores[i], prof_nome->valuestring) == 0) {
                permitido = 1;
                break;
            }
        }

        if (permitido) {
            count++;
            printw_utf8("[%s] %s:\n", data->valuestring, prof_nome->valuestring);
            printw_utf8("  %s\n\n", msg->valuestring);
        }
    }

    if (count == 0) {
        printw_utf8("Nenhum anúncio disponível para suas turmas.\n");
    }

    for (int i = 0; i < num_professores; i++) {
        free(nomes_professores[i]);
    }
    cJSON_Delete(root);
    if (turmas_root) cJSON_Delete(turmas_root);

    linha();
    printw_utf8("Pressione qualquer tecla para voltar...\n");
    getch();
}

void menu_atividades_professor() {
    const char *opcoes[] = {"Criar Atividade", "Ver Submissões", "Voltar"};
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
        refresh();

        tecla = getch();
        switch (tecla) {
            case KEY_UP: selecionado = (selecionado - 1 + num_opcoes) % num_opcoes; break;
            case KEY_DOWN: selecionado = (selecionado + 1) % num_opcoes; break;
            case 10:
                if (selecionado == 0) {
                    menu_criar_atividade();
                } else if (selecionado == 1) {
                    limparTela();
                    int atividade_id;
                    linha();
                    printw_utf8("Ver Submissões\n");
                    linha();
                    printw_utf8("Digite o ID da atividade:\n");
                    refresh();
                    echo();
                    if (scanw("%d", &atividade_id) != 1) {
                        noecho();
                        attron(COLOR_PAIR(2));
                        printw_utf8("Entrada inválida!\n");
                        attroff(COLOR_PAIR(2));
                        napms(1500);
                        continue;
                    }
                    noecho();
                    listar_submissoes(atividade_id);
                    printw_utf8("\nPressione qualquer tecla para continuar...\n");
                    refresh();
                    getch();
                } else if (selecionado == 2) {
                    return;
                }
                break;
            default: break;
        }
    }
}