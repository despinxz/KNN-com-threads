
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#define MAX_LINE_LENGTH 1024

// =================== Funções para ler arquivos ==================
// Função para contar as linhas em um arquivo
int contaLinhas(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return -1;
    }

    int count = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {   // Se o caractere for uma quebra de linha, incrementa o contador
            count++;
        }
    }

    count++;

    fclose(file);
    return count;
}

// Função para checar se uma string é um double válido
bool isDouble(const char *str) {
    char *endptr;
    strtod(str, &endptr);
    if (endptr == str) {
        return false;
    }
    return true;
}

// Função para contar o número de colunas do arquivo
int contaColunas(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return -1;
    }

    char linha[MAX_LINE_LENGTH];
    fgets(linha, sizeof(linha), file);

    int count = 0;

    char *token = strtok((char *)linha, ",");
    while (token != NULL) {
        if (isDouble(token)) {
            count++;
        }
        token = strtok(NULL, ",");
    }

    return count;
}

// Função para armazenar arquivo numa matriz
double** lerArq(const char *filename, int linhas, int colunas) {

    // Aloca memória para a matriz
    double **matriz = (double **)malloc(linhas * sizeof(double *));
    for(int i = 0; i < linhas; i++)
        matriz[i] = (double *)malloc(colunas * sizeof(double));

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return NULL;
    }

    int row = 0, col = 0;
    char linha[MAX_LINE_LENGTH];

    while (fgets(linha, sizeof(linha), file)) {
        char *token = strtok(linha, ",");
        col = 0;
        while (token != NULL) {
            matriz[row][col++] = atof(token);
            token = strtok(NULL, ",");
        }
        row++;
    }
    fclose(file);

    // Retorna ponteiro para a matriz
    return matriz;
}

// Função para printar a matriz
// (usada para testar a leitura dos arquivos)
void printMatriz(double **matriz, int linhas, int colunas) {
    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {
            printf("%.2f ", matriz[i][j]);
        }
    }
}

// Função para escrever o ytest
void escreveArquivo(double *ytest, int linhas, int  tamanho) {
    char filename[50];
    sprintf(filename, "ytest%d.txt", tamanho);

    FILE *file = fopen(filename, "w");

    for (int i = 0; i < linhas; i++) {
        fprintf(file, "%.1f\n", ytest[i]);
    }

    fclose(file);
}

// Função para printar um vetor em formato de vetor em R
// (usada para formar os gráficos de tempo de execução)
void printVetor(double *vetor, int linhas) {
    printf("c(");
    for (int i = 0; i < linhas; i++) {
        printf("%f", vetor[i]);
        if (i < linhas - 1) printf(", ");
    }
    printf(")\n");
}

// ============== Funções para criar a fila de pontos mais próximos =============
// Estrutura para armazenar a informação de cada ponto
typedef struct aux {
    double distancia;
    double classe;
    struct aux * prox;
} Node;

// Estrutura para armazenar a lista de pontos mais próximos
typedef struct {
    Node * primeiro;
    int tam_atual;
    int tam_max;
} Pontos_Proximos;

// Função para inicializar lista
Pontos_Proximos * init(int tamanho) {
    Pontos_Proximos * lista = (Pontos_Proximos *)malloc(sizeof(Pontos_Proximos));
    lista->primeiro = NULL;
    lista->tam_atual = 0;
    lista->tam_max = tamanho;
    return lista;
}

// Função para inserir ponto na lista
void insert(Pontos_Proximos * lista, double distancia, double classe) {
    lista->tam_atual++;

    // Cria novo elemento
    Node * new_node = (Node *)malloc(sizeof(Node));
    new_node->distancia = distancia;
    new_node->classe = classe;
    new_node->prox = NULL;

    // Insere
    if (lista->primeiro == NULL) {    // Caso a lista ainda esteja vazia
        lista->primeiro = new_node;
    } else {                          // Caso a lista já tenha elementos
        Node * atual = lista->primeiro;
        Node * anterior = NULL;
        while (atual != NULL && atual->distancia > distancia) {  // Itera sobre a lista até achar a posição correta
            anterior = atual;
            atual = atual->prox;
        }
        if (anterior == NULL) {     // Caso o novo elemento deve ser inserido no início da lista
            new_node->prox = lista->primeiro;
            lista->primeiro = new_node;
        } else {                    // Caso o novo elemento deve ser inserido no meio ou no final da lista
            new_node->prox = atual;
            anterior->prox = new_node;
        }
    }

    // Checa se a lista excedeu seu tamanho máximo
    if (lista->tam_atual > lista->tam_max) {
        Node * to_remove = lista->primeiro;
        lista->primeiro = lista->primeiro->prox;
        free(to_remove);
        lista->tam_atual--;
    }
}

// Função para exibir lista
// (usada para testar a construção da lista de pontos mais próximos
void print_lista(Pontos_Proximos * lista) {
    Node * atual = lista->primeiro;
    while (atual != NULL) {
        printf("(%.2f, %.2f) ", atual->distancia, atual->classe);
        atual = atual->prox;
        if (atual != NULL) printf("-> ");
        else printf("\n");
    }
}

