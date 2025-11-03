// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <time.h>

void printw_utf8(const char* format, ...);
void limparTela(void);
void linha(void);
void menuLogado(void);
void listar_submissoes(int atividade_id);
void menuLogadoProf(void);
int rodarPythonLoginProfessor(void);
int postar_anuncio(const char* mensagem);
void ver_anuncios(void);
char* capturar_nome_professor();
extern char* nome_professor_logado;
extern char* aluno_matricula_logado; 
char* cp_to_utf8(const char* src);
char* encontrar_arquivo_json(const char* filename);
char* get_caminho_embedded(const char* filename);
int atualizar_json_submissao(int atividade_id, const char* aluno_matricula, const char* caminho_arquivo);
int criar_atividade(const char* caminho_txt, const char* descricao, int turma_id);
int get_next_atividade_id(void);
void exibir_enunciado_atividade(int atividade_id);
int menu_responder_atividade(int atividade_id);
void menu_atividades_aluno(void);

#endif