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
#include "cJSON.h"
#include "utils.h"

extern char *ROOT_PATH;
extern char* nome_professor_logado;
extern char* aluno_matricula_logado;

char* encontrar_arquivo_json(const char* filename) {
    static char path[512];
    
    const char *base = ROOT_PATH ? ROOT_PATH : "python_embedded";
    snprintf(path, sizeof(path), "%s\\%s", base, filename);
    
    if (_access(path, 0) == 0) {
        return path;
    }
    
    snprintf(path, sizeof(path), "Z:\\python_embedded\\%s", filename);
    if (_access(path, 0) == 0) {
        printf("INFO: Usando arquivo de Z: %s\n", filename);
        return path;
    }
    
    snprintf(path, sizeof(path), "python_embedded\\%s", filename);
    return path;
}

char* get_caminho_embedded(const char* filename) {
    return encontrar_arquivo_json(filename);
}
int enviar_arquivo(const char* caminho_origem, const char* aluno_matricula, int atividade_id) {

    const char* ext = strrchr(caminho_origem, '.');
    if (!ext) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Arquivo sem extensão.\n");
        attroff(COLOR_PAIR(2));
        linha();
        refresh();
        napms(2000);
        return -3;
    }
    ext++;

    char ext_lower[5];
    strncpy(ext_lower, ext, 4);
    ext_lower[4] = '\0';
    for (int i = 0; ext_lower[i]; i++) ext_lower[i] = tolower(ext_lower[i]);

    if (strcmp(ext_lower, "txt") != 0 && strcmp(ext_lower, "pdf") != 0) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Extensão inválida. Use TXT ou PDF.\n");
        attroff(COLOR_PAIR(2));
        linha();
        refresh();
        napms(2000);
        return -4;
    }

    const char* base_dir = ROOT_PATH ? ROOT_PATH : "Z:\\python_embedded";
    
    char pasta_sub[512];
    snprintf(pasta_sub, sizeof(pasta_sub), "%s\\submissoes\\%s\\%d\\", base_dir, aluno_matricula, atividade_id);

    char temp[512];
    
    snprintf(temp, sizeof(temp), "%s\\submissoes", base_dir);
    _mkdir(temp);
    
    snprintf(temp, sizeof(temp), "%s\\submissoes\\%s", base_dir, aluno_matricula);
    _mkdir(temp);
    
    _mkdir(pasta_sub);

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char nome_dest[512];
    snprintf(nome_dest, sizeof(nome_dest), "%sresposta_%04d%02d%02d_%02d%02d.%s",
             pasta_sub, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, ext_lower);

    FILE* src = fopen(caminho_origem, "rb");
    if (!src) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Arquivo de origem não encontrado.\n");
        attroff(COLOR_PAIR(2));
        linha();
        refresh();
        napms(2000);
        return -1;
    }
    FILE* dest = fopen(nome_dest, "wb");
    if (!dest) {
        fclose(src);
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Não foi possível criar o arquivo de destino.\n");
        attroff(COLOR_PAIR(2));
        linha();
        refresh();
        napms(2000);
        return -2;
    }

    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }
    fclose(src);
    fclose(dest);

    if (atualizar_json_submissao(atividade_id, aluno_matricula, nome_dest) != 0) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Falha ao atualizar o JSON.\n");
        attroff(COLOR_PAIR(2));
        linha();
        refresh();
        napms(2000);
        return -5;
    }

    attron(COLOR_PAIR(1));
    printw_utf8("Arquivo enviado com sucesso!\n");
    attroff(COLOR_PAIR(1));
    linha();
    refresh();
    napms(2000);
    return 0;
}

int criar_atividade(const char* caminho_txt, const char* descricao, int turma_id) {

    const char* ext = strrchr(caminho_txt, '.');
    if (!ext || _stricmp(ext, ".txt") != 0) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Apenas arquivos .txt são permitidos.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        napms(2000);
        return -1;
    }

    if (_access(caminho_txt, 0) != 0) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Arquivo não encontrado: %s\n", caminho_txt);
        attroff(COLOR_PAIR(2));
        refresh();
        napms(2000);
        return -2;
    }

    int atividade_id = get_next_atividade_id();

    const char* base_dir = ROOT_PATH ? ROOT_PATH : "Z:\\python_embedded";
    
    char pasta[512];
    snprintf(pasta, sizeof(pasta), "%s\\atividades\\%d", base_dir, atividade_id);

    char temp[512];
    snprintf(temp, sizeof(temp), "%s\\atividades", base_dir);
    _mkdir(temp);
    
    _mkdir(pasta);

    char destino[512];
    snprintf(destino, sizeof(destino), "%s\\enunciado.txt", pasta);
    if (!CopyFile(caminho_txt, destino, FALSE)) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Falha ao copiar arquivo.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        napms(2000);
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

