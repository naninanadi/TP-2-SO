#include <math.h>
#include "headers/sistemaDeArquivos.h"
#include <stdarg.h> // Necessário para a função log_verboso (va_list)

// 1. Definição da variável global que controla o modo verboso
int modo_verboso = 0; 

// 2. Função auxiliar para centralizar os prints verbosos com cor no terminal Linux
void log_verboso(const char *format, ...) {
    if (modo_verboso) {
        printf("\033[1;33m[VERBOSO]\033[0m "); // [VERBOSO] em Amarelo Negrito
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

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

    for (int i = 0; i < dir->qtdFilhos; i++){
        if (dir->filhos[i] == id) {
            pos = i;
            break;
        }
    }

    if (pos == -1)
        return -1;

    for (int i = pos; i < dir->qtdFilhos - 1; i++){
        dir->filhos[i] = dir->filhos[i + 1];
    }

    dir->filhos[dir->qtdFilhos - 1] = -1;
    dir->qtdFilhos--;

    return 0;
}

static int inserirFilho(Diretorio *dir, int id){
    if (dir->qtdFilhos >= MAX_FILHOS){
        return -1; 
    }

    dir->filhos[dir->qtdFilhos] = id;
    dir->qtdFilhos++;

    return 0;
}

void destruirFS(SistemaDeArquivos *fs) {
    log_verboso("Destruindo o Sistema de Arquivos em memoria e liberando buffers...\n");
    for(int i = 0; i < fs->super.totalBlocos; i++){
        free(fs->blocos[i].dados);
    }

    free(fs->blocos);
    log_verboso("Memoria liberada com sucesso.\n");
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
    printf("Inicializando o Sistema de Arquivos...\n");

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

    for(int i = 0; i < MAX_INODES; i++){
        fs->inodes[i].usado = 0;
        fs->diretorios[i].qtdFilhos = 0;
    }

    for(int i = 0; i < fs->super.totalBlocos; i++){
        fs->blocos[i].usado = 0;
        fs->blocos[i].bytesUtilizados = 0;
        fs->blocos[i].dados = malloc(tamanhoBloco); 
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

    log_verboso("FS Criado: %d blocos de %d bytes alocados em memoria.\n", fs->super.totalBlocos, tamanhoBloco);
    log_verboso("Diretorio raiz '/' criado e mapeado no i-node [0].\n");
    criarDiretorio(fs, ".lixeira");
}

// Diretorio

int criarDiretorio(SistemaDeArquivos *fs, char nome[]){
    int atual = fs->diretorioAtual;
    log_verboso("Comando mkdir: Tentando criar diretorio '%s' dentro do diretorio ID %d.\n", nome, atual);

    if(procurarFilho(fs, atual, nome) != -1){
        printf("Erro! Diretorio ja existente.\n");
        return -1;
    }

    log_verboso("Chamando criarInode() para alocar estrutura de diretorio...\n");
    int novo = criarInode(fs->inodes, DIRETORIO, nome, atual);

    if(novo == -1) return -1;

    Diretorio *pai = &fs->diretorios[atual];

    log_verboso("Vinculando o novo i-node %d como filho do diretorio atual (%d).\n", novo, atual);
    if (inserirFilho(pai, novo) == -1)
    {
        printf("Erro! Diretorio cheio.\n");
        return -1;
    }

    fs->super.inodesLivres--;
    log_verboso("Diretorio '%s' indexado com sucesso. I-nodes livres: %d.\n", nome, fs->super.inodesLivres);

    fs->qtdDiretorios++;
    return 0;
}

void listarDiretorio(SistemaDeArquivos *fs){
    Diretorio *dir = &fs->diretorios[fs->diretorioAtual];
    log_verboso("Comando ls: Lendo entradas do bloco/vetor de filhos do diretorio atual (ID %d). Total de filhos: %d.\n", fs->diretorioAtual, dir->qtdFilhos);

    printf("\n");
    for(int i = 0; i < dir->qtdFilhos; i++){
        int id = dir->filhos[i];
        if(fs->inodes[id].tipo == DIRETORIO){
            printf("[DIR] %s\n", fs->inodes[id].nome);
        } else {
            printf("[ARQ] %s\n", fs->inodes[id].nome);
        }
    }
    printf("\n");
}

int entrarDiretorio(SistemaDeArquivos *fs, char nome[]){
    log_verboso("Comando cd: Tentando navegar para '%s'.\n", nome);

    if(strcmp(nome, "..") == 0){
        int pai = fs->inodes[fs->diretorioAtual].pai;
        if(pai != -1){
            log_verboso("Subindo de nivel: Diretorio atual mudou de ID %d para pai ID %d.\n", fs->diretorioAtual, pai);
            fs->diretorioAtual = pai;
        } else {
            log_verboso("Ja esta na raiz. Impossivel subir mais.\n");
        }
        return 0;
    }

    int id = procurarFilho(fs, fs->diretorioAtual, nome);

    if(id == -1){
        printf("Erro! Diretorio inexistente!\n");
        return -1;
    }

    if(fs->inodes[id].tipo != DIRETORIO){
        printf("Erro! '%s' nao e um diretorio.\n", nome);
        return -1;
    }

    log_verboso("Navegacao bem sucedida! Diretorio atual atualizado para ID %d ('%s').\n", id, fs->inodes[id].nome);
    fs->diretorioAtual = id;

    return 0;
}

int removerDiretorio(SistemaDeArquivos *fs, char nome[]){
    log_verboso("Comando rmdir: Solicitada a remocao do diretorio '%s'.\n", nome);
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
        printf("Erro! Nao e um diretorio.\n");
        return -1;
    }

    if(fs->diretorios[id].qtdFilhos > 0){
        printf("Erro! O diretorio nao esta vazio (possui %d filhos).\n", fs->diretorios[id].qtdFilhos);
        return -1;
    }

    Diretorio *pai = &fs->diretorios[fs->diretorioAtual];
    log_verboso("Removendo ID %d da lista de referencias do diretorio pai.\n", id);
    
    int pos = -1;
    for(int i = 0; i < pai->qtdFilhos; i++){
        if(pai->filhos[i] == id){
            pos = i;
            break;
        }
    }

    if(pos == -1) return -1;

    for(int i = pos; i < pai->qtdFilhos - 1; i++){
        pai->filhos[i] = pai->filhos[i + 1];
    }
    pai->qtdFilhos--;

    log_verboso("Chamando removerInode() para liberar o i-node %d.\n", id);
    removerInode(fs->inodes, id);

    fs->super.inodesLivres++;
    log_verboso("Diretorio removido. I-nodes livres atualizados para: %d.\n", fs->super.inodesLivres);

    return 0;
}

// Arquivo

int criarArquivo(SistemaDeArquivos *fs, char nome[]){

    int atual = fs->diretorioAtual;
    log_verboso("Comando touch: Criando entrada vazia para arquivo '%s' no diretorio ID %d.\n", nome, atual);

    if (procurarFilho(fs, atual, nome) != -1) {
        printf("Erro! Item ja existente.\n");
        return -1;
    }

    int id = criarInode(fs->inodes, ARQUIVO, nome, atual);
    if (id == -1) return -1;

    fs->inodes[id].tamanho = 0;
    fs->inodes[id].quantidadeBlocos = 0;

    for(int i = 0; i < MAX_BLOCOS_ARQUIVO; i++)
        fs->inodes[id].blocos[i] = -1;

    Diretorio *pai = &fs->diretorios[atual];
    log_verboso("Vinculando i-node do arquivo (%d) no diretorio pai.\n", id);
    if (inserirFilho(pai, id) == -1)
    {
        printf("Erro! Diretorio cheio.\n");
        return -1;
    }

    fs->super.inodesLivres--;
    log_verboso("Arquivo criado com sucesso (Tamanho: 0 bytes, i-node: %d).\n", id);
    fs->qtdArquivos++;

    return 0;
}

void listarConteudoArquivo(SistemaDeArquivos *fs, char nome[]){
    log_verboso("Comando cat: Buscando arquivo '%s' para leitura.\n", nome);
    int id = procurarFilho(fs, fs->diretorioAtual, nome);

    if(id == -1 || fs->inodes[id].tipo != ARQUIVO) {
        printf("Erro! Arquivo invalido ou nao encontrado.\n");
        return;
    }

    log_verboso("Lendo mapeamento de i-node %d. Arquivo mapeia %d blocos fisicos.\n", id, fs->inodes[id].quantidadeBlocos);

    char *buffer = malloc(fs->super.tamanhoBloco + 1);
    printf("\n");
    for(int i = 0; i < fs->inodes[id].quantidadeBlocos; i++)
    {
        int blocoFisico = fs->inodes[id].blocos[i];
        log_verboso("Acessando bloco logico %d -> Bloco Fisico #%d (%d bytes utlizados).\n", i, blocoFisico, fs->blocos[blocoFisico].bytesUtilizados);
        
        lerBloco(fs, blocoFisico, buffer);
        buffer[fs->blocos[blocoFisico].bytesUtilizados] = '\0';
        printf("%s", buffer);
    }
    printf("\n\n");
    free(buffer);
}

int importarArquivo(SistemaDeArquivos *fs, char nomeSimulado[], char caminhoArquivo[]){
    log_verboso("Comando importar: Tentando ler do S.O. Real '%s' para o arquivo virtual '%s'.\n", caminhoArquivo, nomeSimulado);

    int id = procurarFilho(fs, fs->diretorioAtual, nomeSimulado);
    if (id == -1 || fs->inodes[id].tipo != ARQUIVO) {
        printf("Erro: Crie o arquivo virtual com 'touch %s' antes de importar dados.\n", nomeSimulado);
        return -1;
    }
    
    FILE *arquivo = fopen(caminhoArquivo, "rb");
    if (arquivo == NULL) {
        printf("Erro! Arquivo real nao encontrado no Linux.\n");
        return -1;
    }
    
    fseek(arquivo, 0, SEEK_END);
    long tamanho = ftell(arquivo);
    rewind(arquivo);
    
    int quantidadeBlocos = (tamanho + fs->super.tamanhoBloco - 1) / fs->super.tamanhoBloco;
    log_verboso("Analise do arquivo real: %ld bytes detectados. Tamanho Bloco: %d bytes. Blocos necessarios: %d.\n", 
                 tamanho, fs->super.tamanhoBloco, quantidadeBlocos);
    
    if (quantidadeBlocos > MAX_BLOCOS_ARQUIVO){
        printf("Erro! Arquivo excede o limite de %d blocos por arquivo.\n", MAX_BLOCOS_ARQUIVO);
        fclose(arquivo);
        return -1;
    }
    
    if (quantidadeBlocos > fs->super.blocosLivres){
        printf("Erro! Espaço insuficiente em disco virtual (%d blocos livres, necessita %d).\n", fs->super.blocosLivres, quantidadeBlocos);
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
        log_verboso("Processando indexacao do bloco logico [%d/%d]...\n", i+1, quantidadeBlocos);
        int indiceBloco = encontrarBlocoLivre(fs);
        
        if (indiceBloco == -1){
            free(buffer);
            fclose(arquivo);
            return -1;
        }
        
        log_verboso("-> Alocando bloco fisico livre de indice #%d.\n", indiceBloco);
        fs->blocos[indiceBloco].usado = 1;
        fs->super.blocosLivres--;
        
        int bytesLidos = fread(buffer, 1, fs->super.tamanhoBloco, arquivo);
        log_verboso("-> Copiando %d bytes para dentro do bloco #%d.\n", bytesLidos, indiceBloco);
        
        memcpy(fs->blocos[indiceBloco].dados, buffer, bytesLidos);
        fs->blocos[indiceBloco].bytesUtilizados = bytesLidos;
        
        fs->inodes[id].blocos[i] = indiceBloco;
        log_verboso("-> I-node %d apontou vetor 'blocos[%d]' para o bloco fisico #%d.\n", id, i, indiceBloco);
    }
    
    free(buffer);
    fclose(arquivo);
    
    fs->inodes[id].modificado = time(NULL);
    fs->inodes[id].acessado = time(NULL);
    
    log_verboso("Importacao concluida com sucesso! Metadados de tempo atualizados.\n");
    return 0;
}
        
// Bloco de dados

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
    log_verboso("Limpando dados fisicos do bloco #%d e desmarcando no bitmap (usado = 0).\n", bloco);
    fs->blocos[bloco].usado = 0;
    fs->blocos[bloco].bytesUtilizados = 0;
    memset(fs->blocos[bloco].dados, 0, fs->super.tamanhoBloco);
    fs->super.blocosLivres++;
}

