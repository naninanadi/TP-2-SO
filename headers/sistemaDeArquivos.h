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

    // Arquivo arquivos[MAX_INODES];

    Bloco *blocos;

    int raiz;
    int diretorioAtual;

    int qtdArquivos;
    int qtdDiretorios;

    int lixeira;

} SistemaDeArquivos;

#endif

void inicializarFS(SistemaDeArquivos *fs, int tamanhoDisco, int tamanhoBloco);

void destruirFS(SistemaDeArquivos *fs);
void pwd(SistemaDeArquivos *fs);
void obterCaminho(SistemaDeArquivos *fs, char *caminhoFinal);

//funções referentes a diretorio
int criarDiretorio(SistemaDeArquivos *fs, char nome[]);
int removerDiretorio(SistemaDeArquivos *fs, char nome[]);
void listarDiretorio(SistemaDeArquivos *fs);
int entrarDiretorio(SistemaDeArquivos *fs, char nome[]);

//funções referentes a arquivos
int criarArquivo(SistemaDeArquivos *fs, char nome[]);
void listarConteudoArquivo(SistemaDeArquivos *fs, char nome[]);
int importarArquivo(SistemaDeArquivos *fs, char nomeSimulado[], char caminhoArquivo[]);

//funções referentes a blocos
int alocarBloco(SistemaDeArquivos *fs);
void liberarBloco(SistemaDeArquivos *fs, int bloco);
int escreverBloco(SistemaDeArquivos *fs, int bloco, const char *dados, int quantidadeBytes);
int lerBloco(SistemaDeArquivos *fs, int bloco, char *destino);

// Funções gerais
int renomear(SistemaDeArquivos *fs, char nomeAtual[], char novoNome[]);
int mover(SistemaDeArquivos *fs, char nome[], char destino[]);
int apagar(SistemaDeArquivos *fs, char nome[]);
int apagarDaLixeira(SistemaDeArquivos *fs, char nome[]);
int restaurarDaLixeira(SistemaDeArquivos *fs, char nome[]);
void usoDoDisco(SistemaDeArquivos *fs);

//gerais

void imprimirEstadoSistema(SistemaDeArquivos *fs);
void exibirArvore(SistemaDeArquivos *fs);