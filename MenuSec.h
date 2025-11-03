#ifndef MENUSEC_H
#define MENUSEC_H


void menuSecretaria();
void menuCadastroMatricula();
void menuBuscarMatricula();
void menuBuscarDocumento();
void menuEditarMatricula();

// Funções Python wrapper para Secretaria
int pythonCriarMatricula();
int pythonEditarMatricula();
int pythonBuscarPorMatricula();
int pythonBuscarPorDocumento();



#endif