int escreverBloco(SistemaDeArquivos *fs, int bloco, const char *dados, int bytes){
    if(bytes > fs->super.tamanhoBloco)
        return -1;

    memcpy(fs->blocos[bloco].dados, dados, bytes);
    return 0;
}

int lerBloco(SistemaDeArquivos *fs, int bloco, char *destino){
    memcpy(destino, fs->blocos[bloco].dados, fs->super.tamanhoBloco);
    return 0;
}

// Gerais

int renomear(SistemaDeArquivos *fs, char nomeAtual[], char novoNome[]){
    log_verboso("Comando renomear: Alterando nome de '%s' para '%s'.\n", nomeAtual, novoNome);
    int id = procurarFilho(fs, fs->diretorioAtual, nomeAtual);

    if (id == -1) return -1;

    if (procurarFilho(fs, fs->diretorioAtual, novoNome) != -1) {
        printf("Erro! Nome '%s' ja esta em uso neste diretorio.\n", novoNome);
        return -1;
    }

    strcpy(fs->inodes[id].nome, novoNome);
    fs->inodes[id].modificado = time(NULL);
    return 0;
}

int mover(SistemaDeArquivos *fs, char nome[], char destino[])
{
    int diretorioOriginal = fs->diretorioAtual;
    log_verboso("Comando mover: Re-indexando '%s' para o destino '%s'.\n", nome, destino);

    int idParaMover = procurarFilho(fs, diretorioOriginal, nome);
    if (idParaMover == -1)
    {
        printf("Erro! Item '%s' nao encontrado.\n", nome);
        return -1;
    }

    if (entrarDiretorio(fs, destino) == -1)
    {
        fs->diretorioAtual = diretorioOriginal; 
        return -1;
    }

    int novoPai = fs->diretorioAtual; 

    if (idParaMover == novoPai)
    {
        printf("Erro! Impossivel mover um item para dentro de si mesmo.\n");
        fs->diretorioAtual = diretorioOriginal;
        return -1;
    }

    if (procurarFilho(fs, novoPai, nome) != -1)
    {
        printf("Erro! Ja existe um item com o nome '%s' no destino.\n", nome);
        fs->diretorioAtual = diretorioOriginal; 
        return -1;
    }

    fs->diretorioAtual = diretorioOriginal;

    Diretorio *origem = &fs->diretorios[diretorioOriginal];
    Diretorio *dest = &fs->diretorios[novoPai];

    log_verboso("Removendo ID %d do diretorio pai antigo (%d).\n", idParaMover, diretorioOriginal);
    if (removerFilho(origem, idParaMover) == -1)
        return -1;

    log_verboso("Adicionando ID %d no vetor de filhos do novo diretorio pai (%d).\n", idParaMover, novoPai);
    if (inserirFilho(dest, idParaMover) == -1)
    {
        printf("Erro! Diretorio de destino cheio.\n");
        inserirFilho(origem, idParaMover); 
        return -1;
    }

    fs->inodes[idParaMover].pai = novoPai;
    fs->inodes[idParaMover].modificado = time(NULL);

    printf("'%s' movido para '%s' com sucesso!\n", nome, destino);
    return 0;
}

