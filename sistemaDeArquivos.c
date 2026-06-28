#include <math.h>
#include "headers/sistemaDeArquivos.h"

static int procurarFilho(SistemaDeArquivos *fs, int diretorioPai, char nome[]){
    Diretorio *dir = &fs->diretorios[diretorioPai];

    for(int i = 0; i < dir->qtdFilhos; i++){
        int id = dir->filhos[i];

        if(strcmp(fs->inodes[id].nome, nome) == 0){
            return id;
        }
    }

    return -1;
}

static int removerFilho(Diretorio *dir, int id){
    int pos = -1;

    // Procura em qual posição do array 'filhos' o ID está
    for (int i = 0; i < dir->qtdFilhos; i++){
        if (dir->filhos[i] == id) {
            pos = i;
            break;
        }
    }

    // Se não achou o arquivo neste diretório, retorna erro
    if (pos == -1)
        return -1;

    // Desloca todos os filhos seguintes uma posição para trás
    for (int i = pos; i < dir->qtdFilhos - 1; i++){
        dir->filhos[i] = dir->filhos[i + 1];
    }

    // Opcional: limpa a última posição que sobrou para não deixar lixo
    dir->filhos[dir->qtdFilhos - 1] = -1;

    // Diminui a quantidade de filhos do diretório
    dir->qtdFilhos--;

    return 0;
}

static int inserirFilho(Diretorio *dir, int id){
    if (dir->qtdFilhos >= MAX_FILHOS){
        return -1; // Diretório cheio
    }

    // Insere o ID na próxima posição disponível
    dir->filhos[dir->qtdFilhos] = id;
    dir->qtdFilhos++;

    return 0;
}

void destruirFS(SistemaDeArquivos *fs) {
    for(int i = 0; i < fs->super.totalBlocos; i++){
        free(fs->blocos[i].dados);
    }

    free(fs->blocos);
}

static int encontrarBlocoLivre(SistemaDeArquivos *fs){
    for (int i = 0; i < fs->super.totalBlocos; i++){
        if (!fs->blocos[i].usado)
            return i;
    }

    return -1;
}

// Sistema de arquivos

void inicializarFS(SistemaDeArquivos *fs, int tamanhoDisco, int tamanhoBloco){

    fs->super.tamanhoDisco = tamanhoDisco;
    fs->super.tamanhoBloco = tamanhoBloco;

    fs->super.totalBlocos = tamanhoDisco / tamanhoBloco;

    fs->super.blocosLivres = fs->super.totalBlocos;

    fs->super.totalInodes = MAX_INODES;

    fs->super.inodesLivres = MAX_INODES;

    for(int i = 0; i < MAX_INODES; i++){
        fs->inodes[i].usado = 0;
        fs->diretorios[i].qtdFilhos = 0;
    }

    fs->blocos = malloc(fs->super.totalBlocos * sizeof(Bloco));
    if (fs->blocos == NULL) {
        printf("Erro fatal: Falha ao alocar os blocos de dados!\n");
        exit(1);
    }

    // Inicializa os Inodes e Diretores
    for(int i = 0; i < MAX_INODES; i++){
        fs->inodes[i].usado = 0;
        fs->diretorios[i].qtdFilhos = 0;
    }

    // === CORREÇÃO AQUI: Inicializa e aloca o conteúdo de cada bloco ===
    for(int i = 0; i < fs->super.totalBlocos; i++){
        fs->blocos[i].usado = 0;
        fs->blocos[i].bytesUtilizados = 0;
        fs->blocos[i].dados = malloc(tamanhoBloco); // Aloca o tamanho real do bloco
        if (fs->blocos[i].dados == NULL) {
            printf("Erro fatal: Falha ao alocar dados do bloco %d!\n", i);
            exit(1);
        }
    }

    fs->raiz = 0;
    fs->diretorioAtual = 0;
    fs->lixeira = 1;

    fs->qtdArquivos = 0;
    fs->qtdDiretorios = 0;

    // Inicializa a raiz no FS
    fs->inodes[0].usado = 1;
    fs->inodes[0].id = 0;

    fs->inodes[0].tipo = DIRETORIO;

    strcpy(fs->inodes[0].nome, "/");

    fs->inodes[0].pai = -1;

    fs->inodes[0].criado = time(NULL);

    fs->inodes[0].modificado = time(NULL);

    fs->inodes[0].acessado = time(NULL);

    fs->super.inodesLivres--;

    criarDiretorio(fs, ".lixeira");
}

