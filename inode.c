#include <string.h>
#include <time.h>

#include "inode.h"

int criarInode(Inode tabela[], TipoInode tipo, char nome[], int pai){

    for(int i = 0; i < MAX_INODES; i++){
        if(!tabela[i].usado){
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