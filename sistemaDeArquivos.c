#include <math.h>
#include <stdarg.h>
#include "headers/sistemaDeArquivos.h"

int modo_verboso = 0;

/* Centraliza os prints verbosos com cores no terminal */
void log_verboso(const char *format, ...) {
    if (modo_verboso) {
        printf("\033[1;33m[VERBOSO]\033[0m ");
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

// --- FUNÇÕES AUXILIARES INTERNAS CORRIGIDAS ---

/* Procura o ID do i-node de um filho pelo nome dentro de um diretorio pai */
static int procurarFilho(SistemaDeArquivos *fs, int diretorioPai, char nome[]) {
    Diretorio *dir = &fs->diretorios[diretorioPai];
    for(int i = 0; i < dir->qtdFilhos; i++) {
        if(strcmp(dir->filhos[i].nome, nome) == 0) {
            return dir->filhos[i].inodeId;
        }
    }
    return -1;
}

/* Insere uma EntradaDiretorio contendo nome e ID do i-node no pai */
static int inserirFilho(Diretorio *dir, int id, char nome[]) {
    if (dir->qtdFilhos >= MAX_FILHOS) {
        return -1;
    }
    strcpy(dir->filhos[dir->qtdFilhos].nome, nome);
    dir->filhos[dir->qtdFilhos].inodeId = id;
    dir->qtdFilhos++;
    return 0;
}

/* Remove um filho reorganizando o array para nao deixar buracos */
static int removerFilho(Diretorio *dir, int id) {
    int pos = -1;

    for (int i = 0; i < dir->qtdFilhos; i++) {
        if (dir->filhos[i].inodeId == id) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        return -1;
    }

    // Desloca todos os elementos posteriores uma posicao para tras
    for (int i = pos; i < dir->qtdFilhos - 1; i++) {
        dir->filhos[i] = dir->filhos[i + 1];
    }

    // Limpa a ultima posicao antiga
    dir->filhos[dir->qtdFilhos - 1].inodeId = -1;
    dir->filhos[dir->qtdFilhos - 1].nome[0] = '\0';

    dir->qtdFilhos--;
    return 0;
}

/* Obtem o nome de um i-node a partir das entradas do seu pai */
static void obterNomeDoInode(SistemaDeArquivos *fs, int id, char *destino) {
    if (id == fs->raiz) {
        strcpy(destino, "/");
        return;
    }
    int paiId = fs->inodes[id].pai;
    if (paiId == -1) {
        strcpy(destino, "/");
        return;
    }
    Diretorio *paiDir = &fs->diretorios[paiId];
    for (int i = 0; i < paiDir->qtdFilhos; i++) {
        if (paiDir->filhos[i].inodeId == id) {
            strcpy(destino, paiDir->filhos[i].nome);
            return;
        }
    }
    strcpy(destino, "desconhecido");
}

// --- FUNÇÕES DE NAVEGAÇÃO E GERENCIAMENTO DE DIRETÓRIOS ---

void inicializarFS(SistemaDeArquivos *fs, int tamanhoDisco, int tamanhoBloco) {
    fs->super.tamanhoDisco = tamanhoDisco;
    fs->super.tamanhoBloco = tamanhoBloco;
    fs->super.totalBlocos = tamanhoDisco / tamanhoBloco;
    fs->super.blocosLivres = fs->super.totalBlocos;
    fs->super.totalInodes = MAX_INODES;
    fs->super.inodesLivres = MAX_INODES;

    fs->blocos = malloc(fs->super.totalBlocos * sizeof(Bloco));
    for(int i = 0; i < fs->super.totalBlocos; i++) {
        fs->blocos[i].usado = 0;
        fs->blocos[i].bytesUtilizados = 0;
        fs->blocos[i].dados = malloc(tamanhoBloco);
    }

    for(int i = 0; i < MAX_INODES; i++) {
        fs->inodes[i].usado = 0;
        fs->diretorios[i].qtdFilhos = 0;
        for (int j = 0; j < MAX_FILHOS; j++) {
            fs->diretorios[i].filhos[j].inodeId = -1;
            fs->diretorios[i].filhos[j].nome[0] = '\0';
        }
    }

    // Inicializa Raiz
    fs->raiz = 0;
    fs->diretorioAtual = 0;
    fs->inodes[0].usado = 1;
    fs->inodes[0].id = 0;
    fs->inodes[0].tipo = DIRETORIO;
    fs->inodes[0].pai = -1;
    fs->inodes[0].quantidadeBlocos = 0;
    fs->super.inodesLivres--;

    // Inicializa Lixeira (Lixeira sera o inode 1)
    int idLixeira = criarInode(fs->inodes, DIRETORIO, 0);
    fs->lixeira = idLixeira;
    inserirFilho(&fs->diretorios[0], idLixeira, ".lixeira");
    fs->super.inodesLivres--;

    fs->qtdArquivos = 0;
    fs->qtdDiretorios = 1; // Raiz
}

void destruirFS(SistemaDeArquivos *fs) {
    for(int i = 0; i < fs->super.totalBlocos; i++) {
        free(fs->blocos[i].dados);
    }
    free(fs->blocos);
}

int criarDiretorio(SistemaDeArquivos *fs, char nome[]) {
    int atual = fs->diretorioAtual;

    if (procurarFilho(fs, atual, nome) != -1 && fs->inodes->tipo == DIRETORIO) {
        printf("Erro: Ja existe uma pasta com o nome '%s'\n", nome);
        return -1;
    }

    int id = criarInode(fs->inodes, DIRETORIO, atual);
    if (id == -1) return -1;

    if (inserirFilho(&fs->diretorios[atual], id, nome) == -1) {
        removerInode(fs->inodes, id);
        return -1;
    }

    fs->diretorios[id].qtdFilhos = 0;
    fs->super.inodesLivres--;
    fs->qtdDiretorios++;

    log_verboso("Diretorio '%s' criado com sucesso. Inode associado: %d\n", nome, id);
    return id;
}

int entrarDiretorio(SistemaDeArquivos *fs, char nome[]) {
    if (strcmp(nome, "..") == 0) {
        if (fs->diretorioAtual == fs->raiz) {
            log_verboso("Navegacao: Ja se encontra no diretorio raiz.\n");
            return 0;
        }
        int pai = fs->inodes[fs->diretorioAtual].pai;
        fs->diretorioAtual = pai;
        log_verboso("Navegacao bem sucedida! Diretorio atual atualizado para o pai (ID %d).\n", pai);
        return 0;
    }

    int id = procurarFilho(fs, fs->diretorioAtual, nome);
    if (id == -1) {
        printf("Erro: Diretorio '%s' nao encontrado.\n", nome);
        return -1;
    }

    if (fs->inodes[id].tipo != DIRETORIO) {
        printf("Erro: '%s' nao e um diretorio.\n", nome);
        return -1;
    }

    fs->diretorioAtual = id;
    log_verboso("Navegacao bem sucedida! Diretorio atual atualizado para ID %d.\n", id);
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
        if(pai->filhos[i].inodeId == id){
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

int renomear(SistemaDeArquivos *fs, char antigo[], char novo[]) {
    int atual = fs->diretorioAtual;
    int id = procurarFilho(fs, atual, antigo);

    if (id == -1) {
        printf("Erro: Item '%s' nao encontrado.\n", antigo);
        return -1;
    }

    if (procurarFilho(fs, atual, novo) != -1) {
        printf("Erro: Ja existe um item com o nome '%s' neste diretorio.\n", novo);
        return -1;
    }

    Diretorio *dir = &fs->diretorios[atual];
    for (int i = 0; i < dir->qtdFilhos; i++) {
        if (dir->filhos[i].inodeId == id) {
            log_verboso("Comando renomear: Alterando nome de '%s' para '%s'.\n", antigo, novo);
            strcpy(dir->filhos[i].nome, novo);
            fs->inodes[id].modificado = time(NULL);
            return 0;
        }
    }

    return -1;
}

int mover(SistemaDeArquivos *fs, char nome[], char destino[]) {
    int atual = fs->diretorioAtual;
    int idParaMover = procurarFilho(fs, atual, nome);

    if (idParaMover == -1) {
        printf("Erro! Arquivo ou diretorio '%s' nao encontrado aqui.\n", nome);
        return -1;
    }

    if (entrarDiretorio(fs, destino) == -1) {
        printf("Erro! Destino '%s' invalido ou inexistente.\n", destino);
        fs->diretorioAtual = atual;
        return -1;
    }

    int novoPai = fs->diretorioAtual;
    fs->diretorioAtual = atual; // Restaura o diretorio original

    if (procurarFilho(fs, novoPai, nome) != -1) {
        printf("Erro! Ja existe um item com esse nome no diretorio de destino.\n");
        return -1;
    }

    Diretorio *origem = &fs->diretorios[atual];
    Diretorio *dest = &fs->diretorios[novoPai];

    if (removerFilho(origem, idParaMover) == -1) {
        return -1;
    }

    if (inserirFilho(dest, idParaMover, nome) == -1) {
        inserirFilho(origem, idParaMover, nome); // Desfaz
        return -1;
    }

    fs->inodes[idParaMover].pai = novoPai;
    fs->inodes[idParaMover].modificado = time(NULL);

    log_verboso("Mover: Item '%s' movido com sucesso para o diretorio ID %d.\n", nome, novoPai);
    return 0;
}

void listarDiretorio(SistemaDeArquivos *fs){
    Diretorio *dir = &fs->diretorios[fs->diretorioAtual];
    log_verboso("Comando ls: Lendo entradas do bloco/vetor de filhos do diretorio atual (ID %d). Total de filhos: %d.\n", fs->diretorioAtual, dir->qtdFilhos);

    printf("\n");
    for(int i = 0; i < dir->qtdFilhos; i++){
        int id = dir->filhos[i].inodeId;
        if(fs->inodes[id].tipo == DIRETORIO){
            printf("[DIR] %s\n", dir->filhos[i].nome);
        } else {
            printf("[ARQ] %s\n", dir->filhos[i].nome);
        }
    }
    printf("\n");
}

void pwd(SistemaDeArquivos *fs) {
    char caminho[512];
    obterCaminho(fs, caminho);
    printf("%s\n", caminho);
}

void obterCaminho(SistemaDeArquivos *fs, char *caminhoFinal) {
    int caminho[MAX_INODES];
    int n = 0;
    int atual = fs->diretorioAtual;

    while (atual != -1) {
        caminho[n++] = atual;
        atual = fs->inodes[atual].pai;
    }

    strcpy(caminhoFinal, "/");

    for (int i = n - 2; i >= 0; i--) {
        char nomePasta[MAX_NOME];
        obterNomeDoInode(fs, caminho[i], nomePasta);
        strcat(caminhoFinal, nomePasta);
        if (i != 0) {
            strcat(caminhoFinal, "/");
        }
    }
}

// --- VISUALIZAÇÃO GRÁFICA DA ÁRVORE ---

static void desenharArvoreRecursivo(SistemaDeArquivos *fs, int idAtual, char *nomeVisual, int nivel, int ehUltimoFilho, char *prefixo) {
    if (nivel > 0) {
        printf("%s%s ", prefixo, ehUltimoFilho ? "└──" : "├──");
    }

    if (fs->inodes[idAtual].tipo == DIRETORIO) {
        printf("[%s/]\n", nomeVisual);
    } else {
        printf("%s (%d bytes, %d blocos)\n", nomeVisual, fs->inodes[idAtual].tamanho, fs->inodes[idAtual].quantidadeBlocos);
    }

    if (fs->inodes[idAtual].tipo == DIRETORIO) {
        Diretorio *dir = &fs->diretorios[idAtual];
        char *novoPrefixo = malloc(strlen(prefixo) + 16);

        for (int i = 0; i < dir->qtdFilhos; i++) {
            int idFilho = dir->filhos[i].inodeId;
            char *nomeFilho = dir->filhos[i].nome;
            int ultimo = (i == dir->qtdFilhos - 1);

            if (nivel > 0) {
                sprintf(novoPrefixo, "%s%s   ", prefixo, ehUltimoFilho ? " " : "│");
            } else {
                strcpy(novoPrefixo, "");
            }

            desenharArvoreRecursivo(fs, idFilho, nomeFilho, nivel + 1, ultimo, novoPrefixo);
        }
        free(novoPrefixo);
    }
}

void exibirArvore(SistemaDeArquivos *fs) {
    printf("\n");
    desenharArvoreRecursivo(fs, fs->raiz, "", 0, 1, "");
    printf("\n");
}

// --- GERENCIAMENTO DE ARQUIVOS E BLOCOS DE DADOS ---

int alocarBloco(SistemaDeArquivos *fs) {
    for (int i = 0; i < fs->super.totalBlocos; i++) {
        if (!fs->blocos[i].usado) {
            fs->blocos[i].usado = 1;
            fs->blocos[i].bytesUtilizados = 0;
            fs->super.blocosLivres--;
            return i;
        }
    }
    return -1;
}

void liberarBloco(SistemaDeArquivos *fs, int bloco) {
    if (bloco >= 0 && bloco < fs->super.totalBlocos) {
        fs->blocos[bloco].usado = 0;
        fs->blocos[bloco].bytesUtilizados = 0;
        memset(fs->blocos[bloco].dados, 0, fs->super.tamanhoBloco);
        fs->super.blocosLivres++;
    }
}

int escreverBloco(SistemaDeArquivos *fs, int bloco, const char *dados, int quantidadeBytes) {
    if (bloco < 0 || bloco >= fs->super.totalBlocos || quantidadeBytes > fs->super.tamanhoBloco) {
        return -1;
    }
    memcpy(fs->blocos[bloco].dados, dados, quantidadeBytes);
    fs->blocos[bloco].bytesUtilizados = quantidadeBytes;
    return 0;
}

int lerBloco(SistemaDeArquivos *fs, int bloco, char *destino) {
    if (bloco < 0 || bloco >= fs->super.totalBlocos) {
        return -1;
    }
    memcpy(destino, fs->blocos[bloco].dados, fs->blocos[bloco].bytesUtilizados);
    return fs->blocos[bloco].bytesUtilizados;
}

int criarArquivo(SistemaDeArquivos *fs, char nome[]) {
    int atual = fs->diretorioAtual;

    if (procurarFilho(fs, atual, nome) != -1 && fs->inodes->tipo == ARQUIVO) {
        printf("Erro: Já existe um arquivo com o nome '%s'.\n", nome);
        return -1;
    }

    int id = criarInode(fs->inodes, ARQUIVO, atual);
    if (id == -1) {
        return -1;
    }

    Diretorio *pai = &fs->diretorios[atual];
    if (inserirFilho(pai, id, nome) == -1) {
        removerInode(fs->inodes, id);
        return -1;
    }

    fs->super.inodesLivres--;
    fs->qtdArquivos++;

    log_verboso("Criar Arquivo: '%s' alocado sob o Inode ID %d.\n", nome, id);
    return id;
}

int importarArquivo(SistemaDeArquivos *fs, char nomeSimulado[], char caminhoArquivo[]) {
    int id = procurarFilho(fs, fs->diretorioAtual, nomeSimulado);
    if (id == -1 || fs->inodes[id].tipo != ARQUIVO) {
        printf("Arquivo simulado nao encontrado.\n");
        return -1;
    }

    FILE *arquivo = fopen(caminhoArquivo, "rb");
    if (!arquivo) {
        return -1;
    }

    Inode *in = &fs->inodes[id];
    for (int i = 0; i < in->quantidadeBlocos; i++) {
        liberarBloco(fs, in->blocos[i]);
    }
    in->quantidadeBlocos = 0;
    in->tamanho = 0;

    char *buffer = malloc(fs->super.tamanhoBloco);
    int bytesLidos;

    while ((bytesLidos = fread(buffer, 1, fs->super.tamanhoBloco, arquivo)) > 0) {
        if (in->quantidadeBlocos >= MAX_BLOCOS_ARQUIVO) {
            printf("Erro: Arquivo excede o limite de blocos permitido.\n");
            break;
        }
        int indiceBloco = alocarBloco(fs);
        if (indiceBloco == -1) {
            printf("Erro: Disco simulado cheio!\n");
            break;
        }
        escreverBloco(fs, indiceBloco, buffer, bytesLidos);
        in->blocos[in->quantidadeBlocos++] = indiceBloco;
        in->tamanho += bytesLidos;
    }

    free(buffer);
    fclose(arquivo);
    in->modificado = time(NULL);
    return 0;
}

void listarConteudoArquivo(SistemaDeArquivos *fs, char nome[]) {
    int id = procurarFilho(fs, fs->diretorioAtual, nome);
    if (id == -1 || fs->inodes[id].tipo != ARQUIVO) {
        printf("Arquivo nao encontrado.\n");
        return;
    }
    Inode *in = &fs->inodes[id];
    char *buffer = malloc(fs->super.tamanhoBloco);
    for (int i = 0; i < in->quantidadeBlocos; i++) {
        int lidos = lerBloco(fs, in->blocos[i], buffer);
        fwrite(buffer, 1, lidos, stdout);
    }
    free(buffer);
    in->acessado = time(NULL);
    printf("\n");
}

// --- FUNÇÕES DA LIXEIRA (REMOÇÃO LÓGICA E RESTAURAÇÃO) ---

int apagar(SistemaDeArquivos *fs, char nome[]) {
    int atual = fs->diretorioAtual;
    int id = procurarFilho(fs, atual, nome);

    if (id == -1) {
        printf("Erro! Arquivo nao encontrado.\n");
        return -1;
    }

    if (fs->inodes[id].tipo != ARQUIVO) {
        printf("Erro! '%s' nao e um Arquivo.\n", nome);
        return -1;
    }

    // Move para a Lixeira
    removerFilho(&fs->diretorios[atual], id);
    inserirFilho(&fs->diretorios[fs->lixeira], id, nome);
    fs->inodes[id].paiOriginal = atual;
    fs->inodes[id].pai = fs->lixeira;

    printf("Arquivo '%s' enviado para a lixeira!\n", nome);
    return 0;
}

int apagarDaLixeira(SistemaDeArquivos *fs, char nome[]) {
    int lixeira = fs->lixeira;
    int id = procurarFilho(fs, lixeira, nome);

    if (id == -1) {
        printf("Erro! Arquivo nao encontrado na lixeira.\n");
        return -1;
    }

    Inode *in = &fs->inodes[id];
    for (int i = 0; i < in->quantidadeBlocos; i++) {
        int blocoParaLiberar = in->blocos[i];
        if (blocoParaLiberar != -1) {
            log_verboso("-> Desalocando bloco fisico #%d associado ao Arquivo.\n", blocoParaLiberar);
            liberarBloco(fs, blocoParaLiberar);
        }
    }

    removerFilho(&fs->diretorios[lixeira], id);
    removerInode(fs->inodes, id);
    fs->super.inodesLivres++;
    fs->qtdArquivos--;

    printf("Arquivo '%s' removido permanentemente com sucesso!\n", nome);
    return 0;
}

int restaurarDaLixeira(SistemaDeArquivos *fs, char nome[]) {
    int lixeira = fs->lixeira;
    int id = procurarFilho(fs, lixeira, nome);

    if (id == -1) {
        printf("Erro! Arquivo '%s' nao encontrado na lixeira.\n", nome);
        return -1;
    }

    int destino = fs->inodes[id].paiOriginal;
    
    // Se o diretorio original foi apagado, restaura na raiz
    if (!fs->inodes[destino].usado) {
        destino = fs->raiz;
    }

    removerFilho(&fs->diretorios[lixeira], id);
    inserirFilho(&fs->diretorios[destino], id, nome);
    
    fs->inodes[id].pai = destino;
    fs->inodes[id].modificado = time(NULL);

    printf("Arquivo '%s' restaurado com sucesso para o diretorio original!\n", nome);
    return 0;
}

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