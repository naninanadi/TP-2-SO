#include "sistemaDeArquivos.h"

// --- FUNÇÕES AUXILIARES INTERNAS CORRIGIDAS ---

static int procurarFilho(SistemaDeArquivos *fs, int diretorioPai, char nome[]) {
    Diretorio *dir = &fs->diretorios[diretorioPai];

    for(int i = 0; i < dir->qtdFilhos; i++) {
        if(strcmp(dir->filhos[i].nome, nome) == 0) {
            return dir->filhos[i].inodeId;
        }
    }
    return -1;
}

static int inserirFilho(Diretorio *dir, int id, char nome[]) {
    if (dir->qtdFilhos >= MAX_FILHOS) return -1;
    strcpy(dir->filhos[dir->qtdFilhos].nome, nome);
    dir->filhos[dir->qtdFilhos].inodeId = id;
    dir->qtdFilhos++;
    return 0;
}

static int removerFilho(Diretorio *dir, int id) {
    int pos = -1;
    for (int i = 0; i < dir->qtdFilhos; i++) {
        if (dir->filhos[i].inodeId == id) {
            pos = i;
            break;
        }
    }
    if (pos == -1) return -1;
    for (int i = pos; i < dir->qtdFilhos - 1; i++) {
        dir->filhos[i] = dir->filhos[i + 1];
    }
    dir->qtdFilhos--;
    return 0;
}

// Função auxiliar essencial para descobrir o nome de um i-node olhando pelo pai dele
static void obterNomeDoInode(SistemaDeArquivos *fs, int id, char *destino) {
    if (id == fs->raiz) {
        strcpy(destino, "/");
        return;
    }
    int paiId = fs->inodes[id].pai;
    Diretorio *paiDir = &fs->diretorios[paiId];
    for (int i = 0; i < paiDir->qtdFilhos; i++) {
        if (paiDir->filhos[i].inodeId == id) {
            strcpy(destino, paiDir->filhos[i].nome);
            return;
        }
    }
    strcpy(destino, "desconhecido");
}

// --- FUNÇÕES PRINCIPAIS DO SISTEMA ---

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
    }

    fs->raiz = 0;
    fs->diretorioAtual = 0;

    fs->inodes[0].usado = 1;
    fs->inodes[0].id = 0;
    fs->inodes[0].tipo = DIRETORIO;
    fs->inodes[0].pai = -1; // Raiz não tem pai
    fs->inodes[0].quantidadeBlocos = 0;
    fs->super.inodesLivres--;
}

void destruirFS(SistemaDeArquivos *fs) {
    for(int i = 0; i < fs->super.totalBlocos; i++) {
        free(fs->blocos[i].dados);
    }
    free(fs->blocos);
}

