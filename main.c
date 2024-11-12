#include <pthread.h>   // Biblioteca para threads POSIX.
#include <stdio.h>     // Biblioteca padrão de entrada e saída.
#include <stdlib.h>    // Biblioteca padrão para alocação de memória, conversão de tipos, etc.
#include <string.h>    // Biblioteca para manipulação de strings.
#include <sys/types.h> // Tipos de dados do sistema.
#include <sys/stat.h>  // Permite manipulação de atributos de arquivos.
#include <fcntl.h>     // Para controle de arquivos.
#include <unistd.h>    // Biblioteca para POSIX API, como manipulação de arquivos e threads.
#include <time.h>      // Biblioteca para manipulação de tempo.

#define MAX_THREADS 8    // Define o número máximo de threads.
#define BUFFER_SIZE 1024 // Define o tamanho inicial do buffer para armazenamento de inteiros.

typedef struct
{
    int *values;                // Array de valores inteiros que a thread irá ordenar.
    int size;                   // Tamanho do array de valores.
    struct timespec start_time; // Estrutura para armazenar o tempo de início da execução da thread.
    struct timespec end_time;   // Estrutura para armazenar o tempo de término da execução da thread.
} thread_data_t;                // Define um tipo de estrutura para os dados de cada thread.

// Declaração da função de comparação para a função qsort.
int compare(const void *a, const void *b);

// Função para processar os argumentos de entrada.
int process_arguments(int argc, char *argv[], int *num_threads, char ***input_files, int *num_files, char **output_file);

// Função para ler valores dos arquivos de entrada.
int read_input_files(char **input_files, int num_files, int ***values, int *total_values, int *file_sizes);

// Função de ordenação executada por cada thread.
void *thread_func(void *arg);

// Função para dividir o buffer entre as threads.
void divide_buffer(int *buffer, int total_values, int num_threads, thread_data_t *thread_data);

// Função para imprimir tempos de execução de cada thread e do programa.
void print_execution_times(struct timespec total_start_time, struct timespec total_end_time, thread_data_t *thread_data, int num_threads);

// Função para escrever o buffer ordenado no arquivo de saída.
int write_output_file(const char *output_file, int *buffer, int total_values);

int main(int argc, char *argv[])
{
    int num_threads;    // Número de threads especificado pelo usuário.
    char *output_file;  // Nome do arquivo de saída.
    int num_files;      // Número de arquivos de entrada.
    char **input_files; // Array de nomes de arquivos de entrada.

    // Processa os argumentos de entrada e verifica a validade.
    if (!process_arguments(argc, argv, &num_threads, &input_files, &num_files, &output_file))
    {
        return 1; // Retorna erro se os argumentos estiverem incorretos.
    }

    // Declara variáveis para os dados de entrada e lê os valores dos arquivos.
    int **values;              // Array para armazenar arrays de valores de cada arquivo.
    int total_values;          // Total de valores lidos.
    int file_sizes[num_files]; // Tamanho de cada arquivo em número de valores.
    if (read_input_files(input_files, num_files, &values, &total_values, file_sizes) != 0)
    {
        return 1; // Retorna erro se houver falha na leitura.
    }

    // Aloca buffer para armazenar todos os valores inteiros dos arquivos.
    int *buffer = malloc(total_values * sizeof(int));
    if (buffer == NULL)
    {
        perror("malloc");
        return 1;
    }

    // Copia valores lidos para o buffer principal e libera memória de arrays intermediários.
    int buffer_index = 0;
    for (int i = 0; i < num_files; i++)
    {
        memcpy(buffer + buffer_index, values[i], file_sizes[i] * sizeof(int));
        buffer_index += file_sizes[i];
        free(values[i]); // Libera memória do array do arquivo atual.
    }
    free(values); // Libera o array de arrays.

    pthread_t threads[num_threads];         // Array para IDs das threads.
    thread_data_t thread_data[num_threads]; // Dados das threads.

    // Marca o tempo inicial do programa antes de iniciar as threads.
    struct timespec total_start_time, total_end_time;
    clock_gettime(CLOCK_MONOTONIC, &total_start_time);

    // Divide o buffer para as threads e cria as threads.
    divide_buffer(buffer, total_values, num_threads, thread_data);
    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&threads[i], NULL, thread_func, &thread_data[i]);
    }

    // Espera até que todas as threads terminem sua execução.
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &total_end_time);

    // Ordena o buffer completo.
    qsort(buffer, total_values, sizeof(int), compare);

    // Imprime os tempos de execução de cada thread.
    print_execution_times(total_start_time, total_end_time, thread_data, num_threads);

    // Escreve o buffer ordenado no arquivo de saída.
    if (write_output_file(output_file, buffer, total_values) != 0)
    {
        free(buffer);
        return 1;
    }

    // Libera a memória do buffer.
    free(buffer);

    return 0;
}

int compare(const void *a, const void *b)
{
    // Função de comparação para o qsort, que compara inteiros.
    return (*(int *)a - *(int *)b);
}

void *thread_func(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg; // Dados da thread.
    int *values = data->values;                 // Valores que a thread vai ordenar.
    int size = data->size;                      // Tamanho do array.

    clock_gettime(CLOCK_MONOTONIC, &data->start_time); // Marca o início.
    qsort(values, size, sizeof(int), compare);         // Ordena os valores.
    clock_gettime(CLOCK_MONOTONIC, &data->end_time);   // Marca o término.

    return NULL;
}