int atualizar_json_submissao(int atividade_id, const char* aluno_matricula, const char* caminho_arquivo) {
    char* caminho_ativ = encontrar_arquivo_json("atividades.json");
    FILE* fp = fopen(caminho_ativ, "r");
    if (!fp) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Arquivo atividades.json não encontrado.\n");
        attroff(COLOR_PAIR(2));
        linha();
        refresh();
        napms(2000);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(fp);
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Falha ao alocar memória para ler o JSON.\n");
        attroff(COLOR_PAIR(2));
        linha();
        refresh();
        napms(2000);
        return -2;
    }

    fread(buffer, 1, file_size, fp);
    buffer[file_size] = '\0';
    fclose(fp);

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Falha ao parsear o JSON.\n");
        attroff(COLOR_PAIR(2));
        linha();
        refresh();
        napms(2000);
        return -3;
    }

    cJSON* atividades = cJSON_GetObjectItem(root, "atividades");
    if (!cJSON_IsArray(atividades)) {
        cJSON_Delete(root);
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Item 'atividades' não é um array no JSON.\n");
        attroff(COLOR_PAIR(2));
        linha();
        refresh();
        napms(2000);
        return -4;
    }

    cJSON* atividade = NULL;
    int found = 0;
    cJSON_ArrayForEach(atividade, atividades) {
        cJSON* id = cJSON_GetObjectItem(atividade, "atividade_id");
        if (cJSON_IsNumber(id) && id->valueint == atividade_id) {
            found = 1;
            cJSON* submissoes = cJSON_GetObjectItem(atividade, "submissoes");
            if (!cJSON_IsArray(submissoes)) {
                cJSON_Delete(root);
                return -5;
            }

            cJSON* submissao = cJSON_CreateObject();
            cJSON_AddStringToObject(submissao, "aluno_id", aluno_matricula);
            cJSON_AddStringToObject(submissao, "arquivo", caminho_arquivo);

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
    printw_utf8("Digite o caminho do arquivo TXT da atividade:\n");
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

void exibir_enunciado_atividade(int atividade_id) {
    limparTela();
    linha();
    printw_utf8("Enunciado da Atividade ID: %d\n", atividade_id);
    linha();
    
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
            if (cJSON_IsString(desc)) {
                printw_utf8("Descrição: %s\n\n", desc->valuestring);
            }
            
            cJSON* enunciado_path = cJSON_GetObjectItem(ativ, "enunciado");
            if (cJSON_IsString(enunciado_path)) {
                printw_utf8("Enunciado:\n");
                linha();
                
                FILE* fp_enunciado = fopen(enunciado_path->valuestring, "r");
                if (fp_enunciado) {
                    char linha_texto[256];
                    while (fgets(linha_texto, sizeof(linha_texto), fp_enunciado)) {
                        printw_utf8("%s", linha_texto);
                    }
                    fclose(fp_enunciado);
                } else {
                    attron(COLOR_PAIR(2));
                    printw_utf8("Erro: Não foi possível abrir o arquivo do enunciado.\n");
                    printw_utf8("Caminho: %s\n", enunciado_path->valuestring);
                    attroff(COLOR_PAIR(2));
                }
            } else {
                attron(COLOR_PAIR(2));
                printw_utf8("Erro: Enunciado não encontrado para esta atividade.\n");
                attroff(COLOR_PAIR(2));
            }
            break;
        }
    }
    
    if (!encontrada) {
        attron(COLOR_PAIR(2));
        printw_utf8("Erro: Atividade não encontrada.\n");
        attroff(COLOR_PAIR(2));
    }
    
    cJSON_Delete(root_ativ);
    linha();
    refresh();
}

int menu_responder_atividade(int atividade_id) {
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
                    
                    int resultado = enviar_arquivo(caminho, aluno_matricula_logado, atividade_id);
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
                printw_utf8("> ID %d: %s\n", ativ_ids[i], ativ_descs[i]);
                attroff(COLOR_PAIR(1));
            } else {
                printw_utf8("  ID %d: %s\n", ativ_ids[i], ativ_descs[i]);
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
                    exibir_enunciado_atividade(atividade_id);
                    int resultado = menu_responder_atividade(atividade_id);
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

    PyObject *pName = PyUnicode_FromString("listar_submissoes_module"); 
    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (!pModule) {
        PyErr_Print();
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        printw_utf8("ERRO: Falha ao carregar o módulo Python.\n");
        refresh();
        getch();
        return;
    }

    PyObject *pFunc = PyObject_GetAttrString(pModule, "listar_submissoes");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        printw_utf8("ERRO: Função 'listar_submissoes' não encontrada no módulo.\n");
        refresh();
        getch();
        Py_DECREF(pFunc); 
        return;
    }

    PyObject *pArgs = PyTuple_Pack(1, PyLong_FromLong(atividade_id));
    PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);

    if (!pValue) {
        PyErr_Print();
        initscr(); keypad(stdscr, TRUE); cbreak(); noecho();
        start_color(); init_pair(1, COLOR_GREEN, COLOR_BLACK); init_pair(2, COLOR_RED, COLOR_BLACK);
        attron(COLOR_PAIR(2));
        printw_utf8("ERRO: Falha ao chamar a função Python.\n");
        attroff(COLOR_PAIR(2));
        refresh();
        getch();
    } else {
        Py_XDECREF(pValue);
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
} //K