// Diretorio

int criarDiretorio(SistemaDeArquivos *fs, char nome[]){
    int atual = fs->diretorioAtual;

    if(procurarFilho(fs,atual, nome) != -1){
        printf("Erro! Diretorio ja existente.\n");
        return -1;
    }

    int novo = criarInode(fs->inodes,DIRETORIO, nome,atual);

    if(novo == -1) return -1;

    Diretorio *pai = &fs->diretorios[atual];

    if (inserirFilho(pai, novo) == -1)
    {
        printf("Erro! Diretorio cheio.\n");
        return -1;
    }

    fs->super.inodesLivres--;
    fs->qtdDiretorios++;
    return 0;
}

void listarDiretorio(SistemaDeArquivos *fs){

    Diretorio *dir = &fs->diretorios[fs->diretorioAtual];

    printf("\n");

    for(int i = 0; i < dir->qtdFilhos; i++){

        int id =dir->filhos[i];

        if(fs->inodes[id].tipo == DIRETORIO){
            printf("[DIR] %s\n", fs->inodes[id].nome);
        } else {
            printf("[ARQ] %s\n", fs->inodes[id].nome);
        }
    }

    printf("\n");
}

int entrarDiretorio(SistemaDeArquivos *fs, char nome[]){

    if(strcmp(nome, "..") == 0){
        int pai = fs->inodes[fs->diretorioAtual].pai;

        if(pai != -1){
            fs->diretorioAtual = pai;
        }

        return 0;
    }

    int id = procurarFilho(fs, fs->diretorioAtual, nome);

    if(id == -1){
        printf("Erro! Diretorio inexistente!\n");
        return -1;
    }

    if(fs->inodes[id].tipo != DIRETORIO){
        return -1;
    }

    fs->diretorioAtual = id;

    return 0;
}

int removerDiretorio(SistemaDeArquivos *fs, char nome[]){

    int id = procurarFilho(fs, fs->diretorioAtual, nome);

    if(id == 1){
        printf("Esse diretorio nao pode ser removido!\n");
        return -1;
    }

    if(id == -1){
        printf("Erro! Diretorio inexistente!\n");
        return -1;
    }

    if(fs->inodes[id].tipo != DIRETORIO){
        return -1;
    }

    if(fs->diretorios[id].qtdFilhos > 0){
        return -1;
    }

    Diretorio *pai = &fs->diretorios[fs->diretorioAtual];

    int pos = -1;

    for(int i = 0; i < pai->qtdFilhos; i++){
        if(pai->filhos[i] == id){
            pos = i;
            break;
        }
    }

    if(pos == -1) return -1;

    for(int i = pos; i < pai->qtdFilhos - 1; i++){
        pai->filhos[i] =
            pai->filhos[i + 1];
    }

    pai->qtdFilhos--;

    removerInode(fs->inodes, id);

    fs->super.inodesLivres++;

    return 0;
}


//Arquivo

int criarArquivo(SistemaDeArquivos *fs, char nome[]){

    int atual = fs->diretorioAtual;

    if (procurarFilho(fs, atual, nome) != -1)
        return -1;

    int id = criarInode(
        fs->inodes,
        ARQUIVO,
        nome,
        atual
    );

    if (id == -1)
        return -1;

    fs->inodes[id].tamanho = 0;
    fs->inodes[id].quantidadeBlocos = 0;

    for(int i = 0; i < MAX_BLOCOS_ARQUIVO; i++)
        fs->inodes[id].blocos[i] = -1;

    Diretorio *pai = &fs->diretorios[atual];

    if (inserirFilho(pai, id) == -1)
    {
        printf("Erro! Diretorio cheio.\n");
        return -1;
    }

    fs->super.inodesLivres--;
    fs->qtdArquivos++;

    return 0;
}

