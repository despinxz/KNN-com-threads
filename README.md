# KNN com threads
Este repositório contém a implementação do algoritmo KNN na linguagem C, fazendo uso de threads. Diferentes números de threads são testados em cada execução, comparando o desempenho do código para cada quantidade. A diferença de performance é evidenciada no relatório contido no repositório.

## Autores
- Eloisa Antero Guisse
- Rafael Varago de Castro

## Arquivos do repositório
- **knn.c:** Código do programa a ser executado;
- **relatorio.pdf:** Relatório detalhando a construção do programa e evidenciando a diferença de performance para cada número de threads diferente;
- **Saídas para diferentes valores de K:** Diretório contendo os arquivos de saída para diferentes valores de K.

## Como executar
**Obs.:** O programa depende de arquivos `xtrain.txt`e `ytrain.txt`, não disponibilizados por ultrapassarem o limite de tamanho.

Para executar o programa, instale o commpilador GCC e execute o seguinte comando no terminal para compilá-lo:

    gcc knn.c -o knn

Um arquivo executável será gerado, que pode ser executado com o comando:

    ./knn


 
