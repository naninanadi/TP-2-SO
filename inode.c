#include <string.h>
#include <stdio.h>
#include <time.h>

#include "headers/inode.h"
#include "headers/sistemaDeArquivos.h"

extern int modo_verboso;

/* Cria um novo i-node na tabela global */
int criarInode(Inode tabela[], TipoInode tipo, int pai) {
    for(int i = 0; i < MAX_INODES; i++){
        if(!tabela[i].usado){

            if (modo_verboso) {
                printf("\033[1;33m[VERBOSO]\033[0m inode.c -> Encontrado i-node livre no indice [%d]. Alocando...\n", i);
            }

            tabela[i].usado = 1;
            tabela[i].id = i;
            tabela[i].tipo = tipo;
            tabela[i].pai = pai;
            tabela[i].paiOriginal = pai;

            tabela[i].tamanho = 0;
            tabela[i].quantidadeBlocos = 0;
            
            for(int j = 0; j < MAX_BLOCOS_ARQUIVO; j++) {
                tabela[i].blocos[j] = -1;
            }

            tabela[i].criado = time(NULL);
            tabela[i].modificado = time(NULL);
            tabela[i].acessado = time(NULL);

            return i;
        }
    }

    return -1;
}

/* Liberta um i-node na tabela global */
void removerInode(Inode tabela[], int id){
    if (id >= 0 && id < MAX_INODES) {
        tabela[id].usado = 0;
    }
}

/* Exibe informacoes detalhadas de um i-node (Comando stat) */
void exibirInfosInode(Inode tabela[], int id) {
    if (id < 0 || id >= MAX_INODES || !tabela[id].usado) {
        printf("Ficheiro ou diretorio nao encontrado.\n");
        return;
    }

    struct tm *tm_info;
    char acesso[30], modificacao[30], mudanca[30], criacao[30];
    char *formato_linux = "%Y-%m-%d %H:%M:%S -0300";

    tm_info = localtime(&tabela[id].acessado);
    strftime(acesso, sizeof(acesso), formato_linux, tm_info);

    tm_info = localtime(&tabela[id].modificado);
    strftime(modificacao, sizeof(modificacao), formato_linux, tm_info);

    tm_info = localtime(&tabela[id].modificado); 
    strftime(mudanca, sizeof(mudanca), formato_linux, tm_info);

    tm_info = localtime(&tabela[id].criado);
    strftime(criacao, sizeof(criacao), formato_linux, tm_info);

    char tipo_string[20];
    if (tabela[id].tipo == ARQUIVO) {
        strcpy(tipo_string, "regular file");
    } else {
        strcpy(tipo_string, "directory");
    }

    printf("  File: Inode %d\n", id);
    printf("  Size: %-15d Blocks: %-10d IO Block: 512    %s\n", tabela[id].tamanho, tabela[id].quantidadeBlocos, tipo_string);
    printf("Device: simulated \tInode: %-10d Links: 1\n", id);
    printf("Access: %s\n", acesso);
    printf("Modify: %s\n", modificacao);
    printf("Change: %s\n", mudanca);
    printf(" Birth: %s\n", criacao);
}