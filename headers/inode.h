#ifndef INODE_H
#define INODE_H

#include <time.h>

#define MAX_NOME 50
#define MAX_BLOCOS 10
#define MAX_BLOCOS_ARQUIVO 10
#define MAX_INODES 256

typedef enum {
    ARQUIVO,
    DIRETORIO
} TipoInode;

typedef struct {

    int usado;
    int id;

    TipoInode tipo;

    char nome[MAX_NOME];

    int tamanho;

    int quantidadeBlocos;
    int blocos[MAX_BLOCOS_ARQUIVO];

    time_t criado;
    time_t modificado;
    time_t acessado;

    int pai;
    int paiOriginal;

} Inode;

int criarInode(Inode tabela[], TipoInode tipo, char nome[], int pai);

void removerInode(Inode tabela[], int id);

int buscarInodePorNome(Inode tabela[], char nome[]);

void exibirInfosInode(Inode tabela[], int id);

#endif