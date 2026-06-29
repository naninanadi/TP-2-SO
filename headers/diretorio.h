#ifndef DIRETORIO_H
#define DIRETORIO_H

#define MAX_FILHOS 100
#define MAX_NOME 50

typedef struct {
    char nome[MAX_NOME];
    int inodeId;
} EntradaDiretorio;

typedef struct {
    int qtdFilhos;
    EntradaDiretorio filhos[MAX_FILHOS];
} Diretorio;

#endif