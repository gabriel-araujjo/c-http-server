- serve json from route
- use external postgres

# o problema atual eh que ele toma segfault ao tentar liberar a lista duplamente encadeada. Parece que essa lista esta contendo algum ponteiro de connection que ja foi liberado pelo stack_free, então isso eh um erro que não deveria acontecer


# implementar fila de conexoes de clients com limite fixo ou ajustavel
# delegando requisicoes para processamento de threads de um pool que ficam 
# em while(true) aguardando uma request chegar para processar, e em seguida pegam a proxima, por ai vai


# problema atual: parece que ao aumentar o numero de threads do pool http, reduzo o espaco da stack, causando a execucao de regex a falhar
# com 8 threads, ela funciona, com 9 ela falha sem dar erro, com 10 ela estoura segfault explodindo a aplicacao