void listarConteudoArquivo(SistemaDeArquivos *fs, char nome[]){
    int id = procurarFilho(
        fs,
        fs->diretorioAtual,
        nome
    );

    if(id == -1)
        return;

    if(fs->inodes[id].tipo != ARQUIVO)
        return;

    char *buffer =
        malloc(fs->super.tamanhoBloco+1);

          printf("\n");
    for(int i=0;
        i<fs->inodes[id].quantidadeBlocos;
        i++)
    {
        lerBloco(fs, fs->inodes[id].blocos[i], buffer );

        buffer[fs->blocos[fs->inodes[id].blocos[i]].bytesUtilizados] = '\0';

        printf("%s", buffer);
    }
    printf("\n");
      printf("\n");

    free(buffer);
}

int importarArquivo(SistemaDeArquivos *fs, char nomeSimulado[], char caminhoArquivo[]){

    int id = procurarFilho(fs, fs->diretorioAtual, nomeSimulado);
    
    if (id == -1)
    return -1;
    
    if (fs->inodes[id].tipo != ARQUIVO)
    return -1;
    
    FILE *arquivo = fopen(caminhoArquivo, "rb");
    
    if (arquivo == NULL)
    return -1;
    
    /* Descobre o tamanho do arquivo */
    fseek(arquivo, 0, SEEK_END);
    long tamanho = ftell(arquivo);
    rewind(arquivo);
    
    int quantidadeBlocos =
    (tamanho + fs->super.tamanhoBloco - 1) /
    fs->super.tamanhoBloco;
    
    if (quantidadeBlocos > MAX_BLOCOS_ARQUIVO){
        fclose(arquivo);
        return -1;
    }
    
    /* Verifica se há blocos livres suficientes */
    if (quantidadeBlocos > fs->super.blocosLivres){
        fclose(arquivo);
        return -1;
    }
    
    fs->inodes[id].tamanho = tamanho;
    fs->inodes[id].quantidadeBlocos = quantidadeBlocos;
    
    char *buffer = malloc(fs->super.tamanhoBloco);
    
    if (buffer == NULL){
        fclose(arquivo);
        return -1;
    }
    
    for (int i = 0; i < quantidadeBlocos; i++){
        int indiceBloco = encontrarBlocoLivre(fs);
        
        if (indiceBloco == -1){
            free(buffer);
            fclose(arquivo);
            return -1;
        }

        // printf("%d\n", indiceBloco);
        
        fs->blocos[indiceBloco].usado = 1;
        // printf("oi\n");
        fs->super.blocosLivres--;
        
        int bytesLidos = fread(buffer, 1, fs->super.tamanhoBloco, arquivo);
            
            memcpy(fs->blocos[indiceBloco].dados, buffer, bytesLidos);
                
                fs->blocos[indiceBloco].bytesUtilizados = bytesLidos;
                
                fs->inodes[id].blocos[i] = indiceBloco;
            }
            
            free(buffer);
            
            fclose(arquivo);
            
            fs->inodes[id].modificado = time(NULL);
            fs->inodes[id].acessado = time(NULL);
            
            return 0;
        }
        
//Bloco de dados

int alocarBloco(SistemaDeArquivos *fs){
    for(int i = 0; i < fs->super.totalBlocos; i++){
        if(!fs->blocos[i].usado){
            fs->blocos[i].usado = 1;

            fs->super.blocosLivres--;

            return i;
        }
    }

    return -1;
}

void liberarBloco(SistemaDeArquivos *fs, int bloco){
    fs->blocos[bloco].usado = 0;

    memset(fs->blocos[bloco].dados, 0, fs->super.tamanhoBloco);

    fs->super.blocosLivres++;
}

int escreverBloco(SistemaDeArquivos *fs, int bloco, const char *dados, int bytes){
    if(bytes >
       fs->super.tamanhoBloco)
        return -1;

    memcpy(
        fs->blocos[bloco].dados,
        dados,
        bytes
    );

    return 0;
}

int lerBloco(SistemaDeArquivos *fs, int bloco, char *destino){

    memcpy(destino, fs->blocos[bloco].dados, fs->super.tamanhoBloco);

    return 0;
}

// Gerais

int renomear(SistemaDeArquivos *fs, char nomeAtual[], char novoNome[]){
    int id = procurarFilho(
        fs,
        fs->diretorioAtual,
        nomeAtual
    );

    if (id == -1)
        return -1;

    if (procurarFilho(
            fs,
            fs->diretorioAtual,
            novoNome) != -1)
        return -1;

    strcpy(
        fs->inodes[id].nome,
        novoNome
    );

    fs->inodes[id].modificado =
        time(NULL);

    return 0;
}

