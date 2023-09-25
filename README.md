# Crinha

Compilador para [Rinha de Compiladores](https://github.com/aripiprazole/rinha-de-compiler).
- Feito em C
- Bytecode stack-based VM

Fortemente baseado no livro [Crafting Interpreters](https://craftinginterpreters.com/), tmj @munificent 🤙.

## Howto

Para compilar o arquivo utilizando o `gcc` local:
```sh
make clean && make
build/main {{ nome_do_arquivo }} # para executar o repl
build/main {{ nome_do_arquivo.rinha }} # para rodar um arquivo .rinha
```

Para compilar o arquivo utilizando o `Dockerfile`:
```sh
docker build -t crinha .
docker run -v ./source.rinha:/var/rinha/source.rinha --memory=2gb --cpus=2 -it crinha
```

## Benchmarks

Na pasta [`benchmarks/`](benchmarks/) tem implementações em outras linguagens para comparar com a crinha. Abaixo os resultados obtidos:

```bash
./benchmark.sh
```

```text
combination.rinha.............0.011150
combination.c.................0.009300
combination.py................0.015550
combination.rb................0.064000

fib.rinha.....................1.128950
fib.c.........................0.060150
fib.py........................1.053000
fib.rb........................1.014850
```

## Tests

Na pasta [`tests/`] tem testes para diferentes cenários. Para executar, basta rodar o comando abaixo:

```bash
./test.sh
```

## Notas de comentários
- _PERF_ - sinaliza uma possível melhoria de performance
- _MEM_ - sinaliza uma possível melhoria de memória