#include <stdio.h>

#include "sistemaDeArquivos.h" 

int main()
{
    SistemaDeArquivos fs;

    inicializarFS(&fs, 1024 * 1024, 512);

    //testes do chat
    criarDiretorio(&fs, "Documentos");
    criarDiretorio(&fs, "Fotos");

    printf("Raiz:\n");
    listarDiretorio(&fs);

    entrarDiretorio(&fs, "Documentos");

    criarDiretorio(&fs, "Faculdade");
    criarDiretorio(&fs, "SO");

    printf("\nDentro de Documentos:\n");
    listarDiretorio(&fs);

    entrarDiretorio(&fs, ".."); //isso é ele voltando um diretório

    printf("\nVoltando para a raiz:\n");
    listarDiretorio(&fs);

    removerDiretorio(&fs, "Fotos");

    printf("\nApós remover Fotos:\n");
    listarDiretorio(&fs);


    //tentando criar repositório repetido (isso precisa mudar pq depende do pai)
    criarDiretorio(&fs, "Documentos");
    
    //tentando remover e entrar em repos inexistentes
    removerDiretorio(&fs, "Nenhum");
    entrarDiretorio(&fs, "Inexistente");

    criarDiretorio(&fs, "Projetos");

    entrarDiretorio(&fs, "Projetos");

    criarDiretorio(&fs, "SO");

    entrarDiretorio(&fs, "..");
    renomear(&fs, "Documentos", "documentos");

    listarDiretorio(&fs);

    if (removerDiretorio(&fs, "Projetos") == -1)
        printf("Nao foi possivel remover um diretorio que contem arquivos ou subdiretorios.\n");

    printf("\n=== TESTE DE ARQUIVOS ===\n");

    criarArquivo(&fs, "teste.txt");
    criarArquivo(&fs, "imagem.png");

    entrarDiretorio(&fs, "documentos");
    criarArquivo(&fs, "teste.txt");
    criarArquivo(&fs, "imagem.png");

    listarDiretorio(&fs);

    imprimirEstadoSistema(&fs);

    importarArquivo(
    &fs,
    "teste.txt",
    "texto.txt"
    );

    listarConteudoArquivo(
    &fs,
    "teste.txt"
    );

    entrarDiretorio(&fs, "..");

    mover(
    &fs,
    "texto.txt",
    "Projetos"
    );

    imprimirEstadoSistema(&fs);
    return 0;
}