int mover(SistemaDeArquivos *fs, char nome[], char destino[])
{
    int diretorioOriginal = fs->diretorioAtual;

    // 1. Procura o arquivo/diretório que será movido no diretório atual
    int idParaMover = procurarFilho(fs, diretorioOriginal, nome);
    if (idParaMover == -1)
    {
        printf("Erro! Arquivo ou diretorio '%s' nao encontrado aqui.\n", nome);
        return -1;
    }

    // 2. "Viaja" temporariamente para o diretório de destino para validar se ele existe
    // Se o destino for "..", entrarDiretorio vai subir um nível corretamente.
    if (entrarDiretorio(fs, destino) == -1)
    {
        printf("Erro! O destino '%s' nao e um diretorio valido ou nao existe.\n", destino);
        fs->diretorioAtual = diretorioOriginal; // Garante que não ficamos perdidos
        return -1;
    }

    int novoPai = fs->diretorioAtual; // Este é o ID do diretório de destino

    // Ignora a tentativa de mover um diretório para dentro dele mesmo
    if (idParaMover == novoPai)
    {
        printf("Erro! Nao e possivel mover um diretorio para dentro dele mesmo.\n");
        fs->diretorioAtual = diretorioOriginal;
        return -1;
    }

    // 3. Verifica se já existe algo com o mesmo nome lá no destino
    if (procurarFilho(fs, novoPai, nome) != -1)
    {
        printf("Erro! Ja existe um item com o nome '%s' no destino.\n", nome);
        fs->diretorioAtual = diretorioOriginal; // Volta ao normal
        return -1;
    }

    // 4. Volta ao diretório original para fazer a remoção de forma segura
    fs->diretorioAtual = diretorioOriginal;

    Diretorio *origem = &fs->diretorios[diretorioOriginal];
    Diretorio *dest = &fs->diretorios[novoPai];

    // 5. Remove o filho da origem
    if (removerFilho(origem, idParaMover) == -1)
        return -1;

    // 6. Insere o filho no destino
    if (inserirFilho(dest, idParaMover) == -1)
    {
        printf("Erro! Diretorio de destino cheio.\n");
        inserirFilho(origem, idParaMover); // Desfaz a remoção
        return -1;
    }

    // 7. Atualiza os metadados do i-node movido
    fs->inodes[idParaMover].pai = novoPai;
    fs->inodes[idParaMover].modificado = time(NULL);

    printf("'%s' movido para '%s' com sucesso!\n", nome, destino);
    return 0;
}

int apagar(SistemaDeArquivos *fs, char nome[])
{
    int atual = fs->diretorioAtual;

    // 1. Procura o arquivo no diretório atual
    int id = procurarFilho(fs, atual, nome);

    if (id == -1)
    {
        printf("Erro! Arquivo nao encontrado.\n");
        return -1;
    }

    // Garante que o que estamos apagando é um ARQUIVO e não um diretório
    if (fs->inodes[id].tipo != ARQUIVO)
    {
        printf("Erro! '%s' nao e um arquivo (e um diretorio).\n", nome);
        return -1;
    }

    // 3. Remove o arquivo da lista de filhos do diretório atual
    Diretorio *pai = &fs->diretorios[atual];
    if (removerFilho(pai, id) == -1)
    {
        printf("Erro interno ao remover o arquivo do diretorio pai.\n");
        return -1;
    }
    else{
        // 3.1 Adiciona o arquivo na lixeira
        inserirFilho(&fs->diretorios[fs->lixeira], id);
        // 3.2 Adiciona o pai original para saber qual era o diretorio em que estava
        fs->inodes[id].paiOriginal = atual;
        // 3.3 Diz que o pai do arquivo é a lixeira
        fs->inodes[id].pai = fs->lixeira;
    }

    printf("Arquivo '%s' apagado com sucesso e blocos liberados!\n", nome);
    return 0;
}

