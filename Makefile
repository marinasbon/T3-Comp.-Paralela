# As três versões usam o mesmo compilador (mpicc) e as mesmas flags,
# como pede a seção 5 do enunciado. A sequencial não chama MPI.
MPICC  ?= mpicc
CFLAGS ?= -O2 -std=gnu99 -Wall -Wextra

BIN = sequencial paralelo balanceado

all: $(BIN)

%: src/%.c src/ordenacao.h
	$(MPICC) $(CFLAGS) -o $@ $<

# Tabelas de tempos, speed-ups e eficiência e gráficos, a partir das saídas
# do Slurm em resultados/.
analise:
	python3 analise/analisa.py resultados resultados/analise

clean:
	rm -f $(BIN)

.PHONY: all clean analise
