#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <io.h>
#include "headers/sistemaDeArquivos.h"

void configurarSistema(SistemaDeArquivos *fs);
void terminal(SistemaDeArquivos *fs, FILE *input);

int main(int argc, char *argv[]){
    SistemaDeArquivos fs;
    configurarSistema(&fs);
    if (argc == 2){
        FILE *f = fopen(argv[1], "r");
        if (!f){
            printf("Erro ao abrir arquivo %s\n", argv[1]);
            return 1;
        }
        terminal(&fs, f);
        fclose(f);
    }
    else{
        terminal(&fs, stdin);
    }
    destruirFS(&fs);
    printf("\nSistema encerrado.\n");
    return 0;
}

void configurarSistema(SistemaDeArquivos *fs){
    int tamanhoDisco = 0;
    int tamanhoBloco = 0;

    printf("==================================================\n");
    printf("   CONFIGURACAO INICIAL DO SISTEMA DE ARQUIVOS\n");
    printf("==================================================\n");

    while (1){
        printf("Tamanho do disco (102400 ate 10485760): ");

        if (scanf("%d",&tamanhoDisco)!=1)
        {
            while(getchar()!='\n');
            continue;
        }

        if(tamanhoDisco>=102400 && tamanhoDisco<=10485760)
            break;

        printf("Valor invalido.\n");
    }

    while(1){
        printf("Tamanho do bloco (128, 256, 512, 1024): ");
        if(scanf("%d",&tamanhoBloco)!=1)
        {
            while(getchar()!='\n');
            continue;
        }
        if(tamanhoBloco==128 ||
           tamanhoBloco==256 ||
           tamanhoBloco==512 ||
           tamanhoBloco==1024)
            break;

        printf("Valor invalido.\n");
    }

    while(getchar()!='\n');
    inicializarFS(fs,tamanhoDisco,tamanhoBloco);
    printf("\nSistema inicializado com sucesso!\n");
    printf("Digite 'help' para listar os comandos.\n\n");
}

void terminal(SistemaDeArquivos *fs, FILE *input){
    
    char linha[256];
    // Descobre se a entrada é um arquivo de script (0) ou o terminal interativo (1)
    int modo_interativo = isatty(fileno(input));

    while(1){
        char caminho[256];
        obterCaminho(fs, caminho);
        
        // Printa o prompt obrigatório (sempre)
        printf("\033[1;32musuario@fs\033[0m:\033[1;34m%s$\033[0m ", caminho);
        
        if (fgets(linha, sizeof(linha), input) == NULL) {
            // Se o arquivo acabou e não teve quebra de linha, solta um \n para o shell real não quebrar
            if (!modo_interativo) printf("\n");
            break;
        }
            
        // Se for arquivo, "simula" a digitação do usuário mostrando o comando e pulando a linha
        if (!modo_interativo) {
            printf("%s", linha); 
            // Se por acaso a linha não terminar com \n (última linha do arquivo), a gente garante ele
            if (linha[strlen(linha) - 1] != '\n') {
                printf("\n");
            }
        }

        linha[strcspn(linha, "\n")] = '\0';
        char *cmd = strtok(linha, " ");
        if (!cmd)
            continue;
            
        if(strcmp(cmd,"exit")==0)
            break;
            
        else if(strcmp(cmd,"help")==0){
            printf("\nhelp\n");
            printf("mkdir <nome>\n");
            printf("rmdir <nome>\n");
            printf("ls\n");
            printf("pwd\n");
            printf("tree\n");
            printf("cd <diretorio>\n");
            printf("touch <arquivo>\n");
            printf("rm <arquivo>\n");
            printf("rename <antigo> <novo>\n");
            printf("mv <arquivo> <destino>\n");
            printf("cat <arquivo>\n");
            printf("import <arquivo_simulado> <arquivo_real>\n");
            printf("exit\n\n");
        }

        else if(strcmp(cmd,"mkdir")==0){
            char *nome = strtok(NULL," ");
            if(nome == NULL) {
                printf("Uso: mkdir <nome>\n");
                continue;
            }
            criarDiretorio(fs,nome);
        }

        else if(strcmp(cmd,"pwd")==0){
            pwd(fs);
        }

        else if(strcmp(cmd,"rmdir")==0) {
            char *nome = strtok(NULL," ");
            if(nome == NULL){
                printf("Uso: rmdir <nome>\n");
                continue;
            }
            removerDiretorio(fs,nome);
        }

        else if(strcmp(cmd,"touch")==0){
            char *nome = strtok(NULL," ");
            if(nome == NULL){
                printf("Uso: touch <arquivo>\n");
                continue;
            }
            criarArquivo(fs,nome);
        }

        else if(strcmp(cmd,"rm")==0){
            char *nome = strtok(NULL," ");
            if(nome == NULL) {
                printf("Uso: rm <arquivo>\n");
                continue;
            }
            apagar(fs,nome);
        }

        else if(strcmp(cmd,"cd")==0){
            char *nome = strtok(NULL," ");
            if(nome == NULL) {
                printf("Uso: cd <diretorio>\n");
                continue;
            }
            entrarDiretorio(fs,nome);
        }

        else if(strcmp(cmd,"tree")==0){
            exibirArvore(fs);
        }

        else if(strcmp(cmd,"ls")==0){
            listarDiretorio(fs); 
        }

        else if(strcmp(cmd,"cat")==0){
            char *nome = strtok(NULL," ");
            if(nome == NULL) {
                printf("Uso: cat <arquivo>\n");
                continue;
            }
            listarConteudoArquivo(fs,nome);
        }

        else if(strcmp(cmd,"rename")==0){
            char *antigo = strtok(NULL," ");
            char *novo = strtok(NULL," ");
            if(antigo==NULL || novo==NULL){
                printf("Uso: rename <antigo> <novo>\n");
                continue;
            }
            renomear(fs,antigo,novo);
        }

        else if(strcmp(cmd,"mv")==0){
            char *arquivo = strtok(NULL," ");
            char *destino = strtok(NULL," ");
            if(arquivo==NULL || destino==NULL) {
                printf("Uso: mv <arquivo> <destino>\n");
                continue;
            }
            mover(fs,arquivo,destino);
        }

        else if(strcmp(cmd,"import")==0) {
            char *arquivo = strtok(NULL," ");
            char *real = strtok(NULL," ");
            if(arquivo==NULL || real==NULL){
                printf("Uso: import <arquivo_simulado> <arquivo_real>\n");
                continue;
            }
            importarArquivo(fs,arquivo,real);
        }

        else{
            printf("Comando desconhecido. Digite 'help'.\n");
        }
    }
}