int apagarDaLixeira(SistemaDeArquivos *fs, char nome[])
{
    int atual = fs->diretorioAtual;

    // 1. Procura o arquivo no diretório atual
    int id = procurarFilho(fs, atual, nome);

    if (id == -1)
    {
        printf("Erro! Arquivo nao encontrado.\n");
        return -1;
    }

    // Garante que o que estamos apagando é um ARQUIVO e não um diretório
    if (fs->inodes[id].tipo != ARQUIVO)
    {
        printf("Erro! '%s' nao e um arquivo (e um diretorio).\n", nome);
        return -1;
    }

    // 2. Libera todos os blocos de dados associados a este arquivo
    for (int i = 0; i < fs->inodes[id].quantidadeBlocos; i++)
    {
        int blocoParaLiberar = fs->inodes[id].blocos[i];
        if (blocoParaLiberar != -1)
        {
            liberarBloco(fs, blocoParaLiberar);
        }
    }

    // 3. Remove o arquivo da lista de filhos do diretório atual
    Diretorio *pai = &fs->diretorios[atual];
    if (removerFilho(pai, id) == -1)
    {
        printf("Erro interno ao remover o arquivo do diretorio pai.\n");
        return -1;
    }

    // 4. Libera o i-node na tabela de i-nodes
    removerInode(fs->inodes, id);

    // 5. Atualiza os contadores do SuperBloco
    fs->super.inodesLivres++;

    printf("Arquivo '%s' apagado com sucesso e blocos liberados!\n", nome);
    return 0;
}

int restaurarDaLixeira(SistemaDeArquivos *fs, char nome[])
{
    int atual = fs->lixeira;

    // 1. Procura o arquivo no diretório atual
    int id = procurarFilho(fs, atual, nome);

    if (id == -1)
    {
        printf("Erro! Arquivo nao encontrado.\n");
        return -1;
    }

    // Garante que o que estamos apagando é um ARQUIVO e não um diretório
    if (fs->inodes[id].tipo != ARQUIVO)
    {
        printf("Erro! '%s' nao e um arquivo (e um diretorio).\n", nome);
        return -1;
    }

    // 3. Remove o arquivo da lista de filhos do diretório atual
    Diretorio *pai = &fs->diretorios[atual];
    if (removerFilho(pai, id) == -1)
    {
        printf("Erro interno ao remover o arquivo do diretorio pai.\n");
        return -1;
    }
    else{
        // 3.1 Adiciona o arquivo na lixeira
        inserirFilho(&fs->diretorios[fs->inodes[id].paiOriginal], id);
        // 3.2 Devolve o "pai" para o diretorio de origem
        fs->inodes[id].pai = fs->inodes[id].paiOriginal;
    }

    printf("Arquivo '%s' foi restaurado com sucesso!\n", nome);
    return 0;

}

void imprimirEstadoSistema(SistemaDeArquivos *fs)
{
    printf("\n==============================\n");
    printf("ESTADO DO SISTEMA\n");
    printf("==============================\n");

    printf("Diretório atual: %d\n", fs->diretorioAtual);
    printf("Raiz: %d\n", fs->raiz);

    printf("\nInodes livres: %d\n", fs->super.inodesLivres);

    printf("\nTabela de inodes:\n");

    for (int i = 0; i < MAX_INODES; i++)
    {
        if (fs->inodes[i].usado)
        {
            printf("[%d] ", i);

            if (fs->inodes[i].tipo == DIRETORIO)
                printf("DIR  ");
            else
                printf("ARQ  ");

            printf("%s", fs->inodes[i].nome);

            printf("  pai=%d", fs->inodes[i].pai);

            printf("\n");
        }
    }

    printf("==============================\n");
}