int criarDiretorio(SistemaDeArquivos *fs, char nome[]) {
    int atual = fs->diretorioAtual;
    if (procurarFilho(fs, atual, nome) != -1) {
        printf("Erro: Nome '%s' ja existe neste diretorio.\n", nome);
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
    return id;
}

int criarArquivo(SistemaDeArquivos *fs, char nome[]) {
    int atual = fs->diretorioAtual;

    if (procurarFilho(fs, atual, nome) != -1) {
        printf("Erro: Nome '%s' ja existe neste diretorio.\n", nome);
        return -1;
    }

    
    int id = criarInode(fs->inodes, ARQUIVO, atual);
    if (id == -1) return -1;

    if (inserirFilho(&fs->diretorios[atual], id, nome) == -1) {
        removerInode(fs->inodes, id);
        return -1;
    }
    fs->super.inodesLivres--;
    return id;
}

int entrarDiretorio(SistemaDeArquivos *fs, char nome[]) {
    if (strcmp(nome, "..") == 0) {
        if (fs->diretorioAtual == fs->raiz) return 0; // Já está na raiz
        fs->diretorioAtual = fs->inodes[fs->diretorioAtual].pai;
        return 0;
    }
    int id = procurarFilho(fs, fs->diretorioAtual, nome);
    if (id == -1 || fs->inodes[id].tipo != DIRETORIO) {
        printf("Diretorio '%s' nao encontrado.\n", nome);
        return -1;
    }
    fs->diretorioAtual = id;
    return 0;
}

int removerDiretorio(SistemaDeArquivos *fs, char nome[]) {
    int id = procurarFilho(fs, fs->diretorioAtual, nome);
    if (id == -1 || fs->inodes[id].tipo != DIRETORIO) {
        printf("Erro: Diretorio nao encontrado.\n");
        return -1;
    }
    if (fs->diretorios[id].qtdFilhos > 0) {
        printf("Erro: Diretorio '%s' nao esta vazio!\n", nome);
        return -1;
    }
    removerFilho(&fs->diretorios[fs->diretorioAtual], id);
    removerInode(fs->inodes, id);
    fs->super.inodesLivres++;
    return 0;
}

int renomear(SistemaDeArquivos *fs, char antigo[], char novo[]) {
    int atual = fs->diretorioAtual;
    if (procurarFilho(fs, atual, novo) != -1) {
        printf("Erro: Nome '%s' ja existe no destino.\n", novo);
        return -1;
    }
    Diretorio *dir = &fs->diretorios[atual];
    for (int i = 0; i < dir->qtdFilhos; i++) {
        if (strcmp(dir->filhos[i].nome, antigo) == 0) {
            strcpy(dir->filhos[i].nome, novo);
            fs->inodes[dir->filhos[i].inodeId].modificado = time(NULL);
            return 0;
        }
    }
    printf("Erro: Item '%s' nao encontrado.\n", antigo);
    return -1;
}

int mover(SistemaDeArquivos *fs, char nome[], char destino[]) {
    int origPai = fs->diretorioAtual;
    int idParaMover = procurarFilho(fs, origPai, nome);
    if (idParaMover == -1) return -1;

    if (entrarDiretorio(fs, destino) == -1) return -1;
    int novoPai = fs->diretorioAtual;
    fs->diretorioAtual = origPai; // restaura

    if (procurarFilho(fs, novoPai, nome) != -1) {
        printf("Erro: Nome ja existente no destino.\n");
        return -1;
    }

    removerFilho(&fs->diretorios[origPai], idParaMover);
    inserirFilho(&fs->diretorios[novoPai], idParaMover, nome);
    fs->inodes[idParaMover].pai = novoPai;
    fs->inodes[idParaMover].modificado = time(NULL);
    return 0;
}

void listarDiretorio(SistemaDeArquivos *fs) {
    Diretorio *dir = &fs->diretorios[fs->diretorioAtual];
    if(dir->qtdFilhos == 0) {
        printf("(Diretorio Vazio)\n");
        return;
    }
    for(int i = 0; i < dir->qtdFilhos; i++) {
        int id = dir->filhos[i].inodeId;
        printf("%s \t\t [%s] \t Inode: %d \t Tam: %d bytes\n", 
               dir->filhos[i].nome, 
               fs->inodes[id].tipo == DIRETORIO ? "DIR" : "ARQ", 
               id, fs->inodes[id].tamanho);
    }
}

void pwd(SistemaDeArquivos *fs) {
    int caminho[MAX_INODES];
    int n = 0;
    int atual = fs->diretorioAtual;

    while (atual != -1) {
        caminho[n++] = atual;
        atual = fs->inodes[atual].pai;
    }

    printf("/");
    for (int i = n - 2; i >= 0; i--) {
        char nomePasta[MAX_NOME];
        obterNomeDoInode(fs, caminho[i], nomePasta);
        printf("%s", nomePasta);
        if (i != 0) printf("/");
    }
    printf("\n");
}

// --- VISUALIZAÇÃO GRÁFICA DA ÁRVORE TOTALMENTE CORRIGIDA ---

static void desenharArvoreRecursivo(SistemaDeArquivos *fs, int idAtual, char *nomeVisual, int nivel, int ehUltimoFilho, char *prefixo) {
    if (nivel > 0) {
        printf("%s%s ", prefixo, ehUltimoFilho ? "\u2514\u2500\u2500" : "\u251c\u2500\u2500");
    }

    if (fs->inodes[idAtual].tipo == DIRETORIO) {
        printf("[%s/]\n", nomeVisual);
    } else {
        printf("%s (%d bytes, %d blocos)\n", nomeVisual, fs->inodes[idAtual].tamanho, fs->inodes[idAtual].quantidadeBlocos);
    }

    if (fs->inodes[idAtual].tipo == DIRETORIO) {
        Diretorio *dir = &fs->diretorios[idAtual];
        char *novoPrefixo = malloc(strlen(prefixo) + 20);
        
        for (int i = 0; i < dir->qtdFilhos; i++) {
            int idFilho = dir->filhos[i].inodeId;
            char *nomeFilho = dir->filhos[i].nome;
            int ultimo = (i == dir->qtdFilhos - 1);

            if (nivel > 0) {
                sprintf(novoPrefixo, "%s%s   ", prefixo, ehUltimoFilho ? " " : "\u2502");
            } else {
                strcpy(novoPrefixo, "");
            }
            desenharArvoreRecursivo(fs, idFilho, nomeFilho, nivel + 1, ultimo, novoPrefixo);
        }
        free(novoPrefixo);
    }
}

void exibirArvore(SistemaDeArquivos *fs) {
    desenharArvoreRecursivo(fs, fs->raiz, "/", 0, 1, "");
}

// --- SISTEMA DE ALOCAÇÃO DE BLOCOS DE DADOS ---

int alocarBloco(SistemaDeArquivos *fs) {
    for(int i = 0; i < fs->super.totalBlocos; i++) {
        if(!fs->blocos[i].usado) {
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
    if(bloco < 0 || bloco >= fs->super.totalBlocos || quantidadeBytes > fs->super.tamanhoBloco) return -1;
    memcpy(fs->blocos[bloco].dados, dados, quantidadeBytes);
    fs->blocos[bloco].bytesUtilizados = quantidadeBytes;
    return 0;
}

int lerBloco(SistemaDeArquivos *fs, int bloco, char *destino) {
    if(bloco < 0 || bloco >= fs->super.totalBlocos) return -1;
    memcpy(destino, fs->blocos[bloco].dados, fs->blocos[bloco].bytesUtilizados);
    return fs->blocos[bloco].bytesUtilizados;
}

int importarArquivo(SistemaDeArquivos *fs, char nomeSimulado[], char caminhoArquivo[]) {
    
    int id = procurarFilho(fs, fs->diretorioAtual, nomeSimulado);
    if (id == -1 || fs->inodes[id].tipo != ARQUIVO) {
        printf("Arquivo simulado nao encontrado.\n");
        return -1;
    }

    FILE *arquivo = fopen(caminhoArquivo, "rb");
    if (!arquivo) return -1;

    Inode *in = &fs->inodes[id];
    // Limpa blocos antigos se existirem
    for (int i = 0; i < in->quantidadeBlocos; i++) {
        liberarBloco(fs, in->blocos[i]);
    }
    in->quantidadeBlocos = 0;
    in->tamanho = 0;

    char *buffer = malloc(fs->super.tamanhoBloco);
    int bytesLidos;

    while ((bytesLidos = fread(buffer, 1, fs->super.tamanhoBloco, arquivo)) > 0) {
        if (in->quantidadeBlocos >= MAX_BLOCOS_ARQUIVO) {
            printf("Erro: Arquivo excede o tamanho maximo permitido.\n");
            break;
        }
        int indiceBloco = alocarBloco(fs);
        if (indiceBloco == -1) {
            printf("Erro: Disco Cheio.\n");
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
}

int apagar(SistemaDeArquivos *fs, char nome[]) {
    int id = procurarFilho(fs, fs->diretorioAtual, nome);
    if (id == -1 || fs->inodes[id].tipo != ARQUIVO) {
        printf("Erro: Arquivo nao encontrado.\n");
        return -1;
    }
    Inode *in = &fs->inodes[id];
    for (int i = 0; i < in->quantidadeBlocos; i++) {
        liberarBloco(fs, in->blocos[i]);
    }
    removerFilho(&fs->diretorios[fs->diretorioAtual], id);
    removerInode(fs->inodes, id);
    fs->super.inodesLivres++;
    return 0;
}