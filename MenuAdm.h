#ifndef MENUADM_H
#define MENUADM_H

#include <curses.h>

void menuAdmin();
void menuRemoverUsuario();
void menuGerenciarMatriculas();
void menuCriarTurma();
void menuGerenciarTurmas();
void menuRelatorios();

// Funções Python wrapper para ADM
int pythonRegister();
int pythonRemoverUsuario(const char* documento);
int pythonCriarTurma();
int pythonGerenciarTurmas();

// Funções auxiliares
void mostrarMensagem(const char* mensagem, int cor);
void aguardarEnter();

#endif