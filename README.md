# Sistema de Arquivo EXT2 em C

Este projeto é o trabalho final da disciplina de Sistemas Operacionais e implementa um sistema de arquivos EXT2 utilizando a linguagem C. O objetivo é simular as principais funcionalidades do EXT2, como manipulação de arquivos, diretórios, inodes e blocos de dados.

## Funcionalidades

- Criação, leitura, escrita e remoção de arquivos
- Gerenciamento de diretórios
- Alocação e liberação de blocos e inodes
- Estruturas de dados compatíveis com o padrão EXT2

## Instruções do Shell

O shell do sistema de arquivos deve implementar as seguintes operações:

1. **info**: exibe informações do disco e do sistema de arquivos.
2. **cat &lt;file&gt;**: exibe o conteúdo de um arquivo no formato texto.
3. **attr &lt;file | dir&gt;**: exibe os atributos de um arquivo (file) ou diretório (dir).
4. **cd &lt;path&gt;**: altera o diretório corrente para o definido como path.
5. **ls**: lista os arquivos e diretórios do diretório corrente.
6. **pwd**: exibe o diretório corrente (caminho absoluto).
7. **touch &lt;file&gt;**: cria o arquivo file com conteúdo vazio.
8. **mkdir &lt;dir&gt;**: cria o diretório dir vazio.
9. **rm &lt;file&gt;**: remove o arquivo file do sistema.
10. **rmdir &lt;dir&gt;**: remove o diretório dir, se estiver vazio.
11. **rename &lt;file&gt; &lt;newfilename&gt;**: renomeia arquivo file para newfilename.
12. **cp &lt;source_path&gt; &lt;target_path&gt;**: copia um arquivo de origem (source_path) para destino (target_path).
13. **mv &lt;source_path&gt; &lt;target_path&gt;**: move um arquivo de origem (source_path) para destino (target_path).

- As operações de (1) a (6) envolvem somente a leitura da imagem.
- As operações de (7) a (11) envolvem a escrita na imagem.
- As operações (12) e (13), **cp** e **mv**, copiam e movem arquivos entre o sistema de arquivos da partição atual e o sistema de arquivos da imagem. Para referenciar o sistema de arquivos da partição, use sempre o caminho absoluto como parâmetro dessas operações.
- O comando **mv** é opcional e o **cp** faz somente a cópia do arquivo da imagem para o sistema de arquivos do sistema.

## Como Compilar

```bash
make
```

## Como Executar

```bash
./ext2shell <nome_da_imagem>
```

## Requisitos

- GCC
- Make

## Contribuidores

- Yagho Petini
- Daniel Victor Silva Bonfim
- Elder Henrique Alves Correia
- Winicius Abilio de Britto