int process_arguments(int argc, char *argv[], int *num_threads, char ***input_files, int *num_files, char **output_file)
{
    // Valida a quantidade de argumentos fornecidos.
    if (argc < 5)
    {
        fprintf(stderr, "Use: %s <num_threads> <input_file1> <input_file2> ... -o <output_file>\n", argv[0]);
        return 0;
    }

    *num_threads = atoi(argv[1]); // Converte o argumento para o número de threads.
    if (*num_threads != 1 && *num_threads != 2 && *num_threads != 4 && *num_threads != 8)
    {
        fprintf(stderr, "Número de threads inválido. Escolha 1, 2, 4 ou 8 threads.\n");
        return 0;
    }

    *output_file = NULL;
    *num_files = 0;

    // Lê os arquivos de entrada até encontrar o indicador de saída "-o".
    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            *output_file = argv[i + 1];
            break;
        }
        (*num_files)++;
    }

    // Verifica se os argumentos são válidos.
    if (!*output_file || *num_files == 0)
    {
        fprintf(stderr, "Erro: Formato de entrada inválido. Use: %s <num_threads> <input_file1> <input_file2> ... -o <output_file>\n", argv[0]);
        return 0;
    }

    *input_files = argv + 2; // Define os arquivos de entrada a partir do índice 2.
    return 1;
}

int read_input_files(char **input_files, int num_files, int ***values, int *total_values, int *file_sizes)
{
    // Aloca memória para o array que armazenará os valores dos arquivos.
    *values = malloc(num_files * sizeof(int *));
    if (*values == NULL)
    {
        perror("malloc");
        return 1;
    }

    *total_values = 0; // Inicializa a contagem total de valores.
    for (int i = 0; i < num_files; i++)
    {
        FILE *file = fopen(input_files[i], "r"); // Abre o arquivo.
        if (!file)
        {
            perror("fopen");
            return 1;
        }

        int value, count = 0, capacity = BUFFER_SIZE;
        (*values)[i] = malloc(capacity * sizeof(int)); // Aloca buffer inicial para os valores,resultando em uma matriz alocada dinamicamente
        if ((*values)[i] == NULL)
        {
            perror("malloc");
            fclose(file);
            return 1;
        }

        // Lê os valores do arquivo.
        while (fscanf(file, "%d", &value) == 1)
        {
            // Realoca o buffer caso a capacidade seja excedida.
            if (count >= capacity) // Dobra a capacidade para otimizar realocações
            {
                capacity *= 2;
                (*values)[i] = realloc((*values)[i], capacity * sizeof(int));
                if ((*values)[i] == NULL)
                {
                    perror("realloc");
                    fclose(file);
                    return 1;
                }
            }
            (*values)[i][count++] = value; // Armazena o valor lido.
        }
        fclose(file); // Fecha o arquivo.

        file_sizes[i] = count;  // Armazena o número de valores lidos do arquivo atual.
        *total_values += count; // Atualiza a contagem total de valores.
    }

    return 0;
}

void divide_buffer(int *buffer, int total_values, int num_threads, thread_data_t *thread_data)
{
    // Calcula o tamanho do bloco para cada thread.
    int chunk_size = (total_values + num_threads - 1) / num_threads;

    // Associa os blocos de valores a cada thread.
    for (int i = 0; i < num_threads; i++)
    {
        thread_data[i].values = buffer + i * chunk_size; // Define o início do bloco.
        thread_data[i].size = (i == num_threads - 1) ? (total_values - i * chunk_size) : chunk_size;
    }
}

void print_execution_times(struct timespec total_start_time, struct timespec total_end_time, thread_data_t *thread_data, int num_threads)
{
    // Calcula e imprime o tempo total de execução.
    struct timespec total_diff;
    total_diff.tv_sec = total_end_time.tv_sec - total_start_time.tv_sec;
    total_diff.tv_nsec = total_end_time.tv_nsec - total_start_time.tv_nsec;
    if (total_diff.tv_nsec < 0)
    {
        total_diff.tv_sec--;
        total_diff.tv_nsec += 1000000000; // Ajuste para manter tv_nsec entre 0 e 1 bilhão
    }

    // Calcula e imprime o tempo de execução de cada thread.
    for (int i = 0; i < num_threads; i++)
    {
        struct timespec diff;
        diff.tv_sec = thread_data[i].end_time.tv_sec - thread_data[i].start_time.tv_sec;
        diff.tv_nsec = thread_data[i].end_time.tv_nsec - thread_data[i].start_time.tv_nsec;
        if (diff.tv_nsec < 0)
        {
            diff.tv_sec--;
            diff.tv_nsec += 1000000000; // Ajuste de normalização
        }
        printf("Tempo de execução do Thread %d: %ld.%09ld segundos.\n", i, diff.tv_sec, diff.tv_nsec);
    }

    printf("Tempo total de execução: %ld.%09ld segundos.\n", total_diff.tv_sec, total_diff.tv_nsec);
}

int write_output_file(const char *output_file, int *buffer, int total_values)
{
    // Abre o arquivo de saída.
    FILE *fd_out = fopen(output_file, "w");
    if (!fd_out)
    {
        perror("fopen");
        return 1;
    }

    // Escreve os valores ordenados no arquivo.
    for (int i = 0; i < total_values; i++)
    {
        fprintf(fd_out, "%d\n", buffer[i]);
    }
    fclose(fd_out); // Fecha o arquivo.

    return 0;
}
