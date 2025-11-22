// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <time.h>
#include <wchar.h>
#include <Python.h>

// === GLOBALS ===
extern char* nome_professor_logado;
extern char* aluno_matricula_logado;
extern char* ROOT_PATH;

// === UI ===
void limparTela(void);
void linha(void);

// === MENUS ===
void menuPrincipal(void);
void menuAluno(void);
void menuProfessor(void);
void menuLogadoProf(void);
void menuLogadoAluno(void);
void menu_atividades_aluno(void);

// === PYTHON BRIDGE ===
int rodarPythonRegistrar(void);
int rodarPythonLogin(void);
int rodarPythonLoginProfessor(void);
int python_registrar_presenca(const char *username);
int python_atribuir_notas(const char *username);

// === FILE / JSON ===
void garantir_arquivos_json(void);
char* encontrar_arquivo_json(const char* filename);
char* get_caminho_embedded(const char* filename);
int postar_anuncio(const char* mensagem);
void ver_anuncios(void);
int atualizar_json_submissao(int atividade_id, const char* aluno_matricula, const char* caminho_arquivo, int turma_id);

// === ACTIVITIES ===
void listar_submissoes(int atividade_id);
int criar_atividade(const char* caminho_txt, const char* descricao, int turma_id);
int get_next_atividade_id(void);
void exibir_enunciado_atividade(int atividade_id);
int menu_responder_atividade(int atividade_id, int turma_id);

// === TURMA MANAGEMENT ===
char* obter_nome_turma_por_id(int turma_id);
int enviar_arquivo(const char* caminho_origem, const char* aluno_matricula, int atividade_id, int turma_id);

// === PYTHON INIT ===
wchar_t* get_exe_dir(void);
wchar_t* wcs_concat(const wchar_t* a, const wchar_t* b);
int init_python_portable(void);
void debug_python_files(void);
int load_config(void);
PyObject* safe_import_module(const char* module_name);

// === UTF8 ===
void printw_utf8(const char* format, ...);
char* cp_to_utf8(const char* src);

#endif