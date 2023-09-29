# Crinha

Compilador para [Rinha de Compiladores](https://github.com/aripiprazole/rinha-de-compiler).
- Feito em C
- Bytecode stack-based VM

Fortemente baseado no livro [Crafting Interpreters](https://craftinginterpreters.com/), tmj @munificent 🤙.

## Howto

Para compilar o arquivo utilizando o `gcc` local:
```sh
make clean && make
build/main # para executar o repl
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
combination.rinha.............0.002420
combination.c.................0.001000
combination.py................0.022160
combination.rb................0.076440

fib35.rinha...................0.898560
fib35.c.......................0.015920
fib35.py......................1.034320
fib35.rb......................0.848040

fib_tco46.rinha...............0.002120
fib_tco46.c...................0.001000
fib_tco46.py..................0.013120
fib_tco46.rb..................0.061840
```

## Tests

Na pasta [`tests/`](tests/) tem testes para diferentes cenários. Para executar, basta rodar o comando abaixo:

```bash
./test.sh
```

## Notas de comentários
- _PERF_ - sinaliza uma possível melhoria de performance
- _MEM_ - sinaliza uma possível melhoria de memória