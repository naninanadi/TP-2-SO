#ifndef SISTEMADEARQUIVOS_H
#define SISTEMADEARQUIVOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "inode.h"
#include "blocoDeDados.h"
#include "diretorio.h"
#include "superBloco.h"

typedef struct {
    SuperBloco super;
    Inode inodes[MAX_INODES];
    Diretorio diretorios[MAX_INODES];
    Bloco *blocos;
    int raiz;
    int diretorioAtual;

    int qtdArquivos;
    int qtdDiretorios;

    int lixeira;

} SistemaDeArquivos;

void inicializarFS(SistemaDeArquivos *fs, int tamanhoDisco, int tamanhoBloco);
void destruirFS(SistemaDeArquivos *fs);
void pwd(SistemaDeArquivos *fs);
void obterCaminho(SistemaDeArquivos *fs, char *caminhoFinal);
void exibirArvore(SistemaDeArquivos *fs);

int criarDiretorio(SistemaDeArquivos *fs, char nome[]);
int removerDiretorio(SistemaDeArquivos *fs, char nome[]);
void listarDiretorio(SistemaDeArquivos *fs);
int entrarDiretorio(SistemaDeArquivos *fs, char nome[]);
int renomear(SistemaDeArquivos *fs, char antigo[], char novo[]);
int mover(SistemaDeArquivos *fs, char nome[], char destino[]);

int criarArquivo(SistemaDeArquivos *fs, char nome[]);
void listarConteudoArquivo(SistemaDeArquivos *fs, char nome[]);
int importarArquivo(SistemaDeArquivos *fs, char nomeSimulado[], char caminhoArquivo[]);
int apagar(SistemaDeArquivos *fs, char nome[]);
int apagarDaLixeira(SistemaDeArquivos *fs, char nome[]);
int restaurarDaLixeira(SistemaDeArquivos *fs, char nome[]);

int alocarBloco(SistemaDeArquivos *fs);
void liberarBloco(SistemaDeArquivos *fs, int bloco);
int escreverBloco(SistemaDeArquivos *fs, int bloco, const char *dados, int quantidadeBytes);
int lerBloco(SistemaDeArquivos *fs, int bloco, char *destino);

// void imprimirEstadoSistema(SistemaDeArquivos *fs);

extern int modo_verboso;
void log_verboso(const char *format, ...);
void exibirMapeamentoBlocos(SistemaDeArquivos *fs);
#endif