// =================== Funções KNN ==================
// Função para calcular a distância entre dois pontos
double calc_dist(double * line_train, double * line_test, int len_line) {
    double x, y, aux, dist = 0.0;
    for (int i = 0; i < len_line; i++) {
        x = line_train[i];
        y = line_test[i];
        aux = x - y;
        dist += aux * aux;
    }
    return dist;
}

// Função para calcular a classe
double calc_classe(Pontos_Proximos * lista) {
    int count_zero = 0, count_one = 0;

    Node * atual = lista->primeiro;
    Node * ant;
    while (atual != NULL) {
        // Compara a classe com 0.5 para evitar erros de precisão
        if (atual->classe < 0.5) count_zero++;
        else count_one++;
        ant = atual;
        atual = atual->prox;
    }

    // Se o contador de ambos for igual, retorna a classe do número na última posição da lista
    if (count_zero == count_one) return ant->classe;

    // Caso contrário, retorna a classe com maior número de incidência
    else return (count_zero > count_one) ? 0.0 : 1.0;
}

// KNN paralelizada
void knn_threads(int k, double **xtrain, double **ytrain, double **xtest, double *ytest, int n_xtrain, int n_xtest, int num_cols) {
    // Calcula para cada ponto de xtest (paralelizado)
    #pragma omp parallel for
    for (int i = 0; i < n_xtest; i++) {
        // Inicializa lista de pontos próximos
        Pontos_Proximos *lista = init(k);

        // Calcula distância de cada ponto de xtrain
        for (int j = 0; j < n_xtrain; j++) {
            double dist = calc_dist(xtrain[j], xtest[i], num_cols);
            if (lista->tam_atual < k || dist < lista->primeiro->distancia) {
                // Se o ponto for mais próximo que o primeiro da lista, insere
                    insert(lista, dist, ytrain[j][0]);
            }
        }

        // Calcula classe
        double classe = calc_classe(lista);

        // Armazena em ytest
        ytest[i] = classe;

        // Libera memória alocada para a lista de pontos próximos
        free(lista);
    }
}

// KNN não paralelizada
void knn_no_threads(int k, double **xtrain, double **ytrain, double **xtest,
                    double *ytest, int n_xtrain, int n_xtest, int num_cols) {
    // Calcula para cada ponto de xtest
    for (int i = 0; i < n_xtest; i++) {
        // Inicializa lista de pontos próximos
        Pontos_Proximos *lista = init(k);

        // Calcula distância de cada ponto de xtrain
        for (int j = 0; j < n_xtrain; j++) {
            double dist = calc_dist(xtrain[j], xtest[i], num_cols);
            if (lista->tam_atual < k || dist < lista->primeiro->distancia) {
                // Se o ponto for mais próximo que o primeiro da lista, insere
                insert(lista, dist, ytrain[j][0]);
            }
        }

        // Calcula classe
        double classe = calc_classe(lista);

        // Armazena em ytest
        ytest[i] = classe;

        // Libera memória
        free(lista);
    }
}

int main() {
    // Armazenando input de k
    int k;
    printf("Insira o valor de k desejado: ");
    scanf("%d", &k);
    printf("\n");

    // Tamanhos de arquivos
    int sizes[] = {100, 500, 1000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    // Armazenando número de linhas e de colunas de xtest
    int n_xtest = contaLinhas("xtest.txt");
    int num_cols = contaColunas("xtest.txt");

    // Lendo xtest
    double ** xtest = lerArq("xtest.txt", n_xtest, num_cols);

    // Loop para cada tamanho possível de xtrain
    for (int s = 0; s < num_sizes; s++) {
        char xtrain_filename[50], ytrain_filename[50];

        // Cria string com o nome dos arquivos
        sprintf(xtrain_filename, "Arquivos de teste/xtrain%d.txt", sizes[s]);
        sprintf(ytrain_filename, "Arquivos de teste/ytrain%d.txt", sizes[s]);

        // Número de linhas de xtrain
        int n_xtrain = sizes[s];

        // Lendo xtrain
        double ** xtrain = lerArq(xtrain_filename, sizes[s], num_cols);

        // Lendo ytrain
        double ** ytrain = lerArq(ytrain_filename, sizes[s], num_cols);

        // Criando vetor ytest
        double * ytest = (double *)malloc(n_xtest * sizeof(double));

        clock_t start, end;
        double cpu_time_used;

        // Rodando KNN com threads
        start = clock();

        knn_threads(k, xtrain, ytrain, xtest, ytest, n_xtrain, n_xtest, num_cols);

        end = clock();

        cpu_time_used = (((double) (end - start)) / CLOCKS_PER_SEC) * 1000;
        printf("O programa com %d linhas levou %f ms para ser executado COM THREADS.\n", sizes[s], cpu_time_used);

        // Rodando KNN sem threads
        start = clock();

        knn_no_threads(k, xtrain, ytrain, xtest, ytest, n_xtrain, n_xtest, num_cols);

        end = clock();

        cpu_time_used = (((double) (end - start)) / CLOCKS_PER_SEC) * 1000;
        printf("O programa com %d linhas levou %f ms para ser executado SEM THREADS.\n", sizes[s], cpu_time_used);

        // Escrevendo ytest
        escreveArquivo(ytest, n_xtest, sizes[s]);
        free(ytest);
    }
    return 0;
}
