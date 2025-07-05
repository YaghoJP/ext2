# Makefile para o projeto ext2shell

# Compilador e flags de compilação
# -Wall: Ativa todos os avisos do compilador (recomendado)
# -g:    Adiciona informações de debug ao executável
# -I.:   Informa ao compilador para procurar por arquivos de cabeçalho (.h) no diretório atual
CC = gcc
CFLAGS = -Wall -g -I.

# Nome do executável final
TARGET = ext2shell

# Arquivos fonte (.c) do projeto
# Nota: utils.c foi omitido pois sua função principal já existe em ext2_lib.c
SOURCES = ext2_shell.c ext2_lib.c ext2_commands.c

# Arquivos de cabeçalho (.h) do projeto. Usados para checar dependências.
HEADERS = ext2_commands.h ext2_lib.h ext2_fs.h

# Gera automaticamente a lista de arquivos objeto (.o) a partir dos fontes (.c)
# Ex: ext2_shell.c -> ext2_shell.o
OBJECTS = $(SOURCES:.c=.o)

# --- REGRAS ---

# Regra principal e padrão: executada quando você digita apenas "make"
# Depende do alvo $(TARGET), então o make tentará construir o executável.
all: $(TARGET)

# Regra de ligação: cria o executável final a partir dos arquivos objeto
# Esta regra é executada apenas se algum dos arquivos .o for mais novo que o executável.
$(TARGET): $(OBJECTS)
	@echo "Ligando os arquivos objeto para criar o executável: $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "Executável '$(TARGET)' criado com sucesso!"

# Regra de compilação genérica: transforma qualquer arquivo .c em um .o
# $< é uma variável automática que representa o primeiro pré-requisito (o arquivo .c)
# $@ é uma variável automática que representa o nome do alvo (o arquivo .o)
# A regra depende dos arquivos .h. Se qualquer cabeçalho for alterado, os fontes
# serão recompilados para garantir que as mudanças sejam aplicadas.
%.o: %.c $(HEADERS)
	@echo "Compilando $< -> $@..."
	$(CC) $(CFLAGS) -c $< -o $@

# Regra "clean": remove os arquivos gerados pela compilação
# Útil para limpar o diretório do projeto.
clean:
	@echo "Limpando arquivos gerados..."
	rm -f $(TARGET) $(OBJECTS)

# Regra "run": um atalho para compilar e executar o programa
# Primeiro, garante que o alvo "all" (o executável) esteja construído.
# Depois, executa o programa, exigindo a passagem da variável IMG.
# Exemplo de uso no terminal: make run IMG=minha_imagem.img
run: all
	@if [ -z "$(IMG)" ]; then \
		echo ""; \
		echo "ERRO: É necessário especificar a imagem de disco a ser usada."; \
		echo "Exemplo: make run IMG=sua_imagem.img"; \
		exit 1; \
	fi
	./$(TARGET) $(IMG)

# Declara alvos que não são nomes de arquivos reais.
# Isso evita que o make se confunda caso exista um arquivo chamado "clean" ou "run".
.PHONY: all clean run
