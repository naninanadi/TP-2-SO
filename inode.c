#include <string.h>
#include <stdio.h>
#include <time.h>

#include "headers/inode.h"
#include "headers/sistemaDeArquivos.h"

extern int modo_verboso;

int criarInode(Inode tabela[], TipoInode tipo, char nome[], int pai){

    for(int i = 0; i < MAX_INODES; i++){
        if(!tabela[i].usado){

            if (modo_verboso) {
                printf("\033[1;33m[VERBOSO]\033[0m inode.c -> Encontrado i-node livre no índice [%d]. Alocando para '%s'.\n", i, nome);
            }

            tabela[i].usado = 1;

            tabela[i].id = i;

            tabela[i].tipo = tipo;

            strcpy(tabela[i].nome, nome);

            tabela[i].pai = pai;

            tabela[i].tamanho = 0;

            tabela[i].criado = time(NULL);

            tabela[i].modificado = time(NULL);

            tabela[i].acessado = time(NULL);

            return i;
        }
    }

    return -1;
}

void removerInode(Inode tabela[], int id){
    if (modo_verboso) {
            printf("\033[1;33m[VERBOSO]\033[0m inode.c -> Liberando i-node [%d] (Nome: '%s') na tabela de i-nodes.\n", id, tabela[id].nome);
    }
    tabela[id].usado = 0;
}

int buscarInodePorNome(Inode tabela[], char nome[]){
    
    for(int i = 0; i < MAX_INODES; i++){
        if(tabela[i].usado && strcmp(tabela[i].nome,nome) == 0){
            return i;
        }
    }

    return -1;
}

void exibirInfosInode(Inode tabela[], int id) {
    if (id < 0 || id >= MAX_INODES || !tabela[id].usado) {
        printf("Arquivo ou diretório não encontrado.\n");
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

    // Identifica o tipo comparando diretamente com o ENUM do seu sistema
    char tipo_string[20];
    if (tabela[id].tipo == ARQUIVO) {
        strcpy(tipo_string, "regular file");
    } else {
        strcpy(tipo_string, "directory");
    }

    // Print final corrigido e limpo
    printf("  File: %s\n", tabela[id].nome);
    printf("  Size: %-15d Blocks: %-15d Inode: %-15d Tipo: %s\n", 
           tabela[id].tamanho, 
           tabela[id].quantidadeBlocos, // Usei o nome exato que vi no seu criarArquivo!
           id,
           tipo_string);
    
    printf("Access: %s\n", acesso);
    printf("Modify: %s\n", modificacao);
    printf("Change: %s\n", mudanca);
    printf("Birth:  %s\n", criacao);
}