// Função auxiliar recursiva para desenhar a árvore
static void desenharArvoreRecursivo(SistemaDeArquivos *fs, int idAtual, int nivel, int ehUltimoFilho, char *prefixo)
{
    // Desenha o item atual
    if (nivel > 0)
    {
        printf("%s%s ", prefixo, ehUltimoFilho ? "L_" : "|--");
    }

    if (fs->inodes[idAtual].tipo == DIRETORIO)
    {
        // Destaca diretórios (pode usar códigos de cor ANSI se quiser, ex: \033[1;34m)
        if (nivel == 0) {
            printf("[%s]\n", fs->inodes[idAtual].nome);
        } else {
            printf("[%s/]\n", fs->inodes[idAtual].nome);
        }
    }

    else
    {
        printf("%s (%d bytes, %d blocos)\n", 
               fs->inodes[idAtual].nome, 
               fs->inodes[idAtual].tamanho, 
               fs->inodes[idAtual].quantidadeBlocos);
    }

    // Se for diretório, vamos processar os filhos recursivamente
    if (fs->inodes[idAtual].tipo == DIRETORIO)
    {
        Diretorio *dir = &fs->diretorios[idAtual];
        
        // Aloca espaço para o novo prefixo visual dos galhos
        char *novoPrefixo = malloc(strlen(prefixo) + 10);
        
        for (int i = 0; i < dir->qtdFilhos; i++)
        {
            int idFilho = dir->filhos[i];
            int ultimo = (i == dir->qtdFilhos - 1);

            // Prepara o recuo visual para a próxima linha
            if (nivel > 0) {
                sprintf(novoPrefixo, "%s%s   ", prefixo, ehUltimoFilho ? " " : "|");
            } else {
                strcpy(novoPrefixo, "");
            }

            // Chamada recursiva para o filho
            desenharArvoreRecursivo(fs, idFilho, nivel + 1, ultimo, novoPrefixo);
        }
        
        free(novoPrefixo);
    }
}

// Função principal que o usuário chama
void exibirArvore(SistemaDeArquivos *fs)
{
    // printf("\n========================================\n");
    // printf("        ARVORE DO SISTEMA DE ARQUIVOS     \n");
    // printf("========================================\n");
    
    // Começa a partir da raiz (ID 0)
    printf("\n");
    desenharArvoreRecursivo(fs, fs->raiz, 0, 1, "");
    printf("\n");
    // printf("========================================\n");
}

void pwd(SistemaDeArquivos *fs)
{
    int caminho[MAX_INODES];
    int n = 0;

    int atual = fs->diretorioAtual;

    while (atual != -1)
    {
        caminho[n++] = atual;
        atual = fs->inodes[atual].pai;
    }
        printf("\n");
    printf("/");

    for (int i = n - 2; i >= 0; i--)
    {
        printf("%s", fs->inodes[caminho[i]].nome);

        if (i != 0)
            printf("/");
    }

    printf("\n");
        printf("\n");
}

void obterCaminho(SistemaDeArquivos *fs, char *caminhoFinal)
{
    int caminho[MAX_INODES];
    int n = 0;

    int atual = fs->diretorioAtual;

    while (atual != -1)
    {
        caminho[n++] = atual;
        atual = fs->inodes[atual].pai;
    }

    strcpy(caminhoFinal, "/");

    for (int i = n - 2; i >= 0; i--)
    {
        strcat(caminhoFinal, fs->inodes[caminho[i]].nome);

        if (i != 0)
            strcat(caminhoFinal, "/");
    }
}

void usoDoDisco(SistemaDeArquivos *fs)
{
    printf("Analise do uso do disco\n");

    printf("\n========== Sistema de Arquivos ==========\n");
    int espacoTotal = floor(fs->super.tamanhoDisco / 1024);
    printf("\nEspaco total: %dMB\n", espacoTotal);
    int espacoUtilizado = floor((fs->super.tamanhoDisco - 
        fs->super.tamanhoBloco * 
        (fs->super.totalBlocos - fs->super.blocosLivres)) / 1024);
    printf("Espaco utilizado: %dMB\n", espacoUtilizado);
    int espacoLivre = floor((fs->super.blocosLivres * fs->super.tamanhoBloco) / 1024);
    printf("Espaco livre: %dMB\n", espacoLivre);

    printf("\nBlocos");
    printf("Total: %d\n", fs->super.totalBlocos);
    printf("Livres: %d\n", fs->super.blocosLivres);
    int blocosOcupados = fs->super.totalBlocos - fs->super.blocosLivres;
    printf("Ocupados: %d\n", blocosOcupados);

    printf("\nInodes");
    printf("Total: %d\n", fs->super.totalInodes);
    printf("Livres: %d\n", fs->super.inodesLivres);
    int inodesOcupados = fs->super.totalInodes - fs->super.inodesLivres;
    printf("Ocupados: %d\n", inodesOcupados);

    printf("\nArquivos: %d\n", fs->qtdArquivos);
    printf("Diretorios: %d\n", fs->qtdDiretorios);

    printf("\n=========================================\n");
}