int apagar(SistemaDeArquivos *fs, char nome[])
{
    int atual = fs->diretorioAtual;
    log_verboso("Comando rm: Solicitado expurgo de arquivo '%s'.\n", nome);

    int id = procurarFilho(fs, atual, nome);
    if (id == -1) {
        printf("Erro! Arquivo nao encontrado.\n");
        return -1;
    }

    if (fs->inodes[id].tipo != ARQUIVO) {
        printf("Erro! '%s' e um diretorio (use rmdir).\n", nome);
        return -1;
    }

    log_verboso("Varrer mapa de blocos do i-node %d para desalocacao fisica...\n", id);
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
            log_verboso("-> Desalocando bloco fisico #%d associado ao arquivo.\n", blocoParaLiberar);
            liberarBloco(fs, blocoParaLiberar);
        }
    }

    Diretorio *pai = &fs->diretorios[atual];
    log_verboso("Limpando vinculo do arquivo na lista do diretorio pai.\n");
    if (removerFilho(pai, id) == -1) return -1;

    removerInode(fs->inodes, id);
    fs->super.inodesLivres++;

    printf("Arquivo '%s' apagado com sucesso e blocos liberados!\n", nome);
    return 0;
}

// [Opcional - Adicionado para complementar o Modo Verboso e enriquecer o Relatório]
void exibirMapeamentoBlocos(SistemaDeArquivos *fs) {
    printf("\n=== BITMAP / MAPA VISUAL DOS BLOCOS DE DADOS ===\n");
    printf("Total de blocos: %-5d | Livres: %-5d | Usados: %-5d\n\n", 
            fs->super.totalBlocos, fs->super.blocosLivres, fs->super.totalBlocos - fs->super.blocosLivres);
    
    for (int i = 0; i < fs->super.totalBlocos; i++) {
        if (fs->blocos[i].usado) {
            printf("\033[1;31m[X]\033[0m "); // Vermelho para ocupado
        } else {
            printf("[.] "); // Padrão/Branco para livre
        }
        
        if ((i + 1) % 16 == 0) printf("\n"); // Quebra a linha a cada 16 blocos
    }
    printf("\n================================================\n");
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
            printf("%s  pai=%d\n", fs->inodes[i].nome, fs->inodes[i].pai);
        }
    }
    printf("==============================\n");
}

