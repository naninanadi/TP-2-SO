#include "sistemaDeArquivos.h"

static int procurarFilho(SistemaDeArquivos *fs, int diretorioPai, char nome[]){
    Diretorio *dir = &fs->diretorios[diretorioPai];

    for(int i = 0; i < dir->qtdFilhos; i++){
        int id = dir->filhos[i];

        if(strcmp(fs->inodes[id].nome,
                nome
            ) == 0
        )
        {
            return id;
        }
    }

    return -1;
}

static int removerFilho(Diretorio *dir, int id)
{
    int pos = -1;

    for (int i = 0; i < dir->qtdFilhos; i++)
    {
        if (dir->filhos[i] == id)
        {
            pos = i;
            break;
        }
    }

    if (pos == -1)
        return -1;

    for (int i = pos; i < dir->qtdFilhos - 1; i++)
    {
        dir->filhos[i] = dir->filhos[i + 1];
    }

    dir->qtdFilhos--;

    return 0;
}

static int inserirFilho(Diretorio *dir, int id)
{
    if (dir->qtdFilhos >= MAX_FILHOS)
        return -1;

    dir->filhos[dir->qtdFilhos++] = id;

    return 0;
}

void destruirFS(SistemaDeArquivos *fs)
{
    for(int i = 0;
        i < fs->super.totalBlocos;
        i++)
    {
        free(fs->blocos[i].dados);
    }

    free(fs->blocos);
}

static int encontrarBlocoLivre(SistemaDeArquivos *fs)
{
    for (int i = 0; i < fs->super.totalBlocos; i++)
    {
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

    fs->inodes[0].usado = 1;
    fs->inodes[0].id = 0;

    fs->inodes[0].tipo = DIRETORIO;

    strcpy(fs->inodes[0].nome, "/");

    fs->inodes[0].pai = -1;

    fs->inodes[0].criado = time(NULL);

    fs->inodes[0].modificado = time(NULL);

    fs->inodes[0].acessado = time(NULL);

    fs->super.inodesLivres--;
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

    return 0;
}

void listarConteudoArquivo(
    SistemaDeArquivos *fs,
    char nome[]
)
{
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
        malloc(fs->super.tamanhoBloco);

    for(int i=0;
        i<fs->inodes[id].quantidadeBlocos;
        i++)
    {
        lerBloco(
            fs,
            fs->inodes[id].blocos[i],
            buffer
        );

        printf("%s", buffer);
    }

    free(buffer);
}

int importarArquivo(SistemaDeArquivos *fs,
                    char nomeSimulado[],
                    char caminhoArquivo[])
{

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
    
    if (quantidadeBlocos > MAX_BLOCOS_ARQUIVO)
    {
        fclose(arquivo);
        return -1;
    }
    
    /* Verifica se há blocos livres suficientes */
    if (quantidadeBlocos > fs->super.blocosLivres)
    {
        fclose(arquivo);
        return -1;
    }
    
    fs->inodes[id].tamanho = tamanho;
    fs->inodes[id].quantidadeBlocos = quantidadeBlocos;
    
    char *buffer = malloc(fs->super.tamanhoBloco);
    
    if (buffer == NULL)
    {
        fclose(arquivo);
        return -1;
    }
    
    for (int i = 0; i < quantidadeBlocos; i++)
    {
        int indiceBloco = encontrarBlocoLivre(fs);
        
        if (indiceBloco == -1)
        {
            free(buffer);
            fclose(arquivo);
            return -1;
        }

        printf("%d\n", indiceBloco);
        
        fs->blocos[indiceBloco].usado = 1;
        printf("oi\n");
        fs->super.blocosLivres--;
        
        int bytesLidos = fread(
            buffer,
            1,
            fs->super.tamanhoBloco,
            arquivo);
            
            memcpy(
                fs->blocos[indiceBloco].dados,
                buffer,
                bytesLidos);
                
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

int alocarBloco(
    SistemaDeArquivos *fs
)
{
    for(int i = 0;
        i < fs->super.totalBlocos;
        i++)
    {
        if(!fs->blocos[i].usado)
        {
            fs->blocos[i].usado = 1;

            fs->super.blocosLivres--;

            return i;
        }
    }

    return -1;
}

void liberarBloco(
    SistemaDeArquivos *fs,
    int bloco
)
{
    fs->blocos[bloco].usado = 0;

    memset(
        fs->blocos[bloco].dados,
        0,
        fs->super.tamanhoBloco
    );

    fs->super.blocosLivres++;
}

int escreverBloco(
    SistemaDeArquivos *fs,
    int bloco,
    const char *dados,
    int bytes
)
{
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

int lerBloco(
    SistemaDeArquivos *fs,
    int bloco,
    char *destino
)
{
    memcpy(
        destino,
        fs->blocos[bloco].dados,
        fs->super.tamanhoBloco
    );

    return 0;
}

// Gerais

int renomear(
    SistemaDeArquivos *fs,
    char nomeAtual[],
    char novoNome[]
)
{
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
    int atual = fs->diretorioAtual;

    // Procura o arquivo/diretório que será movido
    int id = procurarFilho(fs, atual, nome);

    if (id == -1)
    {
        printf("Erro! Arquivo ou diretorio nao encontrado.\n");
        return -1;
    }

    // Procura o diretório de destino
    int novoPai = procurarFilho(fs, atual, destino);

    if (novoPai == -1)
    {
        printf("Erro! Diretorio de destino inexistente.\n");
        return -1;
    }

    if (fs->inodes[novoPai].tipo != DIRETORIO)
    {
        printf("Erro! O destino nao e um diretorio.\n");
        return -1;
    }

    // Verifica se já existe um arquivo/diretório com o mesmo nome no destino
    if (procurarFilho(fs, novoPai, nome) != -1)
    {
        printf("Erro! Ja existe um item com esse nome no diretorio de destino.\n");
        return -1;
    }

    // Remove do diretório atual
    Diretorio *origem = &fs->diretorios[atual];
    Diretorio *dest = &fs->diretorios[novoPai];

    if (removerFilho(origem, id) == -1)
        return -1;

    if (inserirFilho(dest, id) == -1)
    {
        // desfaz a remoção caso o destino esteja cheio
        inserirFilho(origem, id);
        return -1;
    }

    // Atualiza o pai
    fs->inodes[id].pai = novoPai;
    fs->inodes[id].modificado = time(NULL);

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