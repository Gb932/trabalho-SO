import random

# Gerar 1000 números inteiros aleatórios no intervalo de 0 a 10000
numeros = [str(random.randint(0, 10000)) for _ in range(1000)]

# Número de arquivos
num_arquivos = 10
# Número de números por arquivo
numeros_por_arquivo = 100

# Função para salvar os números em arquivos .dat
def salvar_numeros_em_arquivos(numeros, num_arquivos, numeros_por_arquivo):
    for i in range(num_arquivos):
        # Seleciona os números para este arquivo
        numeros_para_arquivo = numeros[i * numeros_por_arquivo:(i + 1) * numeros_por_arquivo]
        # Nome do arquivo
        nome_arquivo = f"arquivo_{i + 1}.dat"
        # Abrir o arquivo em modo de escrita
        with open(nome_arquivo, 'w') as f:
            # Escrever os números como caracteres, cada um em uma linha
            f.write("\n".join(numeros_para_arquivo) + "\n")

# Salvar os números nos arquivos
salvar_numeros_em_arquivos(numeros, num_arquivos, numeros_por_arquivo)

print("Arquivos gerados com sucesso!")
