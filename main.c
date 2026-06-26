#include <stdio.h>
// #include <stdlib.h>
#include "sistemaDeArquivos.h"


int main()
{
    SistemaDeArquivos fs;

    // 1. Inicializa o Sistema de Arquivos (Simulando um disco de 1MB com blocos de 512 bytes)
    // Conforme exigido: usuário define tamanho de partição e blocos.
    printf("=== 1. INICIALIZANDO O SISTEMA DE ARQUIVOS ===\n");
    inicializarFS(&fs, 1024 * 1024, 512);
    exibirArvore(&fs);

    // 2. Testando a Criação de Diretórios (Hierarquia)
    printf("\n=== 2. CRIANDO ESTRUTURA DE DIRETÓRIOS ===\n");
    criarDiretorio(&fs, "documentos");
    criarDiretorio(&fs, "Projetos");
    criarDiretorio(&fs, "Fotos_Antigas");
    
    // Entrando em documentos para criar subdiretórios
    entrarDiretorio(&fs, "documentos");
    criarDiretorio(&fs, "Faculdade");
    criarDiretorio(&fs, "SO");
    
    // Voltando para a raiz
    entrarDiretorio(&fs, "..");
    
    exibirArvore(&fs);

    // 3. Testando Renomear e Apagar Diretório
    printf("\n=== 3. TESTANDO RENOMEAR E APAGAR DIRETÓRIO ===\n");
    // Renomeia "Fotos_Antigas" para "Midia"
    printf("Renomeando 'Fotos_Antigas' para 'Midia'...\n");
    renomear(&fs, "Fotos_Antigas", "Midia");
    
    // Apaga o diretório "Midia" (está vazio, deve funcionar)
    printf("Apagando diretório 'Midia'...\n");
    removerDiretorio(&fs, "Midia");
    
    // Tentando apagar "documentos" (contém subpastas, deve falhar/impedir)
    printf("Tentando apagar 'documentos' (não deve permitir):\n");
    if (removerDiretorio(&fs, "documentos") == -1) {
        printf("-> Sucesso no teste: O sistema impediu a remoção de um diretório com conteúdo.\n");
    }

    exibirArvore(&fs);

    // 4. Testando Criação e Importação de Arquivos Reais
    printf("\n=== 4. CRIANDO ARQUIVOS E IMPORTANDO CONTEÚDO REAL ===\n");
    // Cria o arquivo na raiz
    criarArquivo(&fs, "teste.txt");
    criarArquivo(&fs, "imagem.png");

    // Importa o conteúdo de um arquivo real da sua máquina ("texto.txt") para o arquivo simulado
    // NOTA: Certifique-se de que o arquivo "texto.txt" existe na mesma pasta do executável da sua máquina!
    printf("Importando dados reais para 'teste.txt'...\n");
    if (importarArquivo(&fs, "teste.txt", "texto.txt") == -1) {
        printf("-> [AVISO] Nao foi possivel ler o arquivo 'texto.txt' real do seu PC. Criando simulado vazio.\n");
    }

    exibirArvore(&fs);

    // 5. Testando a Exibição do Conteúdo do Arquivo
    printf("\n=== 5. EXIBINDO CONTEÚDO DO ARQUIVO SIMULADO ===\n");
    printf("Conteudo de 'teste.txt':\n------------------\n");
    listarConteudoArquivo(&fs, "teste.txt");
    printf("\n------------------\n");

    // 6. Testando Mover Arquivo (A grande correção!)
    printf("\n=== 6. MOVENDO ARQUIVO (Sem duplicações de 0 bytes) ===\n");
    // Vamos mover o "teste.txt" (que está na raiz e tem tamanho/blocos) para dentro de "Projetos"
    printf("Movendo 'teste.txt' da Raiz para 'Projetos'...\n");
    mover(&fs, "teste.txt", "Projetos");

    // Agora vamos mover o "imagem.png" da Raiz para dentro de "documentos"
    printf("Movendo 'imagem.png' da Raiz para 'documentos'...\n");
    mover(&fs, "imagem.png", "documentos");

    exibirArvore(&fs);

    // 7. Testando Apagar Arquivo e Liberação de Blocos
    printf("\n=== 7. APAGANDO ARQUIVO E LIBERANDO MEMÓRIA ===\n");
    // Vamos entrar em Projetos e deletar o teste.txt que movemos para lá
    entrarDiretorio(&fs, "Projetos");
    
    printf("Estado atual antes de deletar em 'Projetos':\n");
    exibirArvore(&fs);

    printf("Apagando 'teste.txt' de dentro de 'Projetos'...\n");
    apagar(&fs, "teste.txt");

    // Voltando para a raiz para ver o resultado final
    entrarDiretorio(&fs, "..");
    
    printf("\n=== ESTADO FINAL DO SISTEMA DE ARQUIVOS ===\n");
    exibirArvore(&fs);

    // 8. Limpeza da memória RAM do simulador
    destruirFS(&fs);
    printf("\nSistema de arquivos desalocado com sucesso. Fim dos testes!\n");

    return 0;
}