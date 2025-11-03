#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <curses.h>
#include "cJSON.h"
#include "utils.h"

// Declarações das funções do arquivos.c
extern void menu_atividades_aluno();
extern char* aluno_matricula_logado;

void ver_anuncios();

void menuLogado() {
    const char *opcoes[] = {"Visualizar Dados", "Notas e Faltas", "Atividades", "Ver Anúncios", "Sair"};
    int num_opcoes = 5;
    int selecionado = 0;
    int tecla;

    while (1) {
        limparTela();
        
        linha();
        printw_utf8("Bem-vindo à área do aluno!\n");
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
                if (selecionado == 0) {
                    limparTela();
                    printw_utf8("Funcionalidade de visualizar dados ainda não implementada.\n");
                    refresh();
                    napms(2000);
                } else if (selecionado == 1) {
                    limparTela();
                    printw_utf8("Funcionalidade de notas e faltas ainda não implementada.\n");
                    refresh();
                    napms(2000);
                } else if (selecionado == 2) {
                    // Menu de Atividades - Agora chama o menu completo de atividades do aluno
                    menu_atividades_aluno();
                } else if (selecionado == 3) { // Ver Anúncios
                    ver_anuncios();
                } else if (selecionado == 4) { // Sair
                    limparTela();
                    return;
                }
                break;	
            default:
                attron(COLOR_PAIR(2));
                printw_utf8("\nTecla inválida. Use setas ou Enter. Tente novamente...\n");
                attroff(COLOR_PAIR(2));
                refresh();
                napms(2000);
                break;
        }
    }
}

void ver_anuncios() {
    limparTela();
    linha();
    printw_utf8("Anúncios do Professor\n");
    linha();

    char* caminho_anuncios = encontrar_arquivo_json("anuncios.json");
	FILE* fp = fopen(caminho_anuncios, "r");
    if (!fp) {
        printw_utf8("Nenhum anúncio disponível.\n");
        linha();
        printw_utf8("Pressione qualquer tecla para voltar...\n");
        refresh();
        getch();
        return;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size <= 0) {
        fclose(fp);
        printw_utf8("Nenhum anúncio disponível.\n");
        linha();
        printw_utf8("Pressione qualquer tecla para voltar...\n");
        refresh();
        getch();
        return;
    }

    char* buffer = malloc(size + 1);
    if (!buffer) { 
        fclose(fp); 
        printw_utf8("Erro de memória.\n"); 
        refresh(); 
        getch(); 
        return; 
    }
    
    fread(buffer, 1, size, fp);
    buffer[size] = '\0';
    fclose(fp);

    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        printw_utf8("Erro ao ler anúncios.\n");
        linha();
        refresh();
        getch();
        return;
    }

    cJSON* anuncios = cJSON_GetObjectItem(root, "anuncios");
    if (!cJSON_IsArray(anuncios) || cJSON_GetArraySize(anuncios) == 0) {
        printw_utf8("Nenhum anúncio disponível.\n");
    } else {
        cJSON* item = NULL;
        int count = 0;
        cJSON_ArrayForEach(item, anuncios) {
            cJSON* data = cJSON_GetObjectItem(item, "data");
            cJSON* prof = cJSON_GetObjectItem(item, "professor");
            cJSON* msg = cJSON_GetObjectItem(item, "mensagem");
            if (cJSON_IsString(data) && cJSON_IsString(msg)) {
                count++;
                printw_utf8("[%d] %s\n", count, data->valuestring);
                printw_utf8("    %s\n", cJSON_IsString(prof) ? prof->valuestring : "Professor");
                printw_utf8("    %s\n\n", msg->valuestring);
            }
        }
        if (count == 0) {
            printw_utf8("Nenhum anúncio disponível.\n");
        }
    }

    cJSON_Delete(root);
    linha();
    printw_utf8("Pressione qualquer tecla para voltar...\n");
    refresh();
    getch();
} 