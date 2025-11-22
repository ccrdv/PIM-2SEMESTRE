#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <curses.h>
#include "cJSON.h"
#include "utils.h"

// Externals
extern char* aluno_matricula_logado;  // Set by login in C
extern char* encontrar_arquivo_json(const char* filename);


void print_banner_aluno(){
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_WHITE, COLOR_BLACK);
    
    char *banner_lines[] = {
        ":::'###::::'##:::::::'##::::'##:'##::: ##::'#######::",
        "::'## ##::: ##::::::: ##:::: ##: ###:: ##:'##.... ##:",
        ":'##:. ##:: ##::::::: ##:::: ##: ####: ##: ##:::: ##:",
        "'##:::. ##: ##::::::: ##:::: ##: ## ## ##: ##:::: ##:",
        " #########: ##::::::: ##:::: ##: ##. ####: ##:::: ##:",
        " ##.... ##: ##::::::: ##:::: ##: ##:. ###: ##:::: ##:",
        " ##:::: ##: ########:. #######:: ##::. ##:. #######::",
        "..:::::..::........:::.......:::..::::..:::.......:::"
    };
    
    // Cores focadas em ciano/azul e amarelo
    int line_colors[] = {3, 4, 3, 5, 4, 3, 5, 4}; // Ciano(3), Amarelo(4), Azul(5)
    
    for (int i = 0; i < 8; i++) {
        attron(COLOR_PAIR(6));  // Branco como cor base
        
        for (int j = 0; banner_lines[i][j] != '\0'; j++) {
            if (banner_lines[i][j] == '#') {
                attroff(COLOR_PAIR(6));
                attron(COLOR_PAIR(line_colors[i]));
                addch('#');
                attroff(COLOR_PAIR(line_colors[i]));
                attron(COLOR_PAIR(6));
            } else {
                addch(banner_lines[i][j]);
            }
        }
        
        attroff(COLOR_PAIR(6));
        addch('\n');
    }
}

static void mostrar_dados_aluno() {
    limparTela();
    linha();
    printw_utf8("=== Seus Dados ===\n");
    linha();

    char* caminho = encontrar_arquivo_json("matriculas.json");
    FILE* f = fopen(caminho, "r");
    if (!f) {
        printw_utf8("Erro: Não foi possível abrir matriculas.json\n");
        linha();
        printw_utf8("Pressione qualquer tecla...\n");
        getch();
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) { fclose(f); return; }
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        printw_utf8("Erro ao ler dados do aluno.\n");
        linha();
        printw_utf8("Pressione qualquer tecla...\n");
        getch();
        return;
    }

    cJSON* aluno = cJSON_GetObjectItem(root, aluno_matricula_logado);
    if (!aluno) {
        cJSON_Delete(root);
        printw_utf8("Matrícula não encontrada.\n");
        linha();
        printw_utf8("Pressione qualquer tecla...\n");
        getch();
        return;
    }

    printw_utf8("Nome: %s\n", cJSON_GetObjectItem(aluno, "nome")->valuestring);
    printw_utf8("CPF: %s\n", cJSON_GetObjectItem(aluno, "documento")->valuestring);
    printw_utf8("Nascimento: %s\n", cJSON_GetObjectItem(aluno, "data_nascimento")->valuestring);
    printw_utf8("Telefone: %s\n", cJSON_GetObjectItem(aluno, "telefone")->valuestring);
    printw_utf8("Endereço: %s\n", cJSON_GetObjectItem(aluno, "endereco")->valuestring);
    printw_utf8("Sexo: %s\n", cJSON_GetObjectItem(aluno, "sexo")->valuestring);

    cJSON_Delete(root);
    linha();
    printw_utf8("Pressione qualquer tecla para voltar...\n");
    getch();
}

