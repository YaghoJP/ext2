# Makefile para o projeto ext2shell

# Compilador e flags
CC = gcc
CFLAGS = -Wall -g -I.

# Nome do executável
TARGET = ext2shell

# Arquivos fonte e objeto
SOURCES = ext2_shell.c ext2_ops.c commands.c
OBJECTS = $(SOURCES:.c=.o)

# Regra principal: compila o executável
all: $(TARGET)

# Regra de ligação: cria o executável a partir dos arquivos objeto
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Regra de compilação: cria cada arquivo .o a partir de seu .c
# A dependência de ext2_fs.h garante que, se o cabeçalho mudar, tudo será recompilado.
%.o: %.c ext2_fs.h
	$(CC) $(CFLAGS) -c $< -o $@

# Regra "clean": remove os arquivos gerados pela compilação
clean:
	rm -f $(TARGET) $(OBJECTS)

# Regra "run": um atalho para compilar e executar o programa
# Uso no terminal: make run IMG=sua_imagem.img
run: all
	@if [ -z "$(IMG)" ]; then \
		echo "ERRO: Especifique a imagem a ser usada. Ex: make run IMG=sua_imagem.img"; \
		exit 1; \
	fi
	./$(TARGET) $(IMG)

# Declara alvos que não são nomes de arquivos
.PHONY: all clean run