static void desenharArvoreRecursivo(SistemaDeArquivos *fs, int idAtual, int nivel, int ehUltimoFilho, char *prefixo)
{
    if (nivel > 0) {
        printf("%s%s ", prefixo, ehUltimoFilho ? "L_" : "|--");
    }

    if (fs->inodes[idAtual].tipo == DIRETORIO) {
        if (nivel == 0) printf("[%s]\n", fs->inodes[idAtual].nome);
        else printf("[%s/]\n", fs->inodes[idAtual].nome);
    } else {
        printf("%s (%d bytes, %d blocos)\n", 
               fs->inodes[idAtual].nome, fs->inodes[idAtual].tamanho, fs->inodes[idAtual].quantidadeBlocos);
    }

    if (fs->inodes[idAtual].tipo == DIRETORIO)
    {
        Diretorio *dir = &fs->diretorios[idAtual];
        char *novoPrefixo = malloc(strlen(prefixo) + 10);
        
        for (int i = 0; i < dir->qtdFilhos; i++)
        {
            int idFilho = dir->filhos[i];
            int ultimo = (i == dir->qtdFilhos - 1);

            if (nivel > 0) sprintf(novoPrefixo, "%s%s   ", prefixo, ehUltimoFilho ? " " : "|");
            else strcpy(novoPrefixo, "");

            desenharArvoreRecursivo(fs, idFilho, nivel + 1, ultimo, novoPrefixo);
        }
        free(novoPrefixo);
    }
}

void exibirArvore(SistemaDeArquivos *fs)
{
    printf("\n");
    desenharArvoreRecursivo(fs, fs->raiz, 0, 1, "");
    printf("\n");
}

void pwd(SistemaDeArquivos *fs)
{
    int caminho[MAX_INODES];
    int n = 0;
    int atual = fs->diretorioAtual;

    while (atual != -1) {
        caminho[n++] = atual;
        atual = fs->inodes[atual].pai;
    }
    printf("\n/");
    for (int i = n - 2; i >= 0; i--) {
        printf("%s", fs->inodes[caminho[i]].nome);
        if (i != 0) printf("/");
    }
    printf("\n\n");
}

void obterCaminho(SistemaDeArquivos *fs, char *caminhoFinal)
{
    int caminho[MAX_INODES];
    int n = 0;
    int atual = fs->diretorioAtual;

    while (atual != -1) {
        caminho[n++] = atual;
        atual = fs->inodes[atual].pai;
    }
    strcpy(caminhoFinal, "/");
    for (int i = n - 2; i >= 0; i--) {
        strcat(caminhoFinal, fs->inodes[caminho[i]].nome);
        if (i != 0) strcat(caminhoFinal, "/");
    }
}