static void ver_notas() {
    limparTela();
    linha();
    printw_utf8("=== Suas Notas ===\n");
    linha();

    char* caminho = encontrar_arquivo_json("notas.json");
    FILE* f = fopen(caminho, "r");
    if (!f) {
        printw_utf8("Nenhuma nota registrada ainda.\n");
        linha();
        printw_utf8("Pressione qualquer tecla...\n");
        getch();
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) { fclose(f); return; }
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        printw_utf8("Erro ao ler notas.\n");
        linha();
        printw_utf8("Pressione qualquer tecla...\n");
        getch();
        return;
    }

    if (!cJSON_HasObjectItem(root, "notas")) {
        printw_utf8("Nenhuma nota registrada ainda.\n");
        cJSON_Delete(root);
        linha();
        printw_utf8("Pressione qualquer tecla...\n");
        getch();
        return;
    }

    cJSON* notas = cJSON_GetObjectItem(root, "notas");
    int found = 0;

    cJSON* nota;
    cJSON_ArrayForEach(nota, notas) {
        cJSON* mat = cJSON_GetObjectItem(nota, "matricula");
        if (!mat || strcmp(mat->valuestring, aluno_matricula_logado) != 0) continue;

        found = 1;
        cJSON* turma_id = cJSON_GetObjectItem(nota, "turma_id");
        cJSON* atv_id = cJSON_GetObjectItem(nota, "atividade_id");
        cJSON* valor = cJSON_GetObjectItem(nota, "nota");

        // Buscar descrição da atividade
        char* desc = "Atividade desconhecida";
        char* atv_caminho = encontrar_arquivo_json("atividades.json");
        FILE* af = fopen(atv_caminho, "r");
        if (af) {
            fseek(af, 0, SEEK_END); long asize = ftell(af); fseek(af, 0, SEEK_SET);
            char* abuf = (char*)malloc(asize + 1);
            if (abuf) {
                fread(abuf, 1, asize, af); abuf[asize] = '\0';
                cJSON* aroot = cJSON_Parse(abuf);
                if (aroot) {
                    cJSON* atvs = cJSON_GetObjectItem(aroot, "atividades");
                    if (atvs) {
                        cJSON* atv;
                        cJSON_ArrayForEach(atv, atvs) {
                            cJSON* aid = cJSON_GetObjectItem(atv, "atividade_id");
                            if (aid && aid->valueint == atv_id->valueint) {
                                cJSON* d = cJSON_GetObjectItem(atv, "descricao");
                                if (d && d->valuestring) desc = d->valuestring;
                                break;
                            }
                        }
                    }
                    cJSON_Delete(aroot);
                }
                free(abuf);
            }
            fclose(af);
        }

        printw_utf8("Turma: %d | Atividade: %s | Nota: %.1f\n",
                    turma_id ? turma_id->valueint : 0,
                    desc,
                    valor ? valor->valuedouble : 0.0);
    }

    if (!found) {
        printw_utf8("Você ainda não tem notas registradas.\n");
    }

    cJSON_Delete(root);
    linha();
    printw_utf8("Pressione qualquer tecla para voltar...\n");
    getch();
}

static void ver_presenca() {
    limparTela();
    linha();
    printw_utf8("=== Sua Frequência ===\n");
    linha();

    char* caminho = encontrar_arquivo_json("presencas.json");
    FILE* f = fopen(caminho, "r");
    if (!f) {
        printw_utf8("Nenhuma presença registrada ainda.\n");
        linha();
        printw_utf8("Pressione qualquer tecla...\n");
        getch();
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) { fclose(f); return; }
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (!root || !cJSON_HasObjectItem(root, "presencas")) {
        printw_utf8("Nenhuma presença registrada ainda.\n");
        linha();
        printw_utf8("Pressione qualquer tecla...\n");
        getch();
        return;
    }

    cJSON* presencas = cJSON_GetObjectItem(root, "presencas");
    int found = 0;
    cJSON* reg;
    cJSON_ArrayForEach(reg, presencas) {
        cJSON* data = cJSON_GetObjectItem(reg, "data");
        cJSON* turma_id = cJSON_GetObjectItem(reg, "turma_id");
        cJSON* lista = cJSON_GetObjectItem(reg, "presencas");

        int presente = 0;
        cJSON* item;
        cJSON_ArrayForEach(item, lista) {
            cJSON* mat = cJSON_GetObjectItem(item, "matricula");
            if (mat && strcmp(mat->valuestring, aluno_matricula_logado) == 0) {
                presente = cJSON_IsTrue(cJSON_GetObjectItem(item, "presente"));
                found = 1;
                break;
            }
        }

        if (found) {
            printw_utf8("Data: %s | Turma ID: %d | Status: %s\n",
                        data->valuestring, turma_id->valueint,
                        presente ? "PRESENTE" : "AUSENTE");
        }
    }

    if (!found) {
        printw_utf8("Nenhuma presença registrada.\n");
    }

    cJSON_Delete(root);
    linha();
    printw_utf8("Pressione qualquer tecla para voltar...\n");
    getch();
}

void menuLogadoAluno() {
    const char* opcoes[] = {
        "Visualizar Dados",
        "Ver Anúncios",
        "Ver Notas",
        "Ver Presença",
        "Atividades",
        "Sair"
    };
    int num_opcoes = 6;
    int selecionado = 0;
    int tecla;
	init_pair(7, COLOR_MAGENTA, COLOR_BLACK);
    while (1) {
        limparTela();
        linha();
        print_banner_aluno();
        linha();
        attron(COLOR_PAIR(7));
        printw_utf8("Bem-vindo, aluno!\n");
        attroff(COLOR_PAIR(7));
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
            case 10: // Enter
                switch (selecionado) {
                    case 0: mostrar_dados_aluno(); break;
                    case 1: ver_anuncios(); break;
                    case 2: ver_notas(); break;
                    case 3: ver_presenca(); break;
                    case 4: menu_atividades_aluno(); break;
                    case 5: limparTela(); return;
                }
                break;
            default:
                break;
        }
    }
}